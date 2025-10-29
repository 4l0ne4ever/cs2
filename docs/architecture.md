II. KIẾN TRÚC HỆ THỐNG
2.1. Sơ đồ tổng quan:
┌─────────────────────────────────────────────────────────┐
│ CLIENT LAYER │
├─────────────────────────────────────────────────────────┤
│ Terminal UI (ANSI) │ Input Handler │ Network Client │
└─────────────────┬───────────────────────────────────────┘
│ TCP Socket Connection
▼
┌─────────────────────────────────────────────────────────┐
│ SERVER LAYER │
├─────────────────────────────────────────────────────────┤
│ Connection Manager (select/epoll) │
│ ├── Thread Pool (pthread) │
│ ├── Session Manager │
│ └── Request Router │
├─────────────────────────────────────────────────────────┤
│ BUSINESS LOGIC │
│ ├── Authentication Module │
│ ├── Market Engine │
│ ├── Trading System │
│ ├── Inventory Manager │
│ ├── Unbox Engine │
├─────────────────────────────────────────────────────────┤
│ DATA LAYER │
│ ├── File-based Database (binary files) │
│ ├── Cache (in-memory hash table) │
│ └── Transaction Log │
└─────────────────────────────────────────────────────────┘

2.2. Kiến trúc Client-Server:
A. Server Architecture:
c/_ Server chính - Single process, multi-threaded _/

int main() {
// 1. Khởi tạo
init_database();
init_market();
load_skins_data();

    // 2. Tạo server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    bind(server_fd, ...);
    listen(server_fd, MAX_CLIENTS);

    // 3. Tạo thread pool
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }

    // 4. Main loop - accept connections
    fd_set master_set, read_set;
    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);

    while (1) {
        read_set = master_set;
        select(max_fd + 1, &read_set, NULL, NULL, NULL);

        // New connection
        if (FD_ISSET(server_fd, &read_set)) {
            int client_fd = accept(server_fd, ...);
            FD_SET(client_fd, &master_set);
            add_to_job_queue(client_fd);
        }

        // Existing connections
        for (each client_fd in read_set) {
            if (data_available) {
                add_to_job_queue(client_fd);
            }
        }
    }

}

