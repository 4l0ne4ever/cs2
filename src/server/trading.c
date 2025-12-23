// trading.c - Trading System Implementation

#include "../include/trading.h"
#include "../include/database.h"
#include "../include/types.h"
#include "../include/quests.h"
#include "../include/achievements.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TRADE_EXPIRY_SECONDS (15 * 60) // 15 minutes

// Send trade offer
int send_trade_offer(int from_user, int to_user, TradeOffer *offer)
{
    if (!offer || from_user <= 0 || to_user <= 0 || from_user == to_user)
        return -1;

    // Validate trade
    if (validate_trade(offer) != 0)
        return -2;

    // Set trade details
    offer->from_user_id = from_user;
    offer->to_user_id = to_user;
    offer->status = TRADE_PENDING;
    offer->created_at = time(NULL);
    offer->expires_at = offer->created_at + TRADE_EXPIRY_SECONDS;
    offer->trade_id = 0; // Auto-increment

    // Save to database
    if (db_save_trade(offer) != 0)
        return -3;

    // Log transaction
    TransactionLog log;
    log.log_id = 0;
    log.type = LOG_TRADE;
    log.user_id = from_user;
    snprintf(log.details, sizeof(log.details), "Sent trade offer %d to user %d", offer->trade_id, to_user);
    log.timestamp = time(NULL);
    db_log_transaction(&log);

    return 0;
}

// Accept trade
int accept_trade(int user_id, int trade_id)
{
    if (user_id <= 0 || trade_id <= 0)
        return -1;

    // Load trade
    TradeOffer trade;
    if (db_load_trade(trade_id, &trade) != 0)
        return -1; // Trade not found

    // Check if user is the receiver
    if (trade.to_user_id != user_id)
        return -2; // Not authorized

    // Check if trade is still pending
    if (trade.status != TRADE_PENDING)
        return -3; // Trade already processed

    // Check if trade expired
    if (time(NULL) > trade.expires_at)
    {
        trade.status = TRADE_EXPIRED;
        db_update_trade(&trade);
        return -4; // Trade expired
    }

    // Execute trade
    if (execute_trade(&trade) != 0)
        return -5; // Execution failed

    // Update trade status
    trade.status = TRADE_ACCEPTED;
    db_update_trade(&trade);

    // Log transaction
    TransactionLog log;
    log.log_id = 0;
    log.type = LOG_TRADE;
    log.user_id = user_id;
    snprintf(log.details, sizeof(log.details), "Accepted trade offer %d", trade_id);
    log.timestamp = time(NULL);
    db_log_transaction(&log);

    return 0;
}

// Decline trade
int decline_trade(int user_id, int trade_id)
{
    if (user_id <= 0 || trade_id <= 0)
        return -1;

    // Load trade
    TradeOffer trade;
    if (db_load_trade(trade_id, &trade) != 0)
        return -1; // Trade not found

    // Check if user is the receiver
    if (trade.to_user_id != user_id)
        return -2; // Not authorized

    // Check if trade is still pending
    if (trade.status != TRADE_PENDING)
        return -3; // Trade already processed

    // Update trade status
    trade.status = TRADE_DECLINED;
    db_update_trade(&trade);

    // Log transaction
    TransactionLog log;
    log.log_id = 0;
    log.type = LOG_TRADE;
    log.user_id = user_id;
    snprintf(log.details, sizeof(log.details), "Declined trade offer %d", trade_id);
    log.timestamp = time(NULL);
    db_log_transaction(&log);

    return 0;
}

// Cancel trade
int cancel_trade(int user_id, int trade_id)
{
    if (user_id <= 0 || trade_id <= 0)
        return -1;

    // Load trade
    TradeOffer trade;
    if (db_load_trade(trade_id, &trade) != 0)
        return -1; // Trade not found

    // Check if user is the sender
    if (trade.from_user_id != user_id)
        return -2; // Not authorized

    // Check if trade is still pending
    if (trade.status != TRADE_PENDING)
        return -3; // Trade already processed

    // Update trade status
    trade.status = TRADE_CANCELLED;
    db_update_trade(&trade);

    // Log transaction
    TransactionLog log;
    log.log_id = 0;
    log.type = LOG_TRADE;
    log.user_id = user_id;
    snprintf(log.details, sizeof(log.details), "Cancelled trade offer %d", trade_id);
    log.timestamp = time(NULL);
    db_log_transaction(&log);

    return 0;
}

// Get active trades for user
int get_user_trades(int user_id, TradeOffer *out_trades, int *count)
{
    if (!out_trades || !count || user_id <= 0)
        return -1;

    return db_get_user_trades(user_id, out_trades, count);
}

