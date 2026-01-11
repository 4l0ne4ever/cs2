// trading.c - Trading System Implementation

#include "../include/trading.h"
#include "../include/database.h"
#include "../include/types.h"
#include "../include/quests.h"
#include "../include/achievements.h"
#include "../include/trading_challenges.h"
#include "../include/logger.h"
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

    // CRITICAL: Validate items are still available before executing trade
    // This prevents duplicate items when multiple trade offers contain the same item
    // Check offered items are still owned by from_user and in their inventory
    for (int i = 0; i < trade.offered_count; i++)
    {
        int instance_id = trade.offered_skins[i];
        if (instance_id <= 0)
            continue;

        int definition_id, owner_id;
        SkinRarity rarity;
        WearCondition wear;
        int pattern_seed, is_stattrak;
        time_t acquired_at;
        int is_tradable;

        if (db_load_skin_instance(instance_id, &definition_id, &rarity, &wear, &pattern_seed, &is_stattrak, &owner_id, &acquired_at, &is_tradable) != 0)
        {
            // Item no longer exists
            trade.status = TRADE_EXPIRED;
            db_update_trade(&trade);
            return -6; // Item no longer available
        }

        if (owner_id != trade.from_user_id)
        {
            // Item no longer owned by from_user (may have been traded/sold)
            trade.status = TRADE_EXPIRED;
            db_update_trade(&trade);
            return -6; // Item no longer available
        }

        // Check if item is still in from_user's inventory
        Inventory inv;
        if (db_load_inventory(trade.from_user_id, &inv) == 0)
        {
            int found = 0;
            for (int j = 0; j < inv.count; j++)
            {
                if (inv.skin_ids[j] == instance_id)
                {
                    found = 1;
                    break;
                }
            }
            if (!found)
            {
                // Item not in inventory (may have been removed by another trade)
                trade.status = TRADE_EXPIRED;
                db_update_trade(&trade);
                return -6; // Item no longer available
            }
        }
    }

    // Check requested items are still owned by to_user and in their inventory
    for (int i = 0; i < trade.requested_count; i++)
    {
        int instance_id = trade.requested_skins[i];
        if (instance_id <= 0)
            continue;

        int definition_id, owner_id;
        SkinRarity rarity;
        WearCondition wear;
        int pattern_seed, is_stattrak;
        time_t acquired_at;
        int is_tradable;

        if (db_load_skin_instance(instance_id, &definition_id, &rarity, &wear, &pattern_seed, &is_stattrak, &owner_id, &acquired_at, &is_tradable) != 0)
        {
            // Item no longer exists
            trade.status = TRADE_EXPIRED;
            db_update_trade(&trade);
            return -6; // Item no longer available
        }

        if (owner_id != trade.to_user_id)
        {
            // Item no longer owned by to_user (may have been traded/sold)
            trade.status = TRADE_EXPIRED;
            db_update_trade(&trade);
            return -6; // Item no longer available
        }

        // Check if item is still in to_user's inventory
        Inventory inv;
        if (db_load_inventory(trade.to_user_id, &inv) == 0)
        {
            int found = 0;
            for (int j = 0; j < inv.count; j++)
            {
                if (inv.skin_ids[j] == instance_id)
                {
                    found = 1;
                    break;
                }
            }
            if (!found)
            {
                // Item not in inventory (may have been removed by another trade)
                trade.status = TRADE_EXPIRED;
                db_update_trade(&trade);
                return -6; // Item no longer available
            }
        }
    }

    // BEGIN TRANSACTION - All operations must succeed or all rollback
    if (db_begin_transaction() != 0)
        return -19; // Failed to begin transaction

    // Atomically accept trade (prevents race condition when multiple clients accept same trade)
    int atomic_result = db_atomic_accept_trade(trade_id);
    if (atomic_result == -1)
    {
        db_rollback_transaction();
        return -1; // Trade not found
    }
    if (atomic_result == -2)
    {
        db_rollback_transaction();
        return -3; // Trade already processed (race condition detected)
    }

    // Execute trade (within transaction)
    if (execute_trade_internal(&trade) != 0)
    {
        db_rollback_transaction();
        return -5; // Execution failed
    }

    // COMMIT TRANSACTION - All operations succeeded
    if (db_commit_transaction() != 0)
    {
        db_rollback_transaction();
        return -20; // Failed to commit transaction
    }

    // Calculate trade values for analytics
    float offered_value = trade.offered_cash;
    float requested_value = trade.requested_cash;
    
    // Calculate value of offered items
    for (int i = 0; i < trade.offered_count; i++)
    {
        int instance_id = trade.offered_skins[i];
        if (instance_id <= 0)
            continue;
        
        int definition_id;
        SkinRarity rarity;
        WearCondition wear;
        int pattern_seed, is_stattrak;
        int owner_id;
        time_t acquired_at;
        int is_tradable;
        
        if (db_load_skin_instance(instance_id, &definition_id, &rarity, &wear, &pattern_seed, &is_stattrak, &owner_id, &acquired_at, &is_tradable) == 0)
        {
            float price = db_calculate_skin_price(definition_id, rarity, wear);
            offered_value += price;
        }
    }
    
    // Calculate value of requested items
    for (int i = 0; i < trade.requested_count; i++)
    {
        int instance_id = trade.requested_skins[i];
        if (instance_id <= 0)
            continue;
        
        int definition_id;
        SkinRarity rarity;
        WearCondition wear;
        int pattern_seed, is_stattrak;
        int owner_id;
        time_t acquired_at;
        int is_tradable;
        
        if (db_load_skin_instance(instance_id, &definition_id, &rarity, &wear, &pattern_seed, &is_stattrak, &owner_id, &acquired_at, &is_tradable) == 0)
        {
            float price = db_calculate_skin_price(definition_id, rarity, wear);
            requested_value += price;
        }
    }

    // Log transaction with trade value information
    // Log for the receiver (user_id = to_user_id)
    // Receiver gave requested_value (what they're giving to sender) and received offered_value (what sender gave them)
    TransactionLog log;
    log.log_id = 0;
    log.type = LOG_TRADE;
    log.user_id = user_id; // user_id is the receiver (to_user_id)
    // Format: "Accepted trade offer X: gave $Y (items + cash), received $Z (items + cash), profit $W"
    float profit_receiver = offered_value - requested_value; // Receiver's profit
    snprintf(log.details, sizeof(log.details), "Accepted trade offer %d: gave $%.2f (items + cash), received $%.2f (items + cash), profit $%.2f", 
             trade_id, requested_value, offered_value, profit_receiver);
    log.timestamp = time(NULL);
    LOG_DEBUG("accept_trade: Logging transaction for receiver (user_id=%d, trade_id=%d): '%s'", 
              user_id, trade_id, log.details);
    int log_result1 = db_log_transaction(&log);
    if (log_result1 != 0)
    {
        LOG_ERROR("accept_trade: Failed to log transaction for receiver (user_id=%d, trade_id=%d): db_log_transaction returned %d", 
                 user_id, trade_id, log_result1);
    }
    else
    {
        LOG_DEBUG("accept_trade: Successfully logged transaction for receiver (user_id=%d, trade_id=%d)", 
                 user_id, trade_id);
    }
    
    // Also log for the sender (from_user_id)
    // Sender gave offered_value (what they're giving to receiver) and received requested_value (what receiver gave them)
    TransactionLog log2;
    log2.log_id = 0;
    log2.type = LOG_TRADE;
    log2.user_id = trade.from_user_id; // FIX: Should be from_user_id, not to_user_id
    float profit_sender = requested_value - offered_value; // Sender's profit
    snprintf(log2.details, sizeof(log2.details), "Accepted trade offer %d: gave $%.2f (items + cash), received $%.2f (items + cash), profit $%.2f", 
             trade_id, offered_value, requested_value, profit_sender);
    log2.timestamp = time(NULL);
    LOG_DEBUG("accept_trade: Logging transaction for sender (user_id=%d, trade_id=%d): '%s'", 
              trade.from_user_id, trade_id, log2.details);
    int log_result2 = db_log_transaction(&log2);
    if (log_result2 != 0)
    {
        LOG_ERROR("accept_trade: Failed to log transaction for sender (user_id=%d, trade_id=%d): db_log_transaction returned %d", 
                 trade.from_user_id, trade_id, log_result2);
    }
    else
    {
        LOG_DEBUG("accept_trade: Successfully logged transaction for sender (user_id=%d, trade_id=%d)", 
                 trade.from_user_id, trade_id);
    }

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

    // NOTE: Items are NOT removed from inventory when sending trade offer
    // They remain in the sender's inventory until trade is accepted
    // So when declining, items are already in the sender's inventory
    // No need to return items - they were never removed
    // Items are also NOT trade locked (only market items are locked)

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

    // NOTE: Items are NOT removed from inventory when sending trade offer
    // They remain in the sender's inventory until trade is accepted
    // So when cancelling, items are already in the sender's inventory
    // No need to return items - they were never removed
    // Items are also NOT trade locked (only market items are locked)

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
    int valid_offered_count = 0;
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
        
        // Check if item is already in a pending trade
        if (db_is_instance_in_pending_trade(instance_id))
            return -13; // Item is already in a pending trade offer

        valid_offered_count++;
        // Note: Trade lock check removed - items are only locked when listed on market
        // Trading between users does not require items to be unlocked
    }

    // Check if to_user owns requested items
    int valid_requested_count = 0;
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
        
        // Check if item is already in a pending trade
        if (db_is_instance_in_pending_trade(instance_id))
            return -14; // Item is already in a pending trade offer

        valid_requested_count++;
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
    // Use valid counts (excluding invalid instance_ids <= 0)
    if (valid_offered_count == 0 && offer->offered_cash == 0.0f && 
        valid_requested_count == 0 && offer->requested_cash == 0.0f)
    {
        return -12; // Empty trade
    }
    
    // If counts don't match valid items, reject (client sent invalid data)
    if (offer->offered_count > 0 && valid_offered_count == 0)
    {
        return -12; // All offered items are invalid
    }
    if (offer->requested_count > 0 && valid_requested_count == 0)
    {
        return -12; // All requested items are invalid
    }

    return 0;
}