/_ Worker thread xử lý requests _/
void* worker_thread(void* arg) {
while (1) {
Job job = get*job_from_queue(); // Thread-safe queue
handle_client_request(job.client_fd, job.request);
}
}
B. Client Architecture:
c/* Client - Single thread, event-driven \_/

int main() {
// 1. Connect to server
int sock = socket(AF_INET, SOCK_STREAM, 0);
connect(sock, server_addr, ...);

    // 2. Login/Register
    authenticate(sock);

    // 3. Main loop
    while (running) {
        display_menu();

        // Non-blocking check for server updates
        if (data_available_from_server(sock)) {
            Message msg = receive_message(sock);
            handle_server_update(msg);
            refresh_display();
        }

        // Handle user input (non-blocking)
        if (kbhit()) {
            char cmd = getchar();
            process_command(sock, cmd);
        }

        usleep(50000); // 50ms delay
    }

}

III. CẤU TRÚC DỮ LIỆU
3.1. Core Data Structures:
c/_ ==================== USER ==================== _/
typedef struct {
int user_id;
char username[32];
char password_hash[65]; // SHA256 hash
float balance;
time_t created_at;
time_t last_login;
int is_banned;
} User;

/_ ==================== SKIN ==================== _/
typedef enum {
RARITY_CONSUMER = 0, // Blue
RARITY_INDUSTRIAL, // Light Blue
RARITY_MIL_SPEC, // Purple
RARITY_RESTRICTED, // Pink
RARITY_CLASSIFIED, // Red
RARITY_COVERT, // Gold
RARITY_CONTRABAND // Special (knife/glove)
} SkinRarity;

typedef enum {
WEAR_FN = 0, // Factory New
WEAR_MW, // Minimal Wear
WEAR_FT, // Field-Tested
WEAR_WW, // Well-Worn
WEAR_BS // Battle-Scarred
} WearCondition;

typedef struct {
int skin_id;
char name[64]; // "AWP | Dragon Lore"
SkinRarity rarity;
WearCondition wear;
float base_price;
float current_price;
int owner_id; // 0 = not owned (in market)
time_t acquired_at;
int is_tradable; // Trade lock 7 days
} Skin;

/_ ==================== INVENTORY ==================== _/
typedef struct {
int user_id;
Skin skins[MAX_INVENTORY_SIZE]; // 200 items
int count;
} Inventory;

/_ ==================== TRADE OFFER ==================== _/
typedef enum {
TRADE_PENDING,
TRADE_ACCEPTED,
TRADE_DECLINED,
TRADE_CANCELLED,
TRADE_EXPIRED
} TradeStatus;

typedef struct {
int trade_id;
int from_user_id;
int to_user_id;

    int offered_skins[10];   // skin IDs
    int offered_count;
    float offered_cash;

    int requested_skins[10];
    int requested_count;
    float requested_cash;

    TradeStatus status;
    time_t created_at;
    time_t expires_at;       // 15 minutes

} TradeOffer;

/_ ==================== MARKET LISTING ==================== _/
typedef struct {
int listing_id;
int seller_id;
int skin_id;
float price;
time_t listed_at;
int is_sold;
} MarketListing;

// BỎ: TradingGroup struct (không cần trading groups)

/_ ==================== CASE ==================== _/
typedef struct {
int case_id;
char name[32]; // "Revolution Case"
float price;
int possible_skins[50]; // skin IDs có thể unbox
float probabilities[50]; // Xác suất tương ứng
int skin_count;
} Case;

/_ ==================== TRANSACTION LOG ==================== _/
typedef enum {
LOG_LOGIN,
LOG_TRADE,
LOG_UNBOX,
LOG_MARKET_BUY,
LOG_MARKET_SELL,
LOG_SCAM_REPORT,
LOG_BAN
} LogType;

typedef struct {
int log_id;
LogType type;
int user_id;
char details[256];
time_t timestamp;
} TransactionLog;

/_ ==================== SESSION ==================== _/
typedef struct {
char session_token[37]; // UUID
int user_id;
int socket_fd;
time_t login_time;
time_t last_activity;
int is_active;
} Session;

```

---

### **3.2. Database Schema (File-based):**
```

data/
├── users.dat # Binary file, mỗi record = sizeof(User)
├── skins.dat # Binary file
├── inventories.dat # Binary file
├── trades.dat # Binary file
├── market.dat # Binary file
├── cases.dat # Binary file
├── logs.dat # Append-only log file
└── config.txt # Text config (market fees, etc.)
Lý do dùng binary files:

Nhanh: đọc/ghi trực tiếp struct
Đơn giản: không cần parse
Phù hợp với C thuần

Indexing strategy:
c/_ In-memory hash table cho fast lookup _/
typedef struct {
int user_id;
off_t file_offset; // Vị trí trong file
} UserIndex;

// Load vào memory lúc startup
UserIndex user_index[MAX_USERS];

IV. GIAO THỨC TRUYỀN THÔNG (PROTOCOL)
4.1. Message Format:
c/_ Fixed-size header _/
typedef struct {
uint16_t magic; // 0xABCD (validate message)
uint16_t msg_type; // Loại message
uint32_t msg_length; // Độ dài payload
uint32_t sequence_num; // Số thứ tự
uint32_t checksum; // CRC32
} MessageHeader;

/_ Variable-size payload _/
typedef struct {
MessageHeader header;
char payload[MAX_PAYLOAD_SIZE]; // 4096 bytes
} Message;

4.2. Message Types:
c/_ ========== AUTHENTICATION ========== _/
#define MSG_REGISTER_REQUEST 0x0001
#define MSG_REGISTER_RESPONSE 0x0002
#define MSG_LOGIN_REQUEST 0x0003
#define MSG_LOGIN_RESPONSE 0x0004
#define MSG_LOGOUT 0x0005

/_ ========== MARKET ========== _/
#define MSG_GET_MARKET_LISTINGS 0x0010
#define MSG_MARKET_DATA 0x0011
#define MSG_BUY_FROM_MARKET 0x0012
#define MSG_SELL_TO_MARKET 0x0013
#define MSG_PRICE_UPDATE 0x0014 // Server broadcasts

/_ ========== TRADING ========== _/
#define MSG_SEND_TRADE_OFFER 0x0020
#define MSG_TRADE_OFFER_NOTIFY 0x0021
#define MSG_ACCEPT_TRADE 0x0022
#define MSG_DECLINE_TRADE 0x0023
#define MSG_CANCEL_TRADE 0x0024
#define MSG_TRADE_COMPLETED 0x0025

/_ ========== INVENTORY ========== _/
#define MSG_GET_INVENTORY 0x0030
#define MSG_INVENTORY_DATA 0x0031
#define MSG_GET_USER_PROFILE 0x0032
#define MSG_USER_PROFILE_DATA 0x0033

/_ ========== UNBOXING ========== _/
#define MSG_UNBOX_CASE 0x0040
#define MSG_UNBOX_RESULT 0x0041
#define MSG_GET_CASES 0x0042
#define MSG_CASES_DATA 0x0043

/_ ========== CHAT ========== _/
#define MSG_CHAT_GLOBAL 0x0060

/_ ========== MISC ========== _/
#define MSG_HEARTBEAT 0x0090
#define MSG_ERROR 0x00FF

4.3. Payload Examples:
c/_ LOGIN_REQUEST payload _/
typedef struct {
char username[32];
char password_hash[65];
} LoginPayload;

/_ LOGIN_RESPONSE payload _/
typedef struct {
uint8_t success; // 1 = success, 0 = fail
char session_token[37]; // UUID
char error_msg[64]; // Nếu fail
User user_data; // Nếu success
} LoginResponsePayload;

/_ TRADE_OFFER payload _/
typedef struct {
int to_user_id;
int offered_skin_ids[10];
int offered_count;
float offered_cash;
int requested_skin_ids[10];
int requested_count;
float requested_cash;
} TradeOfferPayload;

/_ MARKET_DATA payload (broadcast) _/
typedef struct {
int listing_count;
MarketListing listings[100]; // Tối đa 100 listings/message
} MarketDataPayload;

/_ UNBOX_RESULT payload _/
typedef struct {
uint8_t success;
Skin unboxed_skin; // Skin vừa unbox
float new_balance; // Balance sau khi trừ tiền
} UnboxResultPayload;

4.4. Error Codes:
c#define ERR_SUCCESS 0
#define ERR_INVALID_CREDENTIALS 1
#define ERR_USER_EXISTS 2
#define ERR_INSUFFICIENT_FUNDS 3
#define ERR_ITEM_NOT_FOUND 4
#define ERR_PERMISSION_DENIED 5
#define ERR_TRADE_EXPIRED 6
#define ERR_INVALID_TRADE 7
#define ERR_SESSION_EXPIRED 8
#define ERR_SERVER_FULL 9
#define ERR_BANNED 10
#define ERR_TRADE_LOCKED 11 // 7-day trade lock
#define ERR_INVALID_REQUEST 12
#define ERR_DATABASE_ERROR 13

```

---

## **V. WORKFLOW CHI TIẾT**

### **5.1. User Registration & Login:**
```

CLIENT SERVER
| |
|---(1) MSG_REGISTER_REQUEST--->|
| {username, password} |
| |--- Check username exists
| |--- Hash password (SHA256)
| |--- Create User record
| |--- Save to users.dat
| |--- Create empty inventory
| |--- Give starting balance $100
| |
|<--(2) MSG_REGISTER_RESPONSE---|
| {success, error_msg} |
| |
|---(3) MSG_LOGIN_REQUEST------>|
| {username, password} |
| |--- Verify credentials
| |--- Generate session token
| |--- Create Session record
| |--- Update last_login
| |
|<--(4) MSG_LOGIN_RESPONSE------|
| {session_token, user} |
| |
|---(5) MSG_HEARTBEAT---------->| (every 30s)
|<--(6) MSG_HEARTBEAT-----------|

```

---

### **5.2. Market Browse & Buy:**
```

CLIENT SERVER
| |
|---(1) MSG_GET_MARKET_LISTINGS>|
| |--- Query market.dat
| |--- Get top 100 listings
| |--- Sort by price
| |
|<--(2) MSG_MARKET_DATA---------|
| {listings[]} |
| |
| (User selects item to buy) |
| |
|---(3) MSG_BUY_FROM_MARKET---->|
| {listing_id} |
| |--- Verify listing exists
| |--- Check buyer balance
| |--- Calculate fees (15%)
| |--- Deduct buyer balance
| |--- Credit seller (85%)
| |--- Transfer skin ownership
| |--- Remove from market
| |--- Log transaction
| |--- Update market prices
| |
|<--(4) TRANSACTION_COMPLETE----|
| {new_balance, skin} |
| |
| |---(5) Broadcast price update
| | to all clients

```

---

### **5.3. P2P Trading:**
```

CLIENT A SERVER CLIENT B
| | |
|-(1) SEND_TRADE_OFFER->| |
| {to:B, give:[AWP], | |
| want:[AK+$50]} | |
| |---(2) Store trade--->|
| | TRADE_OFFER_NOTIFY
| | |
| | | (B reviews offer)
| | |
| |<-(3) ACCEPT_TRADE----|
| | {trade_id} |
| | |
| |--- Validate: |
| | - Items still exist
| | - Not trade-locked
| | - B has $50 |
| | |
| |--- Execute: |
| | - Transfer AWP A->B
| | - Transfer AK B->A
| | - Transfer $50 B->A
| | - Log trade |
| | - Apply 7-day lock
| | |
|<-(4) TRADE_COMPLETED--|---(5) TRADE_COMPLETED->
| {new_inventory} | {new_inventory} |

```

---

### **5.4. Unboxing Flow:**
```

CLIENT SERVER
| |
|---(1) MSG_GET_CASES---------->|
| |--- Return available cases
|<--(2) MSG_CASES_DATA----------|
| {cases[]} |
| |
| (User selects case) |
| |
|---(3) MSG_UNBOX_CASE--------->|
| {case_id} |
| |--- Check balance >= case price
| |--- Deduct balance
| |--- Run RNG algorithm:
| |
| | float rng = rand() / RAND_MAX;
| | float cumulative = 0;
| | for (each possible_skin) {
| | cumulative += probability;
| | if (rng < cumulative) {
| | selected = skin;
| | break;
| | }
| | }
| |
| |--- Create Skin instance
| |--- Add to user inventory
| |--- Set trade lock (7 days)
| |--- Update market prices
| |--- Log unbox
| |
|<--(4) MSG_UNBOX_RESULT--------|
| {skin, new_balance} |
| |
| (Animation shows result) |

```

---

**// BỎ: Section 5.5 Trading Group System - không implement**
```

LEADER SERVER MEMBER
| | |
|-(1) CREATE_GROUP----->| |
| {name:"ProTraders"} |--- Create group |
| |--- Set leader |
|<-(2) GROUP_CREATED----| |
| | |
|-(3) INVITE_TO_GROUP-->| |
| {user_id:5} |---(4) GROUP_INVITE---->|
| | |
| | | (Member decides)
| | |
| |<-(5) ACCEPT_INVITE-----|
| |--- Add to group |
| | |
|<-(6) MEMBER_JOINED----|---(7) WELCOME_MSG----->|
| | |
| (Members pool money) | |
| | |
|-(8) CONTRIBUTE $500-->| |
| |<-(9) CONTRIBUTE $300---|
| |--- shared_balance=$800 |
| | |
| (Leader uses pool) | |
| | |
|-(10) GROUP_BUY------->| |
| {skin_id, use_pool}|--- Deduct from pool |
| |--- Item -> group inventory
| | |
|<-(11) PURCHASE_OK-----|---(12) NOTIFY--------->|
| | "Group bought knife"|

```

