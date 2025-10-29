#ifndef TRADING_H
#define TRADING_H

#include "types.h"

// Send trade offer
int send_trade_offer(int from_user, int to_user, TradeOffer *offer);

// Accept trade
int accept_trade(int user_id, int trade_id);

// Decline trade
int decline_trade(int user_id, int trade_id);

// Cancel trade
int cancel_trade(int user_id, int trade_id);

// Get active trades for user
int get_user_trades(int user_id, TradeOffer *out_trades, int *count);

// Validate trade offer
int validate_trade(TradeOffer *offer);

// Execute trade (atomic operation)
int execute_trade(TradeOffer *offer);

// Clean expired trades
void clean_expired_trades();

// Check if skin is trade locked
int is_trade_locked(int skin_id);

// Apply trade lock to skin
void apply_trade_lock(int skin_id);

// Check and unlock expired trade locks
void check_expired_locks();

#endif // TRADING_H
