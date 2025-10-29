#ifndef MARKET_H
#define MARKET_H

#include "types.h"

// List all market listings
int get_market_listings(MarketListing *out_listings, int *count);

// List specific skin on market
int list_skin_on_market(int user_id, int skin_id, float price);

// Buy skin from market
int buy_from_market(int buyer_id, int listing_id);

// Remove listing
int remove_listing(int listing_id);

// Update market prices based on supply/demand
void update_market_prices();

// Get current price for a skin type
float get_current_price(int skin_type, WearCondition wear);

#endif // MARKET_H