---

**// BỎ: Section 5.6 Anti-Scam & Report System - không implement**
```

VICTIM SERVER SCAMMER
| | |
| (Receives bad trade) | |
| | |
|-(1) REPORT_SCAMMER--->| |
| {reported_id:99, |--- Store report |
| reason:"Fake link"}|--- Increment flag count|
| | |
|<-(2) REPORT_RECEIVED--| |
| | |
| |--- Check: flags > 3? |
| | YES -> Auto review |
| | |
| |--- Admin review logs |
| |--- Confirm scam |
| | |
| |---(3) BAN_USER-------->|
| | {reason, duration} |
| | |
| |<-(4) CONNECTION_CLOSED-|
| | X
| |  
 | |---(5) NOTIFY_ALL------>ALL
| | "user99 banned"

VI. MODULE IMPLEMENTATION
6.1. Authentication Module:
c/_ auth.h _/
#ifndef AUTH_H
#define AUTH_H

#include "types.h"

// Hash password using SHA256
void hash_password(const char *password, char *output);

// Verify password
int verify_password(const char *password, const char *hash);

// Generate session token (UUID)
void generate_session_token(char \*token);

// Register new user
int register_user(const char *username, const char *password, User \*out_user);

// Login user
int login_user(const char *username, const char *password, Session \*out_session);

// Validate session
int validate_session(const char *token, Session *out_session);

// Logout
void logout_user(const char \*session_token);

#endif
c/_ auth.c - Implementation _/
#include "auth.h"
#include <openssl/sha.h>
#include <uuid/uuid.h>

void hash_password(const char *password, char *output) {
unsigned char hash[SHA256_DIGEST_LENGTH];
SHA256((unsigned char\*)password, strlen(password), hash);

    // Convert to hex string
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[64] = '\0';

}

int register_user(const char *username, const char *password, User \*out_user) {
// 1. Check if username exists
if (user_exists(username)) {
return ERR_USER_EXISTS;
}

    // 2. Create user
    User new_user = {0};
    new_user.user_id = generate_user_id();
    strncpy(new_user.username, username, 31);
    hash_password(password, new_user.password_hash);
    new_user.balance = 100.0f;  // Starting balance
    new_user.created_at = time(NULL);
    new_user.is_banned = 0;

    // 3. Save to database
    if (save_user(&new_user) != 0) {
        return ERR_DATABASE_ERROR;
    }

    // 4. Create empty inventory
    create_inventory(new_user.user_id);

    // 5. Give starter skin
    give_starter_skin(new_user.user_id);

    *out_user = new_user;
    return ERR_SUCCESS;

}

int login_user(const char *username, const char *password, Session \*out_session) {
// 1. Load user
User user;
if (load_user_by_username(username, &user) != 0) {
return ERR_INVALID_CREDENTIALS;
}

    // 2. Check banned
    if (user.is_banned) {
        return ERR_BANNED;
    }

    // 3. Verify password
    char hash[65];
    hash_password(password, hash);
    if (strcmp(hash, user.password_hash) != 0) {
        return ERR_INVALID_CREDENTIALS;
    }

    // 4. Create session
    Session session = {0};
    generate_session_token(session.session_token);
    session.user_id = user.user_id;
    session.socket_fd = -1; // Set by caller
    session.login_time = time(NULL);
    session.last_activity = time(NULL);
    session.is_active = 1;

    // 5. Save session
    save_session(&session);

    // 6. Update user last_login
    user.last_login = time(NULL);
    update_user(&user);

    *out_session = session;
    return ERR_SUCCESS;

}

void generate_session_token(char \*token) {
uuid_t uuid;
uuid_generate(uuid);
uuid_unparse(uuid, token);
}

6.2. Market Engine:
c/_ market.h _/
#ifndef MARKET_H
#define MARKET_H

#include "types.h"

// List all market listings
int get_market_listings(MarketListing *out_listings, int *count);

// List specific skin on market
int list_skin_on_market(int user_id, int skin_id, float price);

// Buy skin from market
int buy_from_market(int buyer_id, int listing_id);

// Remove listing
int remove_listing(int listing_id);

// Update market prices based on supply/demand
void update_market_prices();

// Get current price for a skin type
float get_current_price(int skin_type, WearCondition wear);

#endif
c/_ market.c _/
#include "market.h"

int buy_from_market(int buyer_id, int listing_id) {
// 1. Load listing
MarketListing listing;
if (load_listing(listing_id, &listing) != 0) {
return ERR_ITEM_NOT_FOUND;
}

    // 2. Check if already sold
    if (listing.is_sold) {
        return ERR_ITEM_NOT_FOUND;
    }

    // 3. Load buyer
    User buyer;
    load_user(buyer_id, &buyer);

    // 4. Check balance
    if (buyer.balance < listing.price) {
        return ERR_INSUFFICIENT_FUNDS;
    }

    // 5. Calculate fees
    float market_fee = listing.price * MARKET_FEE_RATE; // 0.15
    float seller_receives = listing.price - market_fee;

    // 6. Update buyer
    buyer.balance -= listing.price;
    update_user(&buyer);

    // 7. Update seller
    User seller;
    load_user(listing.seller_id, &seller);
    seller.balance += seller_receives;
    update_user(&seller);

    // 8. Transfer skin
    Skin skin;
    load_skin(listing.skin_id, &skin);
    skin.owner_id = buyer_id;
    update_skin(&skin);

    // 9. Mark listing as sold
    listing.is_sold = 1;
    update_listing(&listing);

    // 10. Log transaction
    log_transaction(LOG_MARKET_BUY, buyer_id, listing_id, listing.price);

    // 11. Update market prices (supply decreased)
    update_skin_price(skin.name, skin.wear, -0.02); // -2%

    return ERR_SUCCESS;

}

void update_market_prices() {
// Chạy mỗi 5 phút
// Điều chỉnh giá dựa trên:
// - Số lượng listings (supply)
// - Số lượng sales gần đây (demand)
// - Rare unboxes (hype factor)

    for (each skin_type) {
        int supply = count_listings_for_skin(skin_type);
        int demand = count_recent_sales(skin_type, 300); // Last 5 mins

        float price_change = 0;

        if (demand > supply * 2) {
            price_change = 0.05; // +5% nếu demand cao
        } else if (supply > demand * 2) {
            price_change = -0.03; // -3% nếu supply cao
        }

        adjust_skin_price(skin_type, price_change);
    }

}

6.3. Trading System:
c/_ trading.h _/
#ifndef TRADING_H
#define TRADING_H

#include "types.h"

// Send trade offer
int send_trade_offer(int from_user, int to_user, TradeOffer \*offer);

// Accept trade
int accept_trade(int user_id, int trade_id);

// Decline trade
int decline_trade(int user_id, int trade_id);

// Cancel trade
int cancel_trade(int user_id, int trade_id);

// Get active trades for user
int get_user_trades(int user_id, TradeOffer *out_trades, int *count);

// Validate trade offer
int validate_trade(TradeOffer \*offer);

// Execute trade (atomic operation)
int execute_trade(TradeOffer \*offer);

// Clean expired trades
void clean_expired_trades();

#endif
c/_ trading.c _/
int send_trade_offer(int from_user, int to_user, TradeOffer \*offer) {
// 1. Validate offer
if (validate_trade(offer) != ERR_SUCCESS) {
return ERR_INVALID_TRADE;
}

    // 2. Check trade lock on items
    for (int i = 0; i < offer->offered_count; i++) {
        Skin skin;
        load_skin(offer->offered_skins[i], &skin);
        if (!skin.is_tradable) {
            return ERR_TRADE_LOCKED;
        }
    }

    // 3. Create trade record
    offer->trade_id = generate_trade_id();
    offer->from_user_id = from_user;
    offer->to_user_id = to_user;
    offer->status = TRADE_PENDING;
    offer->created_at = time(NULL);
    offer->expires_at = time(NULL) + 900; // 15 minutes

    // 4. Save trade
    save_trade(offer);

    // 5. Notify recipient
    notify_user(to_user, MSG_TRADE_OFFER_NOTIFY, offer);

    return ERR_SUCCESS;

}

int execute_trade(TradeOffer \*offer) {
// CRITICAL: This must be atomic (use file locking)

    int fd = open("data/trades.lock", O_CREAT | O_RDWR, 0666);
    flock(fd, LOCK_EX); // Exclusive lock

    // 1. Re-validate (double check)
    if (validate_trade(offer) != ERR_SUCCESS) {
        flock(fd, LOCK_UN);
        close(fd);
        return ERR_INVALID_TRADE;
    }

    // 2. Transfer items from A to B
    for (int i = 0; i < offer->offered_count; i++) {
        transfer_skin_ownership(
            offer->offered_skins[i],
            offer->from_user_id,
            offer->to_user_id
        );
    }

    // 3. Transfer items from B to A
    for (int i = 0; i < offer->requested_count; i++) {
        transfer_skin_ownership(
            offer->requested_skins[i],
            offer->to_user_id,
            offer->from_user_id
        );
    }

    // 4. Transfer cash
    if (offer->offered_cash > 0) {
        transfer_money(offer->from_user_id, offer->to_user_id, offer->offered_cash);
    }
    if (offer->requested_cash > 0) {
        transfer_money(offer->to_user_id, offer->from_user_id, offer->requested_cash);
    }

    // 5. Apply trade lock (7 days)
    apply_trade_lock_to_items(offer, 7 * 24 * 3600);

    // 6. Update trade status
    offer->status = TRADE_ACCEPTED;
    update_trade(offer);

    // 7. Log
    log_transaction(LOG_TRADE, offer->from_user_id, offer->trade_id, 0);
    log_transaction(LOG_TRADE, offer->to_user_id, offer->trade_id, 0);

    // 8. Unlock
    flock(fd, LOCK_UN);
    close(fd);

    return ERR_SUCCESS;

}

6.4. Unbox Engine:
c/_ unbox.h _/
#ifndef UNBOX_H
#define UNBOX_H

#include "types.h"

// Load all available cases
int get_available_cases(Case *out_cases, int *count);

// Unbox a case
int unbox_case(int user_id, int case_id, Skin \*out_skin);

// Calculate drop rates
void calculate_drop_rates(Case \*case_data);

// RNG for unbox
int roll_unbox(Case \*case_data);

#endif
c/_ unbox.c _/
int unbox_case(int user_id, int case_id, Skin \*out_skin) {
// 1. Load case data
Case case_data;
if (load_case(case_id, &case_data) != 0) {
return ERR_ITEM_NOT_FOUND;
}

    // 2. Load user
    User user;
    load_user(user_id, &user);

    // 3. Check balance
    float total_cost = case_data.price + CASE_KEY_PRICE; // $2.50 + $2.50
    if (user.balance < total_cost) {
        return ERR_INSUFFICIENT_FUNDS;
    }

    // 4. Deduct balance
    user.balance -= total_cost;
    update_user(&user);

    // 5. Roll RNG
    int skin_index = roll_unbox(&case_data);
    int skin_id = case_data.possible_skins[skin_index];

    // 6. Create skin instance
    Skin new_skin;
    create_skin_from_template(skin_id, &new_skin);
    new_skin.owner_id = user_id;
    new_skin.acquired_at = time(NULL);
    new_skin.is_tradable = 0; // 7-day trade lock
    save_skin(&new_skin);

    // 7. Add to inventory
    add_to_inventory(user_id, new_skin.skin_id);

    // 8. Log unbox
    log_transaction(LOG_UNBOX, user_id, new_skin.skin_id, new_skin.current_price);

    // 9. Update market price (new supply)
    if (new_skin.rarity >= RARITY_COVERT) {
        // Rare unbox -> increase hype
        update_skin_price(new_skin.name, new_skin.wear, 0.10); // +10%
    } else {
        // Common unbox -> decrease price slightly
        update_skin_price(new_skin.name, new_skin.wear, -0.01); // -1%
    }

    // 10. Broadcast if rare
    if (new_skin.rarity == RARITY_CONTRABAND) {
        broadcast_rare_unbox(user_id, &new_skin);
    }

    *out_skin = new_skin;
    return ERR_SUCCESS;

}

int roll_unbox(Case \*case_data) {
// Xác suất công khai:
// Contraband (Knife/Glove): 0.26%
// Covert (Red): 0.64%
// Classified (Pink): 3.20%
// Restricted (Purple): 15.98%
// Mil-Spec (Blue): 79.92%

    float rng = (float)rand() / RAND_MAX; // 0.0 - 1.0
    float cumulative = 0.0f;

    for (int i = 0; i < case_data->skin_count; i++) {
        cumulative += case_data->probabilities[i];
        if (rng <= cumulative) {
            return i;
        }
    }

    return case_data->skin_count - 1; // Fallback (shouldn't happen)

}

VII. UI IMPLEMENTATION (Terminal)
7.1. ANSI Escape Codes:
c/_ ui.h _/
#ifndef UI_H
#define UI_H

// Colors
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"
#define COLOR_BRIGHT_RED "\033[91m"
#define COLOR_BRIGHT_GREEN "\033[92m"
#define COLOR_BRIGHT_YELLOW "\033[93m"

// Styles
#define STYLE_BOLD "\033[1m"
#define STYLE_DIM "\033[2m"
#define STYLE_UNDERLINE "\033[4m"
#define STYLE_BLINK "\033[5m"

// Cursor control
#define CURSOR_HOME "\033[H"
#define CURSOR_CLEAR "\033[2J"
#define CURSOR_HIDE "\033[?25l"
#define CURSOR_SHOW "\033[?25h"

// Functions
void clear_screen();
void move_cursor(int row, int col);
void print_colored(const char *text, const char *color);
void print_box(int x, int y, int width, int height, const char \*title);
void print_progress_bar(int x, int y, int width, float progress);

// Rarity colors
const char\* get_rarity_color(SkinRarity rarity);

#endif
c/_ ui.c _/
#include "ui.h"

const char\* get_rarity_color(SkinRarity rarity) {
switch (rarity) {
case RARITY_CONSUMER: return COLOR_BLUE;
case RARITY_INDUSTRIAL: return COLOR_CYAN;
case RARITY_MIL_SPEC: return COLOR_MAGENTA;
case RARITY_RESTRICTED: return COLOR_BRIGHT_RED;
case RARITY_CLASSIFIED: return COLOR_BRIGHT_YELLOW;
case RARITY_COVERT: return COLOR_RED;
case RARITY_CONTRABAND: return COLOR_BRIGHT_GREEN;
default: return COLOR_WHITE;
}
}

void print_box(int x, int y, int width, int height, const char \*title) {
move_cursor(y, x);

    // Top border
    printf("╔");
    for (int i = 0; i < width - 2; i++) printf("═");
    printf("╗\n");

    // Title
    if (title) {
        move_cursor(y, x + 2);
        printf(" %s ", title);
    }

    // Sides
    for (int i = 1; i < height - 1; i++) {
        move_cursor(y + i, x);
        printf("║");
        move_cursor(y + i, x + width - 1);
        printf("║\n");
    }

    // Bottom border
    move_cursor(y + height - 1, x);
    printf("╚");
    for (int i = 0; i < width - 2; i++) printf("═");
    printf("╝\n");

}

void print_progress_bar(int x, int y, int width, float progress) {
move_cursor(y, x);
printf("[");

    int filled = (int)(progress * (width - 2));
    for (int i = 0; i < width - 2; i++) {
        if (i < filled) {
            printf("█");
        } else {
            printf("░");
        }
    }

    printf("]");

}

    // BỎ: reputation stars
    printf("       ║\n");

    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║                                                               ║\n");
    printf("║  " STYLE_BOLD "[M]" COLOR_RESET " Market          - Browse and buy skins                ║\n");
    printf("║  " STYLE_BOLD "[I]" COLOR_RESET " Inventory       - View your items                    ║\n");
    printf("║  " STYLE_BOLD "[T]" COLOR_RESET " Trade           - P2P trading                        ║\n");
    printf("║  " STYLE_BOLD "[U]" COLOR_RESET " Unbox           - Open cases                         ║\n");
    // BỎ: Group menu item
    printf("║  " STYLE_BOLD "[C]" COLOR_RESET " Chat            - Global chat                        ║\n");
    printf("║  " STYLE_BOLD "[P]" COLOR_RESET " Profile         - View profile                       ║\n");
    printf("║  " STYLE_BOLD "[L]" COLOR_RESET " Leaderboard     - Top traders                        ║\n");
    printf("║  " STYLE_BOLD "[Q]" COLOR_RESET " Quit            - Logout                             ║\n");
    printf("║                                                               ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");

    // Recent activity
    printf("║  📰 RECENT ACTIVITY:                                          ║\n");
    for (int i = 0; i < 3 && i < state->recent_activity_count; i++) {
        printf("║  %s%-55s" COLOR_RESET "║\n",
               COLOR_DIM, state->recent_activity[i]);
    }

    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf("\n> ");

}

````

---

### **7.3. Market View:**

```c
void display_market(MarketListing *listings, int count, int page) {
    clear_screen();

    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║                    💼 MARKET LISTINGS 💼                      ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");

    int start = page * ITEMS_PER_PAGE;
    int end = start + ITEMS_PER_PAGE;
    if (end > count) end = count;

    for (int i = start; i < end; i++) {
        Skin skin;
        load_skin(listings[i].skin_id, &skin);

        const char *color = get_rarity_color(skin.rarity);

        printf("║ %2d. %s%-25s" COLOR_RESET " (%s) %s$%-8.2f" COLOR_RESET " ║\n",
               i + 1,
               color,
               skin.name,
               wear_to_string(skin.wear),
               COLOR_GREEN,
               listings[i].price);

        // Price trend indicator
        float change = calculate_price_change(skin.name, 3600); // Last hour
        if (change > 0) {
            printf("║     " COLOR_GREEN "▲ +%.1f%%" COLOR_RESET, change);
        } else if (change < 0) {
            printf("║     " COLOR_RED "▼ %.1f%%" COLOR_RESET, change);
        } else {
            printf("║     ═ 0.0%%");
        }
        printf("                                              ║\n");
    }

    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║ Page %d/%d | Showing %d-%d of %d items                      ║\n",
           page + 1, (count / ITEMS_PER_PAGE) + 1, start + 1, end, count);
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║ [B]uy [N]ext page [P]rev page [S]ort [F]ilter [Q]Back       ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf("\n> ");
}
````

