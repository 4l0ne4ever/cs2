# 📋 MÔ TẢ ĐỀ TÀI CHI TIẾT

## **ĐỀ TÀI: HỆ THỐNG GIAO DỊCH VẬT PHẨM ẢO TRỰC TUYẾN "CS2 SKIN TRADING SIMULATOR"**

---

## **I. GIỚI THIỆU**

### **1.1. Tổng quan:**

Hệ thống giao dịch vật phẩm ảo trực tuyến mô phỏng thị trường trao đổi skins (vỏ vũ khí) trong game CS2. Đây là ứng dụng mạng đa người chơi, cho phép người dùng mua bán, trao đổi, và sưu tầm các vật phẩm ảo trong một nền kinh tế mô phỏng có cung cầu thực tế.

Ứng dụng được xây dựng theo mô hình Client-Server sử dụng ngôn ngữ C thuần túy, áp dụng kiến thức lập trình mạng socket, xử lý đa luồng, thiết kế giao thức truyền thông, và quản lý cơ sở dữ liệu.

### **1.2. Mục tiêu:**

- Xây dựng hệ thống mạng ổn định, xử lý được nhiều kết nối đồng thời
- Thiết kế giao thức truyền thông hiệu quả cho các thao tác giao dịch
- Quản lý trạng thái người chơi, vật phẩm, và giao dịch một cách đồng bộ
- Tạo trải nghiệm game hấp dẫn với cơ chế kinh tế cân bằng
- Đảm bảo tính toàn vẹn dữ liệu trong môi trường đa luồng

### **1.3. Đối tượng người dùng:**

- Người chơi thông thường: Mua bán, trao đổi vật phẩm, tham gia thị trường
- Trader chuyên nghiệp: Phân tích giá cả, đầu tư, kiếm lời từ chênh lệch giá
- Người mới: Học cách giao dịch, kiếm skins đầu tiên

---

## **II. GAMEPLAY - CÁCH CHƠI**

### **2.1. Khởi đầu:**

Khi người chơi mới tham gia hệ thống:

- Đăng ký tài khoản với username và password (mã hóa SHA256)
- Nhận số dư khởi điểm: **$100**
- Nhận 1 skin miễn phí (Consumer rarity - giá trị thấp)

### **2.2. Các hoạt động chính:**

#### **A. Mua bán trên Market (Thị trường tập trung):**

**Mua từ Market:**

- Duyệt danh sách các skins đang được rao bán
- Xem thông tin chi tiết: tên, độ mòn (wear), giá, xu hướng giá
- Mua ngay với giá niêm yết
- Hệ thống tự động chuyển skin vào inventory, trừ tiền từ tài khoản

**Bán lên Market:**

- Chọn skin từ inventory
- Đặt giá bán tùy ý (có tham khảo giá thị trường)
- Đợi người mua
- Nhận 85% giá bán (15% phí hệ thống)

**Đặc điểm:**

- Giá cả dao động theo cung cầu: nhiều người mua → giá tăng, nhiều người bán → giá giảm
- Có xu hướng giá real-time: ▲ +5.2% (tăng), ▼ -2.1% (giảm), ═ 0.0% (đi ngang)
- Lọc theo rarity, wear, khoảng giá
- Sắp xếp theo giá, popularity, thời gian

#### **B. P2P Trading (Giao dịch trực tiếp):**

**Quy trình:**

1. Người A gửi trade offer cho người B:
   - Đưa ra: 2 skins + $50 cash
   - Yêu cầu: 1 skin từ B
2. Người B nhận thông báo, xem chi tiết offer
3. B có 3 lựa chọn:
   - **Accept**: Chấp nhận → giao dịch thực hiện ngay lập tức
   - **Decline**: Từ chối
   - **Counter-offer**: Đề xuất mức khác
4. Trade offer tự động hết hạn sau **15 phút**
5. Sau khi trade thành công, tất cả items bị **trade lock 7 ngày** (không thể trade tiếp)

**Ưu điểm:**

- Không mất phí (khác với market)
- Linh hoạt: có thể đổi items + cash
- Thương lượng trực tiếp

#### **C. Unboxing Cases (Mở hộp):**

**Cơ chế:**

- Mua cases từ shop: Revolution Case ($8), Chroma Case ($2.50)...
- Mua key: $2.50/key
- Mở case → nhận ngẫu nhiên 1 skin theo xác suất:

