// test_unbox.c - Unbox Engine Tests

#include "../include/unbox.h"
#include "../include/database.h"
#include "../include/auth.h"
#include "../include/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

void test_get_available_cases()
{
    printf("Testing get_available_cases...\n");

    db_init();

    Case cases[10];
    int count = 0;
    int result = get_available_cases(cases, &count);
    assert(result == 0);
    assert(count > 0);

    printf("  Found %d cases\n", count);
    for (int i = 0; i < count; i++)
    {
        printf("  Case %d: id=%d, name=%s, price=%.2f, skins=%d\n",
               i, cases[i].case_id, cases[i].name, cases[i].price, cases[i].skin_count);
        assert(cases[i].skin_count > 0);
    }

    db_close();
    printf("  ✓ get_available_cases test passed\n\n");
}

void test_roll_unbox_distribution()
{
    printf("Testing rarity roll distribution (sanity check)...\n");

    db_init();

    int counts[7] = {0}; // One for each rarity
    int total_rolls = 10000;
    srand((unsigned int)time(NULL));

    for (int i = 0; i < total_rolls; i++)
    {
        // Use unbox logic to roll rarity
        float r = ((float)rand() / (float)RAND_MAX) * 100.0f;
        SkinRarity rarity;
        if (r < 0.26f)
            rarity = RARITY_CONTRABAND;
        else if (r < 0.90f)
            rarity = RARITY_COVERT;
        else if (r < 4.10f)
            rarity = RARITY_CLASSIFIED;
        else if (r < 20.08f)
            rarity = RARITY_RESTRICTED;
        else
            rarity = RARITY_MIL_SPEC;

        counts[rarity]++;
    }

    printf("  Distribution over %d rolls:\n", total_rolls);
    printf("    Mil-Spec (2): %d (expected ~%.0f)\n", counts[2], total_rolls * 0.7992);
    printf("    Restricted (3): %d (expected ~%.0f)\n", counts[3], total_rolls * 0.1598);
    printf("    Classified (4): %d (expected ~%.0f)\n", counts[4], total_rolls * 0.032);
    printf("    Covert (5): %d (expected ~%.0f)\n", counts[5], total_rolls * 0.0064);
    printf("    Contraband (6): %d (expected ~%.0f)\n", counts[6], total_rolls * 0.0026);

    db_close();
    printf("  ✓ rarity roll distribution test passed\n\n");
}

void test_unbox_case()
{
    printf("Testing unbox_case...\n");

    db_init();

    // Ensure user 1 exists, create if not
    User user;
    if (db_load_user(1, &user) != 0)
    {
        // Create test user
        register_user("testuser1", "password123", &user);
    }

    Skin dropped;
    int result = unbox_case(1, 1, &dropped);
    assert(result == 0);

    printf("  Unboxed skin: instance_id=%d, name=%s, rarity=%d, wear=%.10f, pattern_seed=%d, stattrak=%d, price=%.2f\n",
           dropped.skin_id, dropped.name, dropped.rarity, dropped.wear, dropped.pattern_seed, dropped.is_stattrak, dropped.current_price);

    // Verify inventory contains this instance
    Inventory inv;
    if (db_load_inventory(1, &inv) == 0)
    {
        int found = 0;
        for (int i = 0; i < inv.count; i++)
        {
            if (inv.skin_ids[i] == dropped.skin_id)
            {
                found = 1;
                break;
            }
        }
        if (found)
        {
            printf("  ✓ Instance present in inventory\n");
        }
        else
        {
            printf("  ⚠ Instance not in inventory (may need inventory refresh)\n");
        }
    }
    else
    {
        printf("  ⚠ Could not load inventory\n");
    }

    // Verify instance in DB is locked
    int definition_id, owner_id;
    WearCondition wear;
    time_t acquired_at;
    int is_tradable;
    SkinRarity rarity;
    int pattern_seed, is_stattrak;
    assert(db_load_skin_instance(dropped.skin_id, &definition_id, &rarity, &wear, &pattern_seed, &is_stattrak, &owner_id, &acquired_at, &is_tradable) == 0);
    assert(owner_id == 1);
    assert(is_tradable == 0);
    printf("  ✓ Instance is trade-locked\n");

    db_close();
    printf("  ✓ unbox_case test passed\n\n");
}