---

### **7.4. Unbox Animation:**

```c
void display_unbox_animation(Case *case_data, Skin *result) {
    clear_screen();

    // Case opening animation
    printf(COLOR_CYAN STYLE_BOLD);
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║               OPENING %s CASE...                  ║\n", case_data->name);
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);

    // Simulate spinning animation
    Skin preview_skins[20];
    generate_preview_skins(case_data, preview_skins, 20);

    for (int frame = 0; frame < 50; frame++) {
        printf("\r║ ");

        int speed = frame < 30 ? 1 : (50 - frame) / 2 + 1;

        for (int i = 0; i < 5; i++) {
            int idx = (frame / speed + i) % 20;
            const char *color = get_rarity_color(preview_skins[idx].rarity);

            if (i == 2) { // Center item (highlighted)
                printf("[%s%-20s" COLOR_RESET "]", color, preview_skins[idx].name);
            } else {
                printf(" %s%-20s" COLOR_RESET " ", color, preview_skins[idx].name);
            }
        }

        printf(" ║");
        fflush(stdout);
        usleep(50000 + frame * 5000); // Slow down gradually
    }

    // Reveal result
    printf("\n\n");
    printf(STYLE_BOLD);
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║                     🎉 YOU UNBOXED! 🎉                        ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");

    const char *color = get_rarity_color(result->rarity);
    printf("║  %s", color);
    printf("%-61s" COLOR_RESET "║\n", result->name);
    printf("║  Wear: %-15s  Value: " COLOR_GREEN "$%.2f" COLOR_RESET "              ║\n",
           wear_to_string(result->wear), result->current_price);
    printf("║  Rarity: %-48s║\n", rarity_to_string(result->rarity));

    if (result->rarity >= RARITY_COVERT) {
        printf("║                                                               ║\n");
        printf("║  " COLOR_BRIGHT_YELLOW "⚡ RARE DROP! ⚡" COLOR_RESET "                                          ║\n");
    }

    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);

    printf("\nPress any key to continue...");
    getchar();
}
```

