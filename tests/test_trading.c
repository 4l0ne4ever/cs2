// test_trading.c - Trading System Tests

#include "../include/trading.h"
#include "../include/database.h"
#include "../include/types.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

void test_send_trade_offer()
{
    printf("Testing send trade offer...\n");

    db_init();

    // Get two users
    User user1, user2;
    if (db_load_user(1, &user1) != 0 || db_load_user(2, &user2) != 0)
    {
        printf("  ⚠ Users not found, skipping test\n\n");
        db_close();
        return;
    }

    // Get instance owned by user1 directly from skin_instances (must be tradable)
    int instance_id = 0;
    sqlite3 *db_conn;
    sqlite3_open("data/database.db", &db_conn);
    sqlite3_stmt *stmt;
    const char *sql = "SELECT instance_id FROM skin_instances WHERE owner_id = 1 AND is_tradable = 1 LIMIT 1";
    sqlite3_prepare_v2(db_conn, sql, -1, &stmt, 0);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        instance_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db_conn);

    if (instance_id == 0)
    {
        printf("  ⚠ User 1 has no tradable items, skipping test\n\n");
        db_close();
        return;
    }

    // Ensure instance is in inventory
    db_add_to_inventory(1, instance_id);

    // Create trade offer
    TradeOffer offer;
    offer.from_user_id = 1;
    offer.to_user_id = 2;
    offer.offered_skins[0] = instance_id;
    offer.offered_count = 1;
    offer.offered_cash = 10.0f;
    offer.requested_skins[0] = 0; // No requested items
    offer.requested_count = 0;
    offer.requested_cash = 0.0f;

    int result = send_trade_offer(1, 2, &offer);
    if (result != 0)
    {
        printf("  ✗ Failed to send trade offer: error code %d\n", result);
        printf("    Instance ID: %d\n", instance_id);
        // Check if instance is locked
        int is_locked = is_trade_locked(instance_id);
        printf("    Instance locked: %d\n", is_locked);
        db_close();
        return;
    }
    assert(result == 0);
    printf("  ✓ Trade offer sent: ID=%d\n", offer.trade_id);
    assert(offer.trade_id > 0);

    // Verify trade was saved
    TradeOffer loaded;
    assert(db_load_trade(offer.trade_id, &loaded) == 0);
    assert(loaded.from_user_id == 1);
    assert(loaded.to_user_id == 2);
    assert(loaded.status == TRADE_PENDING);
    printf("  ✓ Trade offer verified in database\n");

    db_close();
    printf("  ✓ Send trade offer test passed\n\n");
}

void test_accept_trade()
{
    printf("Testing accept trade...\n");

    db_init();

    // Get instance owned by user1 that is not locked
    int instance_id = 0;
    sqlite3 *db_conn;
    sqlite3_open("data/database.db", &db_conn);
    sqlite3_stmt *stmt;
    const char *sql = "SELECT instance_id FROM skin_instances WHERE owner_id = 1 AND is_tradable = 1 LIMIT 1";
    sqlite3_prepare_v2(db_conn, sql, -1, &stmt, 0);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        instance_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db_conn);

    if (instance_id == 0)
    {
        printf("  ⚠ User 1 has no tradable items, skipping test\n\n");
        db_close();
        return;
    }

    // Ensure instance is in inventory
    db_add_to_inventory(1, instance_id);

    TradeOffer offer;
    offer.from_user_id = 1;
    offer.to_user_id = 2;
    offer.offered_skins[0] = instance_id;
    offer.offered_count = 1;
    offer.offered_cash = 0.0f;
    offer.requested_skins[0] = 0;
    offer.requested_count = 0;
    offer.requested_cash = 0.0f;

    send_trade_offer(1, 2, &offer);
    int trade_id = offer.trade_id;

    // Accept trade
    int result = accept_trade(2, trade_id);
    assert(result == 0);
    printf("  ✓ Trade accepted\n");

    // Verify trade status
    TradeOffer loaded;
    db_load_trade(trade_id, &loaded);
    assert(loaded.status == TRADE_ACCEPTED);
    printf("  ✓ Trade status updated to ACCEPTED\n");

    // Verify ownership transfer
    int definition_id, owner_id;
    WearCondition wear;
    time_t acquired_at;
    int is_tradable;
    int pattern_seed, is_stattrak;
    SkinRarity rarity;
    db_load_skin_instance(instance_id, &definition_id, &rarity, &wear, &pattern_seed, &is_stattrak, &owner_id, &acquired_at, &is_tradable);
    assert(owner_id == 2);
    printf("  ✓ Instance ownership transferred to user 2\n");

    // Verify trade lock applied
    assert(is_tradable == 0);
    printf("  ✓ Trade lock applied (is_tradable = 0)\n");

    db_close();
    printf("  ✓ Accept trade test passed\n\n");
}

