// leaderboards.c - Leaderboards Implementation

#include "../include/leaderboards.h"
#include "../include/database.h"
#include "../include/database_internal.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Calculate net worth for a user (balance + inventory value)
static float calculate_net_worth(int user_id)
{
    User user;
    if (db_load_user(user_id, &user) != 0)
        return 0.0f;
    
    float net_worth = user.balance;
    
    // Calculate inventory value
    Inventory inv;
    if (db_load_inventory(user_id, &inv) == 0)
    {
        for (int i = 0; i < inv.count; i++)
        {
            int instance_id = inv.skin_ids[i];
            int definition_id;
            SkinRarity rarity;
            WearCondition wear;
            int pattern_seed, is_stattrak;
            int owner_id;
            time_t acquired_at;
            int is_tradable;
            
            if (db_load_skin_instance(instance_id, &definition_id, &rarity, &wear, &pattern_seed, &is_stattrak, &owner_id, &acquired_at, &is_tradable) == 0)
            {
                float price = db_calculate_skin_price(definition_id, rarity, wear);
                net_worth += price;
            }
        }
    }
    
    return net_worth;
}

// Get top traders by net worth
int get_top_traders(LeaderboardEntry *out_entries, int *count, int limit)
{
    if (!out_entries || !count || limit <= 0)
        return -1;
    
    // Get all users
    // Note: In a full implementation, we'd have a users table query
    // For now, we'll query from transaction_logs to find active users
    // This is simplified - in production, we'd have a proper users list
    
    // For simplicity, we'll get users from a query
    // In SQLite, we can query users table directly
    const char *sql = "SELECT user_id, username FROM users ORDER BY user_id";
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
    
    LeaderboardEntry entries[100];
    int entry_count = 0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW && entry_count < 100)
    {
        int user_id = sqlite3_column_int(stmt, 0);
        const char *username = (const char *)sqlite3_column_text(stmt, 1);
        
        float net_worth = calculate_net_worth(user_id);
        
        entries[entry_count].user_id = user_id;
        strncpy(entries[entry_count].username, username, 31);
        entries[entry_count].username[31] = '\0';
        entries[entry_count].value = net_worth;
        entries[entry_count].details[0] = '\0';
        entry_count++;
    }
    
    sqlite3_finalize(stmt);
    
    // Sort by net worth (descending)
    for (int i = 0; i < entry_count - 1; i++)
    {
        for (int j = i + 1; j < entry_count; j++)
        {
            if (entries[i].value < entries[j].value)
            {
                LeaderboardEntry temp = entries[i];
                entries[i] = entries[j];
                entries[j] = temp;
            }
        }
    }
    
    // Copy top entries
    int copy_count = (entry_count < limit) ? entry_count : limit;
    for (int i = 0; i < copy_count; i++)
    {
        out_entries[i] = entries[i];
    }
    
    *count = copy_count;
    return 0;
}

