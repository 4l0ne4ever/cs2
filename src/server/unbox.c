// unbox.c - Unbox Engine Implementation

#include "../include/unbox.h"
#include "../include/database.h"
#include "../include/types.h"
#include "../include/protocol.h"
#include "../include/quests.h"
#include "../include/achievements.h"
#include "../include/chat.h"
#include "../include/request_handler.h"
#include "../include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

static int rng_initialized = 0;

static void init_rng()
{
    if (!rng_initialized)
    {
        srand((unsigned int)time(NULL));
        rng_initialized = 1;
    }
}

int get_available_cases(Case *out_cases, int *count)
{
    if (!out_cases || !count)
        return -1;

    int result = db_load_cases(out_cases, count);

    return result;
}

void calculate_drop_rates(Case *case_data)
{
    // probabilities đã được lưu trong DB,
    // hàm này giữ lại cho đúng interface, có thể dùng để normalize nếu cần.
    (void)case_data;
}

int roll_unbox(Case *case_data)
{
    if (!case_data || case_data->skin_count <= 0)
        return -1;

    init_rng();

    // Tính tổng xác suất
    float total = 0.0f;
    for (int i = 0; i < case_data->skin_count; i++)
    {
        total += case_data->probabilities[i];
    }

    if (total <= 0.0f)
    {
        // fallback: random đều
        return rand() % case_data->skin_count;
    }

    float r = ((float)rand() / (float)RAND_MAX) * total;
    float accum = 0.0f;

    for (int i = 0; i < case_data->skin_count; i++)
    {
        accum += case_data->probabilities[i];
        if (r <= accum)
            return i;
    }

    return case_data->skin_count - 1;
}

// Roll rarity based on standard CS2 drop rates
// Mil-Spec: 79.92%, Restricted: 15.98%, Classified: 3.2%, Covert: 0.64%, Contraband: 0.26%
static SkinRarity roll_rarity()
{
    float r = ((float)rand() / (float)RAND_MAX) * 100.0f;

    if (r < 0.26f)
        return RARITY_CONTRABAND; // 0.26%
    else if (r < 0.90f)
        return RARITY_COVERT; // 0.64%
    else if (r < 4.10f)
        return RARITY_CLASSIFIED; // 3.2%
    else if (r < 20.08f)
        return RARITY_RESTRICTED; // 15.98%
    else
        return RARITY_MIL_SPEC; // 79.92%
}

