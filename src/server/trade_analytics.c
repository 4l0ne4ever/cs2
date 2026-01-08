// trade_analytics.c - Trade History & Analytics Implementation

#include "../include/trade_analytics.h"
#include "../include/database.h"
#include "../include/database_internal.h"
#include "../include/logger.h"
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
    
    // Get all relevant transaction logs (market buys, market sells, completed trades, unbox)
    // Note: We filter out "Sent trade offer" logs in the result since they're not completed trades
    const char *sql = "SELECT log_id, type, user_id, details, timestamp "
                      "FROM transaction_logs "
                      "WHERE user_id = ? AND (type = ? OR type = ? OR type = ? OR type = ?) "
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
    sqlite3_bind_int(stmt, 5, LOG_UNBOX);
    sqlite3_bind_int(stmt, 6, limit * 2); // Get more to filter out "Sent trade offer" logs
    
    int idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < limit)
    {
        LogType log_type = (LogType)sqlite3_column_int(stmt, 1);
        const char *details = (const char *)sqlite3_column_text(stmt, 3);
        
        // Filter out "Sent trade offer", "Declined", and "Cancelled" logs - only show completed trades
        if (log_type == LOG_TRADE && details)
        {
            if (strstr(details, "Sent trade offer") != NULL ||
                strstr(details, "Declined trade offer") != NULL ||
                strstr(details, "Cancelled trade offer") != NULL)
            {
                continue; // Skip non-completed trade logs
            }
            // Only include "Accepted trade offer" logs (completed trades)
            if (strstr(details, "Accepted trade offer") == NULL)
            {
                continue; // Skip if not an accepted trade
            }
        }
        
        out_logs[idx].log_id = sqlite3_column_int(stmt, 0);
        out_logs[idx].type = log_type;
        out_logs[idx].user_id = sqlite3_column_int(stmt, 2);
        strncpy(out_logs[idx].details, details ? details : "", 255);
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
    
    // Separate counters for different transaction types
    int buy_count = 0;           // Market buys only
    int sell_count = 0;          // Market sells only
    int peer_trades_count = 0;   // Peer-to-peer trades only (LOG_TRADE)
    
    float total_buy = 0.0f;      // Total spent on market buys
    float total_sell = 0.0f;     // Total received from market sells (actual received, not listing price)
    float total_trade_gave = 0.0f;    // Total value given in peer-to-peer trades
    float total_trade_received = 0.0f; // Total value received in peer-to-peer trades
    
    float max_profit = -999999.0f;
    float max_loss = 999999.0f;
    int profitable_trades = 0;
    int total_trades = 0;        // Only peer-to-peer trades (LOG_TRADE)
    
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
            // Parse: "Sold instance X for $Y (received $Z after fee, +$W listing fee refund)"
            int instance_id = 0;
            float price = 0.0f;
            float received = 0.0f;
            float listing_fee_refund = 0.0f;
            
            if (sscanf(logs[i].details, "Sold instance %d for $%f (received $%f after fee, +$%f listing fee refund)", 
                       &instance_id, &price, &received, &listing_fee_refund) == 4 ||
                sscanf(logs[i].details, "Sold instance %d for $%f (received $%f", 
                       &instance_id, &price, &received) >= 3)
            {
                sell_count++;
                
                // Calculate actual received (after fee, including listing fee refund)
                float actual_received = received + listing_fee_refund;
                total_sell += actual_received; // Use actual received, not listing price
                
                // Find original purchase price by parsing backwards through logs
                float original_buy_price = 0.0f;
                for (int j = i + 1; j < log_count; j++) // Look forward (earlier in time, logs are DESC)
                {
                    if (logs[j].type == LOG_MARKET_BUY)
                    {
                        int buy_instance_id = 0;
                        float buy_price = 0.0f;
                        if (sscanf(logs[j].details, "Bought instance %d for $%f", &buy_instance_id, &buy_price) == 2)
                        {
                            if (buy_instance_id == instance_id)
                            {
                                original_buy_price = buy_price;
                                break; // Found the purchase
                            }
                        }
                    }
                    else if (logs[j].type == LOG_UNBOX)
                    {
                        // Check if this is the unbox that created this instance
                        int unbox_instance_id = 0;
                        float unbox_cost = 0.0f;
                        if (sscanf(logs[j].details, "Unboxed case %*d (%*[^)]) -> instance %d", &unbox_instance_id) == 1)
                        {
                            if (unbox_instance_id == instance_id)
                            {
                                // Extract cost from unbox log
                                if (sscanf(logs[j].details, "Unboxed case %*d (%*[^)]) -> instance %*d (def %*d, rarity %*d, wear %*f, pattern %*d, stattrak %*d, cost $%f", 
                                           &unbox_cost) == 1)
                                {
                                    original_buy_price = unbox_cost;
                                    break; // Found the unbox
                                }
                            }
                        }
                    }
                }
                
                // Calculate profit: actual_received - original_buy_price
                float profit = 0.0f;
                if (original_buy_price > 0.0f)
                {
                    profit = actual_received - original_buy_price;
                }
                else
                {
                    // Item might have been obtained via trade - can't calculate profit accurately
                    // Don't add to total_buy if we can't find original cost
                    profit = 0.0f;
                }
                
                if (profit > max_profit)
                    max_profit = profit;
                if (profit < max_loss)
                    max_loss = profit;
                
                // Add original cost to total_buy for net_profit calculation
                if (original_buy_price > 0.0f)
                {
                    total_buy += original_buy_price;
                }
            }
        }
        else if (logs[i].type == LOG_UNBOX)
        {
            // Unbox operations are NOT counted as trades
            // They are separate from peer-to-peer trades and market operations
            // Only count profit if item was later sold on market (handled in LOG_MARKET_SELL)
        }
        else if (logs[i].type == LOG_TRADE)
        {
            // Peer-to-peer trades only
            // Format: "Accepted trade offer X: gave $Y (items + cash), received $Z (items + cash), profit $W"
            float gave_value = 0.0f;
            float received_value = 0.0f;
            float profit = 0.0f;
            
            if (sscanf(logs[i].details, "Accepted trade offer %*d: gave $%f (items + cash), received $%f (items + cash), profit $%f", 
                       &gave_value, &received_value, &profit) == 3)
            {
                // Trade with profit information
                total_trade_gave += gave_value;
                total_trade_received += received_value;
                
                if (profit > max_profit)
                    max_profit = profit;
                if (profit < max_loss)
                    max_loss = profit;
                
                if (profit > 0.0f)
                    profitable_trades++;
                total_trades++;
                peer_trades_count++;
            }
            else
            {
                // Old format or declined/cancelled trade - just count
            total_trades++;
                peer_trades_count++;
            }
        }
    }
    
    // Calculate statistics
    out_stats->trades_completed = peer_trades_count; // Only peer-to-peer trades
    out_stats->items_bought = buy_count;              // Only market buys
    out_stats->items_sold = sell_count;               // Only market sells
    out_stats->avg_buy_price = (buy_count > 0) ? (total_buy / buy_count) : 0.0f;
    out_stats->avg_sell_price = (sell_count > 0) ? (total_sell / sell_count) : 0.0f;
    
    // Net profit = (market sells + trade received) - (market buys + trade gave)
    out_stats->net_profit = (total_sell + total_trade_received) - (total_buy + total_trade_gave);
    out_stats->best_trade_profit = (max_profit > -999999.0f) ? max_profit : 0.0f;
    out_stats->worst_trade_loss = (max_loss < 999999.0f) ? max_loss : 0.0f;
    out_stats->win_rate = (total_trades > 0) ? ((float)profitable_trades / total_trades * 100.0f) : 0.0f;
    
    return 0;
}

