// quests.c - Daily Quests System

#include "../include/quests.h"
#include "../include/database.h"
#include "../include/types.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Initialize daily quests for a new user or reset for existing user
int init_daily_quests(int user_id)
{
    if (user_id <= 0)
        return -1;

    // Delete old incomplete quests for this user
    sqlite3 *db;
    if (sqlite3_open("data/database.db", &db) != SQLITE_OK)
        return -1;

    const char *delete_sql = "DELETE FROM quests WHERE user_id = ? AND is_completed = 0";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, delete_sql, -1, &stmt, 0) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
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

    return 0;
}

// Get user's active quests
int get_user_quests(int user_id, Quest *out_quests, int *count)
{
    if (user_id <= 0 || !out_quests || !count)
        return -1;

    return db_load_user_quests(user_id, out_quests, count);
}

// Update quest progress
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
                quests[i].is_completed = 1;
                quests[i].completed_at = time(NULL);
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
                return -4;

            user.balance += reward;
            if (db_update_user(&user) != 0)
                return -5;

            // Mark as claimed
            quests[i].is_claimed = 1;
            if (db_update_quest(&quests[i]) != 0)
                return -6;

            return 0;
        }
    }

    return -1; // Quest not found
}

// Check and complete quests (called after actions)
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
            quests[i].is_completed = 1;
            quests[i].completed_at = time(NULL);
            db_update_quest(&quests[i]);
        }
    }
}

