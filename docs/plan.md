# 📋 PLAN IMPLEMENTATION - CS2 SKIN TRADING SIMULATOR

## **TỔNG QUAN**

Project này được điều chỉnh để loại bỏ 3 modules phức tạp:

- ❌ **Admin System** - Quá phức tạp cho project học tập
- ❌ **Reputation System** - Không cần thiết, đơn giản hóa anti-scam
- ❌ **Trading Groups** - Feature nâng cao, có thể thêm sau

**Độ khó mới**: ⭐⭐⭐⭐ (Khó nhưng khả thi)
**Thời gian ước tính**: 60-80 giờ
**Lines of code dự kiến**: ~2500 lines

---

## **PHASE 1: ARCHITECTURE & PROTOCOL (Week 1-2)**

### **1.1. Core Data Structures**

**File: `types.h`**

````c
// User - BỎ reputation và is_admin fields
typedef struct {
    int user_id;
    char username[32];
    char password_hash[65]; // SHA256 hash
    float balance;
    time_t created_at;
    time_t last_login;
    int is_banned;
} User;

// Session - giữ nguyên
typedef struct {
    char session_token[37]; // UUID
    int user_id;
    int socket_fd;
    time_t login_time;
    time_t last_activity;
    int is_active;
} Session;

// Skin - giữ nguyên
typedef struct {
    int skin_id;
    char name[64];
    SkinRarity rarity;
    WearCondition wear;
    float base_price;
    float current_price;
    int owner_id; // 0 = not owned (in market)
    time_t acquired_at;
    int is_tradable; // Trade lock 7 days
} Skin;

// TradeOffer - giữ nguyên
typedef struct {
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
    time_t expires_at;
} TradeOffer;

// MarketListing - giữ nguyên
typedef struct {
    int listing_id;
    int seller_id;
    int skin_id;
    float price;
    time_t listed_at;
    int is_sold;
} MarketListing;

// Case - giữ nguyên
typedef struct {
    int case_id;
    char name[32];
    float price;
    int possible_skins[50];
    float probabilities[50];
    int skin_count;
} Case;

// Inventory - giữ nguyên
typedef struct {
    int user_id;
    Skin skins[MAX_INVENTORY_SIZE]; // 200 items
    int count;
} Inventory;

// TransactionLog - giữ nguyên
typedef struct {
    int log_id;
    LogType type;
    int user_id;
    char details[256];
    time_t timestamp;
} TransactionLog;

// BỎ Report struct (không cần report system)

### **1.2. Protocol Messages**

**File: `protocol.h`**

