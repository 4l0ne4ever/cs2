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

typedef enum
{
    WEAR_FN = 0, // Factory New
    WEAR_MW,     // Minimal Wear
    WEAR_FT,     // Field-Tested
    WEAR_WW,     // Well-Worn
    WEAR_BS      // Battle-Scarred
} WearCondition;

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
