#ifndef TRADING_CHALLENGES_H
#define TRADING_CHALLENGES_H

#include "types.h"
#include <time.h>

// Challenge types
typedef enum {
    CHALLENGE_1V1_PROFIT_RACE = 0
} ChallengeType;

// Challenge status
typedef enum {
    CHALLENGE_PENDING = 0,    // Waiting for opponent to accept
    CHALLENGE_ACTIVE,          // Challenge in progress
    CHALLENGE_COMPLETED,       // Challenge finished
    CHALLENGE_CANCELLED        // Challenge cancelled
} ChallengeStatus;

// Trading challenge
typedef struct {
    int challenge_id;
    int challenger_id;
    int opponent_id;
    ChallengeType type;
    ChallengeStatus status;
    float challenger_start_balance;
    float opponent_start_balance;
    float challenger_current_profit;
    float opponent_current_profit;
    time_t start_time;
    time_t end_time;
    int duration_minutes; // Challenge duration in minutes
    int challenger_cancel_vote; // 0 = not voted, 1 = voted to cancel
    int opponent_cancel_vote;   // 0 = not voted, 1 = voted to cancel
} TradingChallenge;

// Create a 1v1 profit race challenge
int create_profit_race_challenge(int challenger_id, int opponent_id, int duration_minutes, int *out_challenge_id);

// Get challenge by ID
int get_challenge(int challenge_id, TradingChallenge *out_challenge);

// Get user's active challenges
int get_user_challenges(int user_id, TradingChallenge *out_challenges, int *count);

// Accept challenge (change status from PENDING to ACTIVE)
int accept_challenge(int challenge_id, int user_id);

// Update challenge progress (calculate current profit)
int update_challenge_progress(int challenge_id);

// Complete challenge and determine winner
int complete_challenge(int challenge_id, int *winner_id);

// Cancel challenge
int cancel_challenge(int challenge_id, int user_id);

// Helper function: Update all active challenges for a user
// Called automatically after actions that affect profit (unbox, market, trading)
void update_user_active_challenges(int user_id);

#endif // TRADING_CHALLENGES_H
