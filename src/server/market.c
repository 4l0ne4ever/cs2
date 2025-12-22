// market.c - Market Engine Implementation

#include "../include/market.h"
#include "../include/database.h"
#include "../include/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MARKET_FEE_RATE 0.15f // 15% fee

// List all active market listings
int get_market_listings(MarketListing *out_listings, int *count)
{
    if (!out_listings || !count)
        return -1;

    return db_load_listings_v2(out_listings, count);
}

// List a skin instance on market
int list_skin_on_market(int user_id, int instance_id, float price)
{
    if (user_id <= 0 || instance_id <= 0 || price <= 0)
        return -1;

    // Verify instance belongs to user
    int definition_id, owner_id;
    SkinRarity rarity;
    WearCondition wear;
    time_t acquired_at;
    int is_tradable;

    int pattern_seed, is_stattrak; // Not used in market operations
    if (db_load_skin_instance(instance_id, &definition_id, &rarity, &wear, &pattern_seed, &is_stattrak, &owner_id, &acquired_at, &is_tradable) != 0)
        return -1; // Instance not found

    if (owner_id != user_id)
        return -2; // Not owner

    // Check if already listed
    // (In a full implementation, we'd check market_listings_v2 for active listings)
    // For now, we'll just create the listing

    // Create listing
    int listing_id;
    if (db_save_listing_v2(user_id, instance_id, price, &listing_id) != 0)
        return -3; // Failed to create listing

    return 0;
}

// Buy skin from market
int buy_from_market(int buyer_id, int listing_id)
{
    if (buyer_id <= 0 || listing_id <= 0)
        return -1;

    // Get listing info
    int seller_id, instance_id, is_sold;
    float price;

    if (db_get_listing_v2(listing_id, &seller_id, &instance_id, &price, &is_sold) != 0)
        return -1; // Listing not found

    if (is_sold)
        return -2; // Already sold

    if (seller_id == buyer_id)
        return -3; // Cannot buy own listing

    // Get buyer balance
    User buyer;
    if (db_load_user(buyer_id, &buyer) != 0)
        return -4; // Buyer not found

    if (buyer.balance < price)
        return -5; // Insufficient funds

    // Get seller info
    User seller;
    if (db_load_user(seller_id, &seller) != 0)
        return -6; // Seller not found

    // Calculate fee and seller payout
    float fee = price * MARKET_FEE_RATE;
    float seller_payout = price - fee;

    // Update balances
    buyer.balance -= price;
    seller.balance += seller_payout;

    if (db_update_user(&buyer) != 0)
        return -7; // Failed to update buyer

    if (db_update_user(&seller) != 0)
        return -8; // Failed to update seller

    // Mark listing as sold first
    if (db_mark_listing_sold(listing_id) != 0)
        return -9; // Failed to mark listing as sold

    // Transfer instance ownership
    if (db_update_skin_instance_owner(instance_id, buyer_id) != 0)
    {
        // Rollback: restore listing and balances
        db_mark_listing_sold(listing_id); // Revert
        buyer.balance += price;
        seller.balance -= seller_payout;
        db_update_user(&buyer);
        db_update_user(&seller);
        return -10; // Failed to transfer ownership
    }

    // Update inventories
    db_remove_from_inventory(seller_id, instance_id);
    if (db_add_to_inventory(buyer_id, instance_id) != 0)
    {
        // Rollback: restore ownership and balances
        db_update_skin_instance_owner(instance_id, seller_id);
        db_mark_listing_sold(listing_id); // Revert
        buyer.balance += price;
        seller.balance -= seller_payout;
        db_update_user(&buyer);
        db_update_user(&seller);
        return -11; // Failed to add to inventory
    }

    // Log transaction
    TransactionLog log;
    log.log_id = 0; // Auto-increment
    log.type = LOG_MARKET_BUY;
    log.user_id = buyer_id;
    snprintf(log.details, sizeof(log.details), "Bought instance %d for $%.2f", instance_id, price);
    log.timestamp = time(NULL);
    db_log_transaction(&log);

    TransactionLog log2;
    log2.log_id = 0;
    log2.type = LOG_MARKET_SELL;
    log2.user_id = seller_id;
    snprintf(log2.details, sizeof(log2.details), "Sold instance %d for $%.2f (received $%.2f after fee)", instance_id, price, seller_payout);
    log2.timestamp = time(NULL);
    db_log_transaction(&log2);

    return 0;
}

// Remove listing from market
int remove_listing(int listing_id)
{
    if (listing_id <= 0)
        return -1;

    // Verify listing exists and is not sold
    int seller_id, instance_id, is_sold;
    float price;

    if (db_get_listing_v2(listing_id, &seller_id, &instance_id, &price, &is_sold) != 0)
        return -1; // Listing not found

    if (is_sold)
        return -2; // Already sold

    // Remove listing
    return db_remove_listing_v2(listing_id);
}

// Update market prices based on supply/demand (simplified)
void update_market_prices()
{
    // This is a placeholder for price dynamics
    // In a full implementation, this would:
    // 1. Analyze supply/demand from recent transactions
    // 2. Adjust base prices in skin_definitions
    // 3. Update current prices for all instances
    // For now, prices are calculated on-the-fly from base_price * wear_multiplier
}

// Get current price for a skin definition with specific rarity and wear
float get_current_price(int definition_id, SkinRarity rarity, WearCondition wear)
{
    return db_calculate_skin_price(definition_id, rarity, wear);
}