---

## **VIII. THREADING & CONCURRENCY**

### **8.1. Thread Pool Implementation:**

```c
/* thread_pool.h */
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include "types.h"

#define MAX_QUEUE_SIZE 1000
#define NUM_WORKER_THREADS 8

typedef struct {
    int client_fd;
    Message request;
} Job;

typedef struct {
    Job queue[MAX_QUEUE_SIZE];
    int head;
    int tail;
    int count;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;

    int shutdown;
} JobQueue;

typedef struct {
    pthread_t threads[NUM_WORKER_THREADS];
    JobQueue job_queue;
} ThreadPool;

// Initialize thread pool
int thread_pool_init(ThreadPool *pool);

// Add job to queue
int thread_pool_add_job(ThreadPool *pool, int client_fd, Message *request);

// Worker thread function
void* worker_thread(void *arg);

// Shutdown thread pool
void thread_pool_shutdown(ThreadPool *pool);

#endif
```

```c
/* thread_pool.c */
#include "thread_pool.h"

int thread_pool_init(ThreadPool *pool) {
    // Initialize queue
    pool->job_queue.head = 0;
    pool->job_queue.tail = 0;
    pool->job_queue.count = 0;
    pool->job_queue.shutdown = 0;

    pthread_mutex_init(&pool->job_queue.mutex, NULL);
    pthread_cond_init(&pool->job_queue.not_empty, NULL);
    pthread_cond_init(&pool->job_queue.not_full, NULL);

    // Create worker threads
    for (int i = 0; i < NUM_WORKER_THREADS; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
            return -1;
        }
    }

    return 0;
}

int thread_pool_add_job(ThreadPool *pool, int client_fd, Message *request) {
    pthread_mutex_lock(&pool->job_queue.mutex);

    // Wait if queue is full
    while (pool->job_queue.count == MAX_QUEUE_SIZE && !pool->job_queue.shutdown) {
        pthread_cond_wait(&pool->job_queue.not_full, &pool->job_queue.mutex);
    }

    if (pool->job_queue.shutdown) {
        pthread_mutex_unlock(&pool->job_queue.mutex);
        return -1;
    }

    // Add job
    Job *job = &pool->job_queue.queue[pool->job_queue.tail];
    job->client_fd = client_fd;
    memcpy(&job->request, request, sizeof(Message));

    pool->job_queue.tail = (pool->job_queue.tail + 1) % MAX_QUEUE_SIZE;
    pool->job_queue.count++;

    pthread_cond_signal(&pool->job_queue.not_empty);
    pthread_mutex_unlock(&pool->job_queue.mutex);

    return 0;
}

void* worker_thread(void *arg) {
    ThreadPool *pool = (ThreadPool*)arg;

    while (1) {
        pthread_mutex_lock(&pool->job_queue.mutex);

        // Wait for job
        while (pool->job_queue.count == 0 && !pool->job_queue.shutdown) {
            pthread_cond_wait(&pool->job_queue.not_empty, &pool->job_queue.mutex);
        }

        if (pool->job_queue.shutdown && pool->job_queue.count == 0) {
            pthread_mutex_unlock(&pool->job_queue.mutex);
            break;
        }

        // Get job
        Job job = pool->job_queue.queue[pool->job_queue.head];
        pool->job_queue.head = (pool->job_queue.head + 1) % MAX_QUEUE_SIZE;
        pool->job_queue.count--;

        pthread_cond_signal(&pool->job_queue.not_full);
        pthread_mutex_unlock(&pool->job_queue.mutex);

        // Process job
        handle_client_request(job.client_fd, &job.request);
    }

    return NULL;
}
```