int unbox_case(int user_id, int case_id, Skin *out_skin)
{
    if (user_id <= 0 || case_id <= 0 || !out_skin)
    {
        return -1;
    }

    init_rng();

    // Step 0: Load case data and verify it exists
    Case case_data;
    int load_case_result = db_load_case(case_id, &case_data);

    if (load_case_result != 0)
    {
        return -2; // Case not found
    }

    // Step 0.1: Load user and check balance
    User user;
    int load_user_result = db_load_user(user_id, &user);

    if (load_user_result != 0)
    {
        return -3; // User not found
    }

    // Step 0.2: Calculate total cost (case price + key price)
    float total_cost = case_data.price + CASE_KEY_PRICE;

    if (user.balance < total_cost)
    {
        return ERR_INSUFFICIENT_FUNDS; // Insufficient funds
    }

    // Step 0.3: Deduct balance BEFORE unboxing (atomic operation)
    user.balance -= total_cost;
    if (db_update_user(&user) != 0)
    {
        return -4; // Failed to update balance
    }

    // CS2 Logic: Roll rarity FIRST, then select skin from that rarity pool
    // Step 1: Get all available rarities in this case first
    SkinRarity available_rarities[7];
    int rarity_count = 0;

    // Check each rarity to see if it exists in this case
    for (SkinRarity r = RARITY_CONSUMER; r <= RARITY_CONTRABAND; r++)
    {
        int test_ids[100];
        int test_count = 0;
        if (db_get_case_skins_by_rarity(case_id, r, test_ids, &test_count) == 0 && test_count > 0)
        {
            available_rarities[rarity_count++] = r;
        }
    }

    if (rarity_count == 0)
    {
        // Rollback balance deduction on error
        user.balance += total_cost;
        db_update_user(&user);
        return -5; // No skins available in this case
    }

    // Step 2: Roll rarity from available rarities only, using CS2 drop rates
    // Mil-Spec: 79.92%, Restricted: 15.98%, Classified: 3.2%, Covert: 0.64%, Contraband: 0.26%
    float r = ((float)rand() / (float)RAND_MAX) * 100.0f;
    SkinRarity rolled_rarity;

    // Try to roll in order of rarity (Contraband -> Covert -> Classified -> Restricted -> Mil-Spec)
    // But only if that rarity is available in the case
    int found = 0;
    if (r < 0.26f)
    {
        // Try Contraband first
        for (int i = 0; i < rarity_count; i++)
        {
            if (available_rarities[i] == RARITY_CONTRABAND)
            {
                rolled_rarity = RARITY_CONTRABAND;
                found = 1;
                break;
            }
        }
    }
    if (!found && r < 0.90f)
    {
        // Try Covert
        for (int i = 0; i < rarity_count; i++)
        {
            if (available_rarities[i] == RARITY_COVERT)
            {
                rolled_rarity = RARITY_COVERT;
                found = 1;
                break;
            }
        }
    }
    if (!found && r < 4.10f)
    {
        // Try Classified
        for (int i = 0; i < rarity_count; i++)
        {
            if (available_rarities[i] == RARITY_CLASSIFIED)
            {
                rolled_rarity = RARITY_CLASSIFIED;
                found = 1;
                break;
            }
        }
    }
    if (!found && r < 20.08f)
    {
        // Try Restricted
        for (int i = 0; i < rarity_count; i++)
        {
            if (available_rarities[i] == RARITY_RESTRICTED)
            {
                rolled_rarity = RARITY_RESTRICTED;
                found = 1;
                break;
            }
        }
    }
    if (!found)
    {
        // Default to Mil-Spec or highest available rarity
        for (int i = 0; i < rarity_count; i++)
        {
            if (available_rarities[i] == RARITY_MIL_SPEC)
            {
                rolled_rarity = RARITY_MIL_SPEC;
                found = 1;
                break;
            }
        }
        // If Mil-Spec not available, use the first available (shouldn't happen, but safety)
        if (!found)
        {
            rolled_rarity = available_rarities[0];
        }
    }

    // Step 3: Get skins from this case that match the rolled rarity
    // IMPORTANT: Each skin definition has a FIXED rarity. We filter by rarity to ensure
    // we only select skins that can actually have that rarity (e.g., AK-47 | Asiimov is always Covert)
    int definition_ids[100];
    int skin_count = 0;
    int db_result = db_get_case_skins_by_rarity(case_id, rolled_rarity, definition_ids, &skin_count);

    if (db_result != 0 || skin_count == 0)
    {
        // Rollback balance deduction on error
        user.balance += total_cost;
        db_update_user(&user);
        return -5; // No skins with this rarity available in this case
    }

    // Step 3: Randomly select one skin from the filtered rarity pool
    int definition_id = definition_ids[rand() % skin_count];

    // Step 3.1: Verify the definition's rarity matches (should always match, but double-check)
    char def_name[MAX_ITEM_NAME_LEN];
    float def_base_price;
    SkinRarity def_rarity;

    if (db_load_skin_definition_with_rarity(definition_id, def_name, &def_base_price, &def_rarity) != 0)
    {
        // Rollback balance deduction on error
        user.balance += total_cost;
        db_update_user(&user);
        return -6; // Failed to load skin definition
    }

    // Use the definition's rarity (should match rolled_rarity, but use definition's to be safe)
    SkinRarity final_rarity = def_rarity;

    // Step 4: Random float wear (0.00-1.00) using CS2 Integer Division method
    // CS2 Research: Generate integer I in [0, 2^31-1], then R = I / (2^31-1)
    // This ensures uniform distribution across all representable float values
    // Float càng thấp = càng hiếm và giá càng cao (do range nhỏ hơn: FN 7% vs BS 55%)
    long long max_int = 2147483647LL; // 2^31 - 1
    long long random_int = ((long long)rand() << 16) | ((long long)rand() & 0xFFFF);
    random_int = random_int % (max_int + 1);
    float wear = (float)random_int / (float)max_int;

    // Round to 10 decimal places precision
    wear = (float)((long long)(wear * 10000000000.0f)) / 10000000000.0f;

    // Step 5: Roll StatTrak™ (10% chance, except for Gold/Contraband items)
    int is_stattrak = 0;
    if (final_rarity != RARITY_CONTRABAND)
    {
        float stattrak_roll = ((float)rand() / (float)RAND_MAX);
        if (stattrak_roll <= 0.10f) // 10% chance
            is_stattrak = 1;
    }

    // Step 6: Roll Pattern Seed (0-999, uniform distribution)
    // Pattern Seed determines texture position/rotation on skin
    // CS2 uses 1000 patterns (0-999 inclusive)
    int pattern_seed = rand() % 1000; // 0-999 inclusive

    // Step 7: Create instance with definition's rarity (not rolled rarity), wear, pattern_seed, and is_stattrak
    int instance_id = 0;
    if (db_create_skin_instance(definition_id, final_rarity, wear, pattern_seed, is_stattrak, user_id, &instance_id) != 0)
    {
        // Rollback balance deduction on error
        user.balance += total_cost;
        db_update_user(&user);
        return -6; // Failed to create skin instance
    }

    // Step 8: Add to inventory
    if (db_add_to_inventory(user_id, instance_id) != 0)
    {
        // Rollback: remove instance and restore balance
        // Note: In production, you might want to keep the instance but just not add to inventory
        user.balance += total_cost;
        db_update_user(&user);
        return -7; // Failed to add to inventory
    }

    // Note: Unboxed items are NOT trade locked - only items listed on market are locked

    // Step 9: Calculate price using definition's rarity
    // Calculate price: base_price * rarity_multiplier * wear_multiplier
    float current_price = db_calculate_skin_price(definition_id, final_rarity, wear);

    // Fill out_skin
    memset(out_skin, 0, sizeof(Skin));
    out_skin->skin_id = instance_id;

    strncpy(out_skin->name, def_name, MAX_ITEM_NAME_LEN);
    out_skin->name[MAX_ITEM_NAME_LEN - 1] = '\0'; // Ensure null terminator
    out_skin->rarity = final_rarity;              // Use definition's rarity, not rolled rarity
    out_skin->wear = wear;
    out_skin->pattern_seed = pattern_seed;
    out_skin->is_stattrak = is_stattrak;

    // Base price is the definition base price * rarity multiplier
    float rarity_mult = 1.0f;
    db_get_rarity_multiplier(final_rarity, &rarity_mult);
    out_skin->base_price = def_base_price * rarity_mult;

    out_skin->current_price = current_price;
    out_skin->owner_id = user_id;
    out_skin->acquired_at = time(NULL);
    out_skin->is_tradable = 0; // trade lock 7 ngày

    // Step 10: Calculate profit if skin value > unbox cost
    float profit = 0.0f;
    if (current_price > total_cost)
    {
        profit = current_price - total_cost;
        LOG_INFO("[UNBOX] User %d unboxed profit: case_id=%d, cost=$%.2f, value=$%.2f, profit=+$%.2f",
                 user_id, case_id, total_cost, current_price, profit);
    }
    else
    {
        float loss = total_cost - current_price;
        LOG_DEBUG("[UNBOX] User %d unboxed: case_id=%d, cost=$%.2f, value=$%.2f, loss=$%.2f",
                  user_id, case_id, total_cost, current_price, loss);
    }

    // Step 11: Log unbox transaction (include profit if any)
    TransactionLog log;
    log.log_id = 0;
    log.type = LOG_UNBOX;
    log.user_id = user_id;
    if (profit > 0.0f)
    {
        snprintf(log.details, sizeof(log.details), "Unboxed case %d (%s) -> instance %d (def %d, rarity %d, wear %.10f, pattern %d, stattrak %d, cost $%.2f, value $%.2f, profit +$%.2f)",
                 case_id, case_data.name, instance_id, definition_id, final_rarity, wear, pattern_seed, is_stattrak, total_cost, current_price, profit);
    }
    else
    {
        snprintf(log.details, sizeof(log.details), "Unboxed case %d (%s) -> instance %d (def %d, rarity %d, wear %.10f, pattern %d, stattrak %d, cost $%.2f, value $%.2f)",
                 case_id, case_data.name, instance_id, definition_id, final_rarity, wear, pattern_seed, is_stattrak, total_cost, current_price);
    }
    log.timestamp = time(NULL);
    db_log_transaction(&log);

    // Step 12: Update quests and achievements
    // Lucky Gambler quest: Unbox 5 cases
    update_quest_progress(user_id, QUEST_LUCKY_GAMBLER, 1);
    
    // Profit Maker quest: Track profit from unbox (if skin value > cost)
    if (profit > 0.0f)
    {
        update_quest_progress(user_id, QUEST_PROFIT_MAKER, (int)profit);
    }

    // First Knife achievement: Unbox Contraband (knife/glove)
    if (final_rarity == RARITY_CONTRABAND)
    {
        unlock_achievement(user_id, ACHIEVEMENT_FIRST_KNIFE);
    }

    // Check quest completion
    check_quest_completion(user_id);

    // Step 13: Broadcast rare drops (Contraband/Covert items)
    if (final_rarity == RARITY_CONTRABAND || final_rarity == RARITY_COVERT)
    {
        broadcast_rare_unbox(user_id, out_skin);
    }

    return 0;
}

