// quests.c - Daily Quests System

#include "../include/quests.h"
#include "../include/database.h"
#include "../include/types.h"
#include "../include/logger.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Check if quests need to be reset (older than 24 hours)
static int should_reset_quests(int user_id)
{
    if (user_id <= 0)
        return 0;

    Quest quests[10];
    int count = 0;
    if (db_load_user_quests(user_id, quests, &count) != 0 || count == 0)
        return 1; // No quests, need to initialize

    time_t now = time(NULL);
    // Check if any quest is older than 24 hours (86400 seconds)
    for (int i = 0; i < count; i++)
    {
        if (now - quests[i].started_at >= 86400) // 24 hours
        {
            LOG_INFO("[QUESTS] Quests need reset for user %d: quest_id=%d started_at=%ld (%.1f hours ago)",
                     user_id, quests[i].quest_id, quests[i].started_at, (now - quests[i].started_at) / 3600.0);
            return 1; // Need reset
        }
    }

    return 0; // No reset needed
}

// Initialize daily quests for a new user or reset for existing user
int init_daily_quests(int user_id)
{
    if (user_id <= 0)
        return -1;

    // Delete ALL old quests for this user (both completed and incomplete, but not claimed)
    // Daily quests reset after 24 hours, so we delete all unclaimed quests
    sqlite3 *db;
    if (sqlite3_open("data/database.db", &db) != SQLITE_OK)
        return -1;

    // Delete all unclaimed quests (both completed and incomplete)
    const char *delete_sql = "DELETE FROM quests WHERE user_id = ? AND is_claimed = 0";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, delete_sql, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        LOG_INFO("[QUESTS] Deleted old unclaimed quests for user %d", user_id);
    }

    sqlite3_close(db);

    // Create 5 daily quests
    Quest quests[5] = {0};
    time_t now = time(NULL);

    quests[0].user_id = user_id;
    quests[0].quest_type = QUEST_FIRST_STEPS;
    quests[0].progress = 0;
    quests[0].target = QUEST_TARGET_FIRST_STEPS;
    quests[0].is_completed = 0;
    quests[0].is_claimed = 0;
    quests[0].started_at = now;

    quests[1].user_id = user_id;
    quests[1].quest_type = QUEST_MARKET_EXPLORER;
    quests[1].progress = 0;
    quests[1].target = QUEST_TARGET_MARKET_EXPLORER;
    quests[1].is_completed = 0;
    quests[1].is_claimed = 0;
    quests[1].started_at = now;

    quests[2].user_id = user_id;
    quests[2].quest_type = QUEST_LUCKY_GAMBLER;
    quests[2].progress = 0;
    quests[2].target = QUEST_TARGET_LUCKY_GAMBLER;
    quests[2].is_completed = 0;
    quests[2].is_claimed = 0;
    quests[2].started_at = now;

    quests[3].user_id = user_id;
    quests[3].quest_type = QUEST_PROFIT_MAKER;
    quests[3].progress = 0;
    quests[3].target = 50; // $50 profit target
    quests[3].is_completed = 0;
    quests[3].is_claimed = 0;
    quests[3].started_at = now;

    quests[4].user_id = user_id;
    quests[4].quest_type = QUEST_SOCIAL_TRADER;
    quests[4].progress = 0;
    quests[4].target = QUEST_TARGET_SOCIAL_TRADER;
    quests[4].is_completed = 0;
    quests[4].is_claimed = 0;
    quests[4].started_at = now;

    // Save all quests
    for (int i = 0; i < 5; i++)
    {
        db_save_quest(&quests[i]);
    }

    LOG_INFO("[QUESTS] Initialized 5 daily quests for user %d", user_id);
    return 0;
}

// Check and reset daily quests if needed (called on login)
int check_and_reset_daily_quests(int user_id)
{
    if (user_id <= 0)
        return -1;

    if (should_reset_quests(user_id))
    {
        LOG_INFO("[QUESTS] Resetting daily quests for user %d (24 hours passed)", user_id);
        return init_daily_quests(user_id);
    }

    return 0; // No reset needed
}

// Get user's active quests
int get_user_quests(int user_id, Quest *out_quests, int *count)
{
    if (user_id <= 0 || !out_quests || !count)
        return -1;

    return db_load_user_quests(user_id, out_quests, count);
}

