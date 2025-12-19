// test_system_integration.c - Complete System Integration Test (Phase 10)

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

void test_complete_workflow()
{
    printf("Testing complete user workflow...\n");
    
    db_init();
    
    // 1. Register user
    User user;
    int result = register_user("workflow_test", "password123", &user);
    assert(result == ERR_SUCCESS || result == ERR_USER_EXISTS);
    printf("  ✓ User registration\n");
    
    // 2. Login
    Session session;
    result = login_user("workflow_test", "password123", &session);
    assert(result == ERR_SUCCESS);
    printf("  ✓ User login\n");
    
    // 3. Get available cases
    Case cases[50];
    int case_count = 0;
    result = get_available_cases(cases, &case_count);
    assert(result == 0 && case_count > 0);
    printf("  ✓ Get available cases (%d cases)\n", case_count);
    
    // 4. Ensure user has balance
    db_load_user(user.user_id, &user);
    if (user.balance < 100.0f)
    {
        user.balance = 1000.0f;
        db_update_user(&user);
    }
    printf("  ✓ User balance: $%.2f\n", user.balance);
    
    // 5. Unbox a case
    Skin unboxed;
    result = unbox_case(user.user_id, cases[0].case_id, &unboxed);
    assert(result == 0);
    printf("  ✓ Unboxed case: %s\n", unboxed.name);
    
    // 6. Check inventory
    Inventory inv;
    result = db_load_inventory(user.user_id, &inv);
    assert(result == 0);
    assert(inv.count > 0);
    printf("  ✓ Inventory has %d items\n", inv.count);
    
    // 7. List skin on market
    result = list_skin_on_market(user.user_id, unboxed.skin_id, unboxed.current_price);
    assert(result == 0);
    printf("  ✓ Listed skin on market\n");
    
    // 8. Get market listings
    MarketListing listings[100];
    int listing_count = 0;
    result = get_market_listings(listings, &listing_count);
    assert(result == 0);
    printf("  ✓ Market has %d listings\n", listing_count);
    
    db_close();
    printf("  ✓ Complete workflow test passed\n\n");
}

void test_error_handling()
{
    printf("Testing error handling...\n");
    
    db_init();
    
    // Test invalid login
    Session session;
    int result = login_user("nonexistent", "wrongpass", &session);
    assert(result == ERR_INVALID_CREDENTIALS);
    printf("  ✓ Invalid login rejected\n");
    
    // Test duplicate registration
    User user;
    result = register_user("error_test", "pass123", &user);
    assert(result == ERR_SUCCESS);
    result = register_user("error_test", "pass123", &user);
    assert(result == ERR_USER_EXISTS);
    printf("  ✓ Duplicate registration rejected\n");
    
    // Test unbox with insufficient funds
    db_load_user(user.user_id, &user);
    float old_balance = user.balance;
    user.balance = 0.0f;
    db_update_user(&user);
    
    Case cases[50];
    int case_count = 0;
    get_available_cases(cases, &case_count);
    
    Skin unboxed;
    result = unbox_case(user.user_id, cases[0].case_id, &unboxed);
    assert(result == ERR_INSUFFICIENT_FUNDS);
    printf("  ✓ Insufficient funds handled\n");
    
    // Restore balance
    user.balance = old_balance;
    db_update_user(&user);
    
    db_close();
    printf("  ✓ Error handling test passed\n\n");
}

void test_data_integrity()
{
    printf("Testing data integrity...\n");
    
    db_init();
    
    // Create user and unbox
    User user;
    register_user("integrity_test", "pass123", &user);
    db_load_user(user.user_id, &user);
    
    if (user.balance < 100.0f)
    {
        user.balance = 1000.0f;
        db_update_user(&user);
    }
    
    Case cases[50];
    int case_count = 0;
    get_available_cases(cases, &case_count);
    
    float balance_before = user.balance;
    Skin unboxed;
    int result = unbox_case(user.user_id, cases[0].case_id, &unboxed);
    assert(result == 0);
    
    // Check balance was deducted
    db_load_user(user.user_id, &user);
    float balance_after = user.balance;
    float expected_cost = cases[0].price + 2.50f; // Case + Key
    assert(balance_before - balance_after >= expected_cost - 0.01f); // Allow small floating point error
    printf("  ✓ Balance correctly deducted\n");
    
    // Check skin was created
    assert(unboxed.skin_id > 0);
    assert(strlen(unboxed.name) > 0);
    assert(unboxed.wear >= 0.0f && unboxed.wear <= 1.0f);
    assert(unboxed.pattern_seed >= 0 && unboxed.pattern_seed <= 999);
    assert(unboxed.is_stattrak == 0 || unboxed.is_stattrak == 1);
    printf("  ✓ Skin attributes valid\n");
    
    // Check inventory
    Inventory inv;
    db_load_inventory(user.user_id, &inv);
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
    printf("  ✓ Skin added to inventory\n");
    
    db_close();
    printf("  ✓ Data integrity test passed\n\n");
}

int main()
{
    printf("=== Complete System Integration Test ===\n\n");
    
    test_complete_workflow();
    test_error_handling();
    test_data_integrity();
    
    printf("=== All System Integration Tests Passed! ===\n");
    
    return 0;
}

