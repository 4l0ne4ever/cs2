#ifndef LOGIN_REWARDS_H
#define LOGIN_REWARDS_H

#include "types.h"

// Daily login rewards (Day 1-7)
#define REWARD_DAY_1 5.0f
#define REWARD_DAY_2 8.0f
#define REWARD_DAY_3 12.0f
#define REWARD_DAY_4 15.0f
#define REWARD_DAY_5 20.0f
#define REWARD_DAY_6 25.0f
#define REWARD_DAY_7 50.0f

// Get login streak info
int get_login_streak(int user_id, LoginStreak *out_streak);

// Claim daily login reward
int claim_daily_reward(int user_id, float *reward_amount, int *streak_day);

// Update login streak on login
int update_login_streak(int user_id);

#endif // LOGIN_REWARDS_H

