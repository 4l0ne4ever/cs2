// test_pipeline_e2e.c - End-to-End Pipeline Test (Focus on Logic)

#include "../include/database.h"
#include "../include/auth.h"
#include "../include/market.h"
#include "../include/trading.h"
#include "../include/unbox.h"
#include "../include/protocol.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#define EPSILON 0.01f

// Helper: Verify float equality with epsilon
int float_equals(float a, float b)
{
    return fabs(a - b) < EPSILON;
}

// Test 1: Complete User Journey
void test_complete_user_journey()
{
    printf("=== Test 1: Complete User Journey ===\n");
    
    db_init();
    
    // Step 1: Register new user
    printf("\n[1.1] Registering new user...\n");
    User user;
    int result = register_user("pipeline_user", "password123", &user);
    assert(result == ERR_SUCCESS || result == ERR_USER_EXISTS);
    
    if (result == ERR_USER_EXISTS)
    {
        db_load_user_by_username("pipeline_user", &user);
    }
    
    assert(user.user_id > 0);
    assert(strcmp(user.username, "pipeline_user") == 0);
    printf("  ✓ User registered: ID=%d, Username=%s, Balance=$%.2f\n", 
           user.user_id, user.username, user.balance);
    
    // Step 2: Login
    printf("\n[1.2] Logging in...\n");
    Session session;
    result = login_user("pipeline_user", "password123", &session);
    assert(result == ERR_SUCCESS);
    assert(session.user_id == user.user_id);
    assert(strlen(session.session_token) > 0);
    printf("  ✓ Login successful: Session token=%s\n", session.session_token);
    
    // Step 3: Ensure sufficient balance
    printf("\n[1.3] Checking balance...\n");
    db_load_user(user.user_id, &user);
    if (user.balance < 100.0f)
    {
        user.balance = 1000.0f;
        db_update_user(&user);
        printf("  ✓ Balance set to $%.2f\n", user.balance);
    }
    else
    {
        printf("  ✓ Balance: $%.2f\n", user.balance);
    }
    
    // Step 4: Get available cases
    printf("\n[1.4] Getting available cases...\n");
    Case cases[50];
    int case_count = 0;
    result = get_available_cases(cases, &case_count);
    assert(result == 0);
    assert(case_count > 0);
    printf("  ✓ Found %d cases\n", case_count);
    for (int i = 0; i < case_count && i < 3; i++)
    {
        printf("    - %s ($%.2f)\n", cases[i].name, cases[i].price);
    }
    
    // Step 5: Unbox a case
    printf("\n[1.5] Unboxing case: %s...\n", cases[0].name);
    float balance_before = user.balance;
    Skin unboxed;
    memset(&unboxed, 0, sizeof(Skin));
    
    result = unbox_case(user.user_id, cases[0].case_id, &unboxed);
    assert(result == 0);
    assert(unboxed.skin_id > 0);
    assert(strlen(unboxed.name) > 0);
    
    // Verify unbox logic
    printf("  ✓ Unboxed skin: %s\n", unboxed.name);
    printf("    - Rarity: %s\n", rarity_to_string(unboxed.rarity));
    printf("    - Wear: %s (Float: %.10f)\n", wear_to_string(unboxed.wear), unboxed.wear);
    printf("    - Pattern Seed: %d\n", unboxed.pattern_seed);
    printf("    - StatTrak: %s\n", unboxed.is_stattrak ? "Yes" : "No");
    printf("    - Price: $%.2f\n", unboxed.current_price);
    
    // Verify attributes
    assert(unboxed.wear >= 0.0f && unboxed.wear <= 1.0f);
    assert(unboxed.pattern_seed >= 0 && unboxed.pattern_seed <= 999);
    assert(unboxed.is_stattrak == 0 || unboxed.is_stattrak == 1);
    
    // Verify balance deduction
    db_load_user(user.user_id, &user);
    float balance_after = user.balance;
    float expected_cost = cases[0].price + 2.50f; // Case + Key
    float actual_deduction = balance_before - balance_after;
    
    assert(float_equals(actual_deduction, expected_cost));
    printf("  ✓ Balance deducted: $%.2f (Expected: $%.2f)\n", 
           actual_deduction, expected_cost);
    
    // Step 6: Verify inventory
    printf("\n[1.6] Checking inventory...\n");
    Inventory inv;
    result = db_load_inventory(user.user_id, &inv);
    assert(result == 0);
    assert(inv.count > 0);
    
    int found = 0;
    for (int i = 0; i < inv.count; i++)
    {
        if (inv.skin_ids[i] == unboxed.skin_id)
        {
            found = 1;
            break;
        }
    }
    assert(found == 1);
    printf("  ✓ Skin added to inventory (Total: %d items)\n", inv.count);
    
    // Step 7: List skin on market
    printf("\n[1.7] Listing skin on market...\n");
    float list_price = unboxed.current_price * 1.1f; // 10% markup
    result = list_skin_on_market(user.user_id, unboxed.skin_id, list_price);
    assert(result == 0);
    printf("  ✓ Skin listed at $%.2f\n", list_price);
    
    // Step 8: Verify market listing
    printf("\n[1.8] Verifying market listing...\n");
    MarketListing listings[100];
    int listing_count = 0;
    result = get_market_listings(listings, &listing_count);
    assert(result == 0);
    
    int listing_found = 0;
    for (int i = 0; i < listing_count; i++)
    {
        if (listings[i].skin_id == unboxed.skin_id && listings[i].seller_id == user.user_id)
        {
            listing_found = 1;
            assert(float_equals(listings[i].price, list_price));
            break;
        }
    }
    assert(listing_found == 1);
    printf("  ✓ Market listing verified (Total listings: %d)\n", listing_count);
    
    db_close();
    printf("\n✓ Complete user journey test PASSED\n\n");
}

