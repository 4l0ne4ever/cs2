# Quick Start Guide - CS2 Skin Trading Simulator

## 1. Setup Database

```bash
# Initialize database với sample data
make init_db
```

Sẽ tạo file `data/database.db` với:

- ✅ 3 sample users: alice, bob, charlie
- ✅ 28 skins (knives, AWPs, AKs, pistols)
- ✅ 3 cases (Revolution, Chroma, Phoenix)
- ✅ 4 active market listings

## 2. View Database

### Option 1: SQLite CLI

```bash
sqlite3 data/database.db

# Xem users
SELECT * FROM users;

# Xem skins với giá cao nhất
SELECT name, rarity, current_price FROM skins
ORDER BY current_price DESC LIMIT 5;

# Xem active market listings
SELECT * FROM market_listings WHERE is_sold = 0;

# Exit
.quit
```

### Option 2: DB Browser (GUI)

1. Download: https://sqlitebrowser.org/
2. Open: `data/database.db`
3. Browse tables với UI

## 3. Check Sample Data

```bash
# Count items
sqlite3 data/database.db "SELECT COUNT(*) FROM skins;"
sqlite3 data/database.db "SELECT COUNT(*) FROM users;"
sqlite3 data/database.db "SELECT COUNT(*) FROM cases;"

# View expensive skins
sqlite3 data/database.db "
SELECT skin_id, name, rarity, current_price
FROM skins
WHERE current_price > 100
ORDER BY current_price DESC;"

# View user inventories
sqlite3 data/database.db "
SELECT u.username, s.name, s.current_price
FROM inventories i
JOIN users u ON i.user_id = u.user_id
JOIN skins s ON i.skin_id = s.skin_id
LIMIT 10;"
```

## 4. Useful Queries

### Find all knives (Contraband rarity = 6)

```sql
SELECT * FROM skins WHERE rarity = 6;
```

### Find market listings with prices > $1000

```sql
SELECT m.*, s.name
FROM market_listings m
JOIN skins s ON m.skin_id = s.skin_id
WHERE m.price > 1000 AND m.is_sold = 0;
```

### View user with most expensive skins

```sql
SELECT u.username, s.name, s.current_price
FROM inventories i
JOIN users u ON i.user_id = u.user_id
JOIN skins s ON i.skin_id = s.skin_id
ORDER BY s.current_price DESC
LIMIT 10;
```

## 5. Reset Database

```bash
# Xóa và tạo lại database
rm data/database.db
make init_db
```

## 6. Database Schema

See `DATABASE.md` for complete schema documentation

### Tables:

- `users` - User accounts
- `skins` - Skin items
- `inventories` - User inventories (many-to-many)
- `market_listings` - Market listings
- `trades` - Trade offers
- `cases` - Case definitions
- `sessions` - Active sessions
- `transaction_logs` - Transaction history

## 7. Test Queries

```bash
# Test all operations
make test_sqlite

# Test auth
make test_auth
```

---

**Next Steps**: Implement market.c, trading.c, unbox.c để sử dụng database này!