// Cache structure for case skin info (to avoid N+1 query problem)
typedef struct
{
    int def_id;
    char name[MAX_ITEM_NAME_LEN]; // From types.h
    SkinRarity rarity;
    float base_price;
    int visual_weight; // Weight for weighted random selection
} CaseSkinCache;

// Helper: Get visual weight based on rarity (for weighted random in animation)
// Weights match actual CS2 drop rates (normalized to avoid large numbers)
// Higher weight = more common in preview (matches actual drop rates)
static int get_visual_weight(SkinRarity r)
{
    switch (r)
    {
    case RARITY_MIL_SPEC:
        return 7992; // 79.92% - most common
    case RARITY_RESTRICTED:
        return 1598; // 15.98% - common
    case RARITY_CLASSIFIED:
        return 320; // 3.2% - uncommon
    case RARITY_COVERT:
        return 64; // 0.64% - rare
    case RARITY_CONTRABAND:
        return 26; // 0.26% - extremely rare
    default:
        return 100; // Default weight for unknown rarities
    }
}

// Generate preview skins for animation display
// OPTIMIZED: Uses single DB query + weighted random for better performance and UX
int generate_unbox_preview(int case_id, Skin *out_preview_skins, int preview_count)
{
    if (!out_preview_skins || preview_count <= 0)
        return -1;

// Step 1: LOAD ALL CASE SKINS INTO RAM (Single DB query - solves N+1 problem)
// Use temporary struct that matches db_fetch_full_case_info layout
#define MAX_CACHE_SKINS 100
    typedef struct
    {
        int def_id;
        char name[MAX_ITEM_NAME_LEN];
        SkinRarity rarity;
        float base_price;
    } CaseSkinInfo;

    CaseSkinInfo temp_skins[MAX_CACHE_SKINS];
    int total_skins = 0;
    if (db_fetch_full_case_info(case_id, (void *)temp_skins, &total_skins) != 0 || total_skins <= 0)
    {
        return -2; // No skins in case or failed to load
    }

    // Buffer overflow protection: ensure we don't exceed cache size
    if (total_skins > MAX_CACHE_SKINS)
    {
        total_skins = MAX_CACHE_SKINS; // Limit to cache size
    }

    // Step 1.5: Copy to CaseSkinCache and calculate weights
    CaseSkinCache cached_skins[MAX_CACHE_SKINS];
    int total_weight = 0;
    for (int k = 0; k < total_skins; k++)
    {
        cached_skins[k].def_id = temp_skins[k].def_id;
        strncpy(cached_skins[k].name, temp_skins[k].name, MAX_ITEM_NAME_LEN - 1);
        cached_skins[k].name[MAX_ITEM_NAME_LEN - 1] = '\0';
        cached_skins[k].rarity = temp_skins[k].rarity;
        cached_skins[k].base_price = temp_skins[k].base_price;
        cached_skins[k].visual_weight = get_visual_weight(cached_skins[k].rarity);
        total_weight += cached_skins[k].visual_weight;
    }

    if (total_weight <= 0)
    {
        return -3; // Invalid weights
    }

    init_rng();

    // Step 3: GENERATE PREVIEW (All operations on RAM - very fast)
    int valid_count = 0;
    for (int i = 0; i < preview_count && i < 50; i++)
    {
        // --- WEIGHTED RANDOM SELECTION (not uniform) ---
        // This ensures preview reflects actual drop rates (more blue, less red)
        int r_val = rand() % total_weight;
        int current_weight = 0;
        int selected_idx = 0;

        for (int k = 0; k < total_skins; k++)
        {
            current_weight += cached_skins[k].visual_weight;
            if (r_val < current_weight)
            {
                selected_idx = k;
                break;
            }
        }

        CaseSkinCache *def = &cached_skins[selected_idx];

        // Debug: Verify cache data is loaded correctly
        if (def->def_id <= 0 || strlen(def->name) == 0)
        {
            // Skip invalid entry (should not happen, but safety check)
            continue;
        }

        // --- GENERATE RANDOM ATTRIBUTES ---
        // Random wear using CS2 Integer Division method
        long long max_int = 2147483647LL; // 2^31 - 1
        long long random_int = ((long long)rand() << 16) | ((long long)rand() & 0xFFFF);
        random_int = random_int % (max_int + 1);
        float preview_wear = (float)random_int / (float)max_int;
        preview_wear = (float)((long long)(preview_wear * 10000000000.0f)) / 10000000000.0f;

        // Random StatTrak (10% chance, except Contraband)
        int preview_stattrak = 0;
        if (def->rarity != RARITY_CONTRABAND)
        {
            if (((float)rand() / (float)RAND_MAX) <= 0.10f)
                preview_stattrak = 1;
        }

        // Random Pattern Seed (0-999) - CS2 uses 1000 patterns (0-999 inclusive)
        int pattern_seed = rand() % 1000;

        // --- FILL PREVIEW SKIN ---
        memset(&out_preview_skins[valid_count], 0, sizeof(Skin));
        out_preview_skins[valid_count].skin_id = 0; // Preview only, no instance
        strncpy(out_preview_skins[valid_count].name, def->name, MAX_ITEM_NAME_LEN - 1);
        out_preview_skins[valid_count].name[MAX_ITEM_NAME_LEN - 1] = '\0';
        out_preview_skins[valid_count].rarity = def->rarity; // From cache (FIXED per definition)
        out_preview_skins[valid_count].wear = preview_wear;
        out_preview_skins[valid_count].pattern_seed = pattern_seed;
        out_preview_skins[valid_count].is_stattrak = preview_stattrak;

        // Calculate price (using cached data, no DB query needed)
        float rarity_mult = 1.0f;
        db_get_rarity_multiplier(def->rarity, &rarity_mult);
        out_preview_skins[valid_count].base_price = def->base_price * rarity_mult;
        out_preview_skins[valid_count].current_price = db_calculate_skin_price(def->def_id, def->rarity, preview_wear);

        valid_count++;
    }

    return valid_count;
}

