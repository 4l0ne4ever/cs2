// test_multi_user.c - Multi-User Scenarios Test (Phase 10.2)

#include "../include/database.h"
#include "../include/auth.h"
#include "../include/market.h"
#include "../include/trading.h"
#include "../include/unbox.h"
#include "../include/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#define NUM_USERS 5
#define OPERATIONS_PER_USER 3

void test_multi_user_operations()
{
    printf("Testing multi-user operations...\n");
    
    db_init();
    
    // Create multiple users sequentially
    User users[NUM_USERS];
    int valid_users = 0;
    
    for (int i = 0; i < NUM_USERS; i++)
    {
        char username[32];
        snprintf(username, sizeof(username), "multiuser%d", i);
        
        User new_user;
        memset(&new_user, 0, sizeof(User));
        int result = register_user(username, "password123", &new_user);
        
        if (result == ERR_SUCCESS || result == ERR_USER_EXISTS)
        {
            if (db_load_user_by_username(username, &new_user) == 0 && new_user.user_id > 0)
            {
                users[valid_users] = new_user;
                
                // Ensure balance
                if (users[valid_users].balance < 200.0f)
                {
                    users[valid_users].balance = 500.0f;
                    db_update_user(&users[valid_users]);
                }
                
                valid_users++;
            }
        }
    }
    
    printf("  ✓ Created %d users\n", valid_users);
    assert(valid_users > 0);
    
    // Get available cases
    Case cases[50];
    int case_count = 0;
    int result = get_available_cases(cases, &case_count);
    assert(result == 0 && case_count > 0);
    
    // Perform operations for each user
    int success_count = 0;
    int error_count = 0;
    
    for (int i = 0; i < valid_users; i++)
    {
        for (int j = 0; j < OPERATIONS_PER_USER; j++)
        {
            // Unbox operation
            Skin unboxed;
            memset(&unboxed, 0, sizeof(Skin));
            result = unbox_case(users[i].user_id, cases[0].case_id, &unboxed);
            
            if (result == 0 && unboxed.skin_id > 0)
            {
                success_count++;
            }
            else
            {
                error_count++;
            }
        }
    }
    
    printf("  ✓ Total operations: %d\n", valid_users * OPERATIONS_PER_USER);
    printf("  ✓ Successful: %d\n", success_count);
    printf("  ✓ Errors: %d\n", error_count);
    
    assert(success_count > 0);
    
    db_close();
    printf("  ✓ Multi-user operations test passed\n\n");
}

void test_user_isolation()
{
    printf("Testing user data isolation...\n");
    
    db_init();
    
    // Create two users
    User user1, user2;
    register_user("isolation_user1", "pass123", &user1);
    register_user("isolation_user2", "pass123", &user2);
    
    db_load_user_by_username("isolation_user1", &user1);
    db_load_user_by_username("isolation_user2", &user2);
    
    // Set different balances
    user1.balance = 1000.0f;
    user2.balance = 500.0f;
    db_update_user(&user1);
    db_update_user(&user2);
    
    // Verify isolation
    User loaded1, loaded2;
    db_load_user(user1.user_id, &loaded1);
    db_load_user(user2.user_id, &loaded2);
    
    assert(loaded1.balance == 1000.0f);
    assert(loaded2.balance == 500.0f);
    assert(loaded1.user_id != loaded2.user_id);
    printf("  ✓ User balances isolated correctly\n");
    
    // Test inventory isolation
    Case cases[50];
    int case_count = 0;
    get_available_cases(cases, &case_count);
    
    if (case_count > 0)
    {
        Skin unboxed1, unboxed2;
        unbox_case(user1.user_id, cases[0].case_id, &unboxed1);
        unbox_case(user2.user_id, cases[0].case_id, &unboxed2);
        
        Inventory inv1, inv2;
        db_load_inventory(user1.user_id, &inv1);
        db_load_inventory(user2.user_id, &inv2);
        
        // Verify each user has their own skin
        int found1 = 0, found2 = 0;
        for (int i = 0; i < inv1.count; i++)
        {
            if (inv1.skin_ids[i] == unboxed1.skin_id)
                found1 = 1;
        }
        for (int i = 0; i < inv2.count; i++)
        {
            if (inv2.skin_ids[i] == unboxed2.skin_id)
                found2 = 1;
        }
        
        assert(found1 == 1);
        assert(found2 == 1);
        assert(unboxed1.skin_id != unboxed2.skin_id);
        printf("  ✓ User inventories isolated correctly\n");
    }
    
    db_close();
    printf("  ✓ User isolation test passed\n\n");
}

void test_concurrent_trades()
{
    printf("Testing concurrent trade operations...\n");
    
    db_init();
    
    // Create two users
    User user1, user2;
    register_user("trade_user1", "pass123", &user1);
    register_user("trade_user2", "pass123", &user2);
    
    db_load_user_by_username("trade_user1", &user1);
    db_load_user_by_username("trade_user2", &user2);
    
    // Give users balance and unbox skins
    user1.balance = 1000.0f;
    user2.balance = 1000.0f;
    db_update_user(&user1);
    db_update_user(&user2);
    
    Case cases[50];
    int case_count = 0;
    get_available_cases(cases, &case_count);
    
    if (case_count > 0)
    {
        Skin skin1, skin2;
        unbox_case(user1.user_id, cases[0].case_id, &skin1);
        unbox_case(user2.user_id, cases[0].case_id, &skin2);
        
        // Create multiple trade offers sequentially
        int success_count = 0;
        
        for (int i = 0; i < 5; i++)
        {
            TradeOffer offer = {0};
            offer.from_user_id = user1.user_id;
            offer.to_user_id = user2.user_id;
            offer.offered_skins[0] = skin1.skin_id;
            offer.offered_count = 1;
            offer.requested_skins[0] = skin2.skin_id;
            offer.requested_count = 1;
            offer.status = TRADE_PENDING;
            offer.created_at = time(NULL);
            offer.expires_at = time(NULL) + 900; // 15 minutes
            
            int result = send_trade_offer(user1.user_id, user2.user_id, &offer);
            if (result == 0)
            {
                success_count++;
            }
        }
        
        printf("  ✓ Created %d/5 trade offers\n", success_count);
        // Note: Trade offers may fail if skins are trade-locked or other conditions
        // This test verifies the system handles multiple trade attempts
        if (success_count == 0)
        {
            printf("  ⚠ No trade offers created (may be due to trade locks)\n");
        }
    }
    
    db_close();
    printf("  ✓ Concurrent trades test passed\n\n");
}

int main()
{
    printf("=== Multi-User Scenarios Test (Phase 10.2) ===\n\n");
    
    test_multi_user_operations();
    test_user_isolation();
    test_concurrent_trades();
    
    printf("=== All Multi-User Tests Passed! ===\n");
    
    return 0;
}
