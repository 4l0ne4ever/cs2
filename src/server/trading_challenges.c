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

// Calculate net worth for a user
// Net worth = cash balance + total value of all items in inventory
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
    
    // Get starting net worth for both players
    // Net Worth = Balance + Inventory Value (used as baseline for profit calculation)
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
                      "start_time, end_time, duration_minutes, "
                      "challenger_cancel_vote, opponent_cancel_vote "
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
        out_challenge->challenger_cancel_vote = sqlite3_column_int(stmt, 12);
        out_challenge->opponent_cancel_vote = sqlite3_column_int(stmt, 13);
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
                      "start_time, end_time, duration_minutes, "
                      "challenger_cancel_vote, opponent_cancel_vote "
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
        out_challenges[idx].challenger_cancel_vote = sqlite3_column_int(stmt, 12);
        out_challenges[idx].opponent_cancel_vote = sqlite3_column_int(stmt, 13);
        idx++;
    }
    
    *count = idx;
    sqlite3_finalize(stmt);
    return 0;
}

// Update challenge progress (calculate current profit)
// 
// Profit Formula (simple and consistent):
//   Profit = Current Net Worth - Start Net Worth
//   where Net Worth = Balance + Inventory Value
//
// This means:
//   Profit = (Current Balance - Start Balance) + (Current Inventory Value - Start Inventory Value)
//
// Examples:
//   - Start: $100 balance, $0 inventory → Net Worth = $100
//   - Unbox $10 case, get $50 item: $90 balance, $50 inventory → Net Worth = $140 → Profit = +$40
//   - Buy $100 item: $0 balance, $100 inventory → Net Worth = $100 → Profit = $0
//   - Sell $100 item for $120: $120 balance, $0 inventory → Net Worth = $120 → Profit = +$20
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
    
    // Calculate profit: Profit = Current Net Worth - Start Net Worth
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

// Challenge reward/penalty constants
#define CHALLENGE_CANCEL_TIMEOUT_SECONDS (24 * 60 * 60) // 24 hours
#define CHALLENGE_CANCEL_PENALTY_AMOUNT 100.0f // $100 penalty for runner
#define CHALLENGE_WINNER_REWARD_AMOUNT 500.0f // $500 reward for winner

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
    
    // BEGIN TRANSACTION - Award reward to winner
    if (db_begin_transaction() != 0)
        return -3;
    
    // Award reward to winner (if not a tie)
    if (*winner_id > 0)
    {
        User winner;
        if (db_load_user(*winner_id, &winner) == 0)
        {
            winner.balance += CHALLENGE_WINNER_REWARD_AMOUNT;
            if (db_update_user(&winner) != 0)
            {
                db_rollback_transaction();
                return -3;
            }
        }
    }
    
    // Update status
    const char *sql = "UPDATE trading_challenges SET status = ?, end_time = ? WHERE challenge_id = ?";
    sqlite3_stmt *stmt;
    sqlite3 *db = db_get_connection();
    if (!db)
    {
        db_rollback_transaction();
        return -3;
    }
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        db_rollback_transaction();
        return -3;
    }
    
    sqlite3_bind_int(stmt, 1, CHALLENGE_COMPLETED);
    sqlite3_bind_int64(stmt, 2, time(NULL));
    sqlite3_bind_int(stmt, 3, challenge_id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE)
    {
        db_rollback_transaction();
        return -3;
    }
    
    // COMMIT TRANSACTION - Reward awarded and challenge completed
    if (db_commit_transaction() != 0)
    {
        db_rollback_transaction();
        return -3;
    }
    
    return 0;
}