// Broadcast rare unbox: Log and prepare notification for rare drops
// In a real server implementation, this would send a message to all connected clients
void broadcast_rare_unbox(int user_id, const Skin *unboxed_skin)
{
    if (!unboxed_skin)
        return;

    // Log rare unbox for server-side tracking
    TransactionLog log;
    log.log_id = 0;
    log.type = LOG_UNBOX;
    log.user_id = user_id;

    const char *rarity_name = "Unknown";
    switch (unboxed_skin->rarity)
    {
    case RARITY_CONTRABAND:
        rarity_name = "Contraband (Gold)";
        break;
    case RARITY_COVERT:
        rarity_name = "Covert (Red)";
        break;
    default:
        return; // Only broadcast Contraband and Covert
    }

    snprintf(log.details, sizeof(log.details),
             "RARE UNBOX: User %d unboxed %s %s (Wear: %.2f, Price: $%.2f)%s",
             user_id, rarity_name, unboxed_skin->name,
             unboxed_skin->wear, unboxed_skin->current_price,
             unboxed_skin->is_stattrak ? " [StatTrak™]" : "");
    log.timestamp = time(NULL);
    db_log_transaction(&log);

    // Broadcast to chat system and all connected clients
    User user;
    if (db_load_user(user_id, &user) == 0)
    {
        char broadcast_msg[256];
        snprintf(broadcast_msg, sizeof(broadcast_msg),
                 "🎉 RARE DROP: %s unboxed %s %s%s (Price: $%.2f)!",
                 user.username, rarity_name, unboxed_skin->name,
                 unboxed_skin->is_stattrak ? " [StatTrak™]" : "",
                 unboxed_skin->current_price);

        // Save as system message (user_id 0 = system)
        db_save_chat_message(0, "SYSTEM", broadcast_msg);

        // Broadcast to all connected clients
        broadcast_to_all_clients("SYSTEM", broadcast_msg);
    }
}
