// add_demo_data.c - Add Demo Users and Market Listings

#include "../include/database.h"
#include "../include/auth.h"
#include "../include/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("=== CS2 Skin Trading - Add Demo Data ===\n\n");

    // Open existing database
    if (db_init() != 0)
    {
        fprintf(stderr, "Failed to open database\n");
        return 1;
    }

    printf("✓ Database opened\n\n");

    // Create demo users
    printf("Creating demo users...\n");

    struct
    {
        const char *username;
        const char *password;
        float balance;
    } demo_users[] = {
        {"player1", "123456", 500.0f},
        {"player2", "123456", 750.0f},
        {"player3", "123456", 1000.0f},
        {"trader1", "123456", 2000.0f},
        {"trader2", "123456", 1500.0f},
        {"richguy", "123456", 5000.0f},
        {"newbie", "123456", 100.0f},
        {"pro", "123456", 3000.0f}};

    int user_ids[8] = {0};
    int user_count = sizeof(demo_users) / sizeof(demo_users[0]);

    for (int i = 0; i < user_count; i++)
    {
        User user = {0};
        user.user_id = 0;
        strncpy(user.username, demo_users[i].username, MAX_USERNAME_LEN - 1);
        hash_password(demo_users[i].password, user.password_hash);
        user.balance = demo_users[i].balance;
        user.created_at = time(NULL) - (3600 * 24 * (i + 1)); // Different creation times
        user.is_banned = 0;

        if (db_save_user(&user) == 0)
        {
            user_ids[i] = user.user_id;
            printf("  ✓ Created user: %s (ID: %d, Balance: $%.2f)\n",
                   user.username, user.user_id, user.balance);
        }
        else
        {
            // User might already exist, try to load
            if (db_load_user_by_username(demo_users[i].username, &user) == 0)
            {
                user_ids[i] = user.user_id;
                // Update balance
                user.balance = demo_users[i].balance;
                db_update_user(&user);
                printf("  ✓ Updated user: %s (ID: %d, Balance: $%.2f)\n",
                       user.username, user.user_id, user.balance);
            }
            else
            {
                printf("  ✗ Failed to create user: %s\n", demo_users[i].username);
            }
        }
    }

    printf("\n✓ Created/Updated %d users\n\n", user_count);

    // Create skin instances for users and add to inventory
    printf("Creating skin instances...\n");

    // Get some skin definitions from database
    sqlite3 *db;
    if (sqlite3_open("data/database.db", &db) != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        db_close();
        return 1;
    }

    // Get skin definitions (get more for variety)
    int definition_ids[100];
    int def_count = 0;
    const char *sql = "SELECT definition_id FROM skin_definitions ORDER BY definition_id LIMIT 100";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW && def_count < 100)
        {
            definition_ids[def_count++] = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);

    printf("  Found %d skin definitions\n", def_count);

    // Create skin instances for each user
    int instance_count = 0;
    int instance_ids[500];    // Store instance IDs for market listings (increased for more listings)
    int instance_owners[500]; // Store owner IDs

    srand(time(NULL));

    for (int u = 0; u < user_count && u < 8; u++)
    {
        int user_id = user_ids[u];
        if (user_id == 0)
            continue;

        // Each user gets 20-30 random skins (ensure all users have items like normal users)
        int skins_per_user = 20 + (rand() % 11);

        for (int s = 0; s < skins_per_user && def_count > 0; s++)
        {
            int def_idx = rand() % def_count;
            int definition_id = definition_ids[def_idx];

            // Load skin definition from database to get actual rarity
            char skin_name[MAX_ITEM_NAME_LEN];
            float base_price;
            SkinRarity rarity;
            if (db_load_skin_definition_with_rarity(definition_id, skin_name, &base_price, &rarity) != 0)
            {
                // Skip if definition not found
                continue;
            }

            // Random wear (0.0 - 0.45 for better prices)
            // Wear is instance-specific, so random is fine
            WearCondition wear = ((float)rand() / (float)RAND_MAX) * 0.45f;

            // Random pattern seed (instance-specific)
            int pattern_seed = rand() % 1001;

            // 10% chance for StatTrak (instance-specific)
            int is_stattrak = (rand() % 10 == 0) ? 1 : 0;

            int instance_id = 0;
            if (db_create_skin_instance(definition_id, rarity, wear, pattern_seed, is_stattrak, user_id, &instance_id) == 0)
            {
                // Note: Items in inventory are NOT trade locked - only items listed on market are locked

                // Add to inventory
                if (db_add_to_inventory(user_id, instance_id) == 0)
                {
                    instance_ids[instance_count] = instance_id;
                    instance_owners[instance_count] = user_id;
                    instance_count++;
                }
            }
        }

        printf("  ✓ User %s: Added %d skins to inventory\n", demo_users[u].username, skins_per_user);
    }

    printf("\n✓ Created %d skin instances\n\n", instance_count);

    // List items on market (aim for 50+ listings)
    printf("Creating market listings...\n");

    int listings_created = 0;
    int target_listings = 60; // Target 60+ listings

    // List items from all users, prioritizing to reach target
    for (int i = 0; i < instance_count && listings_created < target_listings; i++)
    {
        // List more items - every 2nd item or random selection to reach target
        if (i % 2 == 0 || (listings_created < target_listings && rand() % 3 == 0))
        {
            int instance_id = instance_ids[i];
            int owner_id = instance_owners[i];

            // Get skin details to calculate price
            int definition_id;
            SkinRarity rarity;
            WearCondition wear;
            int pattern_seed, is_stattrak;
            time_t acquired_at;
            int is_tradable;

            int temp_owner_id;
            if (db_load_skin_instance(instance_id, &definition_id, &rarity, &wear, &pattern_seed, &is_stattrak, &temp_owner_id, &acquired_at, &is_tradable) == 0)
            {
                // Calculate price from database (ensures sync with UI)
                // Use the actual calculated price, not with markup, to ensure sync
                float market_price = db_calculate_skin_price(definition_id, rarity, wear);

                // Apply trade lock when listing on market (1 day lock)
                db_apply_trade_lock(instance_id);

                // Remove from inventory (item is now on market)
                db_remove_from_inventory(owner_id, instance_id);

                // Create listing with the actual calculated price
                int listing_id = 0;
                if (db_save_listing_v2(owner_id, instance_id, market_price, &listing_id) == 0)
                {
                    listings_created++;
                }
            }
        }
    }

    printf("  ✓ Created %d market listings\n\n", listings_created);

    // Summary
    printf("=== Demo Data Summary ===\n");
    printf("Users created/updated: %d\n", user_count);
    printf("Skin instances created: %d\n", instance_count);
    printf("Market listings created: %d\n", listings_created);
    printf("\n✓ Demo data added successfully!\n");
    printf("\nYou can now login with any of these accounts:\n");
    for (int i = 0; i < user_count; i++)
    {
        printf("  Username: %s, Password: %s, Balance: $%.2f\n",
               demo_users[i].username, demo_users[i].password, demo_users[i].balance);
    }

    sqlite3_close(db);
    db_close();

    return 0;
}