| Rarity     | Màu sắc            | Xác suất  | Giá trị trung bình |
| ---------- | ------------------ | --------- | ------------------ |
| Consumer   | Xanh dương nhạt    | 79.92%    | $0.10 - $2         |
| Industrial | Xanh lam           | -         | $2 - $5            |
| Mil-Spec   | Xanh tím           | 15.98%    | $5 - $20           |
| Restricted | Tím hồng           | 3.20%     | $20 - $100         |
| Classified | Hồng               | -         | $100 - $500        |
| Covert     | Đỏ                 | 0.64%     | $500 - $2000       |
| Contraband | Vàng (Knife/Glove) | **0.26%** | $2000 - $10000     |

**Animation:**

- Khi mở case, hiển thị animation "spinning" (các items lướt nhanh)
- Dần dần chậm lại
- Dừng ở item may mắn
- Nếu unbox được Contraband (knife/glove) → thông báo toàn server

**Trade Lock:**

- Skin vừa unbox bị khóa 7 ngày, không thể trade
- Có thể bán lên market hoặc giữ

#### **D. Daily Quests (Nhiệm vụ hàng ngày):**

Hoàn thành để kiếm thêm tiền:

| Quest           | Yêu cầu                      | Phần thưởng       |
| --------------- | ---------------------------- | ----------------- |
| First Steps     | Hoàn thành 3 trades          | +$15              |
| Market Explorer | Mua 5 items từ market        | +$10              |
| Lucky Gambler   | Unbox 5 cases                | +$25 + 1 free key |
| Profit Maker    | Kiếm $50 lợi nhuận           | +$30              |
| Social Trader   | Trade với 10 người khác nhau | +$50              |

#### **E. Trade Lock System (Khóa giao dịch):**

**Cơ chế Trade Lock:**

- Items mới unbox hoặc vừa trade bị khóa 7 ngày (không thể trade lại)
- Trong thời gian khóa, items vẫn có thể:
  - Bán lên market
  - Giữ trong inventory
- Giúp ngăn chặn scam nhanh chóng và tạo thời gian xử lý

**Trade Lock được áp dụng:**

- Khi unbox case (tất cả items mới đều bị lock)
- Khi hoàn thành trade (tất cả items trong trade đều bị lock)
- Hết hạn sau 7 ngày kể từ khi acquire

---

## **III. CƠ CHẾ GAME CHI TIẾT**

### **3.1. Economy System (Hệ thống kinh tế):**

#### **A. Nguồn thu nhập (Money Sources):**

**1. Daily Login Rewards:**

```
Đăng nhập liên tục được thưởng:
Day 1: $5
Day 2: $8
Day 3: $12
Day 4: $15
Day 5: $20
Day 6: $25
Day 7: $50 + 1 free case
```

Nếu bỏ lỡ 1 ngày → streak reset về Day 1

**2. Daily Quests:**
Như mô tả ở phần Gameplay

**3. Trading Profit:**

- Nguồn thu chính: Mua rẻ bán đắt
- Ví dụ: Mua AWP Asiimov giá panic $40 → đợi giá lên → bán $50 → lời $10

**4. Unboxing Luck:**

- Chi phí: $5 - $10 per unbox
- Expected value: ~70% (trung bình mất 30%)
- Nhưng có thể jackpot: unbox knife $5000 từ case $8

**5. Achievements (One-time):**

```
First Trade Completed     → +$20
First Knife Unboxed       → +$500
Total Profit $1,000       → +$100
100 Successful Trades     → +$200
```

#### **B. Chi phí (Expenses):**

**1. Unboxing:**

- Case: $2.50 - $8.00
- Key: $2.50
- Total: $5 - $10.50 per unbox

**2. Market Fees:**

- Khi bán trên market: **15% fee**
- Ví dụ: Bán $100 → nhận $85, hệ thống thu $15

**3. P2P Trade:**

- **Miễn phí** (khuyến khích trade trực tiếp)

**4. Listing Fee:**

- Đăng bán lên market: $0.50 (hoàn lại nếu bán được)

#### **C. Price Dynamics (Biến động giá):**

**Formula:**

```c
current_price = base_price × supply_demand_modifier × hype_factor

supply_demand_modifier:
- Nếu sales > listings → giá +2% đến +5%
- Nếu listings > sales → giá -2% đến -3%

hype_factor:
- New case released chứa skin này → +20%
- Rare unbox (knife) broadcast → +10%
```

**Ví dụ thực tế:**

```
AWP Dragon Lore:
- Base price: $8,000
- Ngày 1: 5 người unbox được → supply tăng → giá xuống $7,200 (-10%)
- Ngày 2: Streamer nổi tiếng unbox được, viral → demand tăng → giá lên $9,000 (+25%)
- Ngày 3: Ổn định → giá $8,500
```

