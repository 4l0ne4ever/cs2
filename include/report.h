#ifndef REPORT_H
#define REPORT_H

#include "types.h"

// Report threshold: If user has >= this many reports, broadcast warning
#define REPORT_WARNING_THRESHOLD 5

// Core functions:
// Report a user for suspicious behavior
// Returns: 0 on success, negative on error
int report_user(int reporter_id, int reported_id, const char *reason);

// Get all reports for a specific user
// Returns: number of reports loaded, or negative on error
int get_reports_for_user(int user_id, Report *out_reports, int *count);

// Get total report count for a user (for quick threshold check)
// Returns: number of reports, or negative on error
int get_report_count(int user_id);

// Broadcast warning to other users if report count exceeds threshold
// This can be used by server to notify other users during trades
void broadcast_warning(int user_id);

// Check if user should be warned (has >= REPORT_WARNING_THRESHOLD reports)
// Returns: 1 if should warn, 0 if not
int should_warn_user(int user_id);

#endif // REPORT_H

