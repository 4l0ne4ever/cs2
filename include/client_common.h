#ifndef CLIENT_COMMON_H
#define CLIENT_COMMON_H

#include "types.h"
#include "protocol.h"

// Global state (shared across client modules)
extern int g_user_id;
extern char g_session_token[37];

// Helper functions (shared across client modules)
int get_definition_id_from_instance(int instance_id, int *out_definition_id);
int get_price_trend(int definition_id, PriceTrend *out_trend);
int get_price_history(int definition_id, PriceHistoryEntry *out_history, int *count);
int load_skin_details(int instance_id, Skin *out_skin);
float get_user_balance(void);
float calculate_inventory_value(void);
void display_balance_info(void);
int search_user_by_username(const char *username, User *out_user);

#endif // CLIENT_COMMON_H

