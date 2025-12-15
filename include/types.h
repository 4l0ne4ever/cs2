#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <time.h>

// Max constants
#define MAX_INVENTORY_SIZE 200
#define MAX_USERNAME_LEN 32
#define MAX_PASSWORD_HASH_LEN 65
#define MAX_ITEM_NAME_LEN 64

// ==================== ENUMS ====================

typedef enum
{
    RARITY_CONSUMER = 0, // Blue
    RARITY_INDUSTRIAL,   // Light Blue
    RARITY_MIL_SPEC,     // Purple
    RARITY_RESTRICTED,   // Pink
    RARITY_CLASSIFIED,   // Red
    RARITY_COVERT,       // Gold
    RARITY_CONTRABAND    // Special (knife/glove)
} SkinRarity;

// Wear is now a float value (0.00-1.00) instead of enum
// FN: 0.00-0.07, MW: 0.07-0.15, FT: 0.15-0.38, WW: 0.38-0.45, BS: 0.45-1.00
typedef float WearCondition; // Float value 0.00-1.00

// Helper enum for wear condition names (for display/UI)
typedef enum
{
    WEAR_FN = 0, // Factory New (0.00-0.07)
    WEAR_MW,     // Minimal Wear (0.07-0.15)
    WEAR_FT,     // Field-Tested (0.15-0.38)
    WEAR_WW,     // Well-Worn (0.38-0.45)
    WEAR_BS      // Battle-Scarred (0.45-1.00)
} WearConditionName;

// Helper function to get wear condition name from float
static inline WearConditionName get_wear_condition(float wear_float)
{
    if (wear_float < 0.07f) return WEAR_FN;
    if (wear_float < 0.15f) return WEAR_MW;
    if (wear_float < 0.37f) return WEAR_FT; // Fixed: 0.37 not 0.38
    if (wear_float < 0.45f) return WEAR_WW;
    return WEAR_BS;
}

typedef enum
{
    TRADE_PENDING,
    TRADE_ACCEPTED,
    TRADE_DECLINED,
    TRADE_CANCELLED,
    TRADE_EXPIRED
} TradeStatus;

typedef enum
{
    LOG_LOGIN,
    LOG_TRADE,
    LOG_UNBOX,
    LOG_MARKET_BUY,
    LOG_MARKET_SELL
} LogType;

// ==================== STRUCTS ====================

typedef struct
{
    int user_id;
    char username[MAX_USERNAME_LEN];
    char password_hash[MAX_PASSWORD_HASH_LEN]; // SHA256 hash
    float balance;
    time_t created_at;
    time_t last_login;
    int is_banned;
} User;

typedef struct
{
    int skin_id;
    char name[MAX_ITEM_NAME_LEN];
    SkinRarity rarity;
    WearCondition wear;
    int pattern_seed; // Pattern Template/Paint Seed (0-1000) - CS2 research
    int is_stattrak;  // 1 = StatTrak™, 0 = Normal (10% chance, except Gold)
    float base_price;
    float current_price;
    int owner_id; // 0 = not owned (in market)
    time_t acquired_at;
    int is_tradable; // 0 = locked (7 days), 1 = unlocked
} Skin;

typedef struct
{
    int user_id;
    int skin_ids[MAX_INVENTORY_SIZE];
    int count;
} Inventory;

typedef struct
{
    int trade_id;
    int from_user_id;
    int to_user_id;
    int offered_skins[10];
    int offered_count;
    float offered_cash;
    int requested_skins[10];
    int requested_count;
    float requested_cash;
    TradeStatus status;
    time_t created_at;
    time_t expires_at; // 15 minutes
} TradeOffer;

typedef struct
{
    int listing_id;
    int seller_id;
    int skin_id;
    float price;
    time_t listed_at;
    int is_sold;
} MarketListing;

typedef struct
{
    int case_id;
    char name[32];
    float price;
    int possible_skins[50];
    float probabilities[50];
    int skin_count;
} Case;

typedef struct
{
    int log_id;
    LogType type;
    int user_id;
    char details[256];
    time_t timestamp;
} TransactionLog;

typedef struct
{
    char session_token[37]; // UUID
    int user_id;
    int socket_fd;
    time_t login_time;
    time_t last_activity;
    int is_active;
} Session;

#endif // TYPES_H
