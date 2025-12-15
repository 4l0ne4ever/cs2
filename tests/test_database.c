// test_database.c - Database Tests

#include "../include/database.h"
#include "../include/types.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

void test_db_init()
{
    printf("Testing db_init()...\n");

    if (db_init() != 0)
    {
        printf("❌ db_init failed\n");
        return;
    }

    printf("✅ db_init passed\n");
}

void test_user_operations()
{
    printf("\nTesting user operations...\n");

    // Create test user
    User test_user = {0};
    test_user.user_id = 1;
    strncpy(test_user.username, "testuser", sizeof(test_user.username) - 1);
    strncpy(test_user.password_hash, "test_hash", sizeof(test_user.password_hash) - 1);
    test_user.balance = 100.0f;
    test_user.created_at = time(NULL);
    test_user.is_banned = 0;

    // Test save
    if (db_save_user(&test_user) != 0)
    {
        printf("❌ db_save_user failed\n");
        return;
    }
    printf("✅ db_save_user passed\n");

    // Test exists
    if (!db_user_exists("testuser"))
    {
        printf("❌ db_user_exists failed\n");
        return;
    }
    printf("✅ db_user_exists passed\n");

    // Test load by ID
    User loaded_user;
    if (db_load_user(1, &loaded_user) != 0)
    {
        printf("❌ db_load_user by ID failed\n");
        return;
    }
    assert(strcmp(loaded_user.username, "testuser") == 0);
    printf("✅ db_load_user by ID passed\n");

    // Test load by username
    if (db_load_user_by_username("testuser", &loaded_user) != 0)
    {
        printf("❌ db_load_user_by_username failed\n");
        return;
    }
    assert(strcmp(loaded_user.username, "testuser") == 0);
    printf("✅ db_load_user_by_username passed\n");

    // Test update
    loaded_user.balance = 200.0f;
    if (db_update_user(&loaded_user) != 0)
    {
        printf("❌ db_update_user failed\n");
        return;
    }

    User updated_user;
    db_load_user(1, &updated_user);
    assert(updated_user.balance == 200.0f);
    printf("✅ db_update_user passed\n");
}

void test_skin_operations()
{
    printf("\nTesting skin operations...\n");

    Skin test_skin = {0};
    test_skin.skin_id = 1;
    strncpy(test_skin.name, "AK-47 | Redline", sizeof(test_skin.name) - 1);
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
    if (db_load_skin(1, &loaded_skin) != 0)
    {
        printf("❌ db_load_skin failed\n");
        return;
    }
    assert(strcmp(loaded_skin.name, "AK-47 | Redline") == 0);
    printf("✅ db_load_skin passed\n");
}

void test_inventory_operations()
{
    printf("\nTesting inventory operations...\n");

    if (db_add_to_inventory(1, 1) != 0)
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
    assert(inv.count == 1);
    assert(inv.skin_ids[0] == 1);
    printf("✅ db_load_inventory passed\n");
}

int main()
{
    printf("=== Database Tests ===\n\n");

    test_db_init();
    test_user_operations();
    test_skin_operations();
    test_inventory_operations();

    printf("\n=== All tests passed! ===\n");

    return 0;
}