// Update quest progress
// NOTE: This function only marks quest as completed when progress >= target.
// It does NOT automatically claim the reward - player must manually claim via claim_quest_reward()
int update_quest_progress(int user_id, QuestType quest_type, int increment)
{
    if (user_id <= 0 || increment <= 0)
        return -1;

    Quest quests[10];
    int count = 0;
    if (db_load_user_quests(user_id, quests, &count) != 0)
        return -1;

    // Find quest of this type
    for (int i = 0; i < count; i++)
    {
        if (quests[i].quest_type == quest_type && !quests[i].is_completed)
        {
            quests[i].progress += increment;
            if (quests[i].progress >= quests[i].target)
            {
                // Mark quest as completed, but DO NOT claim reward automatically
                // Player must manually claim reward via claim_quest_reward()
                quests[i].is_completed = 1;
                quests[i].completed_at = time(NULL);
                // is_claimed remains 0 - player must claim manually
                LOG_INFO("[QUESTS] Quest completed (not claimed): user_id=%d, quest_id=%d, quest_type=%d, progress=%d/%d",
                         user_id, quests[i].quest_id, quests[i].quest_type, quests[i].progress, quests[i].target);
            }
            return db_update_quest(&quests[i]);
        }
    }

    return -1;
}

// Claim quest reward
int claim_quest_reward(int user_id, int quest_id)
{
    if (user_id <= 0 || quest_id <= 0)
        return -1;

    Quest quests[10];
    int count = 0;
    if (db_load_user_quests(user_id, quests, &count) != 0)
        return -1;

    // Find quest
    for (int i = 0; i < count; i++)
    {
        if (quests[i].quest_id == quest_id)
        {
            if (!quests[i].is_completed)
                return -2; // Not completed
            if (quests[i].is_claimed)
                return -3; // Already claimed

            // Calculate reward
            float reward = 0.0f;
            switch (quests[i].quest_type)
            {
            case QUEST_FIRST_STEPS:
                reward = QUEST_REWARD_FIRST_STEPS;
                break;
            case QUEST_MARKET_EXPLORER:
                reward = QUEST_REWARD_MARKET_EXPLORER;
                break;
            case QUEST_LUCKY_GAMBLER:
                reward = QUEST_REWARD_LUCKY_GAMBLER;
                break;
            case QUEST_PROFIT_MAKER:
                reward = QUEST_REWARD_PROFIT_MAKER;
                break;
            case QUEST_SOCIAL_TRADER:
                reward = QUEST_REWARD_SOCIAL_TRADER;
                break;
            }

            // Add reward to user balance
            User user;
            if (db_load_user(user_id, &user) != 0)
            {
                LOG_ERROR("[QUESTS] Failed to load user %d for quest reward claim", user_id);
                return -4;
            }

            float old_balance = user.balance;
            user.balance += reward;
            LOG_INFO("[QUESTS] Claiming quest reward: user_id=%d, quest_id=%d, quest_type=%d, reward=$%.2f, balance: $%.2f -> $%.2f",
                     user_id, quest_id, quests[i].quest_type, reward, old_balance, user.balance);
            
            if (db_update_user(&user) != 0)
            {
                LOG_ERROR("[QUESTS] Failed to update user balance after quest reward claim: user_id=%d", user_id);
                return -5;
            }

            // Verify balance was actually updated
            User verify_user;
            if (db_load_user(user_id, &verify_user) == 0)
            {
                if (verify_user.balance != user.balance)
                {
                    LOG_ERROR("[QUESTS] Balance verification failed: expected $%.2f, got $%.2f", 
                             user.balance, verify_user.balance);
                    return -7;
                }
                LOG_INFO("[QUESTS] Balance verified: user_id=%d, balance=$%.2f", user_id, verify_user.balance);
            }

            // Mark as claimed
            quests[i].is_claimed = 1;
            if (db_update_quest(&quests[i]) != 0)
            {
                LOG_ERROR("[QUESTS] Failed to mark quest as claimed: quest_id=%d", quest_id);
                return -6;
            }

            LOG_INFO("[QUESTS] Quest reward claimed successfully: user_id=%d, quest_id=%d, reward=$%.2f, new_balance=$%.2f",
                     user_id, quest_id, reward, user.balance);
            return 0;
        }
    }

    return -1; // Quest not found
}

// Check and complete quests (called after actions)
// NOTE: This function only marks quests as completed when progress >= target.
// It does NOT automatically claim rewards - player must manually claim via claim_quest_reward()
void check_quest_completion(int user_id)
{
    Quest quests[10];
    int count = 0;
    if (db_load_user_quests(user_id, quests, &count) != 0)
        return;

    for (int i = 0; i < count; i++)
    {
        if (!quests[i].is_completed && quests[i].progress >= quests[i].target)
        {
            // Mark quest as completed, but DO NOT claim reward automatically
            // Player must manually claim reward via claim_quest_reward()
            quests[i].is_completed = 1;
            quests[i].completed_at = time(NULL);
            // is_claimed remains 0 - player must claim manually
            db_update_quest(&quests[i]);
            LOG_INFO("[QUESTS] Quest auto-completed (not claimed): user_id=%d, quest_id=%d, quest_type=%d, progress=%d/%d",
                     user_id, quests[i].quest_id, quests[i].quest_type, quests[i].progress, quests[i].target);
        }
    }
}

