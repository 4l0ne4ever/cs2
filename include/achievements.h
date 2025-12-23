#ifndef ACHIEVEMENTS_H
#define ACHIEVEMENTS_H

#include "types.h"

// Achievement rewards
#define ACHIEVEMENT_REWARD_FIRST_TRADE 20.0f
#define ACHIEVEMENT_REWARD_FIRST_KNIFE 500.0f
#define ACHIEVEMENT_REWARD_PROFIT_1000 100.0f
#define ACHIEVEMENT_REWARD_100_TRADES 200.0f

// Get user's achievements
int get_user_achievements(int user_id, Achievement *out_achievements, int *count);

// Unlock achievement
int unlock_achievement(int user_id, AchievementType achievement_type);

// Claim achievement reward
int claim_achievement_reward(int user_id, int achievement_id);

// Check achievements that should be unlocked
void check_achievements(int user_id);

#endif // ACHIEVEMENTS_H

