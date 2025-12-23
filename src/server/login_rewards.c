// login_rewards.c - Daily Login Rewards System

#include "../include/login_rewards.h"
#include "../include/database.h"
#include "../include/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Get login streak info
int get_login_streak(int user_id, LoginStreak *out_streak)
{
    if (user_id <= 0 || !out_streak)
        return -1;

    if (db_load_login_streak(user_id, out_streak) == 0)
        return 0;

    // Create new streak if doesn't exist
    out_streak->user_id = user_id;
    out_streak->current_streak = 0;
    out_streak->last_login_date = 0;
    out_streak->last_reward_date = 0;
    return 0;
}

// Claim daily login reward
int claim_daily_reward(int user_id, float *reward_amount, int *streak_day)
{
    if (user_id <= 0 || !reward_amount || !streak_day)
        return -1;

    LoginStreak streak;
    if (get_login_streak(user_id, &streak) != 0)
        return -1;

    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    time_t today = mktime(tm_now);
    tm_now->tm_hour = 0;
    tm_now->tm_min = 0;
    tm_now->tm_sec = 0;
    today = mktime(tm_now);

    // Check if already claimed today
    if (streak.last_reward_date == today)
        return -2; // Already claimed today

    // Check if streak should continue or reset
    int days_diff = (today - streak.last_login_date) / (24 * 3600);
    
    if (days_diff == 1)
    {
        // Continue streak
        streak.current_streak++;
        if (streak.current_streak > 7)
            streak.current_streak = 1; // Reset to day 1 after day 7
    }
    else if (days_diff > 1)
    {
        // Streak broken, reset
        streak.current_streak = 1;
    }
    else if (streak.current_streak == 0)
    {
        // First login
        streak.current_streak = 1;
    }

    // Calculate reward based on streak day
    float reward = 0.0f;
    switch (streak.current_streak)
    {
    case 1:
        reward = REWARD_DAY_1;
        break;
    case 2:
        reward = REWARD_DAY_2;
        break;
    case 3:
        reward = REWARD_DAY_3;
        break;
    case 4:
        reward = REWARD_DAY_4;
        break;
    case 5:
        reward = REWARD_DAY_5;
        break;
    case 6:
        reward = REWARD_DAY_6;
        break;
    case 7:
        reward = REWARD_DAY_7;
        break;
    default:
        reward = REWARD_DAY_1;
        streak.current_streak = 1;
        break;
    }

    // Add reward to user balance
    User user;
    if (db_load_user(user_id, &user) != 0)
        return -3;

    user.balance += reward;
    if (db_update_user(&user) != 0)
        return -4;

    // Update streak
    streak.last_login_date = today;
    streak.last_reward_date = today;
    if (db_save_login_streak(&streak) != 0)
        return -5;

    *reward_amount = reward;
    *streak_day = streak.current_streak;
    return 0;
}

// Update login streak on login
int update_login_streak(int user_id)
{
    if (user_id <= 0)
        return -1;

    LoginStreak streak;
    if (get_login_streak(user_id, &streak) != 0)
        return -1;

    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    tm_now->tm_hour = 0;
    tm_now->tm_min = 0;
    tm_now->tm_sec = 0;
    time_t today = mktime(tm_now);

    // Update last login date (but don't claim reward - that's separate)
    streak.last_login_date = today;
    return db_save_login_streak(&streak);
}