```c
// AUTHENTICATION
#define MSG_REGISTER_REQUEST 0x0001
#define MSG_REGISTER_RESPONSE 0x0002
#define MSG_LOGIN_REQUEST 0x0003
#define MSG_LOGIN_RESPONSE 0x0004
#define MSG_LOGOUT 0x0005

// MARKET
#define MSG_GET_MARKET_LISTINGS 0x0010
#define MSG_MARKET_DATA 0x0011
#define MSG_BUY_FROM_MARKET 0x0012
#define MSG_SELL_TO_MARKET 0x0013
#define MSG_PRICE_UPDATE 0x0014

// TRADING
#define MSG_SEND_TRADE_OFFER 0x0020
#define MSG_TRADE_OFFER_NOTIFY 0x0021
#define MSG_ACCEPT_TRADE 0x0022
#define MSG_DECLINE_TRADE 0x0023
#define MSG_CANCEL_TRADE 0x0024
#define MSG_TRADE_COMPLETED 0x0025

// INVENTORY
#define MSG_GET_INVENTORY 0x0030
#define MSG_INVENTORY_DATA 0x0031
#define MSG_GET_USER_PROFILE 0x0032
#define MSG_USER_PROFILE_DATA 0x0033

// UNBOXING
#define MSG_UNBOX_CASE 0x0040
#define MSG_UNBOX_RESULT 0x0041
#define MSG_GET_CASES 0x0042
#define MSG_CASES_DATA 0x0043

// CHAT - giữ lại nhưng đơn giản hóa
#define MSG_CHAT_GLOBAL 0x0060

// BỎ REPORT messages (không cần report system)

// MISC
#define MSG_HEARTBEAT 0x0090
#define MSG_ERROR 0x00FF

// BỎ CÁC ADMIN MESSAGES
// BỎ CÁC GROUP MESSAGES
````

### **1.3. Database Schema**

**File Structure:**

```
data/
├── users.dat
├── skins.dat
├── inventories.dat
├── trades.dat
├── market.dat
├── cases.dat
├── logs.dat
└── config.txt
```

**BỎ:** `groups.dat`, `reports.dat`

---

## **PHASE 2: DATABASE & INDEXING (Week 2)**

### **2.1. Database Operations**

**Files:** `database.h`, `database.c`

**Core functions:**

- `db_init()` - Initialize database files
- `db_save_user()`, `db_load_user()`, `db_update_user()`
- `db_save_skin()`, `db_load_skin()`, `db_update_skin()`
- `db_save_trade()`, `db_load_trade()`, `db_update_trade()`
- `db_save_listing()`, `db_load_listings()`, `db_update_listing()`
- `db_save_session()`, `db_load_session()`, `db_delete_session()`
- `db_log_transaction()`

**BỎ:** Tất cả group operations, report operations

### **2.2. Indexing System**

**Files:** `index.h`, `index.c`

- Hash table cho fast lookup
- Index users, skins, trades
- Thread-safe với rwlock

---

## **PHASE 3: AUTHENTICATION & SESSION (Week 3)**

### **3.1. Authentication Module**

**Files:** `auth.h`, `auth.c`

```c
// Core functions:
void hash_password(const char *password, char *output);  // SHA256
int verify_password(const char *password, const char *hash);
void generate_session_token(char *token);  // UUID
int register_user(const char *username, const char *password, User *out);
int login_user(const char *username, const char *password, Session *out);
int validate_session(const char *token, Session *out);
void logout_user(const char *session_token);
```

**Thay đổi:**

- BỎ reputation initialization
- BỎ is_admin field

---

## **PHASE 4: MARKET ENGINE (Week 4)**

### **4.1. Market Operations**

**Files:** `market.h`, `market.c`

```c
// Core functions:
int get_market_listings(MarketListing *out_listings, int *count);
int list_skin_on_market(int user_id, int skin_id, float price);
int buy_from_market(int buyer_id, int listing_id);
int sell_to_market(int seller_id, int skin_id, float price);
int remove_listing(int listing_id);
void update_market_prices();  // Supply/demand logic
float get_current_price(int skin_type, WearCondition wear);
```

**Logic:**

- 15% market fee
- Price dynamics dựa trên supply/demand
- Auto price adjustment

---

## **PHASE 5: TRADING SYSTEM (Week 5)**

### **5.1. Trading Operations**

**Files:** `trading.h`, `trading.c`

```c
// Core functions:
int send_trade_offer(int from_user, int to_user, TradeOffer *offer);
int accept_trade(int user_id, int trade_id);
int decline_trade(int user_id, int trade_id);
int cancel_trade(int user_id, int trade_id);
int get_user_trades(int user_id, TradeOffer *out_trades, int *count);
int validate_trade(TradeOffer *offer);
int execute_trade(TradeOffer *offer);  // ATOMIC!
void clean_expired_trades();
```

**Critical:**

- Atomic trade execution với file locking
- 7-day trade lock
- 15-minute expiration

---

## **PHASE 6: UNBOX ENGINE (Week 5)**

### **6.1. Unboxing System**

**Files:** `unbox.h`, `unbox.c`

```c
// Core functions:
int get_available_cases(Case *out_cases, int *count);
int unbox_case(int user_id, int case_id, Skin *out_skin);
int roll_unbox(Case *case_data);  // RNG
```

**Logic:**

- RNG với probabilities: 0.26% Contraband, 0.64% Covert, etc.
- Animation simulation
- Broadcast rare drops
- 7-day trade lock

---

## **PHASE 7: REPORT SYSTEM (Week 6)**

### **7.1. Simple Report System**

**Files:** `report.h`, `report.c`

```c
// Core functions:
int report_user(int reporter_id, int reported_id, const char *reason);
int get_reports_for_user(int user_id, Report *out_reports, int *count);
void broadcast_warning(int user_id);  // If reports > threshold
```

**Thay đổi từ reputation:**

- Đơn giản hơn: chỉ log reports
- Không tự động ban
- Không có reputation score
- Cảnh báo cơ bản trong trade

---

## **PHASE 8: CLIENT-SERVER CORE (Week 6)**

### **8.1. Server Architecture**

**Files:** `server.c`, `thread_pool.h`, `thread_pool.c`, `request_handler.c`

**Main components:**

- Multi-threaded server với pthread
- Thread pool (8 workers)
- select() for connection management
- Request routing

**Request Handler - BỎ group handlers:**

```c
switch (request->header.msg_type) {
    case MSG_REGISTER_REQUEST: ...
    case MSG_LOGIN_REQUEST: ...
    case MSG_GET_MARKET_LISTINGS: ...
    case MSG_BUY_FROM_MARKET: ...
    case MSG_SELL_TO_MARKET: ...
    case MSG_SEND_TRADE_OFFER: ...
    case MSG_ACCEPT_TRADE: ...
    case MSG_UNBOX_CASE: ...
    case MSG_CHAT_GLOBAL: ...
    // BỎ: MSG_CREATE_GROUP
    // BỎ: MSG_REPORT_USER
    // BỎ: MSG_INVITE_TO_GROUP
    // BỎ: MSG_ADMIN_*
    // BỎ: MSG_REPORT_*
}
```

### **8.2. Client Architecture**

**Files:** `client.c`, `network_client.c`

- Connect to server
- Send/receive messages
- Handle server updates
- Basic UI

---

## **PHASE 9: UI IMPLEMENTATION (Week 7)**

### **9.1. Terminal UI**

**Files:** `ui.h`, `ui.c`

**ANSI colors:**

- Rarity colors for skins
- Box drawing
- Progress bars
- Menu navigation

**BỎ:** Reputation stars display
**BỎ:** Group menu items

---

## **PHASE 10: TESTING & INTEGRATION (Week 8)**

### **10.1. Unit Tests**

**Files:** `test_*.c`

- Test market operations
- Test trading operations
- Test unboxing RNG
- Test authentication

### **10.2. Integration Tests**

- Complete workflow tests
- Multi-user scenarios
- Error handling

### **10.3. Load Tests**

- Concurrent connections
- High request volume
- Memory leak detection

---

## **TIMELINE SUMMARY**

| Week | Focus            | Deliverables                   |
| ---- | ---------------- | ------------------------------ |
| 1-2  | Architecture     | types.h, protocol.h, db schema |
| 2    | Database         | database.c, index.c            |
| 3    | Auth             | auth.c, session management     |
| 4    | Market           | market.c, price dynamics       |
| 5    | Trading & Unbox  | trading.c, unbox.c             |
| 6    | Core systems     | server.c, request_handler.c    |
| 7    | UI & Integration | client.c, ui.c                 |
| 8    | Testing & Polish | tests, bug fixes, demo         |

---

## **FILES TỔNG HỢP**

### **Server Files:**

```
server.c              (Main server loop)
thread_pool.h/c       (Thread management)
request_handler.c     (Route messages)
auth.h/c              (Authentication)
market.h/c            (Market engine)
trading.h/c           (P2P trading)
unbox.h/c             (Unbox engine)
// BỎ report.h/c
database.h/c          (Database operations)
index.h/c             (Indexing)
types.h               (Data structures)
protocol.h            (Message protocol)
utils.c               (Helper functions)
```

### **Client Files:**

```
client.c              (Main client loop)
network_client.c      (Network operations)
ui.h/c                (Terminal UI)
```

### **Common Files:**

```
protocol.h/c          (Message protocol)
types.h                (Data structures)
utils.c                (Helper functions)
```

### **Test Files:**

```
test_auth.c
test_market.c
test_trading.c
test_integration.c
test_load.c
```

### **Config Files:**

```
Makefile
config.txt
```

---

## **TASKS CHI TIẾT CHO MỖI TUẦN**

### **Week 1-2: Setup & Architecture**

- [ ] Create project structure
- [ ] Define `types.h` (bỏ reputation, is_admin, TradingGroup)
- [ ] Define `protocol.h` (bỏ admin & group messages)
- [ ] Design database schema
- [ ] Setup Makefile
- [ ] Create skeleton files

### **Week 2: Database**

- [ ] Implement `database.h/c` (bỏ group operations)
- [ ] Implement `index.h/c`
- [ ] Add file locking mechanisms
- [ ] Test database operations

### **Week 3: Authentication**

- [ ] Implement `auth.h/c`
- [ ] Add SHA256 hashing
- [ ] Add UUID generation
- [ ] Implement session management
- [ ] Test auth flow

### **Week 4: Market**

- [ ] Implement `market.h/c`
- [ ] Add price dynamics logic
- [ ] Market buy/sell operations
- [ ] Fee calculation (15%)
- [ ] Test market operations

### **Week 5: Trading & Unbox**

- [ ] Implement `trading.h/c`
- [ ] Atomic trade execution
- [ ] Trade validation
- [ ] Implement `unbox.h/c`
- [ ] RNG algorithm
- [ ] Test both systems

### **Week 6: Core Systems**

- [ ] Implement trade lock logic
- [ ] Implement `server.c`
- [ ] Implement thread pool
- [ ] Implement request handler
- [ ] Test multi-threading

### **Week 7: UI**

- [ ] Implement `client.c`
- [ ] Implement `network_client.c`
- [ ] Implement `ui.h/c`
- [ ] ANSI colors & menus
- [ ] Test UI flow

### **Week 8: Testing**

- [ ] Write unit tests
- [ ] Write integration tests
- [ ] Write load tests
- [ ] Bug fixes
- [ ] Demo preparation

---

## **ĐIỀU CHỈNH CẦN LÀM**

### **1. Trong `types.h`:**

- BỎ field `reputation` trong User struct
- BỎ field `is_admin` trong User struct
- BỎ toàn bộ TradingGroup struct

### **2. Trong `protocol.h`:**

- BỎ `MSG_ADMIN_*` messages (0x0070-0x0073)
- BỎ `MSG_CREATE_GROUP` to `MSG_GROUP_DATA` (0x0050-0x0058)
- BỎ `MSG_REPORT_*` messages (0x0080-0x0081)

### **3. Trong `architecture.md`:**

- Sửa User struct: bỏ reputation, is_admin
- BỎ TradingGroup struct
- BỎ Group workflow section
- BỎ Admin powers section
- Sửa reputation logic thành report logic
- BỎ admin message handlers

### **4. Trong `database.c`:**

- BỎ tất cả group operations (`db_save_group`, etc.)
- BỎ report operations
- BỎ groups.dat, reports.dat files

### **5. Trong `request_handler.c`:**

- BỎ group-related case statements
- BỎ admin-related case statements
- BỎ report-related case statements

### **6. Trong `ui.c`:**

- BỎ reputation star display
- BỎ group menu items
- BỎ admin panel

---

## **KẾT QUẢ MONG MUỐN**

✅ Hệ thống Client-Server hoàn chỉnh
✅ Multi-threaded server với thread pool
✅ Authentication & Session management
✅ Market engine với price dynamics
✅ P2P Trading với atomic operations
✅ Unboxing với RNG và probabilities
✅ Global chat system
✅ Trade lock system (7 days)
✅ Terminal UI với ANSI colors
✅ Database với binary files
✅ Testing suite

**Không bao gồm:**

- Admin system
- Reputation system
- Trading groups
- Anti-scam system (chỉ giữ trade lock 7 ngày)

---

**Chúc bạn implementation thành công! 🚀**