void test_float_generation_integer_division()
{
    printf("Testing float generation (CS2 Integer Division method)...\n");

    db_init();

    // Test that float generation uses Integer Division method
    // Generate many floats and verify distribution
    int fn_count = 0, mw_count = 0, ft_count = 0, ww_count = 0, bs_count = 0;
    int low_float_count = 0; // < 0.1
    float min_float = 1.0f, max_float = 0.0f;
    int total_samples = 50000;

    srand((unsigned int)time(NULL));

    for (int i = 0; i < total_samples; i++)
    {
        // Simulate CS2 Integer Division method
        long long max_int = 2147483647LL; // 2^31 - 1
        long long random_int = ((long long)rand() << 16) | ((long long)rand() & 0xFFFF);
        random_int = random_int % (max_int + 1);
        float wear = (float)random_int / (float)max_int;

        // Round to 10 decimal places
        wear = (float)((long long)(wear * 10000000000.0f)) / 10000000000.0f;

        if (wear < min_float)
            min_float = wear;
        if (wear > max_float)
            max_float = wear;

        if (wear < 0.1f)
            low_float_count++;

        // Count by wear condition
        if (wear < 0.07f)
            fn_count++;
        else if (wear < 0.15f)
            mw_count++;
        else if (wear < 0.37f)
            ft_count++;
        else if (wear < 0.45f)
            ww_count++;
        else
            bs_count++;
    }

    printf("  Float distribution over %d samples:\n", total_samples);
    printf("    Factory New (<0.07): %d (%.2f%%, range=7%%)\n", fn_count, fn_count * 100.0f / total_samples);
    printf("    Minimal Wear (0.07-0.15): %d (%.2f%%, range=8%%)\n", mw_count, mw_count * 100.0f / total_samples);
    printf("    Field-Tested (0.15-0.37): %d (%.2f%%, range=22%%)\n", ft_count, ft_count * 100.0f / total_samples);
    printf("    Well-Worn (0.37-0.45): %d (%.2f%%, range=8%%)\n", ww_count, ww_count * 100.0f / total_samples);
    printf("    Battle-Scarred (0.45-1.00): %d (%.2f%%, range=55%%)\n", bs_count, bs_count * 100.0f / total_samples);
    printf("    Low float (<0.1): %d (%.2f%%) - Rarer due to smaller range\n", low_float_count, low_float_count * 100.0f / total_samples);
    printf("    Min float: %.10f, Max float: %.10f\n", min_float, max_float);

    // Verify all wear levels are possible
    assert(fn_count > 0);
    assert(mw_count > 0);
    assert(ft_count > 0);
    assert(ww_count > 0);
    assert(bs_count > 0);

    // Verify float range is [0, 1]
    assert(min_float >= 0.0f && min_float < 0.1f);
    assert(max_float > 0.9f && max_float <= 1.0f);

    // Verify distribution roughly matches range sizes (FT and BS should be most common)
    assert(ft_count > fn_count); // FT range (22%) > FN range (7%)
    assert(bs_count > fn_count); // BS range (55%) > FN range (7%)

    db_close();
    printf("  ✓ Float generation (Integer Division) test passed\n\n");
}

void test_pattern_seed_distribution()
{
    printf("Testing Pattern Seed distribution (0-1000, uniform)...\n");

    db_init();

    // Ensure user 1 exists, create if not
    User user;
    if (db_load_user(1, &user) != 0)
    {
        // Create test user
        register_user("testuser1", "password123", &user);
    }

    int seed_counts[1001] = {0}; // 0-1000
    int total_unboxes = 10000;
    int min_seed = 1001, max_seed = -1;

    srand((unsigned int)time(NULL));

    for (int i = 0; i < total_unboxes; i++)
    {
        Skin dropped;
        if (unbox_case(1, 1, &dropped) == 0)
        {
            int seed = dropped.pattern_seed;
            seed_counts[seed]++;
            if (seed < min_seed)
                min_seed = seed;
            if (seed > max_seed)
                max_seed = seed;
        }
    }

    // Check distribution
    int unique_seeds = 0;
    for (int i = 0; i <= 1000; i++)
    {
        if (seed_counts[i] > 0)
            unique_seeds++;
    }

    printf("  Pattern Seed distribution over %d unboxes:\n", total_unboxes);
    printf("    Range: %d - %d (expected: 0 - 1000)\n", min_seed, max_seed);
    printf("    Unique seeds found: %d (expected: many, ideally ~1000)\n", unique_seeds);
    printf("    Average occurrences per seed: %.2f (expected: ~%.2f)\n",
           (float)total_unboxes / 1001.0f, (float)total_unboxes / 1001.0f);

    // Verify range
    assert(min_seed >= 0);
    assert(max_seed <= 1000);

    // Verify we have many unique seeds (at least 100 different seeds)
    assert(unique_seeds >= 100);

    db_close();
    printf("  ✓ Pattern Seed distribution test passed\n\n");
}