void test_decline_trade()
{
    printf("Testing decline trade...\n");

    db_init();

    // Get instance owned by user1
    int instance_id = 0;
    sqlite3 *db_conn;
    sqlite3_open("data/database.db", &db_conn);
    sqlite3_stmt *stmt;
    const char *sql = "SELECT instance_id FROM skin_instances WHERE owner_id = 1 LIMIT 1";
    sqlite3_prepare_v2(db_conn, sql, -1, &stmt, 0);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        instance_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db_conn);

    if (instance_id == 0)
    {
        printf("  ⚠ User 1 has no items, skipping test\n\n");
        db_close();
        return;
    }

    db_add_to_inventory(1, instance_id);

    TradeOffer offer;
    offer.from_user_id = 1;
    offer.to_user_id = 2;
    offer.offered_skins[0] = instance_id;
    offer.offered_count = 1;
    offer.offered_cash = 0.0f;
    offer.requested_skins[0] = 0;
    offer.requested_count = 0;
    offer.requested_cash = 0.0f;

    send_trade_offer(1, 2, &offer);
    int trade_id = offer.trade_id;

    // Decline trade
    int result = decline_trade(2, trade_id);
    assert(result == 0);
    printf("  ✓ Trade declined\n");

    // Verify trade status
    TradeOffer loaded;
    db_load_trade(trade_id, &loaded);
    assert(loaded.status == TRADE_DECLINED);
    printf("  ✓ Trade status updated to DECLINED\n");

    db_close();
    printf("  ✓ Decline trade test passed\n\n");
}

void test_cancel_trade()
{
    printf("Testing cancel trade...\n");

    db_init();

    // Get instance owned by user1
    int instance_id = 0;
    sqlite3 *db_conn;
    sqlite3_open("data/database.db", &db_conn);
    sqlite3_stmt *stmt;
    const char *sql = "SELECT instance_id FROM skin_instances WHERE owner_id = 1 LIMIT 1";
    sqlite3_prepare_v2(db_conn, sql, -1, &stmt, 0);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        instance_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db_conn);

    if (instance_id == 0)
    {
        printf("  ⚠ User 1 has no items, skipping test\n\n");
        db_close();
        return;
    }

    db_add_to_inventory(1, instance_id);

    TradeOffer offer;
    offer.from_user_id = 1;
    offer.to_user_id = 2;
    offer.offered_skins[0] = instance_id;
    offer.offered_count = 1;
    offer.offered_cash = 0.0f;
    offer.requested_skins[0] = 0;
    offer.requested_count = 0;
    offer.requested_cash = 0.0f;

    send_trade_offer(1, 2, &offer);
    int trade_id = offer.trade_id;

    // Cancel trade
    int result = cancel_trade(1, trade_id);
    assert(result == 0);
    printf("  ✓ Trade cancelled\n");

    // Verify trade status
    TradeOffer loaded;
    db_load_trade(trade_id, &loaded);
    assert(loaded.status == TRADE_CANCELLED);
    printf("  ✓ Trade status updated to CANCELLED\n");

    db_close();
    printf("  ✓ Cancel trade test passed\n\n");
}

void test_trade_lock()
{
    printf("Testing trade lock...\n");

    db_init();

    // Get an instance
    int instance_id = 0;
    sqlite3 *db_conn;
    sqlite3_open("data/database.db", &db_conn);
    sqlite3_stmt *stmt;
    const char *sql = "SELECT instance_id FROM skin_instances WHERE owner_id = 1 LIMIT 1";
    sqlite3_prepare_v2(db_conn, sql, -1, &stmt, 0);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        instance_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db_conn);

    if (instance_id == 0)
    {
        printf("  ⚠ User 1 has no items, skipping test\n\n");
        db_close();
        return;
    }

    // Check initial lock status
    int is_locked = is_trade_locked(instance_id);
    printf("  Initial lock status: %d\n", is_locked);

    // Apply trade lock
    apply_trade_lock(instance_id);
    printf("  ✓ Trade lock applied\n");

    // Check lock status again
    is_locked = is_trade_locked(instance_id);
    assert(is_locked == 1);
    printf("  ✓ Instance is now locked\n");

    db_close();
    printf("  ✓ Trade lock test passed\n\n");
}

int main()
{
    printf("=== Trading System Tests ===\n\n");

    test_send_trade_offer();
    test_decline_trade();
    test_cancel_trade();
    test_trade_lock();
    test_accept_trade();

    printf("=== All Trading Tests Passed! ===\n");
    return 0;
}

