#ifndef QUESTS_H
#define QUESTS_H

#include "types.h"

// Quest rewards
#define QUEST_REWARD_FIRST_STEPS 15.0f
#define QUEST_REWARD_MARKET_EXPLORER 10.0f
#define QUEST_REWARD_LUCKY_GAMBLER 25.0f
#define QUEST_REWARD_PROFIT_MAKER 30.0f
#define QUEST_REWARD_SOCIAL_TRADER 50.0f

// Quest targets
#define QUEST_TARGET_FIRST_STEPS 3
#define QUEST_TARGET_MARKET_EXPLORER 5
#define QUEST_TARGET_LUCKY_GAMBLER 5
#define QUEST_TARGET_PROFIT_MAKER 50.0f
#define QUEST_TARGET_SOCIAL_TRADER 10

// Initialize daily quests for user
int init_daily_quests(int user_id);

// Get user's active quests
int get_user_quests(int user_id, Quest *out_quests, int *count);

// Update quest progress
int update_quest_progress(int user_id, QuestType quest_type, int increment);

// Claim quest reward
int claim_quest_reward(int user_id, int quest_id);

// Check and complete quests
void check_quest_completion(int user_id);

#endif // QUESTS_H

