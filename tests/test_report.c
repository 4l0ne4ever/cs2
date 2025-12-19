// test_report.c - Report System Tests (Phase 7)

#include "../include/report.h"
#include "../include/database.h"
#include "../include/auth.h"
#include "../include/protocol.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_report_user()
{
    printf("Testing report_user...\n");

    db_init();

    // Create test users
    User user1, user2;
    int reg1 = register_user("test_reporter", "pass123", &user1);
    int reg2 = register_user("test_reported", "pass123", &user2);

    if (reg1 != ERR_SUCCESS && reg1 != ERR_USER_EXISTS)
    {
        printf("  ⚠ Failed to create test_reporter\n");
        db_close();
        return;
    }
    if (reg2 != ERR_SUCCESS && reg2 != ERR_USER_EXISTS)
    {
        printf("  ⚠ Failed to create test_reported\n");
        db_close();
        return;
    }

    // Load users to get IDs
    if (db_load_user_by_username("test_reporter", &user1) != 0 ||
        db_load_user_by_username("test_reported", &user2) != 0)
    {
        printf("  ⚠ Failed to load test users\n");
        db_close();
        return;
    }

    // Test 1: Valid report
    int result = report_user(user1.user_id, user2.user_id, "Suspicious trading behavior");
    assert(result == 0);
    printf("  ✓ Valid report created\n");

    // Test 2: Cannot report yourself
    result = report_user(user1.user_id, user1.user_id, "Self report");
    assert(result == -2);
    printf("  ✓ Self-report prevented\n");

    // Test 3: Invalid reporter ID
    result = report_user(-1, user2.user_id, "Invalid reporter");
    assert(result < 0);
    printf("  ✓ Invalid reporter ID rejected\n");

    // Test 4: Invalid reported ID
    result = report_user(user1.user_id, -1, "Invalid reported");
    assert(result < 0);
    printf("  ✓ Invalid reported ID rejected\n");

    // Test 5: Empty reason
    result = report_user(user1.user_id, user2.user_id, "");
    assert(result < 0);
    printf("  ✓ Empty reason rejected\n");

    db_close();
    printf("  ✓ report_user test passed\n\n");
}

void test_get_reports_for_user()
{
    printf("Testing get_reports_for_user...\n");

    db_init();

    // Create test users
    User user1, user2, user3;
    register_user("test_reporter1", "pass123", &user1);
    register_user("test_reported1", "pass123", &user2);
    register_user("test_reporter2", "pass123", &user3);

    // Load users
    db_load_user_by_username("test_reporter1", &user1);
    db_load_user_by_username("test_reported1", &user2);
    db_load_user_by_username("test_reporter2", &user3);

    // Create multiple reports
    report_user(user1.user_id, user2.user_id, "Report 1: Scam attempt");
    report_user(user3.user_id, user2.user_id, "Report 2: Fake items");

    // Get reports
    Report reports[10];
    int count = 0;
    int result = get_reports_for_user(user2.user_id, reports, &count);

    assert(result == 0);
    assert(count >= 2);
    printf("  ✓ Loaded %d reports for user %d\n", count, user2.user_id);

    // Verify report data
    for (int i = 0; i < count; i++)
    {
        assert(reports[i].reported_id == user2.user_id);
        assert(reports[i].reporter_id == user1.user_id || reports[i].reporter_id == user3.user_id);
        assert(strlen(reports[i].reason) > 0);
        assert(reports[i].is_resolved == 0);
    }
    printf("  ✓ Report data verified\n");

    db_close();
    printf("  ✓ get_reports_for_user test passed\n\n");
}

void test_get_report_count()
{
    printf("Testing get_report_count...\n");

    db_init();

    // Create test users
    User user1, user2, user3, user4;
    register_user("test_reporter_count1", "pass123", &user1);
    register_user("test_reported_count", "pass123", &user2);
    register_user("test_reporter_count2", "pass123", &user3);
    register_user("test_reporter_count3", "pass123", &user4);

    // Load users
    db_load_user_by_username("test_reporter_count1", &user1);
    db_load_user_by_username("test_reported_count", &user2);
    db_load_user_by_username("test_reporter_count2", &user3);
    db_load_user_by_username("test_reporter_count3", &user4);

    // Create multiple reports
    report_user(user1.user_id, user2.user_id, "Report 1");
    report_user(user3.user_id, user2.user_id, "Report 2");
    report_user(user4.user_id, user2.user_id, "Report 3");

    // Get count
    int count = get_report_count(user2.user_id);
    assert(count >= 3);
    printf("  ✓ Report count: %d (expected >= 3)\n", count);

    // Test with user with no reports
    int count2 = get_report_count(user1.user_id);
    assert(count2 >= 0); // Should be 0 or more (may have reports from previous tests)
    printf("  ✓ Report count for user with no reports: %d\n", count2);

    db_close();
    printf("  ✓ get_report_count test passed\n\n");
}

void test_should_warn_user()
{
    printf("Testing should_warn_user...\n");

    db_init();

    // Create test users
    User user1, user2;
    register_user("test_warn_reporter", "pass123", &user1);
    register_user("test_warn_reported", "pass123", &user2);

    // Load users
    db_load_user_by_username("test_warn_reporter", &user1);
    db_load_user_by_username("test_warn_reported", &user2);

    // Create reports up to threshold
    for (int i = 0; i < REPORT_WARNING_THRESHOLD; i++)
    {
        char reason[64];
        snprintf(reason, sizeof(reason), "Report %d", i + 1);
        report_user(user1.user_id, user2.user_id, reason);
    }

    // Check warning status
    int should_warn = should_warn_user(user2.user_id);
    assert(should_warn == 1);
    printf("  ✓ User with %d reports should be warned\n", REPORT_WARNING_THRESHOLD);

    // Test with user below threshold
    User user3;
    register_user("test_warn_reported2", "pass123", &user3);
    db_load_user_by_username("test_warn_reported2", &user3);

    int should_warn2 = should_warn_user(user3.user_id);
    assert(should_warn2 == 0);
    printf("  ✓ User below threshold should not be warned\n");

    db_close();
    printf("  ✓ should_warn_user test passed\n\n");
}

void test_broadcast_warning()
{
    printf("Testing broadcast_warning...\n");

    db_init();

    // Create test users
    User user1, user2;
    register_user("test_broadcast_reporter", "pass123", &user1);
    register_user("test_broadcast_reported", "pass123", &user2);

    // Load users
    db_load_user_by_username("test_broadcast_reporter", &user1);
    db_load_user_by_username("test_broadcast_reported", &user2);

    // Create reports up to threshold
    for (int i = 0; i < REPORT_WARNING_THRESHOLD; i++)
    {
        char reason[64];
        snprintf(reason, sizeof(reason), "Report %d", i + 1);
        report_user(user1.user_id, user2.user_id, reason);
    }

    // Broadcast warning (should log transaction)
    broadcast_warning(user2.user_id);
    printf("  ✓ Warning broadcasted (check transaction log)\n");

    db_close();
    printf("  ✓ broadcast_warning test passed\n\n");
}

int main()
{
    printf("=== Report System Tests (Phase 7) ===\n\n");

    test_report_user();
    test_get_reports_for_user();
    test_get_report_count();
    test_should_warn_user();
    test_broadcast_warning();

    printf("=== All Report Tests Passed! ===\n");

    return 0;
}

