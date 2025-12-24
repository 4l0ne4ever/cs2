// trade_analytics.c - Trade History & Analytics Implementation

#include "../include/trade_analytics.h"
#include "../include/database.h"
#include "../include/database_internal.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Get trade history for a user
int get_trade_history(int user_id, TransactionLog *out_logs, int *count, int limit)
{
    if (!out_logs || !count || user_id <= 0 || limit <= 0)
        return -1;
    
    if (limit > 100)
        limit = 100;
    
    const char *sql = "SELECT log_id, type, user_id, details, timestamp "
                      "FROM transaction_logs "
                      "WHERE user_id = ? AND (type = ? OR type = ? OR type = ?) "
                      "ORDER BY timestamp DESC LIMIT ?";
    sqlite3_stmt *stmt;
    sqlite3 *db = db_get_connection();
    if (!db)
        return -1;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        *count = 0;
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, LOG_MARKET_BUY);
    sqlite3_bind_int(stmt, 3, LOG_MARKET_SELL);
    sqlite3_bind_int(stmt, 4, LOG_TRADE);
    sqlite3_bind_int(stmt, 5, limit);
    
    int idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < limit)
    {
        out_logs[idx].log_id = sqlite3_column_int(stmt, 0);
        out_logs[idx].type = (LogType)sqlite3_column_int(stmt, 1);
        out_logs[idx].user_id = sqlite3_column_int(stmt, 2);
        const char *details = (const char *)sqlite3_column_text(stmt, 3);
        strncpy(out_logs[idx].details, details, 255);
        out_logs[idx].details[255] = '\0';
        out_logs[idx].timestamp = sqlite3_column_int64(stmt, 4);
        idx++;
    }
    
    *count = idx;
    sqlite3_finalize(stmt);
    return 0;
}

// Calculate trade statistics
int calculate_trade_stats(int user_id, TradeStats *out_stats)
{
    if (!out_stats || user_id <= 0)
        return -1;
    
    memset(out_stats, 0, sizeof(TradeStats));
    out_stats->user_id = user_id;
    
    // Get all trade-related transactions
    TransactionLog logs[200];
    int log_count = 0;
    if (get_trade_history(user_id, logs, &log_count, 200) != 0)
        return -1;
    
    int buy_count = 0;
    int sell_count = 0;
    float total_buy = 0.0f;
    float total_sell = 0.0f;
    float max_profit = -999999.0f;
    float max_loss = 999999.0f;
    int profitable_trades = 0;
    int total_trades = 0;
    
    // Parse transaction logs to extract prices
    for (int i = 0; i < log_count; i++)
    {
        if (logs[i].type == LOG_MARKET_BUY)
        {
            // Parse: "Bought instance X for $Y"
            float price = 0.0f;
            if (sscanf(logs[i].details, "Bought instance %*d for $%f", &price) == 1)
            {
                buy_count++;
                total_buy += price;
            }
        }
        else if (logs[i].type == LOG_MARKET_SELL)
        {
            // Parse: "Sold instance X for $Y (received $Z after fee..."
            float price = 0.0f;
            if (sscanf(logs[i].details, "Sold instance %*d for $%f", &price) == 1)
            {
                sell_count++;
                total_sell += price;
                
                // Extract received amount (after fee)
                float received = 0.0f;
                if (sscanf(logs[i].details, "Sold instance %*d for $%*f (received $%f", &received) == 1)
                {
                    // Simplified: assume profit = received - (estimated buy price)
                    // In full implementation, we'd track buy prices
                    float estimated_profit = received * 0.5f; // Simplified
                    if (estimated_profit > max_profit)
                        max_profit = estimated_profit;
                    if (estimated_profit < max_loss)
                        max_loss = estimated_profit;
                    
                    if (estimated_profit > 0)
                        profitable_trades++;
                    total_trades++;
                }
            }
        }
        else if (logs[i].type == LOG_TRADE)
        {
            total_trades++;
        }
    }
    
    out_stats->trades_completed = total_trades;
    out_stats->items_bought = buy_count;
    out_stats->items_sold = sell_count;
    out_stats->avg_buy_price = (buy_count > 0) ? (total_buy / buy_count) : 0.0f;
    out_stats->avg_sell_price = (sell_count > 0) ? (total_sell / sell_count) : 0.0f;
    out_stats->net_profit = total_sell - total_buy; // Simplified
    out_stats->best_trade_profit = (max_profit > -999999.0f) ? max_profit : 0.0f;
    out_stats->worst_trade_loss = (max_loss < 999999.0f) ? max_loss : 0.0f;
    out_stats->win_rate = (total_trades > 0) ? ((float)profitable_trades / total_trades * 100.0f) : 0.0f;
    
    return 0;
}