---

### **8.2. Request Handler:**

```c
/* request_handler.c */
void handle_client_request(int client_fd, Message *request) {
    Message response;
    memset(&response, 0, sizeof(Message));

    // Validate session (except login/register)
    if (request->header.msg_type != MSG_LOGIN_REQUEST &&
        request->header.msg_type != MSG_REGISTER_REQUEST) {

        Session session;
        char *token = extract_session_token(request);

        if (validate_session(token, &session) != ERR_SUCCESS) {
            send_error(client_fd, ERR_SESSION_EXPIRED);
            return;
        }

        // Update last activity
        session.last_activity = time(NULL);
        update_session(&session);
    }

    // Route request
    switch (request->header.msg_type) {
        case MSG_REGISTER_REQUEST:
            handle_register(client_fd, request, &response);
            break;

        case MSG_LOGIN_REQUEST:
            handle_login(client_fd, request, &response);
            break;

        case MSG_GET_MARKET_LISTINGS:
            handle_get_market(client_fd, request, &response);
            break;

        case MSG_BUY_FROM_MARKET:
            handle_buy_market(client_fd, request, &response);
            break;

        case MSG_SELL_TO_MARKET:
            handle_sell_market(client_fd, request, &response);
            break;

        case MSG_SEND_TRADE_OFFER:
            handle_send_trade(client_fd, request, &response);
            break;

        case MSG_ACCEPT_TRADE:
            handle_accept_trade(client_fd, request, &response);
            break;

        case MSG_UNBOX_CASE:
            handle_unbox(client_fd, request, &response);
            break;

        // BỎ: Group handlers

        // BỎ: Report handler

        case MSG_CHAT_GLOBAL:
            handle_chat(client_fd, request, &response);
            break;

        default:
            send_error(client_fd, ERR_INVALID_REQUEST);
            return;
    }

    // Send response
    send_message(client_fd, &response);
}
```

---

## **IX. DATABASE OPERATIONS**

### **9.1. File Structure:**

```c
/* database.h */
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

// BỎ: Group operations

// Transaction log
int db_log_transaction(TransactionLog *log);

// Session operations
int db_save_session(Session *session);
int db_load_session(const char *token, Session *out_session);
int db_delete_session(const char *token);

#endif
```

```c
/* database.c - Example implementation */
#include "database.h"
#include <fcntl.h>
#include <sys/file.h>

#define USERS_FILE "data/users.dat"
#define SKINS_FILE "data/skins.dat"
#define TRADES_FILE "data/trades.dat"

// Global counters (loaded at startup)
static int next_user_id = 1;
static int next_skin_id = 1;
static int next_trade_id = 1;

int db_init() {
    // Create data directory
    mkdir("data", 0755);

    // Create files if not exist
    int fd;

    fd = open(USERS_FILE, O_CREAT | O_RDWR, 0644);
    if (fd < 0) return -1;
    close(fd);

    fd = open(SKINS_FILE, O_CREAT | O_RDWR, 0644);
    if (fd < 0) return -1;
    close(fd);

    // Load counters
    load_counters();

    return 0;
}

int db_save_user(User *user) {
    int fd = open(USERS_FILE, O_WRONLY | O_APPEND);
    if (fd < 0) return -1;

    // File lock
    flock(fd, LOCK_EX);

    // Assign ID if new
    if (user->user_id == 0) {
        user->user_id = next_user_id++;
    }

    // Write
    ssize_t written = write(fd, user, sizeof(User));

    flock(fd, LOCK_UN);
    close(fd);

    return (written == sizeof(User)) ? 0 : -1;
}

int db_load_user(int user_id, User *out_user) {
    int fd = open(USERS_FILE, O_RDONLY);
    if (fd < 0) return -1;

    User temp;
    while (read(fd, &temp, sizeof(User)) == sizeof(User)) {
        if (temp.user_id == user_id) {
            *out_user = temp;
            close(fd);
            return 0;
        }
    }

    close(fd);
    return -1; // Not found
}

int db_load_user_by_username(const char *username, User *out_user) {
    int fd = open(USERS_FILE, O_RDONLY);
    if (fd < 0) return -1;

    User temp;
    while (read(fd, &temp, sizeof(User)) == sizeof(User)) {
        if (strcmp(temp.username, username) == 0) {
            *out_user = temp;
            close(fd);
            return 0;
        }
    }

    close(fd);
    return -1;
}

int db_update_user(User *user) {
    // Read all users
    int fd = open(USERS_FILE, O_RDWR);
    if (fd < 0) return -1;

    flock(fd, LOCK_EX);

    User temp;
    off_t offset = 0;
    int found = 0;

    while (read(fd, &temp, sizeof(User)) == sizeof(User)) {
        if (temp.user_id == user->user_id) {
            // Found - update in place
            lseek(fd, offset, SEEK_SET);
            write(fd, user, sizeof(User));
            found = 1;
            break;
        }
        offset += sizeof(User);
    }

    flock(fd, LOCK_UN);
    close(fd);

    return found ? 0 : -1;
}

// Similar implementations for other db_ functions...
```

