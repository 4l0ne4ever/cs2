// database_sqlite.c - Database Operations with SQLite

#include "../include/database.h"
#include "../include/types.h"
#include "../include/price_tracking.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static sqlite3 *db = NULL;

// Forward declaration
static void db_populate_cases_and_skins();

// Populate database with real CS2 cases and skins
static void db_populate_cases_and_skins()
{
    if (!db)
    {
        fprintf(stderr, "db_populate_cases_and_skins: db is NULL\n");
        return;
    }

    char *err_msg = 0;

    // Insert cases with real CS2 prices (USD)
    const char *cases_sql =
        "INSERT OR IGNORE INTO cases (case_id, name, price, possible_skins, probabilities, skin_count) VALUES "
        "(1, 'Revolution Case', 8.0, '[]', '[]', 0), "        // ~$8 USD
        "(2, 'Chroma Case', 4.5, '[]', '[]', 0), "            // ~$4.47 USD (updated from research)
        "(3, 'Operation Phoenix Case', 5.0, '[]', '[]', 0), " // ~$5 USD
        "(4, 'Chroma 2 Case', 3.0, '[]', '[]', 0), "          // ~$3 USD
        "(5, 'Falchion Case', 4.0, '[]', '[]', 0);";          // ~$4 USD
    sqlite3_exec(db, cases_sql, 0, 0, &err_msg);
    if (err_msg)
    {
        sqlite3_free(err_msg);
        err_msg = 0;
    }

    // Insert skin definitions with real market base prices (for Covert rarity, will be multiplied by rarity)
    // Prices based on real CS2 market values (Covert rarity Factory New)
    const char *skins_sql =
        "INSERT OR IGNORE INTO skin_definitions (definition_id, name, base_price, rarity) VALUES "
        // Revolution Case skins (newer case, moderate prices) - All Covert (RARITY_COVERT = 5)
        "(1, 'AK-47 | Slate', 45.0, 5), "              // ~$45 Covert FN
        "(2, 'AWP | PAW', 60.0, 5), "                  // ~$60 Covert FN
        "(3, 'M4A4 | Spider Lily', 50.0, 5), "         // ~$50 Covert FN
        "(4, 'Glock-18 | Vogue', 35.0, 5), "           // ~$35 Covert FN
        "(5, 'USP-S | Printstream', 80.0, 5), "        // ~$80 Covert FN (popular)
        "(6, 'Desert Eagle | Printstream', 70.0, 5), " // ~$70 Covert FN
        "(7, 'AWP | Duality', 55.0, 5), "              // ~$55 Covert FN
        "(8, 'AK-47 | Nightwish', 48.0, 5), "          // ~$48 Covert FN
        "(9, 'M4A1-S | Blue Phosphor', 52.0, 5), "     // ~$52 Covert FN
        "(10, 'AWP | Gungnir', 120.0, 5), "            // ~$120 Covert FN (rare)
        // Chroma Case skins (classic, higher prices) - All Covert
        "(11, 'AK-47 | Redline', 180.0, 5), "           // ~$180 Covert FN (iconic)
        "(12, 'AWP | Redline', 200.0, 5), "             // ~$200 Covert FN
        "(13, 'M4A4 | Dragon King', 65.0, 5), "         // ~$65 Covert FN
        "(14, 'Glock-18 | Water Elemental', 40.0, 5), " // ~$40 Covert FN
        "(15, 'USP-S | Guardian', 45.0, 5), "           // ~$45 Covert FN
        "(16, 'Desert Eagle | Conspiracy', 55.0, 5), "  // ~$55 Covert FN
        "(17, 'AWP | Man-o''-war', 70.0, 5), "          // ~$70 Covert FN
        "(18, 'AK-47 | Vulcan', 220.0, 5), "            // ~$220 Covert FN (very popular)
        "(19, 'M4A1-S | Hyper Beast', 150.0, 5), "      // ~$150 Covert FN (popular)
        "(20, 'AWP | Asiimov', 300.0, 5), "             // ~$300 Covert FN (iconic, expensive)
        // Operation Phoenix Case skins (legendary, very expensive) - All Covert
        "(21, 'AK-47 | Case Hardened', 250.0, 5), "      // ~$250 Covert FN (Phoenix exclusive)
        "(22, 'AWP | Lightning Strike', 280.0, 5), "     // ~$280 Covert FN (Phoenix exclusive)
        "(23, 'M4A4 | X-Ray', 85.0, 5), "                // ~$85 Covert FN
        "(24, 'Glock-18 | Steel Disruption', 50.0, 5), " // ~$50 Covert FN
        "(25, 'USP-S | Caiman', 60.0, 5), "              // ~$60 Covert FN
        "(26, 'Desert Eagle | Hypnotic', 75.0, 5), "     // ~$75 Covert FN
        "(27, 'AWP | Sun in Leo', 90.0, 5), "            // ~$90 Covert FN
        "(28, 'AK-47 | Fire Serpent', 800.0, 5), "       // ~$800 Covert FN (legendary, very expensive)
        "(29, 'M4A4 | Howl', 1200.0, 5), "               // ~$1200 Covert FN (extremely rare)
        "(30, 'AWP | Dragon Lore', 15000.0, 5), "        // ~$15000 Covert FN (most expensive)
        // Chroma 2 Case skins - All Covert
        "(31, 'AK-47 | Jaguar', 140.0, 5), "             // ~$140 Covert FN
        "(32, 'AWP | Hyper Beast', 150.0, 5), "          // ~$150 Covert FN
        "(33, 'M4A4 | Bullet Rain', 70.0, 5), "          // ~$70 Covert FN
        "(34, 'Glock-18 | Grinder', 45.0, 5), "          // ~$45 Covert FN
        "(35, 'USP-S | Kill Confirmed', 100.0, 5), "     // ~$100 Covert FN (popular)
        "(36, 'Desert Eagle | Sunset Storm', 65.0, 5), " // ~$65 Covert FN
        "(37, 'AWP | Fever Dream', 150.0, 5), "          // ~$150 Covert FN (Chroma 2 exclusive)
        "(38, 'AK-47 | Wasteland Rebel', 160.0, 5), "    // ~$160 Covert FN
        "(39, 'M4A1-S | Master Piece', 200.0, 5), "      // ~$200 Covert FN
        "(40, 'AWP | Medusa', 5000.0, 5), "              // ~$5000 Covert FN (very expensive)
        // Falchion Case skins - All Covert
        "(41, 'AK-47 | Aquamarine Revenge', 130.0, 5), "   // ~$130 Covert FN
        "(42, 'AWP | Pit Viper', 150.0, 5), "              // ~$150 Covert FN (Falchion exclusive)
        "(43, 'M4A4 | Royal Paladin', 75.0, 5), "          // ~$75 Covert FN
        "(44, 'Glock-18 | Twilight Galaxy', 50.0, 5), "    // ~$50 Covert FN
        "(45, 'USP-S | Neo-Noir', 100.0, 5), "             // ~$100 Covert FN (Falchion exclusive)
        "(46, 'Desert Eagle | Kumicho Dragon', 85.0, 5), " // ~$85 Covert FN
        "(47, 'AWP | Phobos', 150.0, 5), "                 // ~$150 Covert FN (Falchion exclusive)
        "(48, 'AK-47 | Fuel Injector', 110.0, 5), "        // ~$110 Covert FN
        "(49, 'M4A1-S | Icarus Fell', 120.0, 5), "         // ~$120 Covert FN
        "(50, 'AWP | Oni Taiji', 180.0, 5), "              // ~$180 Covert FN
        // Knives (Contraband/Gold rarity - 0.26% drop rate) - RARITY_CONTRABAND = 6
        "(51, 'Karambit | Fade', 2000.0, 6), "           // ~$2000 Contraband FN (most popular)
        "(52, 'Karambit | Doppler', 1500.0, 6), "        // ~$1500 Contraband FN
        "(53, 'Karambit | Tiger Tooth', 1200.0, 6), "    // ~$1200 Contraband FN
        "(54, 'Butterfly Knife | Fade', 1800.0, 6), "    // ~$1800 Contraband FN (popular)
        "(55, 'Butterfly Knife | Doppler', 1400.0, 6), " // ~$1400 Contraband FN
        "(56, 'M9 Bayonet | Fade', 1600.0, 6), "         // ~$1600 Contraband FN
        "(57, 'M9 Bayonet | Doppler', 1300.0, 6), "      // ~$1300 Contraband FN
        "(58, 'Bayonet | Fade', 1000.0, 6), "            // ~$1000 Contraband FN
        "(59, 'Bayonet | Doppler', 900.0, 6), "          // ~$900 Contraband FN
        "(60, 'Talon Knife | Fade', 1100.0, 6), "        // ~$1100 Contraband FN
        "(61, 'Talon Knife | Doppler', 950.0, 6), "      // ~$950 Contraband FN
        "(62, 'Huntsman Knife | Fade', 800.0, 6), "      // ~$800 Contraband FN
        "(63, 'Huntsman Knife | Doppler', 700.0, 6), "   // ~$700 Contraband FN
        "(64, 'Falchion Knife | Fade', 750.0, 6), "      // ~$750 Contraband FN
        "(65, 'Falchion Knife | Doppler', 650.0, 6), "   // ~$650 Contraband FN
        "(66, 'Gut Knife | Fade', 600.0, 6), "           // ~$600 Contraband FN
        "(67, 'Gut Knife | Doppler', 500.0, 6), "        // ~$500 Contraband FN
        "(68, 'Shadow Daggers | Fade', 550.0, 6), "      // ~$550 Contraband FN
        "(69, 'Shadow Daggers | Doppler', 450.0, 6), "   // ~$450 Contraband FN
        "(70, 'Bowie Knife | Fade', 700.0, 6), "         // ~$700 Contraband FN
        // Gloves (Contraband/Gold rarity - 0.26% drop rate) - RARITY_CONTRABAND = 6
        "(71, 'Specialist Gloves | Fade', 1200.0, 6), "          // ~$1200 Contraband FN (popular)
        "(72, 'Specialist Gloves | Crimson Kimono', 800.0, 6), " // ~$800 Contraband FN
        "(73, 'Hand Wraps | Slaughter', 600.0, 6), "             // ~$600 Contraband FN
        "(74, 'Hand Wraps | Cobalt Skulls', 550.0, 6), "         // ~$550 Contraband FN
        "(75, 'Sport Gloves | Pandora''s Box', 1500.0, 6), "     // ~$1500 Contraband FN (expensive)
        "(76, 'Sport Gloves | Vice', 900.0, 6), "                // ~$900 Contraband FN
        "(77, 'Driver Gloves | King Snake', 700.0, 6), "         // ~$700 Contraband FN
        "(78, 'Driver Gloves | Crimson Weave', 750.0, 6), "      // ~$750 Contraband FN
        "(79, 'Moto Gloves | Spearmint', 850.0, 6), "            // ~$850 Contraband FN
        "(80, 'Moto Gloves | Cool Mint', 800.0, 6), "            // ~$800 Contraband FN
        // Additional skins with different rarities for testing (Mil-Spec, Restricted, Classified)
        // Mil-Spec (RARITY_MIL_SPEC = 2) - 79.92% drop rate
        "(81, 'AK-47 | Blue Laminate', 15.0, 2), " // ~$15 Mil-Spec FN
        "(82, 'AWP | Worm God', 20.0, 2), "        // ~$20 Mil-Spec FN
        "(83, 'M4A4 | Desert-Strike', 12.0, 2), "  // ~$12 Mil-Spec FN
        "(84, 'Glock-18 | Blue Steel', 8.0, 2), "  // ~$8 Mil-Spec FN
        "(85, 'USP-S | Forest Leaves', 10.0, 2), " // ~$10 Mil-Spec FN
        // Restricted (RARITY_RESTRICTED = 3) - 15.98% drop rate
        "(86, 'AK-47 | Emerald Pinstripe', 35.0, 3), "     // ~$35 Restricted FN
        "(87, 'AWP | Corticera', 40.0, 3), "               // ~$40 Restricted FN
        "(88, 'M4A4 | X-Ray FT', 30.0, 3), "               // ~$30 Restricted FN (renamed to avoid duplicate)
        "(89, 'Glock-18 | Water Elemental FT', 25.0, 3), " // ~$25 Restricted FN (renamed to avoid duplicate)
        "(90, 'USP-S | Guardian FT', 28.0, 3), "           // ~$28 Restricted FN (renamed to avoid duplicate)
        // Classified (RARITY_CLASSIFIED = 4) - 3.2% drop rate
        "(91, 'AK-47 | Jaguar CL', 80.0, 4), "         // ~$80 Classified FN (renamed to avoid duplicate)
        "(92, 'AWP | Hyper Beast CL', 90.0, 4), "      // ~$90 Classified FN (renamed to avoid duplicate)
        "(93, 'M4A4 | Bullet Rain CL', 70.0, 4), "     // ~$70 Classified FN (renamed to avoid duplicate)
        "(94, 'Glock-18 | Grinder CL', 50.0, 4), "     // ~$50 Classified FN (renamed to avoid duplicate)
        "(95, 'USP-S | Kill Confirmed CL', 60.0, 4);"; // ~$60 Classified FN (renamed to avoid duplicate)
    sqlite3_exec(db, skins_sql, 0, 0, &err_msg);
    if (err_msg)
    {
        sqlite3_free(err_msg);
        err_msg = 0;
    }

    // Link skins to cases (Revolution Case)
    // Includes skins from all rarities: Mil-Spec, Restricted, Classified, Covert, Contraband
    const char *case1_sql =
        "INSERT OR IGNORE INTO case_skins (case_id, definition_id) VALUES "
        // Mil-Spec (2)
        "(1, 81), (1, 82), (1, 83), (1, 84), (1, 85), "
        // Restricted (3)
        "(1, 86), (1, 87), (1, 88), (1, 89), (1, 90), "
        // Classified (4)
        "(1, 91), (1, 92), (1, 93), (1, 94), (1, 95), "
        // Covert (5) - Original weapons
        "(1, 1), (1, 2), (1, 3), (1, 4), (1, 5), (1, 6), (1, 7), (1, 8), (1, 9), (1, 10), "
        // Contraband (6) - Knives and Gloves
        "(1, 51), (1, 54), " // Karambit Fade, Butterfly Fade
        "(1, 71), (1, 75);"; // Specialist Fade, Sport Pandora
    int rc1 = sqlite3_exec(db, case1_sql, 0, 0, &err_msg);
    if (err_msg)
    {
        fprintf(stderr, "case1_sql error: %s\n", err_msg);
        sqlite3_free(err_msg);
        err_msg = 0;
    }
    if (rc1 != SQLITE_OK)
    {
        fprintf(stderr, "case1_sql exec failed: %d\n", rc1);
    }

    // Chroma Case - Real CS2 case with all rarities
    // Mil-Spec, Restricted, Classified, Covert, Contraband
    const char *case2_sql =
        "INSERT OR IGNORE INTO case_skins (case_id, definition_id) VALUES "
        // Mil-Spec (2) - 79.92% drop rate
        "(2, 81), (2, 82), (2, 83), (2, 84), (2, 85), "
        // Restricted (3) - 15.98% drop rate
        "(2, 86), (2, 87), (2, 88), (2, 89), (2, 90), "
        // Classified (4) - 3.2% drop rate
        "(2, 91), (2, 92), (2, 93), (2, 94), (2, 95), "
        // Covert (5) - 0.64% drop rate
        "(2, 11), (2, 12), (2, 13), (2, 14), (2, 15), (2, 16), (2, 17), (2, 18), (2, 19), (2, 20), "
        // Contraband (6) - 0.26% drop rate
        "(2, 52), (2, 56), " // Karambit Doppler, M9 Bayonet Fade
        "(2, 72), (2, 76);"; // Specialist Crimson Kimono, Sport Vice
    sqlite3_exec(db, case2_sql, 0, 0, &err_msg);
    if (err_msg)
    {
        sqlite3_free(err_msg);
        err_msg = 0;
    }

    // Operation Phoenix Case - Real CS2 case with all rarities
    // Mil-Spec, Restricted, Classified, Covert, Contraband
    const char *case3_sql =
        "INSERT OR IGNORE INTO case_skins (case_id, definition_id) VALUES "
        // Mil-Spec (2) - 79.92% drop rate
        "(3, 81), (3, 82), (3, 83), (3, 84), (3, 85), "
        // Restricted (3) - 15.98% drop rate
        "(3, 86), (3, 87), (3, 88), (3, 89), (3, 90), "
        // Classified (4) - 3.2% drop rate
        "(3, 91), (3, 92), (3, 93), (3, 94), (3, 95), "
        // Covert (5) - 0.64% drop rate
        "(3, 21), (3, 22), (3, 23), (3, 24), (3, 25), (3, 26), (3, 27), (3, 28), (3, 29), (3, 30), "
        // Note: definition_id 30 exists (AWP | Dragon Lore)
        // Contraband (6) - 0.26% drop rate
        "(3, 55), (3, 59), " // Karambit Doppler, Bayonet Fade
        "(3, 62), (3, 65);"; // Huntsman Fade, Falchion Doppler
    sqlite3_exec(db, case3_sql, 0, 0, &err_msg);
    if (err_msg)
    {
        sqlite3_free(err_msg);
        err_msg = 0;
    }

    // Chroma 2 Case - Real CS2 case with all rarities
    // Mil-Spec, Restricted, Classified, Covert, Contraband
    const char *case4_sql =
        "INSERT OR IGNORE INTO case_skins (case_id, definition_id) VALUES "
        // Mil-Spec (2) - 79.92% drop rate
        "(4, 81), (4, 82), (4, 83), (4, 84), (4, 85), "
        // Restricted (3) - 15.98% drop rate
        "(4, 86), (4, 87), (4, 88), (4, 89), (4, 90), "
        // Classified (4) - 3.2% drop rate
        "(4, 91), (4, 92), (4, 93), (4, 94), (4, 95), "
        // Covert (5) - 0.64% drop rate
        "(4, 31), (4, 32), (4, 33), (4, 34), (4, 35), (4, 36), (4, 38), (4, 39), (4, 40), "
        // Note: definition_id 37 (AWP | Hyper Beast) is duplicate name, use 32 instead
        // Contraband (6) - 0.26% drop rate
        "(4, 58), (4, 60), " // Bayonet Fade, Talon Fade
        "(4, 74), (4, 78);"; // Hand Wraps Cobalt Skulls, Driver Crimson Weave
    sqlite3_exec(db, case4_sql, 0, 0, &err_msg);
    if (err_msg)
    {
        sqlite3_free(err_msg);
        err_msg = 0;
    }

    // Falchion Case - Real CS2 case with all rarities
    // Mil-Spec, Restricted, Classified, Covert, Contraband
    const char *case5_sql =
        "INSERT OR IGNORE INTO case_skins (case_id, definition_id) VALUES "
        // Mil-Spec (2) - 79.92% drop rate
        "(5, 81), (5, 82), (5, 83), (5, 84), (5, 85), "
        // Restricted (3) - 15.98% drop rate
        "(5, 86), (5, 87), (5, 88), (5, 89), (5, 90), "
        // Classified (4) - 3.2% drop rate
        "(5, 91), (5, 92), (5, 93), (5, 94), (5, 95), "
        // Covert (5) - 0.64% drop rate
        "(5, 41), (5, 43), (5, 44), (5, 45), (5, 46), (5, 48), (5, 49), (5, 50), "
        // Note: definition_id 42 (AWP | Hyper Beast) and 47 (AWP | Hyper Beast) are duplicates, use others
        // Contraband (6) - 0.26% drop rate
        "(5, 61), (5, 63), " // Talon Doppler, Huntsman Doppler
        "(5, 67), (5, 68);"; // Gut Doppler, Shadow Daggers Fade
    sqlite3_exec(db, case5_sql, 0, 0, &err_msg);
    if (err_msg)
    {
        sqlite3_free(err_msg);
        err_msg = 0;
    }
}