// Cancel challenge - requires vote from both players
// If only one player votes and challenge is active for >24 hours, challenger (runner) loses money
int cancel_challenge(int challenge_id, int user_id)
{
    TradingChallenge challenge;
    if (get_challenge(challenge_id, &challenge) != 0)
        return -1;
    
    if (challenge.challenger_id != user_id && challenge.opponent_id != user_id)
        return -2; // Not authorized
    
    if (challenge.status == CHALLENGE_COMPLETED || challenge.status == CHALLENGE_CANCELLED)
        return -3; // Cannot cancel completed/cancelled challenge
    
    sqlite3 *db = db_get_connection();
    if (!db)
        return -4;
    
    // Update vote for the user
    int is_challenger = (user_id == challenge.challenger_id);
    int already_voted = is_challenger ? challenge.challenger_cancel_vote : challenge.opponent_cancel_vote;
    
    if (already_voted)
        return -5; // Already voted
    
    // Set vote
    const char *update_vote_sql;
    if (is_challenger)
    {
        update_vote_sql = "UPDATE trading_challenges SET challenger_cancel_vote = 1 WHERE challenge_id = ?";
    }
    else
    {
        update_vote_sql = "UPDATE trading_challenges SET opponent_cancel_vote = 1 WHERE challenge_id = ?";
    }
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, update_vote_sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -4;
    
    sqlite3_bind_int(stmt, 1, challenge_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE)
        return -4;
    
    // Reload challenge to check votes
    if (get_challenge(challenge_id, &challenge) != 0)
        return -4;
    
    // Check if both players voted
    if (challenge.challenger_cancel_vote && challenge.opponent_cancel_vote)
    {
        // Both voted - cancel challenge immediately
        const char *cancel_sql = "UPDATE trading_challenges SET status = ? WHERE challenge_id = ?";
        rc = sqlite3_prepare_v2(db, cancel_sql, -1, &stmt, 0);
        if (rc == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, CHALLENGE_CANCELLED);
            sqlite3_bind_int(stmt, 2, challenge_id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        return 0; // Success: both voted, challenge cancelled
    }
    
    // Only one player voted - check timeout
    if (challenge.status == CHALLENGE_ACTIVE && challenge.start_time > 0)
    {
        time_t now = time(NULL);
        time_t elapsed = now - challenge.start_time;
        
        if (elapsed >= CHALLENGE_CANCEL_TIMEOUT_SECONDS)
        {
            // Timeout reached - challenger (runner) loses money and challenge is cancelled
            // Use transaction to ensure atomicity
            if (db_begin_transaction() == 0)
            {
                User challenger;
                if (db_load_user(challenge.challenger_id, &challenger) == 0)
                {
                    // Apply penalty
                    challenger.balance -= CHALLENGE_CANCEL_PENALTY_AMOUNT;
                    if (challenger.balance < 0.0f)
                        challenger.balance = 0.0f; // Don't allow negative balance
                    
                    if (db_update_user(&challenger) == 0)
                    {
                        // Cancel challenge
                        const char *cancel_sql = "UPDATE trading_challenges SET status = ? WHERE challenge_id = ?";
                        rc = sqlite3_prepare_v2(db, cancel_sql, -1, &stmt, 0);
                        if (rc == SQLITE_OK)
                        {
                            sqlite3_bind_int(stmt, 1, CHALLENGE_CANCELLED);
                            sqlite3_bind_int(stmt, 2, challenge_id);
                            if (sqlite3_step(stmt) == SQLITE_DONE)
                            {
                                sqlite3_finalize(stmt);
                                if (db_commit_transaction() == 0)
                                {
                                    return 0; // Success: timeout reached, challenger penalized, challenge cancelled
                                }
                            }
                            else
                            {
                                sqlite3_finalize(stmt);
                            }
                        }
                    }
                }
                // Rollback on any failure
                db_rollback_transaction();
            }
            return -4; // Failed to process timeout cancellation
        }
    }
    
    // Only one vote, no timeout yet - wait for other player's vote
    return 0; // Success: vote recorded, waiting for other player
}

// Helper function: Update all active challenges for a user
// This is called automatically after actions that affect net worth (unbox, market buy/sell, trading)
//
// Profit is calculated as: Profit = Current Net Worth - Start Net Worth
// Net Worth = Balance + Inventory Value
//
// All actions that change balance or inventory automatically update profit:
// - Unboxing: Changes balance (cost) and inventory (item value)
// - Market buy/sell: Changes balance and inventory
// - Trading: Changes balance (cash) and inventory (items)
void update_user_active_challenges(int user_id)
{
    if (user_id <= 0)
        return;
    
    TradingChallenge challenges[50];
    int count = 0;
    
    // Get all active challenges for this user
    if (get_user_challenges(user_id, challenges, &count) != 0)
        return;
    
    // Update progress for each active challenge
    for (int i = 0; i < count; i++)
    {
        if (challenges[i].status == CHALLENGE_ACTIVE)
        {
            update_challenge_progress(challenges[i].challenge_id);
        }
    }
}