// Get balance history (last N days)
int get_balance_history(int user_id, BalanceHistoryEntry *out_history, int *count, int days)
{
    if (!out_history || !count || user_id <= 0 || days <= 0)
        return -1;
    
    if (days > 30)
        days = 30;
    
    // Simplified: We'll use transaction logs to estimate balance changes
    // In full implementation, we'd have a balance_history table
    time_t now = time(NULL);
    time_t start_time = now - (days * 24 * 60 * 60);
    
    const char *sql = "SELECT timestamp, details FROM transaction_logs "
                      "WHERE user_id = ? AND timestamp >= ? "
                      "ORDER BY timestamp ASC";
    sqlite3_stmt *stmt;
    sqlite3 *db = db_get_connection();
    if (!db)
        return -1;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        *count = 0;
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int64(stmt, 2, start_time);
    
    // Get current balance
    User user;
    float current_balance = 0.0f;
    if (db_load_user(user_id, &user) == 0)
    {
        current_balance = user.balance;
    }
    
    // Build balance history by working backwards from current balance
    // This is simplified - in full implementation, we'd track balance snapshots
    int idx = 0;
    float running_balance = current_balance;
    
    // Get transactions in reverse order
    const char *rev_sql = "SELECT timestamp, type, details FROM transaction_logs "
                          "WHERE user_id = ? AND timestamp >= ? "
                          "ORDER BY timestamp DESC";
    sqlite3_stmt *rev_stmt;
    rc = sqlite3_prepare_v2(db, rev_sql, -1, &rev_stmt, 0);
    if (rc == SQLITE_OK)
    {
        sqlite3_bind_int(rev_stmt, 1, user_id);
        sqlite3_bind_int64(rev_stmt, 2, start_time);
        
        // Sample balance at daily intervals
        time_t last_sample = now;
        out_history[idx].timestamp = now;
        out_history[idx].balance = current_balance;
        idx++;
        
        while (sqlite3_step(rev_stmt) == SQLITE_ROW && idx < days)
        {
            time_t log_time = sqlite3_column_int64(rev_stmt, 1);
            int log_type = sqlite3_column_int(rev_stmt, 2);
            const char *details = (const char *)sqlite3_column_text(rev_stmt, 3);
            
            // Adjust balance based on transaction
            if (log_type == LOG_MARKET_BUY)
            {
                float price = 0.0f;
                if (sscanf(details, "Bought instance %*d for $%f", &price) == 1)
                {
                    running_balance += price; // Reverse: add back what was spent
                }
            }
            else if (log_type == LOG_MARKET_SELL)
            {
                float received = 0.0f;
                if (sscanf(details, "Sold instance %*d for $%*f (received $%f", &received) == 1)
                {
                    running_balance -= received; // Reverse: subtract what was received
                }
            }
            
            // Sample at daily intervals
            if (last_sample - log_time >= 24 * 60 * 60 && idx < days)
            {
                out_history[idx].timestamp = log_time;
                out_history[idx].balance = running_balance;
                idx++;
                last_sample = log_time;
            }
        }
        sqlite3_finalize(rev_stmt);
    }
    
    sqlite3_finalize(stmt);
    
    // Reverse array to get chronological order
    for (int i = 0; i < idx / 2; i++)
    {
        BalanceHistoryEntry temp = out_history[i];
        out_history[i] = out_history[idx - 1 - i];
        out_history[idx - 1 - i] = temp;
    }
    
    *count = idx;
    return 0;
}