// Execute trade internal (without transaction - caller manages transaction)
static int execute_trade_internal(TradeOffer *offer)
{
    if (!offer)
        return -1;

    // Transfer offered items from from_user to to_user
    for (int i = 0; i < offer->offered_count; i++)
    {
        int instance_id = offer->offered_skins[i];
        if (instance_id <= 0)
            continue;

        // Remove from from_user inventory
        if (db_remove_from_inventory(offer->from_user_id, instance_id) != 0)
        {
            db_rollback_transaction();
            return -2; // Failed to remove from inventory
        }

        // Update owner
        if (db_update_skin_instance_owner(instance_id, offer->to_user_id) != 0)
        {
            db_rollback_transaction();
            return -3; // Failed to transfer ownership
        }

        // Add to to_user inventory
        if (db_add_to_inventory(offer->to_user_id, instance_id) != 0)
        {
            db_rollback_transaction();
            return -4; // Failed to add to inventory
        }

        // Apply trade lock to traded items (7 days lock)
        db_apply_trade_lock(instance_id);
    }

    // Transfer requested items from to_user to from_user
    for (int i = 0; i < offer->requested_count; i++)
    {
        int instance_id = offer->requested_skins[i];
        if (instance_id <= 0)
            continue;

        // Remove from to_user inventory
        if (db_remove_from_inventory(offer->to_user_id, instance_id) != 0)
        {
            db_rollback_transaction();
            return -5; // Failed to remove from inventory
        }

        // Update owner
        if (db_update_skin_instance_owner(instance_id, offer->from_user_id) != 0)
        {
            db_rollback_transaction();
            return -6; // Failed to transfer ownership
        }

        // Add to from_user inventory
        if (db_add_to_inventory(offer->from_user_id, instance_id) != 0)
        {
            db_rollback_transaction();
            return -7; // Failed to add to inventory
        }

        // Apply trade lock to traded items (7 days lock)
        db_apply_trade_lock(instance_id);
    }

    // Transfer cash
    if (offer->offered_cash > 0)
    {
        User from_user, to_user;
        if (db_load_user(offer->from_user_id, &from_user) != 0)
        {
            db_rollback_transaction();
            return -8;
        }
        if (db_load_user(offer->to_user_id, &to_user) != 0)
        {
            db_rollback_transaction();
            return -9;
        }

        if (from_user.balance < offer->offered_cash)
        {
            db_rollback_transaction();
            return -10; // Insufficient funds
        }

        from_user.balance -= offer->offered_cash;
        to_user.balance += offer->offered_cash;

        if (db_update_user(&from_user) != 0)
        {
            db_rollback_transaction();
            return -11;
        }
        if (db_update_user(&to_user) != 0)
        {
            db_rollback_transaction();
            return -12;
        }
    }

    if (offer->requested_cash > 0)
    {
        User from_user, to_user;
        if (db_load_user(offer->from_user_id, &from_user) != 0)
        {
            db_rollback_transaction();
            return -13;
        }
        if (db_load_user(offer->to_user_id, &to_user) != 0)
        {
            db_rollback_transaction();
            return -14;
        }

        if (to_user.balance < offer->requested_cash)
        {
            db_rollback_transaction();
            return -15; // Insufficient funds
        }

        to_user.balance -= offer->requested_cash;
        from_user.balance += offer->requested_cash;

        if (db_update_user(&from_user) != 0)
        {
            db_rollback_transaction();
            return -16;
        }
        if (db_update_user(&to_user) != 0)
        {
            db_rollback_transaction();
            return -17;
        }
    }

    // NOTE: Transaction commit is handled by caller
    // This function assumes transaction is already started

    // Update quests and achievements (after commit - these are not critical for atomicity)
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

    // Update active trading challenges (profit from trading)
    // Trading affects profit: balance changes (cash), inventory changes (items)
    update_user_active_challenges(offer->from_user_id);
    update_user_active_challenges(offer->to_user_id);

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

