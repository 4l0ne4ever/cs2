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

### 1. Đăng Ký Tài Khoản

Khi client khởi động, bạn sẽ thấy menu:

```
[1] Login
[2] Register
```

Chọn `2` để đăng ký:

- Nhập username (ví dụ: `player1`)
- Nhập password (ví dụ: `123456`)
- Bạn sẽ nhận $100.00 balance ban đầu

### 2. Đăng Nhập

Sau khi đăng ký, chọn `1` để đăng nhập:

- Nhập username và password vừa tạo
- Sau khi đăng nhập thành công, bạn sẽ thấy menu chính

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

### 5. Demo: Xem Inventory

1. Chọn `1` (Inventory)
2. Bạn sẽ thấy tất cả skins trong inventory
3. Mỗi skin hiển thị:
   - Tên skin
   - Rarity (màu sắc theo CS2)
   - Float value
   - Pattern seed
   - StatTrak (nếu có)
   - Trade lock status

### 6. Demo: Market

**List Skin để bán:**

1. Chọn `2` (Market)
2. Chọn `1` (List Skin)
3. Chọn skin từ inventory
4. Nhập giá bạn muốn bán
5. Skin sẽ xuất hiện trên market

**Mua Skin:**

1. Chọn `2` (Market)
2. Chọn `2` (Browse Market)
3. Xem danh sách skins đang bán
4. Chọn skin và mua
5. Skin sẽ được chuyển vào inventory của bạn

### 7. Demo: Profile

1. Chọn `5` (Profile)
2. Xem thông tin:
   - Username
   - Balance
   - Số lượng skins trong inventory

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
rm -f data/cs2_trading.db
make init_db
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