// Get luckiest unboxers (best unbox value)
int get_luckiest_unboxers(LeaderboardEntry *out_entries, int *count, int limit)
{
    if (!out_entries || !count || limit <= 0)
        return -1;
    
    // Query transaction_logs for unbox transactions
    // Find highest value unboxed items
    const char *sql = "SELECT user_id, details FROM transaction_logs "
                      "WHERE type = ? ORDER BY timestamp DESC";
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
    
    sqlite3_bind_int(stmt, 1, LOG_UNBOX);
    
    LeaderboardEntry entries[100];
    int entry_count = 0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW && entry_count < 100)
    {
        int user_id = sqlite3_column_int(stmt, 0);
        const char *details = (const char *)sqlite3_column_text(stmt, 1);
        
        // Parse details to extract instance_id and calculate value
        // Format: "Unboxed case X -> instance Y (def Z, rarity R, ...)"
        int instance_id = 0;
        if (sscanf(details, "Unboxed case %*d -> instance %d", &instance_id) == 1)
        {
            // Get instance details and calculate price
            int definition_id;
            SkinRarity rarity;
            WearCondition wear;
            int pattern_seed, is_stattrak;
            int owner_id;
            time_t acquired_at;
            int is_tradable;
            
            if (db_load_skin_instance(instance_id, &definition_id, &rarity, &wear, &pattern_seed, &is_stattrak, &owner_id, &acquired_at, &is_tradable) == 0)
            {
                float price = db_calculate_skin_price(definition_id, rarity, wear);
                
                // Get username
                User user;
                if (db_load_user(user_id, &user) == 0)
                {
                    // Get skin name
                    char skin_name[64];
                    float base_price;
                    if (db_load_skin_definition(definition_id, skin_name, &base_price) == 0)
                    {
                        entries[entry_count].user_id = user_id;
                        strncpy(entries[entry_count].username, user.username, 31);
                        entries[entry_count].username[31] = '\0';
                        entries[entry_count].value = price;
                        snprintf(entries[entry_count].details, sizeof(entries[entry_count].details), 
                                "Unboxed: %s", skin_name);
                        entry_count++;
                    }
                }
            }
        }
    }
    
    sqlite3_finalize(stmt);
    
    // Sort by value (descending)
    for (int i = 0; i < entry_count - 1; i++)
    {
        for (int j = i + 1; j < entry_count; j++)
        {
            if (entries[i].value < entries[j].value)
            {
                LeaderboardEntry temp = entries[i];
                entries[i] = entries[j];
                entries[j] = temp;
            }
        }
    }
    
    // Copy top entries
    int copy_count = (entry_count < limit) ? entry_count : limit;
    for (int i = 0; i < copy_count; i++)
    {
        out_entries[i] = entries[i];
    }
    
    *count = copy_count;
    return 0;
}

// Get most profitable traders
int get_most_profitable(LeaderboardEntry *out_entries, int *count, int limit)
{
    if (!out_entries || !count || limit <= 0)
        return -1;
    
    // Calculate profit from market transactions
    // Profit = (sell price - buy price) for each transaction
    // This is simplified - in full implementation, we'd track buy prices
    
    // For now, we'll use a simplified approach:
    // Calculate profit as: (total market sell revenue) - (estimated buy cost)
    // Or use balance change over time
    
    // Get all users and calculate their profit
    const char *sql = "SELECT user_id, username FROM users ORDER BY user_id";
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
    
    LeaderboardEntry entries[100];
    int entry_count = 0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW && entry_count < 100)
    {
        int user_id = sqlite3_column_int(stmt, 0);
        const char *username = (const char *)sqlite3_column_text(stmt, 1);
        
        // Calculate profit from transaction logs
        // Sum up LOG_MARKET_SELL transactions
        const char *profit_sql = "SELECT SUM(CAST(SUBSTR(details, INSTR(details, '$') + 1, 20) AS REAL)) "
                                 "FROM transaction_logs "
                                 "WHERE user_id = ? AND type = ?";
        sqlite3_stmt *profit_stmt;
        rc = sqlite3_prepare_v2(db, profit_sql, -1, &profit_stmt, 0);
        if (rc == SQLITE_OK)
        {
            sqlite3_bind_int(profit_stmt, 1, user_id);
            sqlite3_bind_int(profit_stmt, 2, LOG_MARKET_SELL);
            
            float profit = 0.0f;
            if (sqlite3_step(profit_stmt) == SQLITE_ROW)
            {
                profit = (float)sqlite3_column_double(profit_stmt, 0);
            }
            sqlite3_finalize(profit_stmt);
            
            if (profit > 0)
            {
                entries[entry_count].user_id = user_id;
                strncpy(entries[entry_count].username, username, 31);
                entries[entry_count].username[31] = '\0';
                entries[entry_count].value = profit;
                snprintf(entries[entry_count].details, sizeof(entries[entry_count].details), 
                        "Total profit: +$%.2f", profit);
                entry_count++;
            }
        }
    }
    
    sqlite3_finalize(stmt);
    
    // Sort by profit (descending)
    for (int i = 0; i < entry_count - 1; i++)
    {
        for (int j = i + 1; j < entry_count; j++)
        {
            if (entries[i].value < entries[j].value)
            {
                LeaderboardEntry temp = entries[i];
                entries[i] = entries[j];
                entries[j] = temp;
            }
        }
    }
    
    // Copy top entries
    int copy_count = (entry_count < limit) ? entry_count : limit;
    for (int i = 0; i < copy_count; i++)
    {
        out_entries[i] = entries[i];
    }
    
    *count = copy_count;
    return 0;
}