// Get balance history (last N days)
int get_balance_history(int user_id, BalanceHistoryEntry *out_history, int *count, int days)
{
    LOG_DEBUG("get_balance_history: called with user_id=%d, days=%d", user_id, days);
    
    if (!out_history || !count || user_id <= 0 || days <= 0)
    {
        LOG_ERROR("get_balance_history: invalid parameters");
        return -1;
    }
    
    if (days > 30)
        days = 30;
    
    // Simplified: We'll use transaction logs to estimate balance changes
    // In full implementation, we'd have a balance_history table
    time_t now = time(NULL);
    time_t start_time = now - (days * 24 * 60 * 60);
    
    LOG_DEBUG("get_balance_history: start_time=%ld, now=%ld (days=%d)", start_time, now, days);
    
    const char *sql = "SELECT timestamp, details FROM transaction_logs "
                      "WHERE user_id = ? AND timestamp >= ? "
                      "ORDER BY timestamp ASC";
    sqlite3_stmt *stmt;
    sqlite3 *db = db_get_connection();
    if (!db)
    {
        LOG_ERROR("get_balance_history: db_get_connection() failed");
        return -1;
    }
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        LOG_ERROR("get_balance_history: prepare failed: %s", sqlite3_errmsg(db));
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
        LOG_DEBUG("get_balance_history: current_balance=%.2f", current_balance);
    }
    else
    {
        LOG_ERROR("get_balance_history: db_load_user() failed for user_id=%d", user_id);
    }
    
    // Build balance history by working backwards from current balance
    // This is simplified - in full implementation, we'd track balance snapshots
    int idx = 0;
    float running_balance = current_balance;
    
    // Get transactions in reverse order
    // Note: Column order: 0=log_id, 1=type, 2=user_id, 3=details, 4=timestamp
    const char *rev_sql = "SELECT log_id, type, user_id, details, timestamp FROM transaction_logs "
                          "WHERE user_id = ? AND timestamp >= ? "
                          "ORDER BY timestamp DESC";
    sqlite3_stmt *rev_stmt;
    rc = sqlite3_prepare_v2(db, rev_sql, -1, &rev_stmt, 0);
    if (rc == SQLITE_OK)
    {
        sqlite3_bind_int(rev_stmt, 1, user_id);
        sqlite3_bind_int64(rev_stmt, 2, start_time);
        
        int transaction_count = 0;
        
        // Sample balance at daily intervals
        time_t last_sample = now;
        out_history[idx].timestamp = now;
        out_history[idx].balance = current_balance;
        idx++;
        LOG_DEBUG("get_balance_history: added current balance entry [%d]: timestamp=%ld, balance=%.2f", 
                  idx-1, now, current_balance);
        
        while (sqlite3_step(rev_stmt) == SQLITE_ROW && idx < days)
        {
            transaction_count++;
            time_t log_time = sqlite3_column_int64(rev_stmt, 4); // timestamp is column 4
            int log_type = sqlite3_column_int(rev_stmt, 1);      // type is column 1
            const char *details = (const char *)sqlite3_column_text(rev_stmt, 3); // details is column 3
            
            LOG_DEBUG("get_balance_history: transaction #%d: type=%d, time=%ld, details='%s'", 
                     transaction_count, log_type, log_time, details ? details : "NULL");
            
            // Adjust balance based on transaction type (reverse calculation from current balance)
            float balance_before = running_balance;
            
            if (log_type == LOG_MARKET_BUY)
            {
                // Market buy: spent money, so reverse by adding it back
                float price = 0.0f;
                if (sscanf(details, "Bought instance %*d for $%f", &price) == 1)
                {
                    running_balance += price;
                    LOG_DEBUG("get_balance_history: LOG_MARKET_BUY: price=%.2f, balance %.2f -> %.2f", 
                             price, balance_before, running_balance);
                }
                else
                {
                    LOG_DEBUG("get_balance_history: LOG_MARKET_BUY: failed to parse price from '%s'", details);
                }
            }
            else if (log_type == LOG_MARKET_SELL)
            {
                // Market sell: received money, so reverse by subtracting it
                float received = 0.0f;
                float listing_fee_refund = 0.0f;
                if (sscanf(details, "Sold instance %*d for $%*f (received $%f after fee, +$%f listing fee refund)", 
                          &received, &listing_fee_refund) == 2 ||
                    sscanf(details, "Sold instance %*d for $%*f (received $%f", &received) == 1)
                {
                    running_balance -= (received + listing_fee_refund);
                    LOG_DEBUG("get_balance_history: LOG_MARKET_SELL: received=%.2f, refund=%.2f, balance %.2f -> %.2f", 
                             received, listing_fee_refund, balance_before, running_balance);
                }
                else
                {
                    LOG_DEBUG("get_balance_history: LOG_MARKET_SELL: failed to parse from '%s'", details);
                }
            }
            else if (log_type == LOG_TRADE && details)
            {
                // Trade: parse cash changes from trade details
                // Format: "Accepted trade offer X: gave $Y (items + cash), received $Z (items + cash), profit $W"
                float gave_cash = 0.0f;
                float received_cash = 0.0f;
                if (sscanf(details, "Accepted trade offer %*d: gave $%f (items + cash), received $%f", 
                          &gave_cash, &received_cash) == 2)
                {
                    // Reverse: subtract what was received, add back what was given
                    running_balance -= received_cash;
                    running_balance += gave_cash;
                    LOG_DEBUG("get_balance_history: LOG_TRADE: gave=%.2f, received=%.2f, balance %.2f -> %.2f", 
                             gave_cash, received_cash, balance_before, running_balance);
                }
                else
                {
                    LOG_DEBUG("get_balance_history: LOG_TRADE: failed to parse from '%s'", details);
                }
            }
            else if (log_type == LOG_UNBOX && details)
            {
                // Unbox: spent money on case, so reverse by adding it back
                float cost = 0.0f;
                if (sscanf(details, "Unboxed case %*d (%*[^)]) -> instance %*d (def %*d, rarity %*d, wear %*f, pattern %*d, stattrak %*d, cost $%f", 
                          &cost) == 1)
                {
                    running_balance += cost;
                    LOG_DEBUG("get_balance_history: LOG_UNBOX: cost=%.2f, balance %.2f -> %.2f", 
                             cost, balance_before, running_balance);
                }
                else
                {
                    LOG_DEBUG("get_balance_history: LOG_UNBOX: failed to parse cost from '%s'", details);
                }
            }
            // Note: Quest rewards, daily login rewards, achievement rewards are not logged in transaction_logs
            // They are handled separately, so we can't reverse them here
            
            // Sample at daily intervals
            if (last_sample - log_time >= 24 * 60 * 60 && idx < days)
            {
                out_history[idx].timestamp = log_time;
                out_history[idx].balance = running_balance;
                LOG_DEBUG("get_balance_history: sampled balance [%d]: timestamp=%ld, balance=%.2f", 
                          idx, log_time, running_balance);
                idx++;
                last_sample = log_time;
            }
        }
        
        LOG_DEBUG("get_balance_history: processed %d transactions, sampled %d balance points", 
                 transaction_count, idx);
        sqlite3_finalize(rev_stmt);
    }
    else
    {
        LOG_ERROR("get_balance_history: prepare rev_sql failed: %s", sqlite3_errmsg(db));
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
    LOG_DEBUG("get_balance_history: returning %d balance history entries", idx);
    return 0;
}
