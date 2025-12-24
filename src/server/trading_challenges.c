// trading_challenges.c - Trading Challenges Implementation

#include "../include/trading_challenges.h"
#include "../include/database.h"
#include "../include/database_internal.h"
#include "../include/leaderboards.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Calculate net worth for a user (balance + inventory value)
static float calculate_net_worth(int user_id)
{
    User user;
    if (db_load_user(user_id, &user) != 0)
        return 0.0f;
    
    float net_worth = user.balance;
    
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

// Create a 1v1 profit race challenge
int create_profit_race_challenge(int challenger_id, int opponent_id, int duration_minutes, int *out_challenge_id)
{
    if (!out_challenge_id || challenger_id <= 0 || opponent_id <= 0 || duration_minutes <= 0)
        return -1;
    
    if (challenger_id == opponent_id)
        return -2; // Cannot challenge yourself
    
    // Get starting balances
    User challenger, opponent;
    if (db_load_user(challenger_id, &challenger) != 0)
        return -3;
    if (db_load_user(opponent_id, &opponent) != 0)
        return -4;
    
    float challenger_start = calculate_net_worth(challenger_id);
    float opponent_start = calculate_net_worth(opponent_id);
    
    // Insert challenge
    const char *sql = "INSERT INTO trading_challenges "
                      "(challenger_id, opponent_id, type, status, challenger_start_balance, "
                      "opponent_start_balance, challenger_current_profit, opponent_current_profit, "
                      "start_time, duration_minutes) "
                      "VALUES (?, ?, ?, ?, ?, ?, 0.0, 0.0, ?, ?)";
    sqlite3_stmt *stmt;
    sqlite3 *db = db_get_connection();
    if (!db)
        return -5;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -5;
    
    sqlite3_bind_int(stmt, 1, challenger_id);
    sqlite3_bind_int(stmt, 2, opponent_id);
    sqlite3_bind_int(stmt, 3, CHALLENGE_1V1_PROFIT_RACE);
    sqlite3_bind_int(stmt, 4, CHALLENGE_PENDING);
    sqlite3_bind_double(stmt, 5, challenger_start);
    sqlite3_bind_double(stmt, 6, opponent_start);
    sqlite3_bind_int64(stmt, 7, time(NULL));
    sqlite3_bind_int(stmt, 8, duration_minutes);
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE)
    {
        *out_challenge_id = (int)sqlite3_last_insert_rowid(db);
        sqlite3_finalize(stmt);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    return -5;
}

// Get challenge by ID
int get_challenge(int challenge_id, TradingChallenge *out_challenge)
{
    if (!out_challenge || challenge_id <= 0)
        return -1;
    
    const char *sql = "SELECT challenge_id, challenger_id, opponent_id, type, status, "
                      "challenger_start_balance, opponent_start_balance, "
                      "challenger_current_profit, opponent_current_profit, "
                      "start_time, end_time, duration_minutes "
                      "FROM trading_challenges WHERE challenge_id = ?";
    sqlite3_stmt *stmt;
    sqlite3 *db = db_get_connection();
    if (!db)
        return -1;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;
    
    sqlite3_bind_int(stmt, 1, challenge_id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        out_challenge->challenge_id = sqlite3_column_int(stmt, 0);
        out_challenge->challenger_id = sqlite3_column_int(stmt, 1);
        out_challenge->opponent_id = sqlite3_column_int(stmt, 2);
        out_challenge->type = (ChallengeType)sqlite3_column_int(stmt, 3);
        out_challenge->status = (ChallengeStatus)sqlite3_column_int(stmt, 4);
        out_challenge->challenger_start_balance = (float)sqlite3_column_double(stmt, 5);
        out_challenge->opponent_start_balance = (float)sqlite3_column_double(stmt, 6);
        out_challenge->challenger_current_profit = (float)sqlite3_column_double(stmt, 7);
        out_challenge->opponent_current_profit = (float)sqlite3_column_double(stmt, 8);
        out_challenge->start_time = sqlite3_column_int64(stmt, 9);
        out_challenge->end_time = sqlite3_column_int64(stmt, 10);
        out_challenge->duration_minutes = sqlite3_column_int(stmt, 11);
        sqlite3_finalize(stmt);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    return -1;
}

// Get user's active challenges
int get_user_challenges(int user_id, TradingChallenge *out_challenges, int *count)
{
    if (!out_challenges || !count || user_id <= 0)
        return -1;
    
    const char *sql = "SELECT challenge_id, challenger_id, opponent_id, type, status, "
                      "challenger_start_balance, opponent_start_balance, "
                      "challenger_current_profit, opponent_current_profit, "
                      "start_time, end_time, duration_minutes "
                      "FROM trading_challenges "
                      "WHERE (challenger_id = ? OR opponent_id = ?) AND status IN (?, ?) "
                      "ORDER BY start_time DESC LIMIT 50";
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
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_int(stmt, 3, CHALLENGE_PENDING);
    sqlite3_bind_int(stmt, 4, CHALLENGE_ACTIVE);
    
    int idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < 50)
    {
        out_challenges[idx].challenge_id = sqlite3_column_int(stmt, 0);
        out_challenges[idx].challenger_id = sqlite3_column_int(stmt, 1);
        out_challenges[idx].opponent_id = sqlite3_column_int(stmt, 2);
        out_challenges[idx].type = (ChallengeType)sqlite3_column_int(stmt, 3);
        out_challenges[idx].status = (ChallengeStatus)sqlite3_column_int(stmt, 4);
        out_challenges[idx].challenger_start_balance = (float)sqlite3_column_double(stmt, 5);
        out_challenges[idx].opponent_start_balance = (float)sqlite3_column_double(stmt, 6);
        out_challenges[idx].challenger_current_profit = (float)sqlite3_column_double(stmt, 7);
        out_challenges[idx].opponent_current_profit = (float)sqlite3_column_double(stmt, 8);
        out_challenges[idx].start_time = sqlite3_column_int64(stmt, 9);
        out_challenges[idx].end_time = sqlite3_column_int64(stmt, 10);
        out_challenges[idx].duration_minutes = sqlite3_column_int(stmt, 11);
        idx++;
    }
    
    *count = idx;
    sqlite3_finalize(stmt);
    return 0;
}

// Update challenge progress (calculate current profit)
int update_challenge_progress(int challenge_id)
{
    TradingChallenge challenge;
    if (get_challenge(challenge_id, &challenge) != 0)
        return -1;
    
    if (challenge.status != CHALLENGE_ACTIVE)
        return -2;
    
    // Calculate current net worth
    float challenger_current = calculate_net_worth(challenge.challenger_id);
    float opponent_current = calculate_net_worth(challenge.opponent_id);
    
    // Calculate profit
    float challenger_profit = challenger_current - challenge.challenger_start_balance;
    float opponent_profit = opponent_current - challenge.opponent_start_balance;
    
    // Update database
    const char *sql = "UPDATE trading_challenges SET "
                      "challenger_current_profit = ?, opponent_current_profit = ? "
                      "WHERE challenge_id = ?";
    sqlite3_stmt *stmt;
    sqlite3 *db = db_get_connection();
    if (!db)
        return -3;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -3;
    
    sqlite3_bind_double(stmt, 1, challenger_profit);
    sqlite3_bind_double(stmt, 2, opponent_profit);
    sqlite3_bind_int(stmt, 3, challenge_id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -3;
}

// Accept challenge (change status from PENDING to ACTIVE)
int accept_challenge(int challenge_id, int user_id)
{
    TradingChallenge challenge;
    if (get_challenge(challenge_id, &challenge) != 0)
        return -1;
    
    if (challenge.opponent_id != user_id)
        return -2; // Only opponent can accept
    
    if (challenge.status != CHALLENGE_PENDING)
        return -3; // Challenge not pending
    
    // Update status and start time
    const char *sql = "UPDATE trading_challenges SET status = ?, start_time = ? WHERE challenge_id = ?";
    sqlite3_stmt *stmt;
    sqlite3 *db = db_get_connection();
    if (!db)
        return -4;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -4;
    
    sqlite3_bind_int(stmt, 1, CHALLENGE_ACTIVE);
    sqlite3_bind_int64(stmt, 2, time(NULL));
    sqlite3_bind_int(stmt, 3, challenge_id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -4;
}

// Complete challenge and determine winner
int complete_challenge(int challenge_id, int *winner_id)
{
    if (!winner_id)
        return -1;
    
    TradingChallenge challenge;
    if (get_challenge(challenge_id, &challenge) != 0)
        return -1;
    
    if (challenge.status != CHALLENGE_ACTIVE)
        return -2;
    
    // Update progress one last time
    update_challenge_progress(challenge_id);
    get_challenge(challenge_id, &challenge); // Reload
    
    // Determine winner
    if (challenge.challenger_current_profit > challenge.opponent_current_profit)
        *winner_id = challenge.challenger_id;
    else if (challenge.opponent_current_profit > challenge.challenger_current_profit)
        *winner_id = challenge.opponent_id;
    else
        *winner_id = 0; // Tie
    
    // Update status
    const char *sql = "UPDATE trading_challenges SET status = ?, end_time = ? WHERE challenge_id = ?";
    sqlite3_stmt *stmt;
    sqlite3 *db = db_get_connection();
    if (!db)
        return -3;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -3;
    
    sqlite3_bind_int(stmt, 1, CHALLENGE_COMPLETED);
    sqlite3_bind_int64(stmt, 2, time(NULL));
    sqlite3_bind_int(stmt, 3, challenge_id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -3;
}

// Cancel challenge
int cancel_challenge(int challenge_id, int user_id)
{
    TradingChallenge challenge;
    if (get_challenge(challenge_id, &challenge) != 0)
        return -1;
    
    if (challenge.challenger_id != user_id && challenge.opponent_id != user_id)
        return -2; // Not authorized
    
    if (challenge.status == CHALLENGE_COMPLETED)
        return -3; // Cannot cancel completed challenge
    
    const char *sql = "UPDATE trading_challenges SET status = ? WHERE challenge_id = ?";
    sqlite3_stmt *stmt;
    sqlite3 *db = db_get_connection();
    if (!db)
        return -4;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -4;
    
    sqlite3_bind_int(stmt, 1, CHALLENGE_CANCELLED);
    sqlite3_bind_int(stmt, 2, challenge_id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -4;
}
