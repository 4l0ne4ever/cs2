# CS2 Skin Trading Simulator

Hệ thống giao dịch vật phẩm ảo trực tuyến mô phỏng thị trường trao đổi skins trong game CS2.

## Cấu trúc Project

```
cs2/
├── src/                    # Source code
│   ├── server/            # Server source files
│   ├── client/            # Client source files
│   └── common/            # Shared source files
├── include/               # Header files
├── tests/                 # Test files
├── data/                  # Database files (auto-created)
├── docs/                  # Documentation
└── Makefile               # Build file

```

## Build & Run

```bash
# Build all
make

# Run server
./server 8888

# Run client (separate terminal)
./client localhost 8888
```

## Dependencies

- GCC
- pthread
- OpenSSL (SHA256)
- uuid library

## Features

- ✅ Authentication (Login/Register với SHA256)
- ✅ Market System (Buy/Sell với 15% fee)
- ✅ P2P Trading (Atomic operations)
- ✅ Unboxing (RNG với probabilities)
- ✅ Trade Lock (7 days)
- ✅ Global Chat
- ✅ Terminal UI với ANSI colors
