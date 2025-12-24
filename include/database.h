#ifndef DATABASE_H
#define DATABASE_H

#include "types.h"

// Initialize database files
int db_init();
void db_close();

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
int db_is_instance_in_pending_trade(int instance_id);

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

// Skin definition & instance operations (new model)
int db_load_skin_definition(int definition_id, char *name, float *base_price);
int db_load_skin_definition_with_rarity(int definition_id, char *name, float *base_price, SkinRarity *rarity);
int db_load_skin_instance(int instance_id, int *definition_id, SkinRarity *rarity, WearCondition *wear, int *pattern_seed, int *is_stattrak, int *owner_id, time_t *acquired_at, int *is_tradable);
int db_create_skin_instance(int definition_id, SkinRarity rarity, WearCondition wear, int pattern_seed, int is_stattrak, int owner_id, int *out_instance_id);
int db_update_skin_instance_owner(int instance_id, int new_owner_id);
int db_get_wear_multiplier(WearCondition wear, float *multiplier);
int db_get_rarity_multiplier(SkinRarity rarity, float *multiplier);
float db_calculate_skin_price(int definition_id, SkinRarity rarity, WearCondition wear);
int db_get_case_skins(int case_id, int *definition_ids, int *count);
int db_get_case_skins_by_rarity(int case_id, SkinRarity rarity, int *definition_ids, int *count);

// Fetch full case info with all skin details (optimized for animation preview)
// Returns: definition_id, name, rarity, base_price for all skins in case
// This solves N+1 query problem by loading all data in one query
// out_skins: Array to store skin info (must be allocated by caller, max 100)
// out_count: Output parameter for number of skins loaded
int db_fetch_full_case_info(int case_id, void *out_skins, int *out_count);

// Market listings v2 operations (using instance_id)
int db_save_listing_v2(int seller_id, int instance_id, float price, int *out_listing_id);
int db_load_listings_v2(MarketListing *out_listings, int *count);
int db_search_listings_by_name(const char *search_term, MarketListing *out_listings, int *count);
int db_get_listing_v2(int listing_id, int *seller_id, int *instance_id, float *price, int *is_sold);
int db_mark_listing_sold(int listing_id);
int db_remove_listing_v2(int listing_id);

// Trade lock operations
int db_check_trade_lock(int instance_id, int *is_locked);
int db_apply_trade_lock(int instance_id);
int db_unlock_expired_trades();
int db_clean_expired_trades();

// Case operations
int db_load_cases(Case *out_cases, int *count);
int db_load_case(int case_id, Case *out_case);

// Report operations (Phase 7)
int db_save_report(Report *report);
int db_load_reports_for_user(int user_id, Report *out_reports, int *count);
int db_get_report_count(int user_id);

// Quest operations
int db_save_quest(Quest *quest);
int db_load_user_quests(int user_id, Quest *out_quests, int *count);
int db_update_quest(Quest *quest);

// Achievement operations
int db_save_achievement(Achievement *achievement);
int db_load_user_achievements(int user_id, Achievement *out_achievements, int *count);
int db_update_achievement(Achievement *achievement);

// Login streak operations
int db_save_login_streak(LoginStreak *streak);
int db_load_login_streak(int user_id, LoginStreak *out_streak);

// Chat operations
int db_save_chat_message(int user_id, const char *username, const char *message);
int db_load_recent_chat_messages(ChatMessage *out_messages, int *count, int limit);

// Price history operations
int db_save_price_history(int definition_id, float price, int transaction_type);
int db_get_price_history_24h(int definition_id, PriceHistoryEntry *out_history, int *count);
int db_get_price_24h_ago(int definition_id, float *out_price);

#endif // DATABASE_H
