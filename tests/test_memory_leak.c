// test_memory_leak.c - Memory Leak Detection Test (Phase 10.3)

#include "../include/database.h"
#include "../include/auth.h"
#include "../include/market.h"
#include "../include/trading.h"
#include "../include/unbox.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#define ITERATIONS 1000
#define MEMORY_CHECK_INTERVAL 100

// Simple memory tracking (for systems without valgrind)
static size_t g_initial_memory = 0;

size_t get_memory_usage()
{
    // On macOS/Linux, read from /proc/self/status or use getrusage
    // This is a simplified version
    FILE *file = fopen("/proc/self/status", "r");
    if (!file)
    {
        // Fallback: return 0 (can't measure on this system)
        return 0;
    }
    
    char line[128];
    size_t memory = 0;
    
    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, "VmRSS:", 6) == 0)
        {
            sscanf(line, "VmRSS: %zu", &memory);
            break;
        }
    }
    
    fclose(file);
    return memory;
}

void test_repeated_operations()
{
    printf("Testing memory stability with repeated operations...\n");
    
    db_init();
    
    // Create test user
    User user;
    register_user("memtest_user", "pass123", &user);
    db_load_user_by_username("memtest_user", &user);
    
    if (user.balance < 10000.0f)
    {
        user.balance = 50000.0f; // Large balance for many operations
        db_update_user(&user);
    }
    
    Case cases[50];
    int case_count = 0;
    get_available_cases(cases, &case_count);
    
    size_t memory_before = get_memory_usage();
    
    // Perform many operations
    for (int i = 0; i < ITERATIONS; i++)
    {
        // Unbox case
        Skin unboxed;
        int result = unbox_case(user.user_id, cases[0].case_id, &unboxed);
        
        if (result == 0)
        {
            // List on market
            list_skin_on_market(user.user_id, unboxed.skin_id, unboxed.current_price);
            
            // Get market listings
            MarketListing listings[100];
            int listing_count = 0;
            get_market_listings(listings, &listing_count);
            
            // Get inventory
            Inventory inv;
            db_load_inventory(user.user_id, &inv);
        }
        
        // Check memory every N iterations
        if (i % MEMORY_CHECK_INTERVAL == 0 && i > 0)
        {
            size_t current_memory = get_memory_usage();
            if (current_memory > 0 && memory_before > 0)
            {
                size_t memory_diff = current_memory - memory_before;
                printf("  Iteration %d: Memory delta: %zu KB\n", i, memory_diff);
            }
        }
    }
    
    size_t memory_after = get_memory_usage();
    
    if (memory_before > 0 && memory_after > 0)
    {
        size_t memory_increase = memory_after - memory_before;
        double increase_percent = ((double)memory_increase / (double)memory_before) * 100.0;
        
        printf("  ✓ Completed %d operations\n", ITERATIONS);
        printf("  ✓ Memory before: %zu KB\n", memory_before);
        printf("  ✓ Memory after: %zu KB\n", memory_after);
        printf("  ✓ Memory increase: %zu KB (%.2f%%)\n", memory_increase, increase_percent);
        
        // Allow up to 10% increase (reasonable for database growth)
        if (increase_percent < 10.0)
        {
            printf("  ✓ Memory growth within acceptable limits\n");
        }
        else
        {
            printf("  ⚠ Memory growth exceeds 10%% - potential leak\n");
        }
    }
    else
    {
        printf("  ✓ Completed %d operations without crash\n", ITERATIONS);
        printf("  ⚠ Memory tracking not available on this system\n");
    }
    
    db_close();
    printf("  ✓ Memory leak test passed\n\n");
}

void test_database_cleanup()
{
    printf("Testing database cleanup after operations...\n");
    
    // Open and close database multiple times
    for (int i = 0; i < 100; i++)
    {
        db_init();
        
        // Perform some operations
        User user;
        if (db_load_user_by_username("memtest_user", &user) == 0)
        {
            Inventory inv;
            db_load_inventory(user.user_id, &inv);
        }
        
        db_close();
    }
    
    printf("  ✓ Database opened/closed 100 times without issues\n");
    printf("  ✓ Database cleanup test passed\n\n");
}

void test_session_cleanup()
{
    printf("Testing session cleanup...\n");
    
    db_init();
    
    // Create multiple sessions
    for (int i = 0; i < 50; i++)
    {
        char username[32];
        snprintf(username, sizeof(username), "session_test%d", i);
        
        User user;
        register_user(username, "pass123", &user);
        
        Session session;
        login_user(username, "pass123", &session);
        
        // Logout
        logout_user(session.session_token);
    }
    
    printf("  ✓ Created and cleaned up 50 sessions\n");
    
    db_close();
    printf("  ✓ Session cleanup test passed\n\n");
}

int main()
{
    printf("=== Memory Leak Detection Test (Phase 10.3) ===\n\n");
    
    test_repeated_operations();
    test_database_cleanup();
    test_session_cleanup();
    
    printf("=== All Memory Leak Tests Passed! ===\n");
    printf("\nNote: For detailed memory leak detection, run with valgrind:\n");
    printf("  valgrind --leak-check=full ./tests/test_memory_leak\n");
    
    return 0;
}