// Get database connection (for internal use)
sqlite3 *db_get_connection()
{
    return db;
}

// Initialize database
int db_init()
{
    char *err_msg = 0;

    // Create data directory if it doesn't exist
    system("mkdir -p data");

    int rc = sqlite3_open("data/database.db", &db);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    // Enable foreign key constraints
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", 0, 0, 0);

    // Create schema if tables don't exist
    const char *schema_sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "user_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT UNIQUE NOT NULL, "
        "password_hash TEXT NOT NULL, "
        "balance REAL NOT NULL DEFAULT 100.0, "
        "created_at INTEGER NOT NULL, "
        "last_login INTEGER, "
        "is_banned INTEGER NOT NULL DEFAULT 0"
        ");"
        "CREATE TABLE IF NOT EXISTS skins ("
        "skin_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "rarity INTEGER NOT NULL, "
        "wear REAL NOT NULL, "
        "pattern_seed INTEGER NOT NULL DEFAULT 0, "
        "is_stattrak INTEGER NOT NULL DEFAULT 0, "
        "base_price REAL NOT NULL, "
        "current_price REAL NOT NULL, "
        "owner_id INTEGER, "
        "acquired_at INTEGER, "
        "is_tradable INTEGER NOT NULL DEFAULT 1, "
        "FOREIGN KEY (owner_id) REFERENCES users(user_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS inventories ("
        "inventory_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id INTEGER NOT NULL, "
        "instance_id INTEGER NOT NULL, "
        "FOREIGN KEY (user_id) REFERENCES users(user_id), "
        "FOREIGN KEY (instance_id) REFERENCES skin_instances(instance_id), "
        "UNIQUE(user_id, instance_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS trades ("
        "trade_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "from_user_id INTEGER NOT NULL, "
        "to_user_id INTEGER NOT NULL, "
        "offered_skins TEXT NOT NULL, "
        "offered_count INTEGER NOT NULL, "
        "offered_cash REAL NOT NULL, "
        "requested_skins TEXT NOT NULL, "
        "requested_count INTEGER NOT NULL, "
        "requested_cash REAL NOT NULL, "
        "status INTEGER NOT NULL, "
        "created_at INTEGER NOT NULL, "
        "expires_at INTEGER NOT NULL, "
        "FOREIGN KEY (from_user_id) REFERENCES users(user_id), "
        "FOREIGN KEY (to_user_id) REFERENCES users(user_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS market_listings ("
        "listing_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "seller_id INTEGER NOT NULL, "
        "skin_id INTEGER NOT NULL, "
        "price REAL NOT NULL, "
        "listed_at INTEGER NOT NULL, "
        "is_sold INTEGER NOT NULL DEFAULT 0, "
        "FOREIGN KEY (seller_id) REFERENCES users(user_id), "
        "FOREIGN KEY (skin_id) REFERENCES skins(skin_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS cases ("
        "case_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "price REAL NOT NULL, "
        "possible_skins TEXT NOT NULL, "
        "probabilities TEXT NOT NULL, "
        "skin_count INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS sessions ("
        "session_token TEXT PRIMARY KEY, "
        "user_id INTEGER NOT NULL, "
        "socket_fd INTEGER, "
        "login_time INTEGER NOT NULL, "
        "last_activity INTEGER NOT NULL, "
        "is_active INTEGER NOT NULL DEFAULT 1, "
        "FOREIGN KEY (user_id) REFERENCES users(user_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS transaction_logs ("
        "log_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "type INTEGER NOT NULL, "
        "user_id INTEGER, "
        "details TEXT, "
        "timestamp INTEGER NOT NULL, "
        "FOREIGN KEY (user_id) REFERENCES users(user_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS reports ("
        "report_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "reporter_id INTEGER NOT NULL, "
        "reported_id INTEGER NOT NULL, "
        "reason TEXT NOT NULL, "
        "created_at INTEGER NOT NULL, "
        "is_resolved INTEGER NOT NULL DEFAULT 0, "
        "FOREIGN KEY (reporter_id) REFERENCES users(user_id), "
        "FOREIGN KEY (reported_id) REFERENCES users(user_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS wear_multipliers ("
        "wear_min REAL PRIMARY KEY, "
        "wear_max REAL NOT NULL, "
        "multiplier REAL NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS rarity_multipliers ("
        "rarity INTEGER PRIMARY KEY, "
        "multiplier REAL NOT NULL, "
        "drop_rate REAL NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS skin_definitions ("
        "definition_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL UNIQUE, "
        "base_price REAL NOT NULL, "
        "rarity INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS case_skins ("
        "case_id INTEGER NOT NULL, "
        "definition_id INTEGER NOT NULL, "
        "FOREIGN KEY (case_id) REFERENCES cases(case_id), "
        "FOREIGN KEY (definition_id) REFERENCES skin_definitions(definition_id), "
        "PRIMARY KEY (case_id, definition_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS skin_instances ("
        "instance_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "definition_id INTEGER NOT NULL, "
        "rarity INTEGER NOT NULL, "
        "wear REAL NOT NULL, "
        "pattern_seed INTEGER NOT NULL DEFAULT 0, "
        "is_stattrak INTEGER NOT NULL DEFAULT 0, "
        "owner_id INTEGER, "
        "acquired_at INTEGER, "
        "is_tradable INTEGER NOT NULL DEFAULT 1, "
        "FOREIGN KEY (definition_id) REFERENCES skin_definitions(definition_id), "
        "FOREIGN KEY (owner_id) REFERENCES users(user_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS market_listings_v2 ("
        "listing_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "seller_id INTEGER NOT NULL, "
        "instance_id INTEGER NOT NULL, "
        "price REAL NOT NULL, "
        "listed_at INTEGER NOT NULL, "
        "is_sold INTEGER NOT NULL DEFAULT 0, "
        "FOREIGN KEY (seller_id) REFERENCES users(user_id), "
        "FOREIGN KEY (instance_id) REFERENCES skin_instances(instance_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS quests ("
        "quest_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id INTEGER NOT NULL, "
        "quest_type INTEGER NOT NULL, "
        "progress INTEGER NOT NULL DEFAULT 0, "
        "target INTEGER NOT NULL, "
        "is_completed INTEGER NOT NULL DEFAULT 0, "
        "is_claimed INTEGER NOT NULL DEFAULT 0, "
        "started_at INTEGER NOT NULL, "
        "completed_at INTEGER, "
        "FOREIGN KEY (user_id) REFERENCES users(user_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS achievements ("
        "achievement_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id INTEGER NOT NULL, "
        "achievement_type INTEGER NOT NULL, "
        "is_unlocked INTEGER NOT NULL DEFAULT 0, "
        "is_claimed INTEGER NOT NULL DEFAULT 0, "
        "unlocked_at INTEGER, "
        "FOREIGN KEY (user_id) REFERENCES users(user_id), "
        "UNIQUE(user_id, achievement_type)"
        ");"
        "CREATE TABLE IF NOT EXISTS login_streaks ("
        "user_id INTEGER PRIMARY KEY, "
        "current_streak INTEGER NOT NULL DEFAULT 0, "
        "last_login_date INTEGER NOT NULL, "
        "last_reward_date INTEGER, "
        "FOREIGN KEY (user_id) REFERENCES users(user_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS chat_messages ("
        "message_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id INTEGER NOT NULL, "
        "username TEXT NOT NULL, "
        "message TEXT NOT NULL, "
        "timestamp INTEGER NOT NULL, "
        "FOREIGN KEY (user_id) REFERENCES users(user_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS price_history ("
        "history_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "definition_id INTEGER NOT NULL, "
        "price REAL NOT NULL, "
        "transaction_type INTEGER NOT NULL, "
        "timestamp INTEGER NOT NULL, "
        "FOREIGN KEY (definition_id) REFERENCES skin_definitions(definition_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS trading_challenges ("
        "challenge_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "challenger_id INTEGER NOT NULL, "
        "opponent_id INTEGER NOT NULL, "
        "type INTEGER NOT NULL, "
        "status INTEGER NOT NULL, "
        "challenger_start_balance REAL NOT NULL, "
        "opponent_start_balance REAL NOT NULL, "
        "challenger_current_profit REAL NOT NULL DEFAULT 0.0, "
        "opponent_current_profit REAL NOT NULL DEFAULT 0.0, "
        "start_time INTEGER, "
        "end_time INTEGER, "
        "duration_minutes INTEGER NOT NULL, "
        "challenger_cancel_vote INTEGER NOT NULL DEFAULT 0, "
        "opponent_cancel_vote INTEGER NOT NULL DEFAULT 0, "
        "FOREIGN KEY (challenger_id) REFERENCES users(user_id), "
        "FOREIGN KEY (opponent_id) REFERENCES users(user_id)"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_price_history_definition ON price_history(definition_id, timestamp);"
        "CREATE INDEX IF NOT EXISTS idx_price_history_timestamp ON price_history(timestamp);"
        "CREATE INDEX IF NOT EXISTS idx_challenges_challenger ON trading_challenges(challenger_id);"
        "CREATE INDEX IF NOT EXISTS idx_challenges_opponent ON trading_challenges(opponent_id);"
        "CREATE INDEX IF NOT EXISTS idx_challenges_status ON trading_challenges(status);"
        "CREATE INDEX IF NOT EXISTS idx_skins_owner ON skins(owner_id);"
        "CREATE INDEX IF NOT EXISTS idx_inventories_user ON inventories(user_id);"
        "CREATE INDEX IF NOT EXISTS idx_inventories_instance ON inventories(instance_id);"
        "CREATE INDEX IF NOT EXISTS idx_trades_from ON trades(from_user_id);"
        "CREATE INDEX IF NOT EXISTS idx_trades_to ON trades(to_user_id);"
        "CREATE INDEX IF NOT EXISTS idx_trades_status ON trades(status);"
        "CREATE INDEX IF NOT EXISTS idx_trades_expires ON trades(expires_at);"
        "CREATE INDEX IF NOT EXISTS idx_trades_user_status ON trades(from_user_id, to_user_id, status);"
        "CREATE INDEX IF NOT EXISTS idx_market_active ON market_listings(is_sold);"
        "CREATE INDEX IF NOT EXISTS idx_market_seller ON market_listings(seller_id, is_sold);"
        "CREATE INDEX IF NOT EXISTS idx_sessions_token ON sessions(session_token);"
        "CREATE INDEX IF NOT EXISTS idx_sessions_user ON sessions(user_id, is_active);"
        "CREATE INDEX IF NOT EXISTS idx_instances_owner ON skin_instances(owner_id);"
        "CREATE INDEX IF NOT EXISTS idx_instances_definition ON skin_instances(definition_id);"
        "CREATE INDEX IF NOT EXISTS idx_instances_tradable ON skin_instances(is_tradable, acquired_at);"
        "CREATE INDEX IF NOT EXISTS idx_market_v2_active ON market_listings_v2(is_sold);"
        "CREATE INDEX IF NOT EXISTS idx_market_v2_seller ON market_listings_v2(seller_id, is_sold);"
        "CREATE INDEX IF NOT EXISTS idx_market_v2_instance ON market_listings_v2(instance_id);"
        "CREATE INDEX IF NOT EXISTS idx_case_skins_case ON case_skins(case_id);"
        "CREATE INDEX IF NOT EXISTS idx_reports_reported ON reports(reported_id, is_resolved);"
        "CREATE INDEX IF NOT EXISTS idx_reports_reporter ON reports(reporter_id);"
        "CREATE INDEX IF NOT EXISTS idx_case_skins_definition ON case_skins(definition_id);"
        "CREATE INDEX IF NOT EXISTS idx_transaction_logs_user ON transaction_logs(user_id, timestamp);"
        "CREATE INDEX IF NOT EXISTS idx_skin_definitions_name ON skin_definitions(name);"
        "CREATE INDEX IF NOT EXISTS idx_quests_user ON quests(user_id, is_completed, is_claimed);"
        "CREATE INDEX IF NOT EXISTS idx_achievements_user ON achievements(user_id, is_unlocked, is_claimed);"
        "CREATE INDEX IF NOT EXISTS idx_chat_messages_timestamp ON chat_messages(timestamp);"
        "CREATE INDEX IF NOT EXISTS idx_price_history_definition ON price_history(definition_id, timestamp);"
        "CREATE INDEX IF NOT EXISTS idx_price_history_timestamp ON price_history(timestamp);";

    rc = sqlite3_exec(db, schema_sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Schema creation error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }
    
    // Migration: Add cancel vote columns if they don't exist
    // SQLite doesn't support multiple ALTER TABLE in one statement, so do them separately
    char *migration_err = NULL;
    // Try to add challenger_cancel_vote column (will fail silently if it already exists)
    sqlite3_exec(db, "ALTER TABLE trading_challenges ADD COLUMN challenger_cancel_vote INTEGER NOT NULL DEFAULT 0", 0, 0, &migration_err);
    if (migration_err)
    {
        // Column might already exist, ignore error
        sqlite3_free(migration_err);
        migration_err = NULL;
    }
    // Try to add opponent_cancel_vote column (will fail silently if it already exists)
    sqlite3_exec(db, "ALTER TABLE trading_challenges ADD COLUMN opponent_cancel_vote INTEGER NOT NULL DEFAULT 0", 0, 0, &migration_err);
    if (migration_err)
    {
        // Column might already exist, ignore error
        sqlite3_free(migration_err);
    }

    // Insert initial data if tables are empty
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM wear_multipliers", -1, &stmt, 0);
    int should_populate = 0;
    if (rc == SQLITE_OK)
    {
        if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 0)
        {
            should_populate = 1;
        }
        sqlite3_finalize(stmt);
    }

    if (should_populate)
    {
        const char *init_sql =
            "INSERT OR IGNORE INTO wear_multipliers (wear_min, wear_max, multiplier) VALUES "
            "(0.00, 0.07, 1.00), " // Factory New (FN)
            "(0.07, 0.15, 0.92), " // Minimal Wear (MW)
            "(0.15, 0.37, 0.78), " // Field-Tested (FT) - fixed: 0.37 not 0.38
            "(0.37, 0.45, 0.65), " // Well-Worn (WW)
            "(0.45, 1.00, 0.52); " // Battle-Scarred (BS)
            "INSERT OR IGNORE INTO rarity_multipliers (rarity, multiplier, drop_rate) VALUES "
            "(0, 0.1, 0.0), "   // Consumer - không drop từ case
            "(1, 0.15, 0.0), "  // Industrial - không drop từ case
            "(2, 0.3, 79.92), " // Mil-Spec (Purple) - 79.92% - giá thấp nhất
            "(3, 0.5, 15.98), " // Restricted (Pink) - 15.98% - giá trung bình
            "(4, 0.75, 3.2), "  // Classified (Pink) - 3.2% - giá cao
            "(5, 1.0, 0.64), "  // Covert (Red) - 0.64% - giá rất cao (base price)
            "(6, 1.5, 0.26);";  // Contraband (Gold) - 0.26% - giá cao nhất
        sqlite3_exec(db, init_sql, 0, 0, &err_msg);
        if (err_msg)
        {
            fprintf(stderr, "Init data error: %s\n", err_msg);
            sqlite3_free(err_msg);
        }

        // Populate with real CS2 cases and skins
        db_populate_cases_and_skins();
    }

    // Always check if case_skins needs population (even if wear_multipliers is already populated)
    // Check each case individually to ensure all cases are populated
    for (int case_id = 1; case_id <= 5; case_id++)
    {
        char check_sql[128];
        snprintf(check_sql, sizeof(check_sql), "SELECT COUNT(*) FROM case_skins WHERE case_id = %d", case_id);
        rc = sqlite3_prepare_v2(db, check_sql, -1, &stmt, 0);
        if (rc == SQLITE_OK)
        {
            if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 0)
            {
                sqlite3_finalize(stmt);
                // Populate all cases if any case is missing
                db_populate_cases_and_skins();
                break; // Only need to populate once
            }
            sqlite3_finalize(stmt);
        }
    }

    return 0;
}

