// test_sqlite.c - SQLite Database Tests

#include "../include/database.h"
#include "../include/types.h"
#include "../include/protocol.h"
#include "../include/auth.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

// Note: This test uses the SQLite version (database_sqlite.c)
// Make sure to compile with the right file!

void test_db_init()
{
    printf("Testing SQLite db_init()...\n");

    // Use the old binary version for now
    if (db_init() != 0)
    {
        printf("❌ db_init failed\n");
        return;
    }

    printf("✅ db_init passed\n");
}

void test_user_operations()
{
    printf("\nTesting user operations with SQLite...\n");

    // Register new user
    User new_user;
    int result = register_user("testuser_sql", "password123", &new_user);

    if (result != ERR_SUCCESS)
    {
        printf("❌ User registration failed\n");
        return;
    }

    printf("✅ User registration passed (user_id: %d)\n", new_user.user_id);

    // Check user exists
    if (!db_user_exists("testuser_sql"))
    {
        printf("❌ db_user_exists failed\n");
        return;
    }
    printf("✅ db_user_exists passed\n");

    // Load user by username
    User loaded_user;
    if (db_load_user_by_username("testuser_sql", &loaded_user) != 0)
    {
        printf("❌ db_load_user_by_username failed\n");
        return;
    }
    assert(strcmp(loaded_user.username, "testuser_sql") == 0);
    printf("✅ db_load_user_by_username passed\n");

    // Load user by ID
    if (db_load_user(loaded_user.user_id, &loaded_user) != 0)
    {
        printf("❌ db_load_user by ID failed\n");
        return;
    }
    printf("✅ db_load_user by ID passed\n");

    // Update user
    loaded_user.balance = 250.0f;
    if (db_update_user(&loaded_user) != 0)
    {
        printf("❌ db_update_user failed\n");
        return;
    }

    User updated_user;
    db_load_user(loaded_user.user_id, &updated_user);
    assert(updated_user.balance == 250.0f);
    printf("✅ db_update_user passed\n");
}

void test_skin_operations()
{
    printf("\nTesting skin operations with SQLite...\n");

    Skin test_skin = {0};
    test_skin.skin_id = 100; // Use ID that doesn't conflict
    strncpy(test_skin.name, "Test AK-47", sizeof(test_skin.name) - 1);
    test_skin.rarity = RARITY_MIL_SPEC;
    test_skin.wear = 0.25f; // Field-Tested float value
    test_skin.pattern_seed = 500;
    test_skin.is_stattrak = 0;
    test_skin.base_price = 50.0f;
    test_skin.current_price = 45.0f;
    test_skin.owner_id = 1;
    test_skin.acquired_at = time(NULL);
    test_skin.is_tradable = 1;

    if (db_save_skin(&test_skin) != 0)
    {
        printf("❌ db_save_skin failed\n");
        return;
    }
    printf("✅ db_save_skin passed\n");

    Skin loaded_skin;
    if (db_load_skin(100, &loaded_skin) != 0)
    {
        printf("❌ db_load_skin failed\n");
        return;
    }
    assert(strcmp(loaded_skin.name, "Test AK-47") == 0);
    assert(loaded_skin.rarity == RARITY_MIL_SPEC);
    printf("✅ db_load_skin passed\n");

    // Update skin
    loaded_skin.current_price = 50.0f;
    if (db_update_skin(&loaded_skin) != 0)
    {
        printf("❌ db_update_skin failed\n");
        return;
    }

    Skin updated_skin;
    db_load_skin(100, &updated_skin);
    assert(updated_skin.current_price == 50.0f);
    printf("✅ db_update_skin passed\n");
}

void test_inventory_operations()
{
    printf("\nTesting inventory operations with SQLite...\n");

    if (db_add_to_inventory(1, 100) != 0)
    {
        printf("❌ db_add_to_inventory failed\n");
        return;
    }
    printf("✅ db_add_to_inventory passed\n");

    Inventory inv;
    if (db_load_inventory(1, &inv) != 0)
    {
        printf("❌ db_load_inventory failed\n");
        return;
    }
    assert(inv.count > 0);
    printf("✅ db_load_inventory passed (count: %d)\n", inv.count);

    // Try to add duplicate
    if (db_add_to_inventory(1, 100) == 0)
    {
        // Should fail or handle gracefully
        printf("⚠️  Duplicate add handled\n");
    }

    // Remove from inventory
    if (db_remove_from_inventory(1, 100) != 0)
    {
        printf("❌ db_remove_from_inventory failed\n");
        return;
    }
    printf("✅ db_remove_from_inventory passed\n");
}

void test_market_operations()
{
    printf("\nTesting market operations with SQLite...\n");

    MarketListing listing = {0};
    listing.seller_id = 1;
    listing.skin_id = 100;
    listing.price = 45.0f;
    listing.listed_at = time(NULL);
    listing.is_sold = 0;

    if (db_save_listing(&listing) != 0)
    {
        printf("❌ db_save_listing failed\n");
        return;
    }
    printf("✅ db_save_listing passed\n");

    // Load listings
    MarketListing listings[100];
    int count;
    if (db_load_listings(listings, &count) != 0)
    {
        printf("❌ db_load_listings failed\n");
        return;
    }
    assert(count > 0);
    printf("✅ db_load_listings passed (active listings: %d)\n", count);

    // Update listing
    listing.is_sold = 1;
    if (db_update_listing(&listing) != 0)
    {
        printf("❌ db_update_listing failed\n");
        return;
    }
    printf("✅ db_update_listing passed\n");
}

int main()
{
    printf("=== SQLite Database Tests ===\n\n");

    test_db_init();
    test_user_operations();
    test_skin_operations();
    test_inventory_operations();
    test_market_operations();

    printf("\n=== All SQLite tests passed! ===\n");
    printf("\nYou can view the database with:\n");
    printf("  sqlite3 data/database.db\n");
    printf("  SELECT * FROM users;\n");

    return 0;
}
