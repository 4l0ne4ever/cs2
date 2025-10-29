#ifndef DATABASE_H
#define DATABASE_H

#include "types.h"

// Initialize database files
int db_init();

// User operations
int db_save_user(User *user);
int db_load_user(int user_id, User *out_user);
int db_load_user_by_username(const char *username, User *out_user);
int db_update_user(User *user);
int db_user_exists(const char *username);

// Skin operations
int db_save_skin(Skin *skin);
int db_load_skin(int skin_id, Skin *out_skin);
int db_update_skin(Skin *skin);

// Inventory operations
int db_load_inventory(int user_id, Inventory *out_inv);
int db_add_to_inventory(int user_id, int skin_id);
int db_remove_from_inventory(int user_id, int skin_id);

// Trade operations
int db_save_trade(TradeOffer *trade);
int db_load_trade(int trade_id, TradeOffer *out_trade);
int db_update_trade(TradeOffer *trade);
int db_get_user_trades(int user_id, TradeOffer *out_trades, int *count);

// Market operations
int db_save_listing(MarketListing *listing);
int db_load_listings(MarketListing *out_listings, int *count);
int db_update_listing(MarketListing *listing);

// Transaction log
int db_log_transaction(TransactionLog *log);

// Session operations
int db_save_session(Session *session);
int db_load_session(const char *token, Session *out_session);
int db_delete_session(const char *token);

#endif // DATABASE_H
