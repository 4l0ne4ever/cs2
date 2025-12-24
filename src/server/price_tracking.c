// price_tracking.c - Price Tracking and Chart Implementation

#include "../include/price_tracking.h"
#include "../include/database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Save price history when transaction occurs
int save_price_history(int definition_id, float price, int transaction_type)
{
    return db_save_price_history(definition_id, price, transaction_type);
}

// Get price history for last 24 hours
int get_price_history_24h(int definition_id, PriceHistoryEntry *out_history, int *count)
{
    return db_get_price_history_24h(definition_id, out_history, count);
}

// Calculate price trend (compare current vs 24h ago)
int calculate_price_trend(int definition_id, PriceTrend *out_trend)
{
    if (!out_trend || definition_id <= 0)
        return -1;

    // Get current price (from latest transaction or calculated price)
    float current_price = 0.0f;
    PriceHistoryEntry history[100];
    int history_count = 0;
    
    if (db_get_price_history_24h(definition_id, history, &history_count) == 0 && history_count > 0)
    {
        // Use most recent price as current
        current_price = history[history_count - 1].price;
    }
    else
    {
        // No history, use base price calculation
        // For now, return error - in full implementation, calculate from definition
        return -1;
    }

    // Get price 24h ago
    float price_24h_ago = 0.0f;
    if (db_get_price_24h_ago(definition_id, &price_24h_ago) != 0)
    {
        // No price 24h ago, use current price as baseline
        price_24h_ago = current_price;
    }

    // Calculate percentage change
    float price_change_percent = 0.0f;
    if (price_24h_ago > 0.0f)
    {
        price_change_percent = ((current_price - price_24h_ago) / price_24h_ago) * 100.0f;
    }

    // Determine trend symbol
    const char *trend_symbol = "═"; // Default: no change
    if (price_change_percent > 0.1f) // More than 0.1% increase
    {
        trend_symbol = "▲";
    }
    else if (price_change_percent < -0.1f) // More than 0.1% decrease
    {
        trend_symbol = "▼";
    }

    // Fill output
    out_trend->definition_id = definition_id;
    out_trend->current_price = current_price;
    out_trend->price_24h_ago = price_24h_ago;
    out_trend->price_change_percent = price_change_percent;
    strncpy(out_trend->trend_symbol, trend_symbol, 3);
    out_trend->trend_symbol[3] = '\0';

    return 0;
}

// Get price trend for a skin definition
int get_price_trend(int definition_id, PriceTrend *out_trend)
{
    return calculate_price_trend(definition_id, out_trend);
}

