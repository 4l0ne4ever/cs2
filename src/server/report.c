// report.c - Report System Implementation (Phase 7)

#include "../include/report.h"
#include "../include/database.h"
#include "../include/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Report a user for suspicious behavior
int report_user(int reporter_id, int reported_id, const char *reason)
{
    // Validate inputs
    if (reporter_id <= 0 || reported_id <= 0 || !reason)
        return -1;

    // Cannot report yourself
    if (reporter_id == reported_id)
        return -2;

    // Validate users exist
    User reporter, reported;
    if (db_load_user(reporter_id, &reporter) != 0)
        return -3; // Reporter not found
    if (db_load_user(reported_id, &reported) != 0)
        return -4; // Reported user not found

    // Validate reason length
    if (strlen(reason) == 0 || strlen(reason) > 255)
        return -5; // Invalid reason

    // Create report
    Report report;
    report.report_id = 0; // Will be set by db_save_report
    report.reporter_id = reporter_id;
    report.reported_id = reported_id;
    strncpy(report.reason, reason, sizeof(report.reason) - 1);
    report.reason[sizeof(report.reason) - 1] = '\0';
    report.created_at = time(NULL);
    report.is_resolved = 0;

    // Save to database
    if (db_save_report(&report) != 0)
        return -6; // Failed to save report

    // Log transaction
    TransactionLog log;
    log.log_id = 0;
    log.type = LOG_TRADE; // Use LOG_TRADE as closest match
    log.user_id = reporter_id;
    snprintf(log.details, sizeof(log.details),
             "User %d reported user %d: %s", reporter_id, reported_id, reason);
    log.timestamp = time(NULL);
    db_log_transaction(&log);

    // Check if we should broadcast warning
    int report_count = db_get_report_count(reported_id);
    if (report_count >= REPORT_WARNING_THRESHOLD)
    {
        broadcast_warning(reported_id);
    }

    return 0;
}

// Get all reports for a specific user
int get_reports_for_user(int user_id, Report *out_reports, int *count)
{
    if (user_id <= 0 || !out_reports || !count)
        return -1;

    return db_load_reports_for_user(user_id, out_reports, count);
}

// Get total report count for a user (for quick threshold check)
int get_report_count(int user_id)
{
    if (user_id <= 0)
        return -1;

    return db_get_report_count(user_id);
}

// Check if user should be warned (has >= REPORT_WARNING_THRESHOLD reports)
int should_warn_user(int user_id)
{
    if (user_id <= 0)
        return 0;

    int count = db_get_report_count(user_id);
    return (count >= REPORT_WARNING_THRESHOLD) ? 1 : 0;
}

// Broadcast warning to other users if report count exceeds threshold
// This can be used by server to notify other users during trades
void broadcast_warning(int user_id)
{
    if (user_id <= 0)
        return;

    User user;
    if (db_load_user(user_id, &user) != 0)
        return; // User not found

    int report_count = db_get_report_count(user_id);
    if (report_count < REPORT_WARNING_THRESHOLD)
        return; // Below threshold, no warning needed

    // Log warning broadcast
    TransactionLog log;
    log.log_id = 0;
    log.type = LOG_TRADE;
    log.user_id = user_id;
    snprintf(log.details, sizeof(log.details),
             "WARNING: User %d (%s) has %d reports. Exercise caution when trading.",
             user_id, user.username, report_count);
    log.timestamp = time(NULL);
    db_log_transaction(&log);

    // In a real server implementation, you would:
    // 1. Get all active sessions
    // 2. Send warning message to all connected clients
    // 3. Include user_id and report_count in broadcast
    // For now, this is logged and can be retrieved by clients via transaction log
}