#### **D. Balance Mechanisms (Cân bằng):**

**Money Sink (Hút tiền):**

- Market fees: 15% mỗi giao dịch
- Unboxing: EV = 70% (loss 30%)

**Money Faucet (Bơm tiền):**

- Daily login: ~$150/week
- Daily quests: ~$100/week
- Achievements

**Mục tiêu:**

```
Week 1: $100 → $250 (reasonable growth)
Month 1: $250 → $800
Month 3: $800 → $2000 (có thể mua knife entry-level)
```

### **3.2. Item System (Hệ thống vật phẩm):**

#### **A. Skin Categories:**

**30+ loại skins thuộc các nhóm:**

**1. Knives (Dao):**

- Karambit: Fade, Doppler, Tiger Tooth, Slaughter, Crimson Web
- Butterfly: Fade, Doppler, Marble Fade
- Bayonet: Fade, Doppler, Autotronic
- M9 Bayonet: Fade, Doppler
- Flip Knife: Fade, Doppler

**2. Gloves (Găng tay):**

- Sport Gloves: Pandora's Box, Vice, Superconductor
- Driver Gloves: Crimson Weave, King Snake
- Hand Wraps: Cobalt Skulls, Badlands

**3. Rifles:**

- AK-47: Fire Serpent, Redline, Vulcan, Neon Revolution, Fuel Injector
- M4A4: Howl, Asiimov, Desolate Space, Neo-Noir
- M4A1-S: Hyper Beast, Golden Coil, Decimator

**4. AWP (Sniper):**

- Dragon Lore (legendary)
- Asiimov
- Hyper Beast
- Redline
- Containment Breach

**5. Pistols:**

- Glock: Fade, Water Elemental
- USP-S: Kill Confirmed, Neo-Noir
- Desert Eagle: Blaze, Kumicho Dragon

#### **B. Wear Conditions:**

Mỗi skin có độ mòn, ảnh hưởng giá:

| Wear           | Ký hiệu | Giá % |
| -------------- | ------- | ----- |
| Factory New    | FN      | 100%  |
| Minimal Wear   | MW      | 85%   |
| Field-Tested   | FT      | 65%   |
| Well-Worn      | WW      | 45%   |
| Battle-Scarred | BS      | 30%   |

**Ví dụ:**

- AWP Asiimov (FT): $45
- AWP Asiimov (BS): $28

#### **C. Rarity Tiers:**

Quyết định màu sắc hiển thị và giá trị:

```
Consumer        [Xanh nhạt]  - $0.10 - $2
Industrial      [Xanh lam]   - $2 - $5
Mil-Spec        [Xanh tím]   - $5 - $20
Restricted      [Tím]        - $20 - $100
Classified      [Hồng]       - $100 - $500
Covert          [Đỏ]         - $500 - $2000
Contraband      [Vàng]       - $2000 - $10000
```

### **3.3. Trading Mechanics (Cơ chế giao dịch):**

#### **A. Trade States:**

```
PENDING      → Trade offer đã gửi, chờ response
ACCEPTED     → Đã chấp nhận, items đã chuyển
DECLINED     → Từ chối
CANCELLED    → Người gửi hủy
EXPIRED      → Quá 15 phút không phản hồi
```

#### **B. Trade Validation:**

Trước khi thực hiện trade, system check:

1. ✓ Cả 2 bên đều online
2. ✓ Items vẫn tồn tại trong inventory
3. ✓ Items không bị trade lock
4. ✓ Người yêu cầu cash có đủ tiền
5. ✓ Không có duplicate trade (tránh double-spend)

Nếu fail bất kỳ điều kiện nào → trade cancelled

#### **C. Trade Execution (Atomic Operation):**

**CRITICAL**: Trade phải là atomic (tất cả hoặc không)

```
BEGIN TRANSACTION:
1. Lock database
2. Transfer items A → B
3. Transfer items B → A
4. Transfer cash
5. Apply trade lock (7 days)
6. Update both inventories
7. Log transaction
8. Unlock database
COMMIT

Nếu bất kỳ bước nào fail → ROLLBACK tất cả
```

### **3.4. Market Dynamics (Động lực thị trường):**

#### **A. Supply & Demand:**

**Demand tăng khi:**

- New case released chứa skin
- Streamer/pro player sử dụng
- Skin hiếm (ít listings)
- Event đặc biệt (giảm giá case)

**Supply tăng khi:**

- Nhiều người unbox được
- Panic sell (giá đang giảm)
- Skin cũ (không còn hot)

#### **B. Market Orders:**

**Market Order (Instant Buy/Sell):**