// Validate trade offer
int validate_trade(TradeOffer *offer)
{
    if (!offer)
        return -1;

    // Check if from_user owns offered items
    for (int i = 0; i < offer->offered_count; i++)
    {
        int instance_id = offer->offered_skins[i];
        if (instance_id <= 0)
            continue;

        // Check ownership
        int definition_id, owner_id;
        WearCondition wear;
        time_t acquired_at;
        int is_tradable;

        SkinRarity rarity;
        int pattern_seed, is_stattrak; // Not used in trading operations
        if (db_load_skin_instance(instance_id, &definition_id, &rarity, &wear, &pattern_seed, &is_stattrak, &owner_id, &acquired_at, &is_tradable) != 0)
            return -2; // Instance not found

        if (owner_id != offer->from_user_id)
            return -3; // Not owner

        // Note: Trade lock check removed - items are only locked when listed on market
        // Trading between users does not require items to be unlocked
    }

    // Check if to_user owns requested items
    for (int i = 0; i < offer->requested_count; i++)
    {
        int instance_id = offer->requested_skins[i];
        if (instance_id <= 0)
            continue;

        // Check ownership
        int definition_id, owner_id;
        SkinRarity rarity;
        WearCondition wear;
        int pattern_seed2, is_stattrak2; // Not used in trading operations
        time_t acquired_at;
        int is_tradable;

        if (db_load_skin_instance(instance_id, &definition_id, &rarity, &wear, &pattern_seed2, &is_stattrak2, &owner_id, &acquired_at, &is_tradable) != 0)
            return -5; // Instance not found

        if (owner_id != offer->to_user_id)
            return -6; // Not owner

        // Note: Trade lock check removed - items are only locked when listed on market
        // Trading between users does not require items to be unlocked
    }

    // Check cash balances
    if (offer->offered_cash > 0)
    {
        User from_user;
        if (db_load_user(offer->from_user_id, &from_user) != 0)
            return -8; // User not found

        if (from_user.balance < offer->offered_cash)
            return -9; // Insufficient funds
    }

    if (offer->requested_cash > 0)
    {
        User to_user;
        if (db_load_user(offer->to_user_id, &to_user) != 0)
            return -10; // User not found

        if (to_user.balance < offer->requested_cash)
            return -11; // Insufficient funds
    }

    // Validate: must offer something OR request something (or both)
    if (offer->offered_count == 0 && offer->offered_cash == 0.0f && 
        offer->requested_count == 0 && offer->requested_cash == 0.0f)
    {
        return -12; // Empty trade
    }

    return 0;
}

// Execute trade (atomic operation)
int execute_trade(TradeOffer *offer)
{
    if (!offer)
        return -1;

    // Start transaction (SQLite auto-commits, but we'll do manual checks)
    // In a real implementation, we'd use BEGIN TRANSACTION

    // Transfer offered items from from_user to to_user
    for (int i = 0; i < offer->offered_count; i++)
    {
        int instance_id = offer->offered_skins[i];
        if (instance_id <= 0)
            continue;

        // Remove from from_user inventory
        db_remove_from_inventory(offer->from_user_id, instance_id);

        // Update owner
        if (db_update_skin_instance_owner(instance_id, offer->to_user_id) != 0)
            return -2; // Failed to transfer ownership

        // Add to to_user inventory
        if (db_add_to_inventory(offer->to_user_id, instance_id) != 0)
            return -3; // Failed to add to inventory

        // Note: Items traded between users are NOT trade locked - only items listed on market are locked
    }

    // Transfer requested items from to_user to from_user
    for (int i = 0; i < offer->requested_count; i++)
    {
        int instance_id = offer->requested_skins[i];
        if (instance_id <= 0)
            continue;

        // Remove from to_user inventory
        db_remove_from_inventory(offer->to_user_id, instance_id);

        // Update owner
        if (db_update_skin_instance_owner(instance_id, offer->from_user_id) != 0)
            return -4; // Failed to transfer ownership

        // Add to from_user inventory
        if (db_add_to_inventory(offer->from_user_id, instance_id) != 0)
            return -5; // Failed to add to inventory

        // Note: Items traded between users are NOT trade locked - only items listed on market are locked
    }

    // Transfer cash
    if (offer->offered_cash > 0)
    {
        User from_user, to_user;
        if (db_load_user(offer->from_user_id, &from_user) != 0)
            return -6;
        if (db_load_user(offer->to_user_id, &to_user) != 0)
            return -7;

        from_user.balance -= offer->offered_cash;
        to_user.balance += offer->offered_cash;

        if (db_update_user(&from_user) != 0)
            return -8;
        if (db_update_user(&to_user) != 0)
            return -9;
    }

    if (offer->requested_cash > 0)
    {
        User from_user, to_user;
        if (db_load_user(offer->from_user_id, &from_user) != 0)
            return -10;
        if (db_load_user(offer->to_user_id, &to_user) != 0)
            return -11;

        to_user.balance -= offer->requested_cash;
        from_user.balance += offer->requested_cash;

        if (db_update_user(&from_user) != 0)
            return -12;
        if (db_update_user(&to_user) != 0)
            return -13;
    }

    // Update quests and achievements
    // First Steps quest: Complete 3 trades
    update_quest_progress(offer->from_user_id, QUEST_FIRST_STEPS, 1);
    update_quest_progress(offer->to_user_id, QUEST_FIRST_STEPS, 1);
    
    // Social Trader quest: Trade with different users
    // Track unique users traded with (simplified - just increment)
    update_quest_progress(offer->from_user_id, QUEST_SOCIAL_TRADER, 1);
    update_quest_progress(offer->to_user_id, QUEST_SOCIAL_TRADER, 1);
    
    // Check achievements
    // First Trade achievement
    unlock_achievement(offer->from_user_id, ACHIEVEMENT_FIRST_TRADE);
    unlock_achievement(offer->to_user_id, ACHIEVEMENT_FIRST_TRADE);
    
    // Check quest completion
    check_quest_completion(offer->from_user_id);
    check_quest_completion(offer->to_user_id);

    return 0;
}

// Clean expired trades
void clean_expired_trades()
{
    db_clean_expired_trades();
}

// Check if skin instance is trade locked
int is_trade_locked(int instance_id)
{
    int is_locked = 0;
    if (db_check_trade_lock(instance_id, &is_locked) == 0)
        return is_locked;
    return 1; // Assume locked if check fails
}

// Apply trade lock to skin instance
void apply_trade_lock(int instance_id)
{
    db_apply_trade_lock(instance_id);
}

// Check and unlock expired trade locks
void check_expired_locks()
{
    db_unlock_expired_trades();
}