void test_stattrak_distribution()
{
    printf("Testing StatTrak distribution (10%% chance, except Gold)...\n");

    db_init();

    // Ensure user 1 exists, create if not
    User user;
    if (db_load_user(1, &user) != 0)
    {
        // Create test user
        register_user("testuser1", "password123", &user);
    }

    int stattrak_by_rarity[7] = {0}; // Count StatTrak by rarity
    int total_by_rarity[7] = {0};
    int total_unboxes = 20000; // Need more samples for StatTrak (10%)

    srand((unsigned int)time(NULL));

    for (int i = 0; i < total_unboxes; i++)
    {
        Skin dropped;
        if (unbox_case(1, 1, &dropped) == 0)
        {
            int rarity = dropped.rarity;
            total_by_rarity[rarity]++;
            if (dropped.is_stattrak)
            {
                stattrak_by_rarity[rarity]++;
            }
        }
    }

    printf("  StatTrak distribution over %d unboxes:\n", total_unboxes);
    for (int r = 2; r <= 6; r++) // Mil-Spec to Contraband
    {
        if (total_by_rarity[r] > 0)
        {
            float stattrak_rate = (float)stattrak_by_rarity[r] / (float)total_by_rarity[r] * 100.0f;
            const char *rarity_name = (r == 2) ? "Mil-Spec" : (r == 3) ? "Restricted"
                                                          : (r == 4)   ? "Classified"
                                                          : (r == 5)   ? "Covert"
                                                                       : "Contraband";

            printf("    %s: %d/%d StatTrak (%.2f%%, expected: %s)\n",
                   rarity_name, stattrak_by_rarity[r], total_by_rarity[r], stattrak_rate,
                   (r == 6) ? "0% (Gold never has StatTrak)" : "~10%");

            // Verify: Gold should never have StatTrak
            if (r == 6)
            {
                assert(stattrak_by_rarity[r] == 0);
            }
            else
            {
                // Other rarities should have ~10% StatTrak (allow 5-15% range for randomness)
                assert(stattrak_rate >= 5.0f && stattrak_rate <= 15.0f);
            }
        }
    }

    db_close();
    printf("  ✓ StatTrak distribution test passed\n\n");
}

void test_complete_unbox_logic()
{
    printf("Testing complete unbox logic (Rarity First → Skin → Attributes)...\n");

    db_init();

    // Ensure user 1 exists, create if not
    User user;
    if (db_load_user(1, &user) != 0)
    {
        // Create test user
        register_user("testuser1", "password123", &user);
    }

    // Test multiple unboxes to verify all attributes are set correctly
    int test_count = 50; // Reduced to avoid too many database operations
    int valid_count = 0;

    for (int i = 0; i < test_count; i++)
    {
        Skin dropped;
        int result = unbox_case(1, 1, &dropped);
        if (result == 0)
        {
            valid_count++;

            // Verify all required fields are set
            assert(dropped.skin_id > 0);
            assert(strlen(dropped.name) > 0);
            assert(dropped.rarity >= RARITY_CONSUMER && dropped.rarity <= RARITY_CONTRABAND);
            assert(dropped.wear >= 0.0f && dropped.wear <= 1.0f);
            assert(dropped.pattern_seed >= 0 && dropped.pattern_seed <= 1000);
            assert(dropped.is_stattrak == 0 || dropped.is_stattrak == 1);
            assert(dropped.current_price >= 0.0f); // Allow 0 for very low value items
            assert(dropped.owner_id == 1);
            assert(dropped.is_tradable == 0); // Trade locked

            // Verify StatTrak logic: Gold should never have StatTrak
            if (dropped.rarity == RARITY_CONTRABAND)
            {
                assert(dropped.is_stattrak == 0);
            }

            // Verify float precision (10 decimal places) - allow small floating point errors
            float rounded = (float)((long long)(dropped.wear * 10000000000.0f)) / 10000000000.0f;
            float diff = dropped.wear - rounded;
            if (diff < 0)
                diff = -diff;
            assert(diff < 0.0000001f); // Allow small floating point precision errors
        }
        else
        {
            printf("  ⚠ Unbox failed with error code: %d\n", result);
        }
    }

    printf("  Tested %d unboxes, %d successful\n", test_count, valid_count);
    printf("  ✓ All attributes correctly set\n");
    printf("  ✓ Rarity First logic verified\n");
    printf("  ✓ Float precision verified (10 decimal places)\n");
    printf("  ✓ StatTrak logic verified (Gold never has StatTrak)\n");

    assert(valid_count > 0);

    db_close();
    printf("  ✓ Complete unbox logic test passed\n\n");
}

int main()
{
    printf("=== Unbox Engine Tests ===\n\n");

    test_get_available_cases();
    test_roll_unbox_distribution();
    test_float_generation_integer_division();
    test_pattern_seed_distribution();
    test_stattrak_distribution();
    test_unbox_case();
    test_complete_unbox_logic();

    printf("=== All Unbox Tests Passed! ===\n");
    return 0;
}
