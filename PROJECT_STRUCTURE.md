# Project Structure - CS2 Skin Trading Simulator

```
cs2/
├── README.md                    # Project overview
├── Makefile                      # Build configuration
├── .gitignore                    # Git ignore rules
├── PROJECT_STRUCTURE.md          # This file
│
├── src/                          # Source code
│   ├── server/                   # Server source files
│   │   ├── server.c              # Main server loop
│   │   ├── auth.c                # Authentication
│   │   ├── market.c              # Market engine
│   │   ├── trading.c             # P2P trading
│   │   ├── unbox.c               # Unbox engine
│   │   ├── database.c            # Database operations
│   │   ├── thread_pool.c         # Thread pool
│   │   ├── request_handler.c     # Request routing
│   │   └── index.c               # Hash table indexing
│   │
│   ├── client/                   # Client source files
│   │   ├── client.c              # Main client loop
│   │   ├── ui.c                  # Terminal UI
│   │   └── network_client.c      # Network operations
│   │
│   └── common/                   # Shared source files
│       ├── protocol.c            # Message protocol
│       └── utils.c               # Utility functions
│
├── include/                      # Header files
│   ├── types.h                   # Data structures ✅
│   ├── protocol.h                # Message protocol ✅
│   ├── auth.h                    # Authentication ✅
│   ├── market.h                  # Market engine ✅
│   ├── trading.h                 # P2P trading ✅
│   ├── unbox.h                   # Unbox engine ✅
│   ├── database.h                # Database ✅
│   ├── thread_pool.h             # Thread pool ✅
│   ├── index.h                   # Indexing ✅
│   └── ui.h                      # UI ✅
│
├── tests/                        # Test files
│   ├── test_auth.c
│   ├── test_market.c
│   ├── test_trading.c
│   └── test_integration.c
│
├── data/                         # Database files (auto-created)
│   ├── users.dat
│   ├── skins.dat
│   ├── inventories.dat
│   ├── trades.dat
│   ├── market.dat
│   ├── cases.dat
│   └── logs.dat
│
└── docs/                         # Documentation
    └── implementation_plan.md    # Implementation roadmap ✅
```

## Build & Run

```bash
# Build project
make

# Run server (terminal 1)
make run_server

# Run client (terminal 2)
make run_client localhost 8888

# Clean build artifacts
make clean
```

## Implementation Order

1. ✅ Setup project structure
2. ✅ Create header files
3. 🚧 Implement common utilities (protocol.c, utils.c)
4. 🚧 Implement database layer
5. 🚧 Implement authentication
6. 🚧 Implement market engine
7. 🚧 Implement trading system
8. 🚧 Implement unbox engine
9. 🚧 Implement server core
10. 🚧 Implement client & UI
11. 🚧 Write tests
12. 🚧 Polish & demo

## Key Points

- ✅ All header files created
- 🚧 Implementation in progress
- 📝 Skeleton files ready for development
- 📝 Follow `docs/implementation_plan.md` for detailed roadmap