// Cleanup database connection
void db_close()
{
    if (db)
    {
        sqlite3_close(db);
    }
}

// ==================== USER OPERATIONS ====================

int db_save_user(User *user)
{
    if (!user)
        return -1;

    if (!db)
        return -1;

    // If user_id is 0, let SQLite AUTOINCREMENT assign it
    // Otherwise, use the provided user_id
    const char *sql;
    if (user->user_id == 0)
    {
        sql = "INSERT INTO users (username, password_hash, balance, created_at, is_banned) "
              "VALUES (?, ?, ?, ?, ?)";
    }
    else
    {
        sql = "INSERT INTO users (user_id, username, password_hash, balance, created_at, is_banned) "
              "VALUES (?, ?, ?, ?, ?, ?)";
    }

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    int bind_idx = 1;
    if (user->user_id != 0)
    {
        sqlite3_bind_int(stmt, bind_idx++, user->user_id);
    }
    sqlite3_bind_text(stmt, bind_idx++, user->username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, bind_idx++, user->password_hash, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, bind_idx++, user->balance);
    sqlite3_bind_int64(stmt, bind_idx++, user->created_at);
    sqlite3_bind_int(stmt, bind_idx++, user->is_banned);

    rc = sqlite3_step(stmt);

    // If user_id was 0, get the assigned ID from SQLite
    if (user->user_id == 0 && rc == SQLITE_DONE)
    {
        user->user_id = (int)sqlite3_last_insert_rowid(db);
    }

    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_load_user(int user_id, User *out_user)
{
    if (!out_user)
        return -1;

    const char *sql = "SELECT * FROM users WHERE user_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, user_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        out_user->user_id = sqlite3_column_int(stmt, 0);
        strncpy(out_user->username, (char *)sqlite3_column_text(stmt, 1), MAX_USERNAME_LEN);
        strncpy(out_user->password_hash, (char *)sqlite3_column_text(stmt, 2), MAX_PASSWORD_HASH_LEN);
        out_user->balance = sqlite3_column_double(stmt, 3);
        out_user->created_at = sqlite3_column_int64(stmt, 4);
        out_user->last_login = sqlite3_column_int64(stmt, 5);
        out_user->is_banned = sqlite3_column_int(stmt, 6);

        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_load_user_by_username(const char *username, User *out_user)
{
    if (!username || !out_user)
        return -1;

    if (!db)
        return -1;

    const char *sql = "SELECT * FROM users WHERE username = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        out_user->user_id = sqlite3_column_int(stmt, 0);
        strncpy(out_user->username, (char *)sqlite3_column_text(stmt, 1), MAX_USERNAME_LEN);
        strncpy(out_user->password_hash, (char *)sqlite3_column_text(stmt, 2), MAX_PASSWORD_HASH_LEN);
        out_user->balance = sqlite3_column_double(stmt, 3);
        out_user->created_at = sqlite3_column_int64(stmt, 4);
        out_user->last_login = sqlite3_column_int64(stmt, 5);
        out_user->is_banned = sqlite3_column_int(stmt, 6);

        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_update_user(User *user)
{
    if (!user)
        return -1;

    if (!db)
        return -1;

    const char *sql = "UPDATE users SET username = ?, password_hash = ?, balance = ?, "
                      "created_at = ?, last_login = ?, is_banned = ? WHERE user_id = ?";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_text(stmt, 1, user->username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user->password_hash, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 3, user->balance);
    sqlite3_bind_int64(stmt, 4, user->created_at);
    sqlite3_bind_int64(stmt, 5, user->last_login);
    sqlite3_bind_int(stmt, 6, user->is_banned);
    sqlite3_bind_int(stmt, 7, user->user_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_user_exists(const char *username)
{
    const char *sql = "SELECT COUNT(*) FROM users WHERE username = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return 0;

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return count > 0;
    }

    sqlite3_finalize(stmt);
    return 0;
}

// ==================== SKIN OPERATIONS ====================

int db_save_skin(Skin *skin)
{
    if (!skin)
        return -1;

    const char *sql = "INSERT INTO skins (skin_id, name, rarity, wear, pattern_seed, is_stattrak, base_price, current_price, owner_id, acquired_at, is_tradable) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, skin->skin_id);
    sqlite3_bind_text(stmt, 2, skin->name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, skin->rarity);
    sqlite3_bind_double(stmt, 4, skin->wear); // REAL column
    sqlite3_bind_int(stmt, 5, skin->pattern_seed);
    sqlite3_bind_int(stmt, 6, skin->is_stattrak);
    sqlite3_bind_double(stmt, 7, skin->base_price);
    sqlite3_bind_double(stmt, 8, skin->current_price);
    sqlite3_bind_int(stmt, 9, skin->owner_id);
    sqlite3_bind_int64(stmt, 10, skin->acquired_at);
    sqlite3_bind_int(stmt, 11, skin->is_tradable);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_load_skin(int skin_id, Skin *out_skin)
{
    if (!out_skin)
        return -1;

    const char *sql = "SELECT * FROM skins WHERE skin_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, skin_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        out_skin->skin_id = sqlite3_column_int(stmt, 0);
        const char *skin_name = (char *)sqlite3_column_text(stmt, 1);
        if (skin_name) {
            size_t name_len = strlen(skin_name);
            size_t copy_len = (name_len < MAX_ITEM_NAME_LEN - 1) ? name_len : MAX_ITEM_NAME_LEN - 1;
            memcpy(out_skin->name, skin_name, copy_len);
            out_skin->name[copy_len] = '\0';
        } else {
            out_skin->name[0] = '\0';
        }
        out_skin->rarity = sqlite3_column_int(stmt, 2);
        out_skin->wear = (WearCondition)sqlite3_column_double(stmt, 3); // REAL column
        out_skin->pattern_seed = sqlite3_column_int(stmt, 4);
        out_skin->is_stattrak = sqlite3_column_int(stmt, 5);
        out_skin->base_price = sqlite3_column_double(stmt, 6);
        out_skin->current_price = sqlite3_column_double(stmt, 7);
        out_skin->owner_id = sqlite3_column_int(stmt, 8);
        out_skin->acquired_at = sqlite3_column_int64(stmt, 9);
        out_skin->is_tradable = sqlite3_column_int(stmt, 10);

        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_update_skin(Skin *skin)
{
    if (!skin)
        return -1;

    const char *sql = "UPDATE skins SET name = ?, rarity = ?, wear = ?, pattern_seed = ?, is_stattrak = ?, base_price = ?, "
                      "current_price = ?, owner_id = ?, acquired_at = ?, is_tradable = ? WHERE skin_id = ?";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_text(stmt, 1, skin->name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, skin->rarity);
    sqlite3_bind_double(stmt, 3, skin->wear); // REAL column
    sqlite3_bind_int(stmt, 4, skin->pattern_seed);
    sqlite3_bind_int(stmt, 5, skin->is_stattrak);
    sqlite3_bind_double(stmt, 6, skin->base_price);
    sqlite3_bind_double(stmt, 7, skin->current_price);
    sqlite3_bind_int(stmt, 8, skin->owner_id);
    sqlite3_bind_int64(stmt, 9, skin->acquired_at);
    sqlite3_bind_int(stmt, 10, skin->is_tradable);
    sqlite3_bind_int(stmt, 11, skin->skin_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ==================== INVENTORY OPERATIONS ====================

int db_load_inventory(int user_id, Inventory *out_inv)
{
    if (!out_inv)
        return -1;

    if (!db)
        return -1;

    out_inv->user_id = user_id;
    out_inv->count = 0;
    memset(out_inv->skin_ids, 0, sizeof(out_inv->skin_ids));

    // Load instance_id from inventories table
    const char *sql = "SELECT instance_id FROM inventories WHERE user_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return 0; // Empty inventory

    sqlite3_bind_int(stmt, 1, user_id);

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < MAX_INVENTORY_SIZE)
    {
        out_inv->skin_ids[count++] = sqlite3_column_int(stmt, 0); // Store instance_id
    }

    out_inv->count = count;
    sqlite3_finalize(stmt);
    return 0;
}

int db_add_to_inventory(int user_id, int skin_id)
{
    if (!db)
        return -1;

    // Note: skin_id parameter is actually instance_id in the new system
    const char *sql = "INSERT OR IGNORE INTO inventories (user_id, instance_id) VALUES (?, ?)";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, skin_id); // Actually instance_id

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_remove_from_inventory(int user_id, int skin_id)
{
    if (!db)
        return -1;

    // Note: skin_id parameter is actually instance_id in the new system
    const char *sql = "DELETE FROM inventories WHERE user_id = ? AND instance_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, skin_id); // Actually instance_id

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ==================== TRADE OPERATIONS ====================

int db_save_trade(TradeOffer *trade)
{
    if (!trade)
        return -1;

    if (!db)
        return -1;

    // Serialize arrays to JSON strings
    char offered_str[512] = {0};
    char requested_str[512] = {0};

    strcat(offered_str, "[");
    for (int i = 0; i < trade->offered_count; i++)
    {
        if (i > 0)
            strcat(offered_str, ",");
        char num[16];
        sprintf(num, "%d", trade->offered_skins[i]);
        strcat(offered_str, num);
    }
    strcat(offered_str, "]");

    strcat(requested_str, "[");
    for (int i = 0; i < trade->requested_count; i++)
    {
        if (i > 0)
            strcat(requested_str, ",");
        char num[16];
        sprintf(num, "%d", trade->requested_skins[i]);
        strcat(requested_str, num);
    }
    strcat(requested_str, "]");

    const char *sql = "INSERT INTO trades (from_user_id, to_user_id, offered_skins, offered_count, "
                      "offered_cash, requested_skins, requested_count, requested_cash, status, created_at, expires_at) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, trade->from_user_id);
    sqlite3_bind_int(stmt, 2, trade->to_user_id);
    sqlite3_bind_text(stmt, 3, offered_str, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, trade->offered_count);
    sqlite3_bind_double(stmt, 5, trade->offered_cash);
    sqlite3_bind_text(stmt, 6, requested_str, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, trade->requested_count);
    sqlite3_bind_double(stmt, 8, trade->requested_cash);
    sqlite3_bind_int(stmt, 9, trade->status);
    sqlite3_bind_int64(stmt, 10, trade->created_at);
    sqlite3_bind_int64(stmt, 11, trade->expires_at);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE && trade->trade_id == 0)
    {
        trade->trade_id = (int)sqlite3_last_insert_rowid(db);
    }
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_load_trade(int trade_id, TradeOffer *out_trade)
{
    if (!out_trade)
        return -1;

    if (!db)
        return -1;

    const char *sql = "SELECT * FROM trades WHERE trade_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, trade_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        // Column order from SELECT * FROM trades:
        // 0: trade_id, 1: from_user_id, 2: to_user_id, 3: offered_skins, 4: offered_count,
        // 5: offered_cash, 6: requested_skins, 7: requested_count, 8: requested_cash,
        // 9: status, 10: created_at, 11: expires_at
        out_trade->trade_id = sqlite3_column_int(stmt, 0);
        out_trade->from_user_id = sqlite3_column_int(stmt, 1);
        out_trade->to_user_id = sqlite3_column_int(stmt, 2);

        const char *offered_str = (char *)sqlite3_column_text(stmt, 3);
        out_trade->offered_count = sqlite3_column_int(stmt, 4);
        out_trade->offered_cash = sqlite3_column_double(stmt, 5);
        const char *requested_str = (char *)sqlite3_column_text(stmt, 6);
        out_trade->requested_count = sqlite3_column_int(stmt, 7);
        out_trade->requested_cash = sqlite3_column_double(stmt, 8);
        out_trade->status = (TradeStatus)sqlite3_column_int(stmt, 9);
        out_trade->created_at = sqlite3_column_int64(stmt, 10);
        out_trade->expires_at = sqlite3_column_int64(stmt, 11);

        // Initialize arrays
        memset(out_trade->offered_skins, 0, sizeof(out_trade->offered_skins));
        memset(out_trade->requested_skins, 0, sizeof(out_trade->requested_skins));

        // Parse JSON arrays
        if (offered_str && offered_str[0] == '[')
        {
            int idx = 0;
            const char *p = offered_str + 1;
            while (*p && *p != ']' && idx < 10)
            {
                if (*p >= '0' && *p <= '9')
                {
                    out_trade->offered_skins[idx++] = atoi(p);
                    while (*p >= '0' && *p <= '9')
                        p++;
                }
                else
                    p++;
            }
            // Ensure count matches parsed items
            if (idx != out_trade->offered_count)
            {
                out_trade->offered_count = idx;
            }
        }
        else
        {
            // If no valid JSON, reset count
            out_trade->offered_count = 0;
        }

        if (requested_str && requested_str[0] == '[')
        {
            int idx = 0;
            const char *p = requested_str + 1;
            while (*p && *p != ']' && idx < 10)
            {
                if (*p >= '0' && *p <= '9')
                {
                    out_trade->requested_skins[idx++] = atoi(p);
                    while (*p >= '0' && *p <= '9')
                        p++;
                }
                else
                    p++;
            }
            // Ensure count matches parsed items
            if (idx != out_trade->requested_count)
            {
                out_trade->requested_count = idx;
            }
        }
        else
        {
            // If no valid JSON, reset count
            out_trade->requested_count = 0;
        }

        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_update_trade(TradeOffer *trade)
{
    if (!trade)
        return -1;

    // Serialize arrays to JSON strings
    char offered_str[512] = {0};
    char requested_str[512] = {0};

    strcat(offered_str, "[");
    for (int i = 0; i < trade->offered_count; i++)
    {
        if (i > 0)
            strcat(offered_str, ",");
        char num[16];
        sprintf(num, "%d", trade->offered_skins[i]);
        strcat(offered_str, num);
    }
    strcat(offered_str, "]");

    strcat(requested_str, "[");
    for (int i = 0; i < trade->requested_count; i++)
    {
        if (i > 0)
            strcat(requested_str, ",");
        char num[16];
        sprintf(num, "%d", trade->requested_skins[i]);
        strcat(requested_str, num);
    }
    strcat(requested_str, "]");

    const char *sql = "UPDATE trades SET from_user_id = ?, to_user_id = ?, offered_skins = ?, offered_count = ?, "
                      "offered_cash = ?, requested_skins = ?, requested_count = ?, requested_cash = ?, "
                      "status = ?, created_at = ?, expires_at = ? WHERE trade_id = ?";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, trade->from_user_id);
    sqlite3_bind_int(stmt, 2, trade->to_user_id);
    sqlite3_bind_text(stmt, 3, offered_str, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, trade->offered_count);
    sqlite3_bind_double(stmt, 5, trade->offered_cash);
    sqlite3_bind_text(stmt, 6, requested_str, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, trade->requested_count);
    sqlite3_bind_double(stmt, 8, trade->requested_cash);
    sqlite3_bind_int(stmt, 9, trade->status);
    sqlite3_bind_int64(stmt, 10, trade->created_at);
    sqlite3_bind_int64(stmt, 11, trade->expires_at);
    sqlite3_bind_int(stmt, 12, trade->trade_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_get_user_trades(int user_id, TradeOffer *out_trades, int *count)
{
    if (!out_trades || !count)
        return -1;

    // Get ALL trades (pending, accepted, declined, cancelled, expired) for the user
    // Order by: pending first, then by created_at DESC
    const char *sql = "SELECT * FROM trades WHERE (from_user_id = ? OR to_user_id = ?) ORDER BY CASE WHEN status = 0 THEN 0 ELSE 1 END, created_at DESC";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        *count = 0;
        return 0;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, user_id);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && found < 50)
    {
        TradeOffer *trade = &out_trades[found];
        // Column order from SELECT * FROM trades:
        // 0: trade_id, 1: from_user_id, 2: to_user_id, 3: offered_skins, 4: offered_count,
        // 5: offered_cash, 6: requested_skins, 7: requested_count, 8: requested_cash,
        // 9: status, 10: created_at, 11: expires_at
        trade->trade_id = sqlite3_column_int(stmt, 0);
        trade->from_user_id = sqlite3_column_int(stmt, 1);
        trade->to_user_id = sqlite3_column_int(stmt, 2);

        const char *offered_str = (char *)sqlite3_column_text(stmt, 3);
        trade->offered_count = sqlite3_column_int(stmt, 4);
        trade->offered_cash = sqlite3_column_double(stmt, 5);
        const char *requested_str = (char *)sqlite3_column_text(stmt, 6);
        trade->requested_count = sqlite3_column_int(stmt, 7);
        trade->requested_cash = sqlite3_column_double(stmt, 8);
        trade->status = (TradeStatus)sqlite3_column_int(stmt, 9);
        trade->created_at = sqlite3_column_int64(stmt, 10);
        trade->expires_at = sqlite3_column_int64(stmt, 11);

        // Initialize arrays
        memset(trade->offered_skins, 0, sizeof(trade->offered_skins));
        memset(trade->requested_skins, 0, sizeof(trade->requested_skins));
        
        // Parse JSON arrays
        if (offered_str && offered_str[0] == '[')
        {
            int idx = 0;
            const char *p = offered_str + 1;
            while (*p && *p != ']' && idx < 10)
            {
                if (*p >= '0' && *p <= '9')
                {
                    trade->offered_skins[idx++] = atoi(p);
                    while (*p >= '0' && *p <= '9')
                        p++;
                }
                else
                    p++;
            }
            // Ensure count matches parsed items
            if (idx != trade->offered_count)
            {
                trade->offered_count = idx;
            }
        }
        else
        {
            // If no valid JSON, reset count
            trade->offered_count = 0;
        }

        if (requested_str && requested_str[0] == '[')
        {
            int idx = 0;
            const char *p = requested_str + 1;
            while (*p && *p != ']' && idx < 10)
            {
                if (*p >= '0' && *p <= '9')
                {
                    trade->requested_skins[idx++] = atoi(p);
                    while (*p >= '0' && *p <= '9')
                        p++;
                }
                else
                    p++;
            }
            // Ensure count matches parsed items
            if (idx != trade->requested_count)
            {
                trade->requested_count = idx;
            }
        }
        else
        {
            // If no valid JSON, reset count
            trade->requested_count = 0;
        }

        found++;
    }

    *count = found;
    sqlite3_finalize(stmt);
    return 0;
}

// ==================== MARKET OPERATIONS ====================

int db_save_listing(MarketListing *listing)
{
    if (!listing)
        return -1;

    const char *sql = "INSERT INTO market_listings (listing_id, seller_id, skin_id, price, listed_at, is_sold) "
                      "VALUES (?, ?, ?, ?, ?, ?)";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, listing->listing_id);
    sqlite3_bind_int(stmt, 2, listing->seller_id);
    sqlite3_bind_int(stmt, 3, listing->skin_id);
    sqlite3_bind_double(stmt, 4, listing->price);
    sqlite3_bind_int64(stmt, 5, listing->listed_at);
    sqlite3_bind_int(stmt, 6, listing->is_sold);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_load_listings(MarketListing *out_listings, int *count)
{
    if (!out_listings || !count)
        return -1;

    const char *sql = "SELECT * FROM market_listings WHERE is_sold = 0";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        *count = 0;
        return 0;
    }

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && found < 100)
    {
        MarketListing *listing = &out_listings[found];
        listing->listing_id = sqlite3_column_int(stmt, 0);
        listing->seller_id = sqlite3_column_int(stmt, 1);
        listing->skin_id = sqlite3_column_int(stmt, 2);
        listing->price = sqlite3_column_double(stmt, 3);
        listing->listed_at = sqlite3_column_int64(stmt, 4);
        listing->is_sold = sqlite3_column_int(stmt, 5);
        found++;
    }

    *count = found;
    sqlite3_finalize(stmt);
    return 0;
}

int db_update_listing(MarketListing *listing)
{
    if (!listing)
        return -1;

    const char *sql = "UPDATE market_listings SET seller_id = ?, skin_id = ?, price = ?, listed_at = ?, is_sold = ? WHERE listing_id = ?";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, listing->seller_id);
    sqlite3_bind_int(stmt, 2, listing->skin_id);
    sqlite3_bind_double(stmt, 3, listing->price);
    sqlite3_bind_int64(stmt, 4, listing->listed_at);
    sqlite3_bind_int(stmt, 5, listing->is_sold);
    sqlite3_bind_int(stmt, 6, listing->listing_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ==================== TRANSACTION LOG ====================

int db_log_transaction(TransactionLog *log)
{
    if (!log)
        return -1;

    if (!db)
    {
        // Can't use logger here as it might cause circular dependency
        fprintf(stderr, "db_log_transaction: database connection is NULL\n");
        return -1;
    }

    // log_id is AUTOINCREMENT, so we should use NULL or omit it
    const char *sql = "INSERT INTO transaction_logs (type, user_id, details, timestamp) "
                      "VALUES (?, ?, ?, ?)";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "db_log_transaction: sqlite3_prepare_v2 failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // Bind parameters (log_id is auto-generated, so skip it)
    sqlite3_bind_int(stmt, 1, log->type);
    sqlite3_bind_int(stmt, 2, log->user_id);
    sqlite3_bind_text(stmt, 3, log->details, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, log->timestamp);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "db_log_transaction: sqlite3_step failed: %s (rc=%d)\n", sqlite3_errmsg(db), rc);
        sqlite3_finalize(stmt);
        return -1;
    }
    
    sqlite3_finalize(stmt);
    return 0;
}

// ==================== SESSION OPERATIONS ====================

int db_save_session(Session *session)
{
    if (!session)
        return -1;

    const char *sql = "INSERT INTO sessions (session_token, user_id, socket_fd, login_time, last_activity, is_active) "
                      "VALUES (?, ?, ?, ?, ?, ?)";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_text(stmt, 1, session->session_token, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, session->user_id);
    sqlite3_bind_int(stmt, 3, session->socket_fd);
    sqlite3_bind_int64(stmt, 4, session->login_time);
    sqlite3_bind_int64(stmt, 5, session->last_activity);
    sqlite3_bind_int(stmt, 6, session->is_active);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_load_session(const char *token, Session *out_session)
{
    if (!token || !out_session)
        return -1;

    const char *sql = "SELECT * FROM sessions WHERE session_token = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_text(stmt, 1, token, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        strncpy(out_session->session_token, (char *)sqlite3_column_text(stmt, 0), 36);
        out_session->user_id = sqlite3_column_int(stmt, 1);
        out_session->socket_fd = sqlite3_column_int(stmt, 2);
        out_session->login_time = sqlite3_column_int64(stmt, 3);
        out_session->last_activity = sqlite3_column_int64(stmt, 4);
        out_session->is_active = sqlite3_column_int(stmt, 5);

        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_delete_session(const char *token)
{
    if (!token)
        return -1;

    const char *sql = "DELETE FROM sessions WHERE session_token = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_text(stmt, 1, token, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ==================== SKIN DEFINITION & INSTANCE OPERATIONS ====================

int db_load_skin_definition(int definition_id, char *name, float *base_price)
{
    if (!name || !base_price)
        return -1;

    const char *sql = "SELECT name, base_price FROM skin_definitions WHERE definition_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, definition_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *def_name_src = (char *)sqlite3_column_text(stmt, 0);
        if (def_name_src) {
            size_t name_len = strlen(def_name_src);
            size_t copy_len = (name_len < MAX_ITEM_NAME_LEN - 1) ? name_len : MAX_ITEM_NAME_LEN - 1;
            memcpy(name, def_name_src, copy_len);
            name[copy_len] = '\0';
        } else {
            name[0] = '\0';
        }
        *base_price = (float)sqlite3_column_double(stmt, 1);
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

// Load skin definition with rarity
int db_load_skin_definition_with_rarity(int definition_id, char *name, float *base_price, SkinRarity *rarity)
{
    if (!name || !base_price || !rarity)
        return -1;

    if (!db)
        return -1;

    const char *sql = "SELECT name, base_price, rarity FROM skin_definitions WHERE definition_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, definition_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        strncpy(name, (char *)sqlite3_column_text(stmt, 0), MAX_ITEM_NAME_LEN - 1);
        name[MAX_ITEM_NAME_LEN - 1] = '\0'; // Ensure null terminator
        *base_price = (float)sqlite3_column_double(stmt, 1);
        *rarity = (SkinRarity)sqlite3_column_int(stmt, 2);
        
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

// Get case skins filtered by rarity
int db_get_case_skins_by_rarity(int case_id, SkinRarity rarity, int *definition_ids, int *count)
{
    if (!definition_ids || !count)
        return -1;

    if (!db)
        return -1;

    const char *sql = "SELECT DISTINCT cs.definition_id FROM case_skins cs "
                      "INNER JOIN skin_definitions sd ON cs.definition_id = sd.definition_id "
                      "WHERE cs.case_id = ? AND sd.rarity = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        *count = 0;
        return -1;
    }

    sqlite3_bind_int(stmt, 1, case_id);
    sqlite3_bind_int(stmt, 2, (int)rarity);

    int idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < 100)
    {
        definition_ids[idx] = sqlite3_column_int(stmt, 0);
        idx++;
    }

    *count = idx;
    sqlite3_finalize(stmt);
    return 0;
}

int db_load_skin_instance(int instance_id, int *definition_id, SkinRarity *rarity, WearCondition *wear, int *pattern_seed, int *is_stattrak, int *owner_id, time_t *acquired_at, int *is_tradable)
{
    if (!definition_id || !rarity || !wear || !pattern_seed || !is_stattrak || !owner_id || !acquired_at || !is_tradable)
        return -1;

    const char *sql = "SELECT definition_id, rarity, wear, pattern_seed, is_stattrak, owner_id, acquired_at, is_tradable FROM skin_instances WHERE instance_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, instance_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        *definition_id = sqlite3_column_int(stmt, 0);
        *rarity = (SkinRarity)sqlite3_column_int(stmt, 1);
        *wear = (WearCondition)sqlite3_column_double(stmt, 2); // REAL column
        *pattern_seed = sqlite3_column_int(stmt, 3);
        *is_stattrak = sqlite3_column_int(stmt, 4);
        *owner_id = sqlite3_column_int(stmt, 5);
        *acquired_at = sqlite3_column_int64(stmt, 6);
        *is_tradable = sqlite3_column_int(stmt, 7);
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_create_skin_instance(int definition_id, SkinRarity rarity, WearCondition wear, int pattern_seed, int is_stattrak, int owner_id, int *out_instance_id)
{
    if (!out_instance_id)
        return -1;

    if (!db)
        return -1;

    const char *sql = "INSERT INTO skin_instances (definition_id, rarity, wear, pattern_seed, is_stattrak, owner_id, acquired_at, is_tradable) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, 1)"; // Default is_tradable = 1 (tradable)
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, definition_id);
    sqlite3_bind_int(stmt, 2, rarity);
    sqlite3_bind_double(stmt, 3, wear); // REAL column
    sqlite3_bind_int(stmt, 4, pattern_seed);
    sqlite3_bind_int(stmt, 5, is_stattrak);
    sqlite3_bind_int(stmt, 6, owner_id);
    sqlite3_bind_int64(stmt, 7, time(NULL));

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE)
    {
        *out_instance_id = (int)sqlite3_last_insert_rowid(db);
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_update_skin_instance_owner(int instance_id, int new_owner_id)
{
    const char *sql = "UPDATE skin_instances SET owner_id = ? WHERE instance_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, new_owner_id);
    sqlite3_bind_int(stmt, 2, instance_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_get_wear_multiplier(WearCondition wear_float, float *multiplier)
{
    if (!multiplier || wear_float < 0.0f || wear_float > 1.0f)
        return -1;

    // Find multiplier based on float range
    // Special case: wear = 1.00 should match BS range (0.45-1.00 inclusive)
    const char *sql = "SELECT multiplier FROM wear_multipliers WHERE ? >= wear_min AND (? <= wear_max OR (wear_max = 1.0 AND ? = 1.0)) LIMIT 1";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_double(stmt, 1, wear_float);
    sqlite3_bind_double(stmt, 2, wear_float);
    sqlite3_bind_double(stmt, 3, wear_float);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        *multiplier = sqlite3_column_double(stmt, 0);
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_get_rarity_multiplier(SkinRarity rarity, float *multiplier)
{
    if (!multiplier)
        return -1;

    const char *sql = "SELECT multiplier FROM rarity_multipliers WHERE rarity = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, rarity);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        *multiplier = (float)sqlite3_column_double(stmt, 0);
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

float db_calculate_skin_price(int definition_id, SkinRarity rarity, WearCondition wear)
{
    char name[MAX_ITEM_NAME_LEN];
    float base_price;
    float rarity_mult;
    float wear_mult;

    if (db_load_skin_definition(definition_id, name, &base_price) != 0)
        return 0.0f;

    if (db_get_rarity_multiplier(rarity, &rarity_mult) != 0)
        return 0.0f;

    if (db_get_wear_multiplier(wear, &wear_mult) != 0)
        return 0.0f;

    return base_price * rarity_mult * wear_mult;
}

// Get all skin definitions for a case (regardless of rarity)
int db_get_case_skins(int case_id, int *definition_ids, int *count)
{
    if (!definition_ids || !count)
        return -1;

    const char *sql = "SELECT DISTINCT definition_id FROM case_skins WHERE case_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        *count = 0;
        return 0;
    }

    sqlite3_bind_int(stmt, 1, case_id);

    int idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < 100)
    {
        definition_ids[idx] = sqlite3_column_int(stmt, 0);
        idx++;
    }

    *count = idx;
    sqlite3_finalize(stmt);
    return 0;
}

// Fetch full case info with all skin details (optimized for animation preview)
// This solves N+1 query problem by loading all data in a single query
// Note: out_skins must be cast to appropriate struct type by caller
int db_fetch_full_case_info(int case_id, void *out_skins, int *out_count)
{
    if (!out_skins || !out_count)
        return -1;

    const char *sql = "SELECT sd.definition_id, sd.name, sd.rarity, sd.base_price "
                      "FROM case_skins cs "
                      "INNER JOIN skin_definitions sd ON cs.definition_id = sd.definition_id "
                      "WHERE cs.case_id = ? "
                      "ORDER BY sd.definition_id";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        *out_count = 0;
        return -1;
    }

    sqlite3_bind_int(stmt, 1, case_id);

    // Cast to struct with proper layout
    typedef struct
    {
        int def_id;
        char name[64]; // MAX_ITEM_NAME_LEN
        SkinRarity rarity;
        float base_price;
    } CaseSkinInfo;

    CaseSkinInfo *skins = (CaseSkinInfo *)out_skins;

    int idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < 100)
    {
        skins[idx].def_id = sqlite3_column_int(stmt, 0);
        const char *name = (const char *)sqlite3_column_text(stmt, 1);
        strncpy(skins[idx].name, name ? name : "", 63);
        skins[idx].name[63] = '\0';
        skins[idx].rarity = (SkinRarity)sqlite3_column_int(stmt, 2);
        skins[idx].base_price = (float)sqlite3_column_double(stmt, 3);
        idx++;
    }

    *out_count = idx;
    sqlite3_finalize(stmt);
    return 0;
}

// Check if instance is in any pending trade
int db_is_instance_in_pending_trade(int instance_id)
{
    if (!db || instance_id <= 0)
        return 0; // Not in trade if invalid

    // Check if instance is in any pending trade (either offered or requested)
    const char *sql = "SELECT COUNT(*) FROM trades WHERE status = ? AND (offered_skins LIKE ? OR requested_skins LIKE ?)";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return 0; // Assume not in trade if query fails

    sqlite3_bind_int(stmt, 1, TRADE_PENDING);

    // Build LIKE pattern: [instance_id] or [instance_id, or ,instance_id, or ,instance_id]
    char pattern1[64], pattern2[64], pattern3[64], pattern4[64];
    snprintf(pattern1, sizeof(pattern1), "[%d]", instance_id);
    snprintf(pattern2, sizeof(pattern2), "[%d,", instance_id);
    snprintf(pattern3, sizeof(pattern3), ",%d,", instance_id);
    snprintf(pattern4, sizeof(pattern4), ",%d]", instance_id);

    // Check offered_skins
    sqlite3_bind_text(stmt, 2, pattern1, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, pattern1, -1, SQLITE_STATIC);

    int result = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (result > 0)
        return 1;

    // Also check with other patterns
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, TRADE_PENDING);
        sqlite3_bind_text(stmt, 2, pattern2, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, pattern2, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) > 0)
        {
            sqlite3_finalize(stmt);
            return 1;
        }
        sqlite3_finalize(stmt);
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, TRADE_PENDING);
        sqlite3_bind_text(stmt, 2, pattern3, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, pattern3, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) > 0)
        {
            sqlite3_finalize(stmt);
            return 1;
        }
        sqlite3_finalize(stmt);
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, TRADE_PENDING);
        sqlite3_bind_text(stmt, 2, pattern4, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, pattern4, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) > 0)
        {
            sqlite3_finalize(stmt);
            return 1;
        }
        sqlite3_finalize(stmt);
    }

    return 0; // Not in any pending trade
}

// ==================== MARKET LISTINGS V2 OPERATIONS ====================

int db_save_listing_v2(int seller_id, int instance_id, float price, int *out_listing_id)
{
    if (!out_listing_id)
        return -1;

    if (!db)
        return -1;

    const char *sql = "INSERT INTO market_listings_v2 (seller_id, instance_id, price, listed_at, is_sold) "
                      "VALUES (?, ?, ?, ?, 0)";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, seller_id);
    sqlite3_bind_int(stmt, 2, instance_id);
    sqlite3_bind_double(stmt, 3, price);
    sqlite3_bind_int64(stmt, 4, time(NULL));

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE)
    {
        *out_listing_id = (int)sqlite3_last_insert_rowid(db);
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_load_listings_v2(MarketListing *out_listings, int *count)
{
    if (!out_listings || !count)
        return -1;

    const char *sql = "SELECT listing_id, seller_id, instance_id, price, listed_at, is_sold "
                      "FROM market_listings_v2 WHERE is_sold = 0 ORDER BY listed_at DESC";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        *count = 0;
        return 0;
    }

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && found < 100)
    {
        MarketListing *listing = &out_listings[found];
        listing->listing_id = sqlite3_column_int(stmt, 0);
        listing->seller_id = sqlite3_column_int(stmt, 1);
        listing->skin_id = sqlite3_column_int(stmt, 2); // Store instance_id in skin_id field for compatibility
        listing->price = sqlite3_column_double(stmt, 3);
        listing->listed_at = sqlite3_column_int64(stmt, 4);
        listing->is_sold = sqlite3_column_int(stmt, 5);
        found++;
    }

    *count = found;
    sqlite3_finalize(stmt);
    return 0;
}