- Mua/bán ngay ở giá hiện tại
- Thực hiện tức thì

#### **C. Price Floors & Ceilings:**

```
Price Floor (giá sàn):
- Không skin nào < $0.10
- Ngăn deflation quá mức

Price Ceiling (giá trần):
- Knife không vượt quá $50,000
- Ngăn inflation không kiểm soát
```

### **3.5. Unbox Probabilities (Xác suất mở hộp):**

#### **A. Probability Distribution:**

```
Contraband (Knife/Glove): 0.26% (1/385 cases)
Covert (Red):             0.64% (1/156 cases)
Classified (Pink):        3.20% (1/31 cases)
Restricted (Purple):     15.98% (1/6 cases)
Mil-Spec (Blue):         79.92% (4/5 cases)
```

#### **B. RNG Algorithm:**

```c
float roll_unbox() {
    float rng = (float)rand() / RAND_MAX; // 0.0 - 1.0

    if (rng < 0.0026) return CONTRABAND;      // 0.26%
    if (rng < 0.0090) return COVERT;          // 0.64%
    if (rng < 0.0410) return CLASSIFIED;      // 3.20%
    if (rng < 0.2008) return RESTRICTED;      // 15.98%
    return MIL_SPEC;                          // 79.92%
}
```

#### **C. Expected Value:**

```
Revolution Case ($8 + $2.50 key = $10.50):

Expected drops:
- 79.92% × $1 =     $0.80
- 15.98% × $10 =    $1.60
- 3.20% × $75 =     $2.40
- 0.64% × $600 =    $3.84
- 0.26% × $5000 =  $13.00

Total EV = $21.64

Nhưng thực tế: 99.74% cases mất tiền, chỉ 0.26% lời lớn
```

---

## **IV. TÍNH NĂNG ĐẶC BIỆT**

### **4.1. Live Price Tracking:**

Mỗi skin có "price chart" đơn giản:

```
AWP Dragon Lore (FN) - Last 24 hours:

$8,500 ┤            ╭─╮
$8,000 ┤        ╭───╯ ╰╮
$7,500 ┤    ╭───╯      ╰─╮
$7,000 ┤────╯            ╰───
       └─────────────────────
       0h  6h  12h  18h  24h

Current: $8,200 ▲ +2.5% (24h)
```

### **4.2. Leaderboards:**

**Top Traders (by Net Worth):**

```
1. pro_trader     $125,000  ↑ +$5,000 (this week)
2. skin_king      $98,500   ↑ +$2,300
3. lucky_guy      $87,000   ↓ -$1,200
...
```

**Luckiest Unboxers:**

```
1. blessed_one    Unboxed: Karambit Fade $12,000
2. rng_god        Unboxed: Gloves Pandora $8,500
3. gambler123     Unboxed: Butterfly Doppler $7,200
```

**Most Profitable:**

```
1. flipper_pro    Total profit: +$45,000
2. market_genius  Total profit: +$32,000
3. whale_trader   Total profit: +$28,000
```

### **4.3. Global Chat:**

```
[GLOBAL CHAT]
───────────────────────────────────
[18:45:32] pro_trader: WTB Karambit Fade < $11k
[18:45:45] newbie123: How to make money fast?
[18:45:50] market_god: @newbie123 learn to flip
[18:46:01] 🎉 RARE DROP: lucky_guy unboxed AWP Dragon Lore!
[18:46:10] everyone: GG! / Congrats! / wow!
[18:46:25] scammer99: Hey bro check this link: ste4m-tr4de.com
[18:46:30] [SYSTEM] scammer99 has been reported
───────────────────────────────────
Type message: _
```

### **4.4. Trading Challenges:**

Người chơi có thể thách đấu nhau:

**1v1 Profit Race:**

```
Challenge: You vs pro_trader
Duration: 10 minutes
Goal: Who makes more profit from trading

Timer: 07:23 remaining

Your profit:  +$125
Their profit: +$180

You are losing! Trade smarter!
```

### **4.5. Trade History & Analytics:**

Mỗi user có thể xem:

```
YOUR TRADE HISTORY:

Last 7 days:
- Trades completed: 23
- Items bought: 15 (avg $32)
- Items sold: 18 (avg $38)
- Net profit: +$108
- Best trade: Sold Butterfly +$45
- Worst trade: Bought Gloves -$12
- Win rate: 65% (profitable trades)

Graph:
Balance ↑
$500 ┤              ╭───
$400 ┤          ╭───╯
$300 ┤      ╭───╯
$200 ┤  ╭───╯
$100 ┤──╯
     └──────────────────
     Mon Tue Wed Thu Fri
```

---
