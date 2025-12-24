#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <time.h>

// Max constants
#define MAX_INVENTORY_SIZE 200
#define MAX_USERNAME_LEN 32
#define MAX_PASSWORD_HASH_LEN 65
#define MAX_ITEM_NAME_LEN 64
#define MAX_CHAT_HISTORY 50

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
// FN: 0.00-0.07, MW: 0.07-0.15, FT: 0.15-0.37, WW: 0.37-0.45, BS: 0.45-1.00
typedef float WearCondition; // Float value 0.00-1.00

// Helper enum for wear condition names (for display/UI)
typedef enum
{
    WEAR_FN = 0, // Factory New (0.00-0.07)
    WEAR_MW,     // Minimal Wear (0.07-0.15)
    WEAR_FT,     // Field-Tested (0.15-0.37)
    WEAR_WW,     // Well-Worn (0.37-0.45)
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

// Price history entry for tracking price changes
typedef struct
{
    int definition_id;
    float price;
    time_t timestamp;
    int transaction_type; // 0 = buy, 1 = sell
} PriceHistoryEntry;

// Price trend data
typedef struct
{
    int definition_id;
    float current_price;
    float price_24h_ago;
    float price_change_percent; // +5.2% or -2.1% or 0.0%
    char trend_symbol[4]; // "▲", "▼", or "═"
} PriceTrend;

typedef struct
{
    char session_token[37]; // UUID
    int user_id;
    int socket_fd;
    time_t login_time;
    time_t last_activity;
    int is_active;
} Session;

// Report System (Phase 7)
typedef struct
{
    int report_id;
    int reporter_id;      // User who made the report
    int reported_id;      // User being reported
    char reason[256];     // Reason for report
    time_t created_at;    // When report was created
    int is_resolved;      // 0 = pending, 1 = resolved
} Report;

// Quest System
typedef enum
{
    QUEST_FIRST_STEPS = 0,    // Complete 3 trades
    QUEST_MARKET_EXPLORER,    // Buy 5 items from market
    QUEST_LUCKY_GAMBLER,      // Unbox 5 cases
    QUEST_PROFIT_MAKER,       // Make $50 profit
    QUEST_SOCIAL_TRADER       // Trade with 10 different users
} QuestType;

typedef struct
{
    int quest_id;
    int user_id;
    QuestType quest_type;
    int progress;          // Current progress
    int target;             // Target value
    int is_completed;       // 0 = in progress, 1 = completed
    int is_claimed;         // 0 = not claimed, 1 = claimed
    time_t started_at;      // When quest started
    time_t completed_at;     // When quest completed
} Quest;

// Achievement System
typedef enum
{
    ACHIEVEMENT_FIRST_TRADE = 0,      // First Trade Completed
    ACHIEVEMENT_FIRST_KNIFE,          // First Knife Unboxed
    ACHIEVEMENT_PROFIT_1000,          // Total Profit $1,000
    ACHIEVEMENT_100_TRADES            // 100 Successful Trades
} AchievementType;

typedef struct
{
    int achievement_id;
    int user_id;
    AchievementType achievement_type;
    int is_unlocked;        // 0 = locked, 1 = unlocked
    int is_claimed;         // 0 = not claimed, 1 = claimed
    time_t unlocked_at;     // When achievement was unlocked
} Achievement;

// Daily Login Rewards
typedef struct
{
    int user_id;
    int current_streak;      // Current login streak (1-7)
    time_t last_login_date;  // Last login date (date only, no time)
    time_t last_reward_date; // Last reward claimed date
} LoginStreak;

// Chat Message
typedef struct
{
    int message_id;
    int user_id;
    char username[MAX_USERNAME_LEN];
    char message[256];
    time_t timestamp;
} ChatMessage;

// Leaderboard Entry
typedef struct
{
    int user_id;
    char username[MAX_USERNAME_LEN];
    float value; // Net worth, unbox value, or profit
    char details[128]; // Additional info (e.g., "Unboxed: AK-47 Redline")
} LeaderboardEntry;

// Trade Statistics
typedef struct
{
    int user_id;
    int trades_completed;
    int items_bought;
    int items_sold;
    float avg_buy_price;
    float avg_sell_price;
    float net_profit;
    float best_trade_profit;
    float worst_trade_loss;
    float win_rate; // Percentage
} TradeStats;

// Balance History Entry
typedef struct
{
    time_t timestamp;
    float balance;
} BalanceHistoryEntry;

// Trading Challenge (already defined in trading_challenges.h, but adding here for completeness)
// Note: Full definition is in trading_challenges.h

#endif // TYPES_H