int db_search_listings_by_name(const char *search_term, MarketListing *out_listings, int *count)
{
    if (!search_term || !out_listings || !count)
        return -1;

    // Search listings by skin name using LIKE (case-insensitive)
    const char *sql = "SELECT ml.listing_id, ml.seller_id, ml.instance_id, ml.price, ml.listed_at, ml.is_sold "
                      "FROM market_listings_v2 ml "
                      "INNER JOIN skin_instances si ON ml.instance_id = si.instance_id "
                      "INNER JOIN skin_definitions sd ON si.definition_id = sd.definition_id "
                      "WHERE ml.is_sold = 0 AND sd.name LIKE ? "
                      "ORDER BY ml.listed_at DESC";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        *count = 0;
        return 0;
    }

    // Build search pattern with wildcards
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "%%%s%%", search_term);
    sqlite3_bind_text(stmt, 1, search_pattern, -1, SQLITE_STATIC);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && found < 100)
    {
        MarketListing *listing = &out_listings[found];
        listing->listing_id = sqlite3_column_int(stmt, 0);
        listing->seller_id = sqlite3_column_int(stmt, 1);
        listing->skin_id = sqlite3_column_int(stmt, 2); // Store instance_id in skin_id field
        listing->price = sqlite3_column_double(stmt, 3);
        listing->listed_at = sqlite3_column_int64(stmt, 4);
        listing->is_sold = sqlite3_column_int(stmt, 5);
        found++;
    }

    *count = found;
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_listing_v2(int listing_id, int *seller_id, int *instance_id, float *price, int *is_sold)
{
    if (!seller_id || !instance_id || !price || !is_sold)
        return -1;

    const char *sql = "SELECT seller_id, instance_id, price, is_sold FROM market_listings_v2 WHERE listing_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, listing_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        *seller_id = sqlite3_column_int(stmt, 0);
        *instance_id = sqlite3_column_int(stmt, 1);
        *price = sqlite3_column_double(stmt, 2);
        *is_sold = sqlite3_column_int(stmt, 3);
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_mark_listing_sold(int listing_id)
{
    const char *sql = "UPDATE market_listings_v2 SET is_sold = 1 WHERE listing_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, listing_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_remove_listing_v2(int listing_id)
{
    const char *sql = "DELETE FROM market_listings_v2 WHERE listing_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, listing_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// Load user's listing history (both sold and unsold)
int db_load_user_listing_history(int user_id, MarketListing *out_listings, int *count)
{
    if (!out_listings || !count || user_id <= 0)
        return -1;

    const char *sql = "SELECT listing_id, seller_id, instance_id, price, listed_at, is_sold "
                      "FROM market_listings_v2 WHERE seller_id = ? ORDER BY listed_at DESC LIMIT 100";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        *count = 0;
        return 0;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && found < 100)
    {
        MarketListing *listing = &out_listings[found];
        listing->listing_id = sqlite3_column_int(stmt, 0);
        listing->seller_id = sqlite3_column_int(stmt, 1);
        listing->skin_id = sqlite3_column_int(stmt, 2); // Store instance_id in skin_id field
        listing->price = sqlite3_column_double(stmt, 3);
        listing->listed_at = sqlite3_column_int64(stmt, 4);
        listing->is_sold = sqlite3_column_int(stmt, 5);
        found++;
    }

    *count = found;
    sqlite3_finalize(stmt);
    return 0;
}

// ==================== TRADE LOCK OPERATIONS ====================

int db_check_trade_lock(int instance_id, int *is_locked)
{
    if (!is_locked)
        return -1;

    const char *sql = "SELECT is_tradable, acquired_at FROM skin_instances WHERE instance_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, instance_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int is_tradable = sqlite3_column_int(stmt, 0);
        time_t acquired_at = sqlite3_column_int64(stmt, 1);
        time_t now = time(NULL);
        time_t lock_duration = TRADE_LOCK_DURATION_SECONDS;

        if (is_tradable == 0 || (now - acquired_at) < lock_duration)
        {
            *is_locked = 1;
        }
        else
        {
            *is_locked = 0;
        }

        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_apply_trade_lock(int instance_id)
{
    const char *sql = "UPDATE skin_instances SET is_tradable = 0, acquired_at = ? WHERE instance_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int64(stmt, 1, time(NULL));
    sqlite3_bind_int(stmt, 2, instance_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_unlock_expired_trades()
{
    time_t now = time(NULL);
    time_t lock_duration = TRADE_LOCK_DURATION_SECONDS;
    time_t threshold = now - lock_duration;

    const char *sql = "UPDATE skin_instances SET is_tradable = 1 WHERE is_tradable = 0 AND acquired_at <= ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int64(stmt, 1, threshold);

    rc = sqlite3_step(stmt);
    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);

    return changes;
}

int db_clean_expired_trades()
{
    time_t now = time(NULL);

    const char *sql = "UPDATE trades SET status = ? WHERE status = ? AND expires_at < ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, TRADE_EXPIRED);
    sqlite3_bind_int(stmt, 2, TRADE_PENDING);
    sqlite3_bind_int64(stmt, 3, now);

    rc = sqlite3_step(stmt);
    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);

    return changes;
}

// ==================== CASE OPERATIONS ====================

static void parse_int_array(const char *json, int *out_array, int *out_count, int max_count)
{
    if (!json || !out_array || !out_count)
        return;

    int idx = 0;
    const char *p = json;

    while (*p && *p != '[')
        p++;

    if (*p != '[')
    {
        *out_count = 0;
        return;
    }

    p++; // skip '['

    while (*p && *p != ']' && idx < max_count)
    {
        if (*p >= '0' && *p <= '9')
        {
            out_array[idx++] = atoi(p);
            while (*p >= '0' && *p <= '9')
                p++;
        }
        else
        {
            p++;
        }
    }

    *out_count = idx;
}

static void parse_float_array(const char *json, float *out_array, int *out_count, int max_count)
{
    if (!json || !out_array || !out_count)
        return;

    int idx = 0;
    const char *p = json;

    while (*p && *p != '[')
        p++;

    if (*p != '[')
    {
        *out_count = 0;
        return;
    }

    p++; // skip '['

    while (*p && *p != ']' && idx < max_count)
    {
        if ((*p >= '0' && *p <= '9') || *p == '.')
        {
            out_array[idx++] = (float)atof(p);
            while ((*p >= '0' && *p <= '9') || *p == '.')
                p++;
        }
        else
        {
            p++;
        }
    }

    *out_count = idx;
}

int db_load_cases(Case *out_cases, int *count)
{
    if (!out_cases || !count)
        return -1;

    if (!db)
        return -1;

    const char *sql = "SELECT c.case_id, c.name, c.price, "
                      "COUNT(cs.definition_id) as skin_count "
                      "FROM cases c "
                      "LEFT JOIN case_skins cs ON c.case_id = cs.case_id "
                      "GROUP BY c.case_id, c.name, c.price "
                      "ORDER BY c.case_id";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    
    if (rc != SQLITE_OK)
    {
        *count = 0;
        return 0;
    }

    int idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < 50)
    {
        Case *c = &out_cases[idx];
        memset(c, 0, sizeof(Case)); // Clear entire struct first
        
        c->case_id = sqlite3_column_int(stmt, 0);
        const char *name = (const char *)sqlite3_column_text(stmt, 1);
        double price_val = sqlite3_column_double(stmt, 2);
        int skin_count_val = sqlite3_column_int(stmt, 3);
        
        // Clear name field first
        memset(c->name, 0, sizeof(c->name));
        
        if (name && sqlite3_column_bytes(stmt, 1) > 0)
        {
            int name_bytes = sqlite3_column_bytes(stmt, 1);
            size_t copy_len = (name_bytes < (int)sizeof(c->name) - 1) ? name_bytes : sizeof(c->name) - 1;
            memcpy(c->name, name, copy_len);
            c->name[copy_len] = '\0';
        }
        else
        {
            c->name[0] = '\0';
        }
        c->price = (float)price_val;
        c->skin_count = skin_count_val;

        // Clear arrays (not used in new model)
        memset(c->possible_skins, 0, sizeof(c->possible_skins));
        memset(c->probabilities, 0, sizeof(c->probabilities));

        idx++;
    }

    *count = idx;
    sqlite3_finalize(stmt);
    return 0;
}

int db_load_case(int case_id, Case *out_case)
{
    if (!out_case)
        return -1;

    if (!db)
        return -1;

    const char *sql = "SELECT case_id, name, price, possible_skins, probabilities, skin_count FROM cases WHERE case_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, case_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        // Clear struct first to avoid any leftover data
        memset(out_case, 0, sizeof(Case));
        
        out_case->case_id = sqlite3_column_int(stmt, 0);
        const char *name = (const char *)sqlite3_column_text(stmt, 1);
        double price_val = sqlite3_column_double(stmt, 2);
        
        if (name && strlen(name) > 0)
        {
            size_t name_len = strlen(name);
            size_t copy_len = (name_len < sizeof(out_case->name) - 1) ? name_len : sizeof(out_case->name) - 1;
            memcpy(out_case->name, name, copy_len);
            out_case->name[copy_len] = '\0';
        }
        else
        {
            out_case->name[0] = '\0';
        }
        out_case->price = (float)price_val;

        const char *skins_json = (const char *)sqlite3_column_text(stmt, 3);
        const char *probs_json = (const char *)sqlite3_column_text(stmt, 4);
        int skin_count = sqlite3_column_int(stmt, 5);

        (void)skin_count; // currently unused, we rely on parsed count

        int parsed_skin_count = 0;
        parse_int_array(skins_json, out_case->possible_skins, &parsed_skin_count, 50);
        out_case->skin_count = parsed_skin_count;

        int parsed_prob_count = 0;
        parse_float_array(probs_json, out_case->probabilities, &parsed_prob_count, 50);
        (void)parsed_prob_count;

        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

// ==================== REPORT OPERATIONS (Phase 7) ====================

int db_save_report(Report *report)
{
    if (!report)
        return -1;

    const char *sql = "INSERT INTO reports (reporter_id, reported_id, reason, created_at, is_resolved) "
                      "VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, report->reporter_id);
    sqlite3_bind_int(stmt, 2, report->reported_id);
    sqlite3_bind_text(stmt, 3, report->reason, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, report->created_at);
    sqlite3_bind_int(stmt, 5, report->is_resolved);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE)
    {
        report->report_id = (int)sqlite3_last_insert_rowid(db);
    }
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_load_reports_for_user(int user_id, Report *out_reports, int *count)
{
    if (!out_reports || !count)
        return -1;

    const char *sql = "SELECT report_id, reporter_id, reported_id, reason, created_at, is_resolved "
                      "FROM reports WHERE reported_id = ? AND is_resolved = 0 "
                      "ORDER BY created_at DESC";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        *count = 0;
        return -1;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    int idx = 0;
    const int max_reports = 100; // Limit to prevent buffer overflow
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < max_reports)
    {
        out_reports[idx].report_id = sqlite3_column_int(stmt, 0);
        out_reports[idx].reporter_id = sqlite3_column_int(stmt, 1);
        out_reports[idx].reported_id = sqlite3_column_int(stmt, 2);
        const char *reason = (const char *)sqlite3_column_text(stmt, 3);
        strncpy(out_reports[idx].reason, reason ? reason : "", sizeof(out_reports[idx].reason) - 1);
        out_reports[idx].reason[sizeof(out_reports[idx].reason) - 1] = '\0';
        out_reports[idx].created_at = (time_t)sqlite3_column_int64(stmt, 4);
        out_reports[idx].is_resolved = sqlite3_column_int(stmt, 5);
        idx++;
    }

    *count = idx;
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_report_count(int user_id)
{
    const char *sql = "SELECT COUNT(*) FROM reports WHERE reported_id = ? AND is_resolved = 0";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, user_id);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

// ==================== QUESTS OPERATIONS ====================

int db_save_quest(Quest *quest)
{
    if (!quest)
        return -1;

    const char *sql = "INSERT INTO quests (user_id, quest_type, progress, target, is_completed, is_claimed, started_at, completed_at) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, quest->user_id);
    sqlite3_bind_int(stmt, 2, quest->quest_type);
    sqlite3_bind_int(stmt, 3, quest->progress);
    sqlite3_bind_int(stmt, 4, quest->target);
    sqlite3_bind_int(stmt, 5, quest->is_completed);
    sqlite3_bind_int(stmt, 6, quest->is_claimed);
    sqlite3_bind_int64(stmt, 7, quest->started_at);
    if (quest->completed_at > 0)
        sqlite3_bind_int64(stmt, 8, quest->completed_at);
    else
        sqlite3_bind_null(stmt, 8);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE && quest->quest_id == 0)
    {
        quest->quest_id = (int)sqlite3_last_insert_rowid(db);
    }
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_load_user_quests(int user_id, Quest *out_quests, int *count)
{
    if (!out_quests || !count)
        return -1;

    // Load all quests that are either not completed OR completed but not claimed yet
    // This allows players to see and claim completed quest rewards
    const char *sql = "SELECT quest_id, user_id, quest_type, progress, target, is_completed, is_claimed, started_at, completed_at "
                      "FROM quests WHERE user_id = ? AND is_claimed = 0 ORDER BY quest_type";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        *count = 0;
        return -1;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    int idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < 10)
    {
        out_quests[idx].quest_id = sqlite3_column_int(stmt, 0);
        out_quests[idx].user_id = sqlite3_column_int(stmt, 1);
        out_quests[idx].quest_type = (QuestType)sqlite3_column_int(stmt, 2);
        out_quests[idx].progress = sqlite3_column_int(stmt, 3);
        out_quests[idx].target = sqlite3_column_int(stmt, 4);
        out_quests[idx].is_completed = sqlite3_column_int(stmt, 5);
        out_quests[idx].is_claimed = sqlite3_column_int(stmt, 6);
        out_quests[idx].started_at = sqlite3_column_int64(stmt, 7);
        const char *completed_at_str = (const char *)sqlite3_column_text(stmt, 8);
        out_quests[idx].completed_at = completed_at_str ? sqlite3_column_int64(stmt, 8) : 0;
        idx++;
    }

    *count = idx;
    sqlite3_finalize(stmt);
    return 0;
}

int db_update_quest(Quest *quest)
{
    if (!quest)
        return -1;

    const char *sql = "UPDATE quests SET progress = ?, is_completed = ?, is_claimed = ?, completed_at = ? WHERE quest_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, quest->progress);
    sqlite3_bind_int(stmt, 2, quest->is_completed);
    sqlite3_bind_int(stmt, 3, quest->is_claimed);
    if (quest->completed_at > 0)
        sqlite3_bind_int64(stmt, 4, quest->completed_at);
    else
        sqlite3_bind_null(stmt, 4);
    sqlite3_bind_int(stmt, 5, quest->quest_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ==================== ACHIEVEMENTS OPERATIONS ====================

int db_save_achievement(Achievement *achievement)
{
    if (!achievement)
        return -1;

    const char *sql = "INSERT OR REPLACE INTO achievements (user_id, achievement_type, is_unlocked, is_claimed, unlocked_at) "
                      "VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, achievement->user_id);
    sqlite3_bind_int(stmt, 2, achievement->achievement_type);
    sqlite3_bind_int(stmt, 3, achievement->is_unlocked);
    sqlite3_bind_int(stmt, 4, achievement->is_claimed);
    if (achievement->unlocked_at > 0)
        sqlite3_bind_int64(stmt, 5, achievement->unlocked_at);
    else
        sqlite3_bind_null(stmt, 5);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE && achievement->achievement_id == 0)
    {
        achievement->achievement_id = (int)sqlite3_last_insert_rowid(db);
    }
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_load_user_achievements(int user_id, Achievement *out_achievements, int *count)
{
    if (!out_achievements || !count)
        return -1;

    const char *sql = "SELECT achievement_id, user_id, achievement_type, is_unlocked, is_claimed, unlocked_at "
                      "FROM achievements WHERE user_id = ? ORDER BY achievement_type";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        *count = 0;
        return -1;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    int idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < 10)
    {
        out_achievements[idx].achievement_id = sqlite3_column_int(stmt, 0);
        out_achievements[idx].user_id = sqlite3_column_int(stmt, 1);
        out_achievements[idx].achievement_type = (AchievementType)sqlite3_column_int(stmt, 2);
        out_achievements[idx].is_unlocked = sqlite3_column_int(stmt, 3);
        out_achievements[idx].is_claimed = sqlite3_column_int(stmt, 4);
        const char *unlocked_at_str = (const char *)sqlite3_column_text(stmt, 5);
        out_achievements[idx].unlocked_at = unlocked_at_str ? sqlite3_column_int64(stmt, 5) : 0;
        idx++;
    }

    *count = idx;
    sqlite3_finalize(stmt);
    return 0;
}

int db_update_achievement(Achievement *achievement)
{
    if (!achievement)
        return -1;

    const char *sql = "UPDATE achievements SET is_unlocked = ?, is_claimed = ?, unlocked_at = ? WHERE achievement_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, achievement->is_unlocked);
    sqlite3_bind_int(stmt, 2, achievement->is_claimed);
    if (achievement->unlocked_at > 0)
        sqlite3_bind_int64(stmt, 3, achievement->unlocked_at);
    else
        sqlite3_bind_null(stmt, 3);
    sqlite3_bind_int(stmt, 4, achievement->achievement_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ==================== LOGIN STREAK OPERATIONS ====================

int db_save_login_streak(LoginStreak *streak)
{
    if (!streak)
        return -1;

    const char *sql = "INSERT OR REPLACE INTO login_streaks (user_id, current_streak, last_login_date, last_reward_date) "
                      "VALUES (?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, streak->user_id);
    sqlite3_bind_int(stmt, 2, streak->current_streak);
    sqlite3_bind_int64(stmt, 3, streak->last_login_date);
    if (streak->last_reward_date > 0)
        sqlite3_bind_int64(stmt, 4, streak->last_reward_date);
    else
        sqlite3_bind_null(stmt, 4);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_load_login_streak(int user_id, LoginStreak *out_streak)
{
    if (!out_streak)
        return -1;

    const char *sql = "SELECT user_id, current_streak, last_login_date, last_reward_date FROM login_streaks WHERE user_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, user_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        out_streak->user_id = sqlite3_column_int(stmt, 0);
        out_streak->current_streak = sqlite3_column_int(stmt, 1);
        out_streak->last_login_date = sqlite3_column_int64(stmt, 2);
        const char *last_reward_str = (const char *)sqlite3_column_text(stmt, 3);
        out_streak->last_reward_date = last_reward_str ? sqlite3_column_int64(stmt, 3) : 0;
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

// ==================== CHAT OPERATIONS ====================

int db_save_chat_message(int user_id, const char *username, const char *message)
{
    if (!username || !message)
        return -1;

    const char *sql = "INSERT INTO chat_messages (user_id, username, message, timestamp) VALUES (?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, message, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, time(NULL));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_load_recent_chat_messages(ChatMessage *out_messages, int *count, int limit)
{
    if (!out_messages || !count)
        return -1;

    if (limit <= 0 || limit > MAX_CHAT_HISTORY)
        limit = MAX_CHAT_HISTORY;

    const char *sql = "SELECT message_id, user_id, username, message, timestamp "
                      "FROM chat_messages ORDER BY timestamp DESC LIMIT ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        *count = 0;
        return -1;
    }

    sqlite3_bind_int(stmt, 1, limit);

    int idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < limit)
    {
        out_messages[idx].message_id = sqlite3_column_int(stmt, 0);
        out_messages[idx].user_id = sqlite3_column_int(stmt, 1);
        strncpy(out_messages[idx].username, (char *)sqlite3_column_text(stmt, 2), MAX_USERNAME_LEN - 1);
        out_messages[idx].username[MAX_USERNAME_LEN - 1] = '\0';
        strncpy(out_messages[idx].message, (char *)sqlite3_column_text(stmt, 3), 255);
        out_messages[idx].message[255] = '\0';
        out_messages[idx].timestamp = sqlite3_column_int64(stmt, 4);
        idx++;
    }

    *count = idx;
    sqlite3_finalize(stmt);
    return 0;
}

// ==================== PRICE HISTORY OPERATIONS ====================

int db_save_price_history(int definition_id, float price, int transaction_type)
{
    if (definition_id <= 0 || price <= 0)
        return -1;

    const char *sql = "INSERT INTO price_history (definition_id, price, transaction_type, timestamp) "
                      "VALUES (?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, definition_id);
    sqlite3_bind_double(stmt, 2, price);
    sqlite3_bind_int(stmt, 3, transaction_type);
    sqlite3_bind_int64(stmt, 4, time(NULL));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_get_price_history_24h(int definition_id, PriceHistoryEntry *out_history, int *count)
{
    if (!out_history || !count || definition_id <= 0)
        return -1;

    time_t now = time(NULL);
    time_t day_ago = now - (24 * 60 * 60); // 24 hours ago

    const char *sql = "SELECT price, transaction_type, timestamp "
                      "FROM price_history "
                      "WHERE definition_id = ? AND timestamp >= ? "
                      "ORDER BY timestamp ASC "
                      "LIMIT 100";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        *count = 0;
        return 0;
    }

    sqlite3_bind_int(stmt, 1, definition_id);
    sqlite3_bind_int64(stmt, 2, day_ago);

    int idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < 100)
    {
        out_history[idx].definition_id = definition_id;
        out_history[idx].price = (float)sqlite3_column_double(stmt, 0);
        out_history[idx].transaction_type = sqlite3_column_int(stmt, 1);
        out_history[idx].timestamp = sqlite3_column_int64(stmt, 2);
        idx++;
    }

    *count = idx;
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_price_24h_ago(int definition_id, float *out_price)
{
    if (!out_price || definition_id <= 0)
        return -1;

    time_t now = time(NULL);
    time_t day_ago = now - (24 * 60 * 60); // 24 hours ago

    const char *sql = "SELECT price FROM price_history "
                      "WHERE definition_id = ? AND timestamp >= ? "
                      "ORDER BY timestamp ASC LIMIT 1";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, definition_id);
    sqlite3_bind_int64(stmt, 2, day_ago);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        *out_price = (float)sqlite3_column_double(stmt, 0);
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1; // No price history found
}