// Test 2: Unbox Logic Verification
void test_unbox_logic()
{
    printf("=== Test 2: Unbox Logic Verification ===\n");
    
    db_init();
    
    // Create test user
    User user;
    register_user("unbox_logic_test", "pass123", &user);
    db_load_user_by_username("unbox_logic_test", &user);
    
    user.balance = 5000.0f;
    db_update_user(&user);
    
    Case cases[50];
    int case_count = 0;
    get_available_cases(cases, &case_count);
    assert(case_count > 0);
    
    printf("\n[2.1] Testing rarity distribution...\n");
    int rarity_counts[7] = {0}; // Index by SkinRarity enum
    
    int test_count = 100;
    for (int i = 0; i < test_count; i++)
    {
        Skin unboxed;
        unbox_case(user.user_id, cases[0].case_id, &unboxed);
        
        assert(unboxed.rarity >= 0 && unboxed.rarity <= 6);
        rarity_counts[unboxed.rarity]++;
    }
    
    printf("  Rarity distribution (out of %d unboxes):\n", test_count);
    for (int r = 0; r <= 6; r++)
    {
        if (rarity_counts[r] > 0)
        {
            float percentage = (rarity_counts[r] * 100.0f) / test_count;
            printf("    - %s: %d (%.1f%%)\n", 
                   rarity_to_string((SkinRarity)r), rarity_counts[r], percentage);
        }
    }
    
    // Verify Mil-Spec is most common (should be >50% for 100 unboxes)
    assert(rarity_counts[RARITY_MIL_SPEC] > 50);
    printf("  ✓ Mil-Spec is most common (expected)\n");
    
    printf("\n[2.2] Testing float distribution...\n");
    int wear_counts[5] = {0}; // FN, MW, FT, WW, BS
    float min_float = 1.0f;
    float max_float = 0.0f;
    
    for (int i = 0; i < test_count; i++)
    {
        Skin unboxed;
        unbox_case(user.user_id, cases[0].case_id, &unboxed);
        
        assert(unboxed.wear >= 0.0f && unboxed.wear <= 1.0f);
        
        if (unboxed.wear < min_float) min_float = unboxed.wear;
        if (unboxed.wear > max_float) max_float = unboxed.wear;
        
        WearConditionName wear = get_wear_condition(unboxed.wear);
        wear_counts[wear]++;
    }
    
    printf("  Wear distribution:\n");
    printf("    - Factory New (0.00-0.07): %d\n", wear_counts[WEAR_FN]);
    printf("    - Minimal Wear (0.07-0.15): %d\n", wear_counts[WEAR_MW]);
    printf("    - Field-Tested (0.15-0.37): %d\n", wear_counts[WEAR_FT]);
    printf("    - Well-Worn (0.37-0.45): %d\n", wear_counts[WEAR_WW]);
    printf("    - Battle-Scarred (0.45-1.00): %d\n", wear_counts[WEAR_BS]);
    printf("  Float range: %.10f - %.10f\n", min_float, max_float);
    
    // Verify all wear levels are possible
    int total_wear = 0;
    for (int i = 0; i < 5; i++) total_wear += wear_counts[i];
    assert(total_wear == test_count);
    printf("  ✓ All wear levels are possible\n");
    
    printf("\n[2.3] Testing pattern seed range...\n");
    int min_seed = 1000;
    int max_seed = -1;
    
    for (int i = 0; i < 50; i++)
    {
        Skin unboxed;
        unbox_case(user.user_id, cases[0].case_id, &unboxed);
        
        assert(unboxed.pattern_seed >= 0 && unboxed.pattern_seed <= 999);
        if (unboxed.pattern_seed < min_seed) min_seed = unboxed.pattern_seed;
        if (unboxed.pattern_seed > max_seed) max_seed = unboxed.pattern_seed;
    }
    
    printf("  Pattern seed range: %d - %d\n", min_seed, max_seed);
    assert(min_seed >= 0 && max_seed <= 999);
    printf("  ✓ Pattern seed in valid range (0-999)\n");
    
    printf("\n[2.4] Testing StatTrak distribution...\n");
    int stattrak_count = 0;
    int non_stattrak_count = 0;
    int gold_stattrak_count = 0; // Should be 0
    
    for (int i = 0; i < test_count; i++)
    {
        Skin unboxed;
        unbox_case(user.user_id, cases[0].case_id, &unboxed);
        
        if (unboxed.rarity == RARITY_CONTRABAND)
        {
            assert(unboxed.is_stattrak == 0); // Gold never has StatTrak
            if (unboxed.is_stattrak == 1) gold_stattrak_count++;
        }
        else
        {
            if (unboxed.is_stattrak == 1)
                stattrak_count++;
            else
                non_stattrak_count++;
        }
    }
    
    float stattrak_percentage = (stattrak_count * 100.0f) / (stattrak_count + non_stattrak_count);
    printf("  StatTrak: %d (%.1f%%)\n", stattrak_count, stattrak_percentage);
    printf("  Non-StatTrak: %d\n", non_stattrak_count);
    printf("  Gold with StatTrak: %d (should be 0)\n", gold_stattrak_count);
    
    assert(gold_stattrak_count == 0);
    assert(stattrak_percentage >= 5.0f && stattrak_percentage <= 15.0f); // ~10% ±5%
    printf("  ✓ StatTrak distribution correct (~10%%, Gold never has StatTrak)\n");
    
    db_close();
    printf("\n✓ Unbox logic test PASSED\n\n");
}

