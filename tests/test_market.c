// test_market.c - Market Engine Tests

#include "../include/market.h"
#include "../include/database.h"
#include "../include/types.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

void test_market_listings()
{
    printf("Testing market listings...\n");

    db_init();

    MarketListing listings[100];
    int count = 0;

    int result = get_market_listings(listings, &count);
    assert(result == 0);
    printf("  Found %d active listings\n", count);

    if (count > 0)
    {
        printf("  First listing: ID=%d, Seller=%d, Instance=%d, Price=$%.2f\n",
               listings[0].listing_id, listings[0].seller_id, listings[0].skin_id, listings[0].price);
    }

    db_close();
    printf("  ✓ Market listings test passed\n\n");
}

void test_list_skin_on_market()
{
    printf("Testing list skin on market...\n");

    db_init();

    // Find an instance owned by a user (from existing data)
    // Query for instances with owner_id > 0
    const char *sql = "SELECT instance_id, owner_id FROM skin_instances WHERE owner_id > 0 LIMIT 1";
    sqlite3 *db_conn;
    sqlite3_open("data/database.db", &db_conn);
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_conn, sql, -1, &stmt, 0);

    int instance_id = 0;
    int owner_id = 0;

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        instance_id = sqlite3_column_int(stmt, 0);
        owner_id = sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db_conn);

    if (instance_id == 0)
    {
        printf("  ⚠ No instances found, skipping test\n\n");
        db_close();
        return;
    }

    float price = 50.0f;

    int result = list_skin_on_market(owner_id, instance_id, price);
    assert(result == 0);
    printf("  ✓ Listed instance %d (owner %d) for $%.2f\n", instance_id, owner_id, price);

    // Verify listing exists
    MarketListing listings[100];
    int count = 0;
    get_market_listings(listings, &count);

    int found = 0;
    for (int i = 0; i < count; i++)
    {
        if (listings[i].skin_id == instance_id && listings[i].seller_id == owner_id)
        {
            found = 1;
            printf("  ✓ Listing verified: ID=%d, Price=$%.2f\n", listings[i].listing_id, listings[i].price);
            break;
        }
    }
    assert(found == 1);

    db_close();
    printf("  ✓ List skin on market test passed\n\n");
}

void test_buy_from_market()
{
    printf("Testing buy from market...\n");

    db_init();

    // Create a new listing with affordable price for testing
    // Find an instance and create a cheap listing
    sqlite3 *db_conn;
    sqlite3_open("data/database.db", &db_conn);
    sqlite3_stmt *stmt;
    const char *sql = "SELECT instance_id, owner_id FROM skin_instances WHERE owner_id > 0 LIMIT 1";
    sqlite3_prepare_v2(db_conn, sql, -1, &stmt, 0);

    int instance_id = 0;
    int seller_id = 0;

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        instance_id = sqlite3_column_int(stmt, 0);
        seller_id = sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db_conn);

    if (instance_id == 0)
    {
        printf("  ⚠ No instances found, skipping test\n\n");
        db_close();
        return;
    }

    // Create a cheap listing
    float listing_price = 50.0f;
    int listing_id;
    db_save_listing_v2(seller_id, instance_id, listing_price, &listing_id);
    printf("  Created test listing: ID=%d, Price=$%.2f\n", listing_id, listing_price);

    // Get or create buyer (user 2)
    User buyer;
    if (db_load_user(2, &buyer) != 0)
    {
        // Create user 2 if doesn't exist
        buyer.user_id = 2;
        strncpy(buyer.username, "buyer", MAX_USERNAME_LEN);
        strncpy(buyer.password_hash, "hash", MAX_PASSWORD_HASH_LEN);
        buyer.balance = 200.0f;
        buyer.created_at = time(NULL);
        buyer.last_login = time(NULL);
        buyer.is_banned = 0;
        db_save_user(&buyer);
    }

    float initial_balance = buyer.balance;
    if (initial_balance < listing_price)
    {
        // Increase buyer balance
        buyer.balance = listing_price + 50.0f;
        db_update_user(&buyer);
        initial_balance = buyer.balance;
    }
    printf("  Buyer initial balance: $%.2f\n", initial_balance);

    User seller;
    db_load_user(seller_id, &seller);
    float seller_initial_balance = seller.balance;
    printf("  Seller initial balance: $%.2f\n", seller_initial_balance);
    printf("  Listing price: $%.2f\n", listing_price);

    // Buy
    int result = buy_from_market(2, listing_id);
    assert(result == 0);
    printf("  ✓ Purchase successful\n");

    // Verify balances
    db_load_user(2, &buyer);
    db_load_user(seller_id, &seller);

    float expected_buyer_balance = initial_balance - listing_price;
    float fee = listing_price * 0.15f;
    float seller_payout = listing_price - fee;
    float expected_seller_balance = seller_initial_balance + seller_payout;

    printf("  Buyer balance after: $%.2f (expected $%.2f)\n", buyer.balance, expected_buyer_balance);
    printf("  Seller balance after: $%.2f (expected $%.2f)\n", seller.balance, expected_seller_balance);
    printf("  Fee: $%.2f (15%%)\n", fee);

    assert(buyer.balance == expected_buyer_balance);
    assert(seller.balance == expected_seller_balance);

    // Verify listing is marked as sold
    int check_seller_id, check_instance_id, check_is_sold;
    float check_price;
    db_get_listing_v2(listing_id, &check_seller_id, &check_instance_id, &check_price, &check_is_sold);
    assert(check_is_sold == 1);
    printf("  ✓ Listing marked as sold\n");

    // Verify instance ownership transferred
    int definition_id, owner_id;
    WearCondition wear;
    time_t acquired_at;
    int is_tradable;
    SkinRarity rarity;
    int pattern_seed, is_stattrak;
    db_load_skin_instance(check_instance_id, &definition_id, &rarity, &wear, &pattern_seed, &is_stattrak, &owner_id, &acquired_at, &is_tradable);
    assert(owner_id == 2);
    printf("  ✓ Instance ownership transferred to buyer\n");

    db_close();
    printf("  ✓ Buy from market test passed\n\n");
}