---

### **9.2. Indexing for Performance:**

```c
/* index.h */
#ifndef INDEX_H
#define INDEX_H

#include "types.h"

// Hash table for fast lookups
#define HASH_TABLE_SIZE 10007

typedef struct IndexNode {
    int key;                  // user_id, skin_id, etc.
    off_t file_offset;        // Position in file
    struct IndexNode *next;   // Collision handling
} IndexNode;

typedef struct {
    IndexNode *buckets[HASH_TABLE_SIZE];
    pthread_rwlock_t lock;
} Index;

// Initialize index
void index_init(Index *idx);

// Add entry
void index_add(Index *idx, int key, off_t offset);

// Lookup entry
off_t index_lookup(Index *idx, int key);

// Remove entry
void index_remove(Index *idx, int key);

// Hash function
unsigned int hash(int key);

#endif
```

```c
/* index.c */
#include "index.h"

unsigned int hash(int key) {
    return key % HASH_TABLE_SIZE;
}

void index_init(Index *idx) {
    memset(idx->buckets, 0, sizeof(idx->buckets));
    pthread_rwlock_init(&idx->lock, NULL);
}

void index_add(Index *idx, int key, off_t offset) {
    pthread_rwlock_wrlock(&idx->lock);

    unsigned int h = hash(key);

    IndexNode *node = malloc(sizeof(IndexNode));
    node->key = key;
    node->file_offset = offset;
    node->next = idx->buckets[h];

    idx->buckets[h] = node;

    pthread_rwlock_unlock(&idx->lock);
}

off_t index_lookup(Index *idx, int key) {
    pthread_rwlock_rdlock(&idx->lock);

    unsigned int h = hash(key);
    IndexNode *node = idx->buckets[h];

    while (node) {
        if (node->key == key) {
            off_t offset = node->file_offset;
            pthread_rwlock_unlock(&idx->lock);
            return offset;
        }
        node = node->next;
    }

    pthread_rwlock_unlock(&idx->lock);
    return -1; // Not found
}

// Load index at startup
void load_indexes() {
    // Build user index
    int fd = open(USERS_FILE, O_RDONLY);
    User temp;
    off_t offset = 0;

    while (read(fd, &temp, sizeof(User)) == sizeof(User)) {
        index_add(&user_index, temp.user_id, offset);
        offset += sizeof(User);
    }
    close(fd);

    // Similar for skins, trades, etc.
}
```

---

## **X. TESTING STRATEGY**

### **10.1. Unit Tests:**

```c
/* test_market.c */
#include "market.h"
#include <assert.h>

void test_buy_from_market() {
    // Setup
    User buyer = create_test_user("buyer", 1000.0);
    User seller = create_test_user("seller", 500.0);
    Skin skin = create_test_skin("AWP | Asiimov", 100.0);

    MarketListing listing;
    listing.seller_id = seller.user_id;
    listing.skin_id = skin.skin_id;
    listing.price = 100.0;
    listing.is_sold = 0;
    save_listing(&listing);

    // Execute
    int result = buy_from_market(buyer.user_id, listing.listing_id);

    // Verify
    assert(result == ERR_SUCCESS);

    load_user(buyer.user_id, &buyer);
    assert(buyer.balance == 900.0); // 1000 - 100

    load_user(seller.user_id, &seller);
    assert(seller.balance == 585.0); // 500 + (100 * 0.85)

    load_skin(skin.skin_id, &skin);
    assert(skin.owner_id == buyer.user_id);

    printf("✓ test_buy_from_market passed\n");
}

void test_insufficient_funds() {
    User buyer = create_test_user("poor", 10.0);
    MarketListing listing;
    listing.price = 100.0;

    int result = buy_from_market(buyer.user_id, listing.listing_id);
    assert(result == ERR_INSUFFICIENT_FUNDS);

    printf("✓ test_insufficient_funds passed\n");
}

int main() {
    db_init();
    test_buy_from_market();
    test_insufficient_funds();
    // More tests...
    return 0;
}
```

---

### **10.2. Integration Tests:**

```c
/* test_integration.c */
void test_complete_trade_workflow() {
    // 1. Two users register
    User alice = register_and_login("alice", "pass123");
    User bob = register_and_login("bob", "pass456");

    // 2. Alice lists skin on market
    Skin skin = create_test_skin("AK-47 | Redline", 50.0);
    give_skin_to_user(alice.user_id, skin.skin_id);

    int listing_id = list_skin_on_market(alice.user_id, skin.skin_id, 50.0);

    // 3. Bob buys from market
    int result = buy_from_market(bob.user_id, listing_id);
    assert(result == ERR_SUCCESS);

    // 4. Verify ownership transferred
    load_skin(skin.skin_id, &skin);
    assert(skin.owner_id == bob.user_id);

    // 5. Bob sends trade offer to Alice
    TradeOffer offer;
    offer.from_user_id = bob.user_id;
    offer.to_user_id = alice.user_id;
    offer.offered_skins[0] = skin.skin_id;
    offer.offered_count = 1;
    offer.requested_cash = 60.0;

    send_trade_offer(bob.user_id, alice.user_id, &offer);

    // 6. Alice accepts
    result = accept_trade(alice.user_id, offer.trade_id);
    assert(result == ERR_SUCCESS);

    // 7. Verify trade completed
    load_skin(skin.skin_id, &skin);
    assert(skin.owner_id == alice.user_id);

    load_user(alice.user_id, &alice);
    load_user(bob.user_id, &bob);
    assert(alice.balance < bob.balance);

    printf("✓ test_complete_trade_workflow passed\n");
}
```

---

### **10.3. Load Testing:**