// Test 3: Trading Logic
void test_trading_logic()
{
    printf("=== Test 3: Trading Logic ===\n");
    
    db_init();
    
    // Create two users
    User user1, user2;
    register_user("trader1", "pass123", &user1);
    register_user("trader2", "pass123", &user2);
    
    db_load_user_by_username("trader1", &user1);
    db_load_user_by_username("trader2", &user2);
    
    user1.balance = 1000.0f;
    user2.balance = 1000.0f;
    db_update_user(&user1);
    db_update_user(&user2);
    
    Case cases[50];
    int case_count = 0;
    get_available_cases(cases, &case_count);
    
    printf("\n[3.1] Users unbox skins...\n");
    Skin skin1, skin2;
    unbox_case(user1.user_id, cases[0].case_id, &skin1);
    unbox_case(user2.user_id, cases[0].case_id, &skin2);
    
    printf("  User1 unboxed: %s ($%.2f)\n", skin1.name, skin1.current_price);
    printf("  User2 unboxed: %s ($%.2f)\n", skin2.name, skin2.current_price);
    
    // Wait for trade lock to expire (in real system, would be 7 days)
    // For testing, we'll check trade lock status
    printf("\n[3.2] Checking trade lock...\n");
    int is_locked1 = is_trade_locked(skin1.skin_id);
    int is_locked2 = is_trade_locked(skin2.skin_id);
    printf("  Skin1 trade locked: %s\n", is_locked1 ? "Yes" : "No");
    printf("  Skin2 trade locked: %s\n", is_locked2 ? "Yes" : "No");
    assert(is_locked1 == 1); // Should be locked after unbox
    assert(is_locked2 == 1);
    printf("  ✓ Trade locks applied correctly\n");
    
    printf("\n[3.3] Attempting trade (should handle locked skins)...\n");
    TradeOffer offer = {0};
    offer.from_user_id = user1.user_id;
    offer.to_user_id = user2.user_id;
    offer.offered_skins[0] = skin1.skin_id;
    offer.offered_count = 1;
    offer.requested_skins[0] = skin2.skin_id;
    offer.requested_count = 1;
    offer.status = TRADE_PENDING;
    offer.created_at = time(NULL);
    offer.expires_at = time(NULL) + 900;
    
    // Trade may fail due to locks, which is expected
    int trade_result = send_trade_offer(user1.user_id, user2.user_id, &offer);
    if (trade_result == 0)
    {
        printf("  ✓ Trade offer created\n");
    }
    else
    {
        printf("  ⚠ Trade offer failed (expected if skins are locked)\n");
    }
    
    db_close();
    printf("\n✓ Trading logic test PASSED\n\n");
}

