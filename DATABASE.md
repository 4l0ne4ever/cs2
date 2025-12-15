# CS2 Skin Trading - Database Guide

## Database: SQLite

Project sử dụng SQLite thay vì binary files để:

- ✅ Dễ debug và check data
- ✅ Query linh hoạt
- ✅ ACID transactions
- ✅ Indexed queries (nhanh hơn)

---

## View Database

### Cách 1: SQLite CLI

```bash
sqlite3 data/database.db

# Xem tất cả tables
.tables

# Xem schema của table
.schema skins

# Query users
SELECT * FROM users;

# Query skins với filter
SELECT skin_id, name, current_price FROM skins
WHERE rarity >= 4 ORDER BY current_price DESC;

# Query market listings (active)
SELECT * FROM market_listings WHERE is_sold = 0;

# Query inventories
SELECT * FROM inventories WHERE user_id = 1;

# Exit
.quit
```

### Cách 2: DB Browser for SQLite (GUI)

1. Download tại: https://sqlitebrowser.org/
2. Mở file `data/database.db`
3. Browse data với UI thân thiện

---

## Database Tables

### 1. users

Stores user accounts:

```sql
SELECT user_id, username, balance FROM users;
```

### 2. skins

Stores all skins:

```sql
SELECT skin_id, name, rarity, current_price, owner_id FROM skins;
```

### 3. inventories

User inventories (many-to-many):

```sql
SELECT * FROM inventories WHERE user_id = 1;
```

### 4. market_listings

Market listings:

```sql
SELECT * FROM market_listings WHERE is_sold = 0;
```

### 5. trades

Trade offers:

```sql
SELECT * FROM trades WHERE status = 0;
```

### 6. cases

Case definitions:

```sql
SELECT * FROM cases;
```

### 7. sessions

Active sessions:

```sql
SELECT * FROM sessions WHERE is_active = 1;
```

### 8. transaction_logs

Transaction history:

```sql
SELECT * FROM transaction_logs ORDER BY timestamp DESC LIMIT 20;
```

---

## Useful Queries

### Count skins by rarity

```sql
SELECT rarity, COUNT(*) as count
FROM skins
GROUP BY rarity;
```

### Find expensive skins (> $100)

```sql
SELECT name, current_price
FROM skins
WHERE current_price > 100
ORDER BY current_price DESC;
```

### User inventories

```sql
SELECT u.username, COUNT(i.skin_id) as skin_count
FROM users u
LEFT JOIN inventories i ON u.user_id = i.user_id
GROUP BY u.user_id;
```

### Market listings by user

```sql
SELECT u.username, COUNT(m.listing_id) as active_listings
FROM users u
LEFT JOIN market_listings m ON u.user_id = m.seller_id
WHERE m.is_sold = 0 OR m.is_sold IS NULL
GROUP BY u.user_id;
```

### Trade history

```sql
SELECT
    t.trade_id,
    u1.username as from_user,
    u2.username as to_user,
    t.status,
    datetime(t.created_at, 'unixepoch') as created
FROM trades t
LEFT JOIN users u1 ON t.from_user_id = u1.user_id
LEFT JOIN users u2 ON t.to_user_id = u2.user_id
ORDER BY t.created_at DESC;
```

---

## Initialize Database

```bash
# Clean và tạo lại database với data mẫu
make init_db

# Nếu muốn tạo database thủ công
sqlite3 data/database.db < data/schema.sql
sqlite3 data/database.db < data/init_data.sql
```

---

## Database Features

- **ACID Transactions**: Atomic operations
- **Indexes**: Fast lookups
- **Foreign Keys**: Data integrity
- **Views**: Pre-defined queries (có thể thêm sau)
- **Triggers**: Auto-update logic (có thể thêm sau)

---

## Migration từ Binary Files

Nếu cần migrate data cũ:

```bash
# Export từ binary
# (Cần tool convert)

# Import vào SQLite
sqlite3 data/database.db
.mode csv
.import data.csv table_name
```