```c
/* test_load.c */
void simulate_concurrent_users(int num_users) {
    pthread_t threads[num_users];

    for (int i = 0; i < num_users; i++) {
        pthread_create(&threads[i], NULL, simulate_user_activity, (void*)(long)i);
    }

    for (int i = 0; i < num_users; i++) {
        pthread_join(threads[i], NULL);
    }
}

void* simulate_user_activity(void *arg) {
    int user_num = (int)(long)arg;

    // Connect to server
    int sock = connect_to_server();

    // Register/Login
    char username[32];
    sprintf(username, "user%d", user_num);
    login(sock, username, "password");

    // Perform random actions for 60 seconds
    time_t start = time(NULL);
    while (time(NULL) - start < 60) {
        int action = rand() % 5;

        switch (action) {
            case 0: browse_market(sock); break;
            case 1: buy_random_item(sock); break;
            case 2: list_random_item(sock); break;
            case 3: unbox_case(sock); break;
            case 4: send_chat_message(sock); break;
        }

        sleep(rand() % 5 + 1);
    }

    close(sock);
    return NULL;
}

int main() {
    printf("Starting load test with 100 concurrent users...\n");

    time_t start = time(NULL);
    simulate_concurrent_users(100);
    time_t end = time(NULL);

    printf("Load test completed in %ld seconds\n", end - start);

    // Check server logs for errors
    check_server_logs();

    return 0;
}
```

---

## **XI. DEPLOYMENT & RUNNING**

### **11.1. Makefile:**

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2 -g
LDFLAGS = -lssl -lcrypto -luuid -lm

SERVER_SRC = server.c auth.c market.c trading.c unbox.c database.c \
             thread_pool.c request_handler.c index.c
CLIENT_SRC = client.c ui.c network_client.c
COMMON_SRC = protocol.c utils.c

SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
COMMON_OBJ = $(COMMON_SRC:.c=.o)

all: server client

server: $(SERVER_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

client: $(CLIENT_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $

clean:
	rm -f *.o server client
	rm -rf data/*.dat

test: all
	./test_market
	./test_integration
	./test_load

run_server: server
	./server 8888

run_client: client
	./client localhost 8888

.PHONY: all clean test run_server run_client
```

---

### **11.2. Configuration File:**

```ini
# config.ini

[server]
port = 8888
max_clients = 200
thread_pool_size = 8
session_timeout = 3600

[database]
data_dir = data/
auto_backup = true
backup_interval = 3600

[market]
fee_rate = 0.15
min_listing_price = 0.10
max_listing_price = 100000.00

[unbox]
case_key_price = 2.50
trade_lock_days = 7

[economy]
starting_balance = 100.00
daily_login_bonus = 5.00
max_balance = 1000000.00

[security]
password_min_length = 6
max_login_attempts = 5
ban_duration = 86400
scam_report_threshold = 3
```

---

### **11.3. Startup Scripts:**

```bash
#!/bin/bash
# start_server.sh

echo "Starting CS2 Skin Trading Server..."

# Create data directory
mkdir -p data logs

# Initialize database
./server --init

# Start server
./server 8888 > logs/server.log 2>&1 &

echo "Server started on port 8888"
echo "PID: $!"
echo $! > server.pid
```

```bash
#!/bin/bash
# start_client.sh

echo "Starting CS2 Skin Trading Client..."

# Check server is running
if ! nc -z localhost 8888; then
    echo "Error: Server is not running on port 8888"
    exit 1
fi

# Start client
./client localhost 8888
```

---

## **XII. PHÁT TRIỂN MỞ RỘNG (Optional)**

### **12.1. Tính năng nâng cao:**

1. **Web Dashboard** (Bonus):

   - Server cung cấp HTTP API đơn giản
   - HTML/CSS/JS hiển thị stats real-time
   - Chart giá cả, leaderboard

2. **Bot Trading** (AI):

   - Simulate bot traders với chiến lược đơn giản
   - Tạo liquidity cho market
   - Tăng tính sống động

3. **Achievement System**:

   - Unlock badges khi đạt milestone
   - Hiển thị trong profile
   - Reward extra balance

4. **Seasonal Events**:
   - Limited-time cases
   - Discount periods
   - Special tournaments

---

### **12.2. Performance Optimization:**

```c
// Cache frequently accessed data
typedef struct {
    int user_id;
    User data;
    time_t cached_at;
} CachedUser;

#define CACHE_SIZE 1000
#define CACHE_TTL 300 // 5 minutes

CachedUser user_cache[CACHE_SIZE];
pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

User* get_user_cached(int user_id) {
    pthread_mutex_lock(&cache_mutex);

    // Check cache
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (user_cache[i].user_id == user_id) {
            if (time(NULL) - user_cache[i].cached_at < CACHE_TTL) {
                pthread_mutex_unlock(&cache_mutex);
                return &user_cache[i].data;
            }
        }
    }

    // Load from database
    User user;
    db_load_user(user_id, &user);

    // Add to cache
    int slot = user_id % CACHE_SIZE;
    user_cache[slot].user_id = user_id;
    user_cache[slot].data = user;
    user_cache[slot].cached_at = time(NULL);

    pthread_mutex_unlock(&cache_mutex);
    return &user_cache[slot].data;
}
```

---

## **XIII. TÀI LIỆU DEMO**

### **13.1. Demo Scenario:**

```
=== DEMO SCRIPT ===

1. Start Server:
   ./server 8888

2. Terminal 1 - Alice:
   ./client localhost 8888
   > register
   Username: alice
   Password: ****
   > market
   > buy 5 (mua AWP Asiimov $45)
   > inventory
   > sell 1 $50 (list lên market)

3. Terminal 2 - Bob:
   ./client localhost 8888
   > register
   Username: bob
   Password: ****
   > unbox
   > select Revolution Case
   > [Animation shows... unboxed AK-47 Redline]
   > trade alice
   > offer AK-47, want $30

4. Terminal 1 - Alice:
   > [notification: Bob sent trade offer]
   > trades
   > view 1
   > accept
   > [trade completed!]

// BỎ: Admin terminal demo

6. All terminals:
   > [notification: Free case dropped!]
   > grab case
   > unbox
```

---

## **XIV. KẾT LUẬN**

### **14.1. Tổng kết:**

Project **CS2 Skin Trading Simulator** đáp ứng đầy đủ:

✅ **19 tiêu chí chấm điểm** học phần  
✅ **Lập trình mạng C thuần** - Socket, threading, protocol  
✅ **Giao diện text-based** - Dễ code, không cần graphics lib  
✅ **Logic đơn giản** - CRUD operations, RNG, market dynamics  
✅ **Khả thi cao** - Tất cả module đều implement được bằng C  
✅ **Không có yếu tố cờ bạc** - Unbox = reward system với xác suất công khai  
✅ **Mở rộng tốt** - Có thể thêm nhiều tính năng

### **14.2. Công việc phân chia (nếu làm nhóm):**

- **Thành viên 1**: Server core + Threading + Protocol
- **Thành viên 2**: Authentication + Database + Session
- **Thành viên 3**: Market + Trading System
- **Thành viên 4**: Unbox + Chat
- **Thành viên 5**: Client UI + Testing

### **14.3. Timeline:**

- **Week 1-2**: Core architecture, protocol, database
- **Week 3-4**: Market + Trading implementation
- **Week 5**: Unbox system
- **Week 6**: Client UI + Integration
- **Week 7**: Testing + Bug fixes
- **Week 8**: Demo preparation + Documentation

---

**Bạn có muốn tôi chi tiết hóa thêm phần nào không? Hoặc cần code mẫu cụ thể hơn cho module nào?** 💻