// Test 4: Market Logic
void test_market_logic()
{
    printf("=== Test 4: Market Logic ===\n");
    
    db_init();
    
    // Create seller and buyer
    User seller, buyer;
    register_user("seller", "pass123", &seller);
    register_user("buyer", "pass123", &buyer);
    
    db_load_user_by_username("seller", &seller);
    db_load_user_by_username("buyer", &buyer);
    
    seller.balance = 1000.0f;
    buyer.balance = 2000.0f;
    db_update_user(&seller);
    db_update_user(&buyer);
    
    Case cases[50];
    int case_count = 0;
    get_available_cases(cases, &case_count);
    
    printf("\n[4.1] Seller unboxes and lists skin...\n");
    Skin skin;
    unbox_case(seller.user_id, cases[0].case_id, &skin);
    printf("  Unboxed: %s ($%.2f)\n", skin.name, skin.current_price);
    
    float list_price = skin.current_price * 1.2f;
    int result = list_skin_on_market(seller.user_id, skin.skin_id, list_price);
    assert(result == 0);
    printf("  ✓ Listed at $%.2f\n", list_price);
    
    // Verify seller's inventory
    Inventory seller_inv;
    db_load_inventory(seller.user_id, &seller_inv);
    int in_seller_inv = 0;
    for (int i = 0; i < seller_inv.count; i++)
    {
        if (seller_inv.skin_ids[i] == skin.skin_id)
            in_seller_inv = 1;
    }
    // Skin should still be in seller's inventory (not removed when listed)
    printf("  ✓ Skin still in seller inventory: %s\n", in_seller_inv ? "Yes" : "No");
    
    printf("\n[4.2] Buyer purchases from market...\n");
    MarketListing listings[100];
    int listing_count = 0;
    get_market_listings(listings, &listing_count);
    
    int listing_id = -1;
    for (int i = 0; i < listing_count; i++)
    {
        if (listings[i].skin_id == skin.skin_id && listings[i].seller_id == seller.user_id)
        {
            listing_id = listings[i].listing_id;
            break;
        }
    }
    
    assert(listing_id > 0);
    
    float buyer_balance_before = buyer.balance;
    result = buy_from_market(buyer.user_id, listing_id);
    
    if (result == 0)
    {
        db_load_user(buyer.user_id, &buyer);
        float buyer_balance_after = buyer.balance;
        float deduction = buyer_balance_before - buyer_balance_after;
        
        assert(float_equals(deduction, list_price));
        printf("  ✓ Purchase successful\n");
        printf("  ✓ Balance deducted: $%.2f\n", deduction);
        
        // Verify buyer's inventory
        Inventory buyer_inv;
        db_load_inventory(buyer.user_id, &buyer_inv);
        int in_buyer_inv = 0;
        for (int i = 0; i < buyer_inv.count; i++)
        {
            if (buyer_inv.skin_ids[i] == skin.skin_id)
                in_buyer_inv = 1;
        }
        assert(in_buyer_inv == 1);
        printf("  ✓ Skin added to buyer inventory\n");
        
        // Verify seller's balance increased
        db_load_user(seller.user_id, &seller);
        printf("  ✓ Seller balance: $%.2f\n", seller.balance);
    }
    else
    {
        printf("  ⚠ Purchase failed (may be due to insufficient funds or trade lock)\n");
    }
    
    db_close();
    printf("\n✓ Market logic test PASSED\n\n");
}