void test_get_current_price()
{
    printf("Testing get current price...\n");

    db_init();

    // Test price calculation for different wears
    // Get any definition_id and rarity from an instance
    sqlite3 *db_conn;
    sqlite3_open("data/database.db", &db_conn);
    sqlite3_stmt *stmt;
    const char *sql = "SELECT sd.definition_id, sd.base_price, si.rarity FROM skin_definitions sd JOIN skin_instances si ON sd.definition_id = si.definition_id LIMIT 1";
    sqlite3_prepare_v2(db_conn, sql, -1, &stmt, 0);
    
    int definition_id = 0;
    float base_price = 0.0f;
    SkinRarity rarity = RARITY_MIL_SPEC;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        definition_id = sqlite3_column_int(stmt, 0);
        base_price = sqlite3_column_double(stmt, 1);
        rarity = (SkinRarity)sqlite3_column_int(stmt, 2);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db_conn);

    if (definition_id == 0 || base_price <= 0)
    {
        printf("  ⚠ No skin definitions found, skipping test\n\n");
        db_close();
        return;
    }

    // Use float wear values: FN (0.03), MW (0.10), FT (0.25)
    float price_fn = get_current_price(definition_id, rarity, 0.03f); // Factory New
    float price_mw = get_current_price(definition_id, rarity, 0.10f); // Minimal Wear
    float price_ft = get_current_price(definition_id, rarity, 0.25f); // Field-Tested

    printf("  Definition %d (base_price=%.2f, rarity=%d) prices:\n", definition_id, base_price, rarity);
    printf("    Factory New: $%.2f\n", price_fn);
    printf("    Minimal Wear: $%.2f\n", price_mw);
    printf("    Field-Tested: $%.2f\n", price_ft);

    // Verify prices decrease with worse wear (multipliers: FN=1.0, MW=0.92, FT=0.78)
    assert(price_fn > 0);
    assert(price_mw > 0);
    assert(price_ft > 0);
    assert(price_fn >= price_mw); // FN should be >= MW
    assert(price_mw >= price_ft); // MW should be >= FT

    db_close();
    printf("  ✓ Get current price test passed\n\n");
}

int main()
{
    printf("=== Market Engine Tests ===\n\n");

    test_market_listings();
    test_get_current_price();
    test_list_skin_on_market();
    test_buy_from_market();

    printf("=== All Market Tests Passed! ===\n");
    return 0;
}

