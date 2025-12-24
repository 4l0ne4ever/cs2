#ifndef PRICE_TRACKING_H
#define PRICE_TRACKING_H

#include "types.h"
#include <time.h>

// Save price history when transaction occurs
int save_price_history(int definition_id, float price, int transaction_type);

// Get price history for last 24 hours
int get_price_history_24h(int definition_id, PriceHistoryEntry *out_history, int *count);

// Calculate price trend (compare current vs 24h ago)
int calculate_price_trend(int definition_id, PriceTrend *out_trend);

// Get price trend for a skin definition
int get_price_trend(int definition_id, PriceTrend *out_trend);

#endif // PRICE_TRACKING_H

