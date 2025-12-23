# 🚀 CS2 Skin Trading Simulator - Demo Guide

## Bước 1: Cài Đặt (Chỉ cần làm 1 lần)

**macOS:**

```bash
brew install sqlite3
```

**Linux (Ubuntu/Debian):**

```bash
sudo apt-get install build-essential libsqlite3-dev
```

---

## Bước 2: Build Project

```bash
make all
```

---

## Bước 3: Khởi Tạo Database (Chỉ cần làm 1 lần)

```bash
make init_db
```

## Bước 3.5: Thêm Demo Data (Users và Market Listings) - Khuyến nghị

Để demo dễ dàng hơn, bạn có thể thêm sẵn users và items trên market:

```bash
make add_demo
```

Script này sẽ tạo:
- **8 demo users** với balance khác nhau:
  - `player1` - $500.00
  - `player2` - $750.00
  - `player3` - $1000.00
  - `trader1` - $2000.00
  - `trader2` - $1500.00
  - `richguy` - $5000.00
  - `newbie` - $100.00
  - `pro` - $3000.00
- **Skin instances** cho mỗi user (3-8 skins mỗi user)
- **Market listings** (một số items đã được list sẵn trên market)

**Tất cả users có password:** `123456`

**Lưu ý:** Script này có thể chạy nhiều lần - nó sẽ update balance nếu user đã tồn tại.

---

## Bước 4: Chạy Demo

### Terminal 1: Chạy Server

```bash
make run_server
```

Bạn sẽ thấy:

```
=== CS2 Skin Trading Server ===
Starting on port 8888
✓ Database initialized
✓ Thread pool initialized (8 workers)
✓ Server socket listening on port 8888
Server ready to accept connections
```

**Giữ terminal này mở!**

### Terminal 2: Chạy Client

```bash
make run_client
```

---

## Demo Flow

### 1. Đăng Nhập (Nếu đã chạy `make add_demo`)

Nếu bạn đã chạy `make add_demo`, bạn có thể đăng nhập ngay với các tài khoản demo:

- Username: `player1`, `player2`, `player3`, `trader1`, `trader2`, `richguy`, `newbie`, hoặc `pro`
- Password: `123456`

### 2. Đăng Ký Tài Khoản Mới (Tùy chọn)

Nếu muốn tạo tài khoản mới, chọn `2` để đăng ký:

- Nhập username (tối thiểu 3 ký tự)
- Nhập password (tối thiểu 6 ký tự)
- Bạn sẽ nhận $100.00 balance ban đầu

### 3. Menu Chính

```
[1] Inventory      - Xem skins của bạn
[2] Market         - Mua/bán skins
[3] Trading        - Trading (coming soon)
[4] Unbox Cases    - Mở case để nhận skin
[5] Profile        - Xem thông tin tài khoản
[6] Logout         - Đăng xuất
[7] Exit           - Thoát
```

### 4. Demo: Mở Case (Unbox)

1. Chọn `4` (Unbox Cases)
2. Bạn sẽ thấy danh sách cases có sẵn
3. Chọn case bạn muốn mở (ví dụ: `1`)
4. Xác nhận mở case (Case + Key = $2.50)
5. Hệ thống sẽ hiển thị animation và skin bạn nhận được
6. Skin sẽ được thêm vào inventory

**Lưu ý:** Skin mới unbox sẽ bị trade lock 7 ngày.

### 5. Demo: Xem Market

1. Chọn `2` (Market)
2. Bạn sẽ thấy danh sách items đang được bán trên market
3. Có thể:
   - **Mua item**: Nhập số thứ tự của listing
   - **Tìm kiếm**: Nhập `S` để search theo tên skin
   - **Xóa search**: Nhập `C` để clear search filter
   - **Gỡ listing**: Nhập `R<number>` để remove listing của bạn (nếu có)

**Lưu ý:** Nếu đã chạy `make add_demo`, bạn sẽ thấy một số items đã được list sẵn trên market.

### 6. Demo: Xem Inventory

1. Chọn `1` (Inventory)
2. Bạn sẽ thấy danh sách skins trong inventory với đầy đủ thông tin:
   - Rarity với màu sắc
   - StatTrak™ indicator
   - Skin name
   - Wear condition
   - Pattern seed
   - Price
3. Có thể **bán item lên market**: Nhập số thứ tự của item, sau đó nhập giá
4. Skin mới unbox sẽ có `[Trade Locked]` - không thể trade/bán trong 7 ngày

### 7. Demo: Trading

1. Chọn `3` (Trading)
2. Xem pending trade offers:
   - Incoming trades (từ user khác)
   - Outgoing trades (bạn đã gửi)
3. Có thể:
   - **Accept/Decline** incoming trades
   - **Cancel** outgoing trades
   - **Gửi trade offer mới**: Nhập `N` để search user và gửi trade offer

### 8. Demo: Profile & Search Users

1. Chọn `5` (Profile)
2. Menu options:
   - **Option 1**: Xem profile của bạn (balance, inventory value, total value)
   - **Option 2**: Tìm kiếm user khác bằng username
3. Khi tìm thấy user khác, bạn có thể:
   - Xem profile của họ
   - **Gửi trade offer** cho họ

---

## Troubleshooting

### Server không chạy được

**Lỗi: "Address already in use"**

```bash
# Tìm và kill process đang dùng port 8888
lsof -i :8888
kill -9 <PID>
```

**Lỗi: "setsockopt: Protocol not available"**

```bash
# Build lại
make clean
make all
```

### Client không kết nối được

1. Kiểm tra server đã chạy chưa (Terminal 1)
2. Kiểm tra port có đúng không (mặc định 8888)

### Database lỗi

```bash
# Xóa database cũ và tạo lại
rm -f data/database.db
make init_db
make add_demo  # Thêm demo data
```

---

## Tips

- **Balance ban đầu:** Mỗi user mới có $100.00
- **Case giá:** Mỗi case có giá khác nhau (xem danh sách khi unbox)
- **Rarity rates:**
  - Mil-Spec: ~80%
  - Restricted: ~16%
  - Classified: ~3%
  - Covert: ~0.6%
  - Contraband: ~0.3%
- **Trade lock:** Skin mới unbox bị lock 7 ngày

---

## Xem Database

```bash
sqlite3 data/cs2_trading.db

# Các lệnh hữu ích:
.tables                    # Xem tất cả tables
SELECT * FROM users;       # Xem users
SELECT * FROM cases;       # Xem cases
.quit                      # Thoát
```

---

**Chúc bạn demo vui vẻ! 🎮**
