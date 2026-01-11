// achievements.c - Achievements System

#include "../include/achievements.h"
#include "../include/database.h"
#include "../include/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Get user's achievements
int get_user_achievements(int user_id, Achievement *out_achievements, int *count)
{
    if (user_id <= 0 || !out_achievements || !count)
        return -1;

    return db_load_user_achievements(user_id, out_achievements, count);
}

// Unlock achievement (with atomic check-and-unlock to prevent race conditions)
int unlock_achievement(int user_id, AchievementType achievement_type)
{
    if (user_id <= 0)
        return -1;

    // BEGIN TRANSACTION - Atomic check-and-unlock to prevent race conditions
    if (db_begin_transaction() != 0)
        return -1;

    // Check if already unlocked (within transaction)
    Achievement achievements[10];
    int count = 0;
    if (db_load_user_achievements(user_id, achievements, &count) == 0)
    {
        for (int i = 0; i < count; i++)
        {
            if (achievements[i].achievement_type == achievement_type && achievements[i].is_unlocked)
            {
                db_rollback_transaction();
                return 0; // Already unlocked
            }
        }
    }

    // Create new achievement
    Achievement achievement = {0};
    achievement.user_id = user_id;
    achievement.achievement_type = achievement_type;
    achievement.is_unlocked = 1;
    achievement.is_claimed = 0;
    achievement.unlocked_at = time(NULL);

    int result = db_save_achievement(&achievement);
    if (result == 0)
    {
        // COMMIT TRANSACTION - Unlock succeeded
        if (db_commit_transaction() != 0)
        {
            db_rollback_transaction();
            return -1;
        }
        return 0;
    }
    else
    {
        db_rollback_transaction();
        return -1;
    }
}

// Claim achievement reward
int claim_achievement_reward(int user_id, int achievement_id)
{
    if (user_id <= 0 || achievement_id <= 0)
        return -1;

    Achievement achievements[10];
    int count = 0;
    if (db_load_user_achievements(user_id, achievements, &count) != 0)
        return -1;

    // Find achievement
    for (int i = 0; i < count; i++)
    {
        if (achievements[i].achievement_id == achievement_id)
        {
            if (!achievements[i].is_unlocked)
                return -2; // Not unlocked
            if (achievements[i].is_claimed)
                return -3; // Already claimed

            // Calculate reward
            float reward = 0.0f;
            switch (achievements[i].achievement_type)
            {
            case ACHIEVEMENT_FIRST_TRADE:
                reward = ACHIEVEMENT_REWARD_FIRST_TRADE;
                break;
            case ACHIEVEMENT_FIRST_KNIFE:
                reward = ACHIEVEMENT_REWARD_FIRST_KNIFE;
                break;
            case ACHIEVEMENT_PROFIT_1000:
                reward = ACHIEVEMENT_REWARD_PROFIT_1000;
                break;
            case ACHIEVEMENT_100_TRADES:
                reward = ACHIEVEMENT_REWARD_100_TRADES;
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
            achievements[i].is_claimed = 1;
            if (db_update_achievement(&achievements[i]) != 0)
                return -6;

            return 0;
        }
    }

    return -1; // Achievement not found
}

// Check achievements that should be unlocked
void check_achievements(int user_id)
{
    // This function should be called after significant actions
    // Implementation depends on tracking stats (trades completed, profit made, etc.)
    // For now, this is a placeholder - actual checking will be done in trading.c, market.c, etc.
}