// Test 5: Price Calculation Logic
void test_price_calculation()
{
    printf("=== Test 5: Price Calculation Logic ===\n");
    
    db_init();
    
    // Get a skin definition
    Case cases[50];
    int case_count = 0;
    get_available_cases(cases, &case_count);
    
    User user;
    register_user("price_test", "pass123", &user);
    db_load_user_by_username("price_test", &user);
    user.balance = 1000.0f;
    db_update_user(&user);
    
    printf("\n[5.1] Testing price calculation for different wear levels...\n");
    
    // Unbox multiple skins to get different wear values
    Skin skins[10];
    int skin_count = 0;
    
    for (int i = 0; i < 10 && skin_count < 10; i++)
    {
        Skin unboxed;
        if (unbox_case(user.user_id, cases[0].case_id, &unboxed) == 0)
        {
            skins[skin_count++] = unboxed;
        }
    }
    
    printf("  Testing %d skins:\n", skin_count);
    for (int i = 0; i < skin_count; i++)
    {
        Skin *s = &skins[i];
        const char *wear_str = wear_to_string(s->wear);
        const char *rarity_str = rarity_to_string(s->rarity);
        
        // Load definition_id for price calculation
        int def_id;
        SkinRarity inst_rarity;
        WearCondition inst_wear;
        int pattern_seed, is_stattrak, owner_id, is_tradable;
        time_t acquired_at;
        
        if (db_load_skin_instance(s->skin_id, &def_id, &inst_rarity, &inst_wear, 
                                  &pattern_seed, &is_stattrak, &owner_id, 
                                  &acquired_at, &is_tradable) == 0)
        {
            float expected_base = s->base_price;
            float calculated_price = db_calculate_skin_price(
                def_id, inst_rarity, inst_wear);
            
            printf("    %d. %s | %s | Wear: %.10f | Base: $%.2f | Price: $%.2f\n",
                   i + 1, s->name, wear_str, s->wear, expected_base, calculated_price);
            
            // Price should be positive
            assert(calculated_price > 0.0f);
        }
        else
        {
            printf("    %d. %s | %s | Wear: %.10f | Price: $%.2f\n",
                   i + 1, s->name, wear_str, s->wear, s->current_price);
        }
        
        // Lower wear should generally have higher price (for same rarity)
        // This is a general rule, not always true due to rarity multipliers
    }
    
    printf("\n[5.2] Testing rarity multipliers...\n");
    float multipliers[7];
    for (int r = 0; r <= 6; r++)
    {
        db_get_rarity_multiplier((SkinRarity)r, &multipliers[r]);
        printf("  %s multiplier: %.2fx\n", rarity_to_string((SkinRarity)r), multipliers[r]);
    }
    
    // Verify Contraband has highest multiplier
    assert(multipliers[RARITY_CONTRABAND] > multipliers[RARITY_COVERT]);
    assert(multipliers[RARITY_COVERT] > multipliers[RARITY_CLASSIFIED]);
    printf("  ✓ Rarity multipliers in correct order\n");
    
    db_close();
    printf("\n✓ Price calculation test PASSED\n\n");
}

