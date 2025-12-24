#ifndef LEADERBOARDS_H
#define LEADERBOARDS_H

#include "types.h"

// Leaderboard types
typedef enum {
    LEADERBOARD_TOP_TRADERS = 0,    // By net worth
    LEADERBOARD_LUCKIEST_UNBOXERS,  // By best unbox value
    LEADERBOARD_MOST_PROFITABLE     // By total profit
} LeaderboardType;

// Get top traders by net worth
int get_top_traders(LeaderboardEntry *out_entries, int *count, int limit);

// Get luckiest unboxers (best unbox value)
int get_luckiest_unboxers(LeaderboardEntry *out_entries, int *count, int limit);

// Get most profitable traders
int get_most_profitable(LeaderboardEntry *out_entries, int *count, int limit);

#endif // LEADERBOARDS_H
