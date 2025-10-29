# Implementation Plan - CS2 Skin Trading Simulator

## Tổng quan

Độ khó: ⭐⭐⭐⭐ (Khó nhưng khả thi)
Thời gian: 60-80 giờ
Lines of code: ~2000 lines

## Timeline 8 tuần

### Week 1-2: Architecture & Protocol

- [x] Setup project structure
- [ ] Create types.h với data structures
- [ ] Create protocol.h với message types
- [ ] Design database schema
- [ ] Tạo skeleton files

### Week 2: Database

- [ ] Implement database.h/c
- [ ] Implement index.h/c
- [ ] File locking mechanisms
- [ ] Test database operations

### Week 3: Authentication

- [ ] Implement auth.h/c
- [ ] SHA256 hashing
- [ ] UUID generation
- [ ] Session management
- [ ] Test auth flow

### Week 4: Market

- [ ] Implement market.h/c
- [ ] Price dynamics logic
- [ ] Buy/sell operations
- [ ] Fee calculation (15%)
- [ ] Test market operations

### Week 5: Trading & Unbox

- [ ] Implement trading.h/c
- [ ] Atomic trade execution
- [ ] Trade validation
- [ ] Trade lock logic
- [ ] Implement unbox.h/c
- [ ] RNG algorithm
- [ ] Test both systems

### Week 6: Core Systems

- [ ] Implement server.c
- [ ] Implement thread pool
- [ ] Implement request handler
- [ ] Test multi-threading

### Week 7: UI

- [ ] Implement client.c
- [ ] Implement network_client.c
- [ ] Implement ui.h/c
- [ ] ANSI colors & menus
- [ ] Test UI flow

### Week 8: Testing

- [ ] Write unit tests
- [ ] Write integration tests
- [ ] Write load tests
- [ ] Bug fixes
- [ ] Demo preparation

## Files cần implement

### Core (10 files)

- types.h, protocol.h ✅
- auth.h/c
- market.h/c
- trading.h/c
- unbox.h/c
- database.h/c

### Server (3 files)

- server.c
- thread_pool.c
- request_handler.c

### Client (3 files)

- client.c
- network_client.c
- ui.c

### Common (2 files)

- utils.c
- index.c

**Total: 18 source files**
