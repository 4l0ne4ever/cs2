#ifndef TRADE_ANALYTICS_H
#define TRADE_ANALYTICS_H

#include "types.h"
#include <time.h>

// Get trade history for a user
int get_trade_history(int user_id, TransactionLog *out_logs, int *count, int limit);

// Calculate trade statistics
int calculate_trade_stats(int user_id, TradeStats *out_stats);

// Get balance history (last N days)
int get_balance_history(int user_id, BalanceHistoryEntry *out_history, int *count, int days);

#endif // TRADE_ANALYTICS_H