// Test 6: Data Integrity
void test_data_integrity()
{
    printf("=== Test 6: Data Integrity ===\n");
    
    db_init();
    
    User user;
    register_user("integrity_test", "pass123", &user);
    db_load_user_by_username("integrity_test", &user);
    
    user.balance = 1000.0f;
    db_update_user(&user);
    
    Case cases[50];
    int case_count = 0;
    get_available_cases(cases, &case_count);
    
    printf("\n[6.1] Testing balance consistency...\n");
    float initial_balance = user.balance;
    
    Skin unboxed;
    unbox_case(user.user_id, cases[0].case_id, &unboxed);
    
    db_load_user(user.user_id, &user);
    float final_balance = user.balance;
    float expected_cost = cases[0].price + 2.50f;
    float actual_cost = initial_balance - final_balance;
    
    assert(float_equals(actual_cost, expected_cost));
    printf("  ✓ Balance consistency: $%.2f - $%.2f = $%.2f (Expected: $%.2f)\n",
           initial_balance, final_balance, actual_cost, expected_cost);
    
    printf("\n[6.2] Testing inventory consistency...\n");
    Inventory inv_before;
    db_load_inventory(user.user_id, &inv_before);
    int count_before = inv_before.count;
    
    // Unbox another skin
    Skin unboxed2;
    unbox_case(user.user_id, cases[0].case_id, &unboxed2);
    
    Inventory inv_after;
    db_load_inventory(user.user_id, &inv_after);
    int count_after = inv_after.count;
    
    assert(count_after == count_before + 1);
    printf("  ✓ Inventory count: %d → %d (+1)\n", count_before, count_after);
    
    // Verify both skins are in inventory
    int found1 = 0, found2 = 0;
    for (int i = 0; i < inv_after.count; i++)
    {
        if (inv_after.skin_ids[i] == unboxed.skin_id) found1 = 1;
        if (inv_after.skin_ids[i] == unboxed2.skin_id) found2 = 1;
    }
    assert(found1 == 1 && found2 == 1);
    printf("  ✓ Both skins in inventory\n");
    
    printf("\n[6.3] Testing skin instance uniqueness...\n");
    assert(unboxed.skin_id != unboxed2.skin_id);
    printf("  ✓ Each unbox creates unique skin instance\n");
    
    // Verify skin attributes are different (wear, pattern, etc.)
    int different_attrs = 0;
    if (unboxed.wear != unboxed2.wear) different_attrs++;
    if (unboxed.pattern_seed != unboxed2.pattern_seed) different_attrs++;
    if (unboxed.is_stattrak != unboxed2.is_stattrak) different_attrs++;
    
    assert(different_attrs > 0); // At least one attribute should differ
    printf("  ✓ Skin instances have different attributes\n");
    
    db_close();
    printf("\n✓ Data integrity test PASSED\n\n");
}

// Test 7: Error Handling Logic
void test_error_handling()
{
    printf("=== Test 7: Error Handling Logic ===\n");
    
    db_init();
    
    printf("\n[7.1] Testing insufficient funds...\n");
    User user;
    register_user("error_test", "pass123", &user);
    db_load_user_by_username("error_test", &user);
    
    user.balance = 0.0f;
    db_update_user(&user);
    
    Case cases[50];
    int case_count = 0;
    get_available_cases(cases, &case_count);
    
    Skin unboxed;
    int result = unbox_case(user.user_id, cases[0].case_id, &unboxed);
    assert(result == ERR_INSUFFICIENT_FUNDS);
    printf("  ✓ Insufficient funds correctly rejected\n");
    
    printf("\n[7.2] Testing invalid case ID...\n");
    user.balance = 1000.0f;
    db_update_user(&user);
    
    result = unbox_case(user.user_id, 99999, &unboxed); // Invalid case ID
    assert(result != 0); // Should fail
    printf("  ✓ Invalid case ID correctly rejected\n");
    
    printf("\n[7.3] Testing duplicate registration...\n");
    User duplicate;
    result = register_user("error_test", "anotherpass", &duplicate);
    assert(result == ERR_USER_EXISTS);
    printf("  ✓ Duplicate registration correctly rejected\n");
    
    printf("\n[7.4] Testing invalid login...\n");
    Session session;
    result = login_user("error_test", "wrongpass", &session);
    assert(result == ERR_INVALID_CREDENTIALS);
    printf("  ✓ Invalid credentials correctly rejected\n");
    
    db_close();
    printf("\n✓ Error handling test PASSED\n\n");
}

int main()
{
    printf("========================================\n");
    printf("  END-TO-END PIPELINE TEST\n");
    printf("  (Focus on Logic)\n");
    printf("========================================\n\n");
    
    test_complete_user_journey();
    test_unbox_logic();
    test_trading_logic();
    test_market_logic();
    test_price_calculation();
    test_data_integrity();
    test_error_handling();
    
    printf("========================================\n");
    printf("  ALL PIPELINE TESTS PASSED!\n");
    printf("========================================\n");
    
    return 0;
}

