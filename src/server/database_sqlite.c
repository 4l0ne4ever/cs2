// database_sqlite.c - Database Operations with SQLite

#include "../include/database.h"
#include "../include/types.h"
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
        "(21, 'AK-47 | Redline', 180.0, 5), "            // ~$180 Covert FN
        "(22, 'AWP | Redline', 200.0, 5), "              // ~$200 Covert FN
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
        "(37, 'AWP | Hyper Beast', 150.0, 5), "          // ~$150 Covert FN
        "(38, 'AK-47 | Wasteland Rebel', 160.0, 5), "    // ~$160 Covert FN
        "(39, 'M4A1-S | Master Piece', 200.0, 5), "      // ~$200 Covert FN
        "(40, 'AWP | Medusa', 5000.0, 5), "              // ~$5000 Covert FN (very expensive)
        // Falchion Case skins - All Covert
        "(41, 'AK-47 | Aquamarine Revenge', 130.0, 5), "   // ~$130 Covert FN
        "(42, 'AWP | Hyper Beast', 150.0, 5), "            // ~$150 Covert FN
        "(43, 'M4A4 | Royal Paladin', 75.0, 5), "          // ~$75 Covert FN
        "(44, 'Glock-18 | Twilight Galaxy', 50.0, 5), "    // ~$50 Covert FN
        "(45, 'USP-S | Kill Confirmed', 100.0, 5), "       // ~$100 Covert FN
        "(46, 'Desert Eagle | Kumicho Dragon', 85.0, 5), " // ~$85 Covert FN
        "(47, 'AWP | Hyper Beast', 150.0, 5), "            // ~$150 Covert FN
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
        "(81, 'AK-47 | Blue Laminate', 15.0, 2), "              // ~$15 Mil-Spec FN
        "(82, 'AWP | Worm God', 20.0, 2), "                     // ~$20 Mil-Spec FN
        "(83, 'M4A4 | Desert-Strike', 12.0, 2), "              // ~$12 Mil-Spec FN
        "(84, 'Glock-18 | Steel Disruption', 8.0, 2), "        // ~$8 Mil-Spec FN
        "(85, 'USP-S | Forest Leaves', 10.0, 2), "             // ~$10 Mil-Spec FN
        // Restricted (RARITY_RESTRICTED = 3) - 15.98% drop rate
        "(86, 'AK-47 | Emerald Pinstripe', 35.0, 3), "         // ~$35 Restricted FN
        "(87, 'AWP | Corticera', 40.0, 3), "                   // ~$40 Restricted FN
        "(88, 'M4A4 | X-Ray', 30.0, 3), "                      // ~$30 Restricted FN
        "(89, 'Glock-18 | Water Elemental', 25.0, 3), "        // ~$25 Restricted FN
        "(90, 'USP-S | Guardian', 28.0, 3), "                  // ~$28 Restricted FN
        // Classified (RARITY_CLASSIFIED = 4) - 3.2% drop rate
        "(91, 'AK-47 | Jaguar', 80.0, 4), "                     // ~$80 Classified FN
        "(92, 'AWP | Hyper Beast', 90.0, 4), "                  // ~$90 Classified FN
        "(93, 'M4A4 | Bullet Rain', 70.0, 4), "                // ~$70 Classified FN
        "(94, 'Glock-18 | Grinder', 50.0, 4), "                // ~$50 Classified FN
        "(95, 'USP-S | Kill Confirmed', 60.0, 4);";            // ~$60 Classified FN
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
        "(1, 51), (1, 54), "                                                                // Karambit Fade, Butterfly Fade
        "(1, 71), (1, 75);";                                                                // Specialist Fade, Sport Pandora
    sqlite3_exec(db, case1_sql, 0, 0, &err_msg);
    if (err_msg)
    {
        sqlite3_free(err_msg);
        err_msg = 0;
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
        "(2, 52), (2, 56), "                                                                         // Karambit Doppler, M9 Bayonet Fade
        "(2, 72), (2, 76);";                                                                         // Specialist Crimson Kimono, Sport Vice
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
        // Contraband (6) - 0.26% drop rate
        "(3, 53), (3, 57), "                                                                         // Karambit Tiger Tooth, M9 Bayonet Doppler
        "(3, 73), (3, 77);";                                                                         // Hand Wraps Slaughter, Driver King Snake
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
        "(4, 31), (4, 32), (4, 33), (4, 34), (4, 35), (4, 36), (4, 37), (4, 38), (4, 39), (4, 40), "
        // Contraband (6) - 0.26% drop rate
        "(4, 58), (4, 60), "                                                                         // Bayonet Fade, Talon Fade
        "(4, 74), (4, 78);";                                                                         // Hand Wraps Cobalt Skulls, Driver Crimson Weave
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
        "(5, 41), (5, 42), (5, 43), (5, 44), (5, 45), (5, 46), (5, 47), (5, 48), (5, 49), (5, 50), "
        // Contraband (6) - 0.26% drop rate
        "(5, 64), (5, 66), "                                                                         // Falchion Fade, Gut Fade
        "(5, 79), (5, 80);";                                                                         // Moto Spearmint, Moto Cool Mint
    sqlite3_exec(db, case5_sql, 0, 0, &err_msg);
    if (err_msg)
    {
        sqlite3_free(err_msg);
        err_msg = 0;
    }
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
        "skin_id INTEGER NOT NULL, "
        "FOREIGN KEY (user_id) REFERENCES users(user_id), "
        "FOREIGN KEY (skin_id) REFERENCES skins(skin_id), "
        "UNIQUE(user_id, skin_id)"
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
        "CREATE INDEX IF NOT EXISTS idx_skins_owner ON skins(owner_id);"
        "CREATE INDEX IF NOT EXISTS idx_inventories_user ON inventories(user_id);"
        "CREATE INDEX IF NOT EXISTS idx_inventories_skin ON inventories(skin_id);"
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
        "CREATE INDEX IF NOT EXISTS idx_skin_definitions_name ON skin_definitions(name);";

    rc = sqlite3_exec(db, schema_sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Schema creation error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    // Insert initial data if tables are empty
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM wear_multipliers", -1, &stmt, 0);
    if (rc == SQLITE_OK)
    {
        if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 0)
        {
            sqlite3_finalize(stmt);
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
        else
        {
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
        strncpy(out_skin->name, (char *)sqlite3_column_text(stmt, 1), MAX_ITEM_NAME_LEN);
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

    out_inv->user_id = user_id;
    out_inv->count = 0;
    memset(out_inv->skin_ids, 0, sizeof(out_inv->skin_ids));

    const char *sql = "SELECT skin_id FROM inventories WHERE user_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return 0; // Empty inventory

    sqlite3_bind_int(stmt, 1, user_id);

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < MAX_INVENTORY_SIZE)
    {
        out_inv->skin_ids[count++] = sqlite3_column_int(stmt, 0);
    }

    out_inv->count = count;
    sqlite3_finalize(stmt);
    return 0;
}

int db_add_to_inventory(int user_id, int skin_id)
{
    const char *sql = "INSERT INTO inventories (user_id, skin_id) VALUES (?, ?)";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, skin_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_remove_from_inventory(int user_id, int skin_id)
{
    const char *sql = "DELETE FROM inventories WHERE user_id = ? AND skin_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, skin_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ==================== TRADE OPERATIONS ====================

int db_save_trade(TradeOffer *trade)
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

    const char *sql = "SELECT * FROM trades WHERE trade_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, trade_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        out_trade->trade_id = sqlite3_column_int(stmt, 0);
        out_trade->from_user_id = sqlite3_column_int(stmt, 1);
        out_trade->to_user_id = sqlite3_column_int(stmt, 2);

        // Parse JSON arrays
        const char *offered_str = (char *)sqlite3_column_text(stmt, 3);
        out_trade->offered_count = sqlite3_column_int(stmt, 4);
        out_trade->offered_cash = sqlite3_column_double(stmt, 5);
        const char *requested_str = (char *)sqlite3_column_text(stmt, 6);
        out_trade->requested_count = sqlite3_column_int(stmt, 7);
        out_trade->requested_cash = sqlite3_column_double(stmt, 8);
        out_trade->status = sqlite3_column_int(stmt, 9);
        out_trade->created_at = sqlite3_column_int64(stmt, 10);
        out_trade->expires_at = sqlite3_column_int64(stmt, 11);

        // Simple JSON parsing: "[1,2,3]" -> array
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

    // Optimized: use index on (from_user_id, to_user_id, status)
    const char *sql = "SELECT * FROM trades WHERE (from_user_id = ? OR to_user_id = ?) AND status = ? ORDER BY created_at DESC";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        *count = 0;
        return 0;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_int(stmt, 3, TRADE_PENDING); // status = 0 (pending)

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && found < 50)
    {
        TradeOffer *trade = &out_trades[found];
        trade->trade_id = sqlite3_column_int(stmt, 0);
        trade->from_user_id = sqlite3_column_int(stmt, 1);
        trade->to_user_id = sqlite3_column_int(stmt, 2);

        const char *offered_str = (char *)sqlite3_column_text(stmt, 3);
        const char *requested_str = (char *)sqlite3_column_text(stmt, 4);

        trade->offered_count = sqlite3_column_int(stmt, 5);
        trade->offered_cash = sqlite3_column_double(stmt, 6);
        trade->requested_count = sqlite3_column_int(stmt, 8);
        trade->requested_cash = sqlite3_column_double(stmt, 9);
        trade->status = sqlite3_column_int(stmt, 10);
        trade->created_at = sqlite3_column_int64(stmt, 11);
        trade->expires_at = sqlite3_column_int64(stmt, 12);

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

    const char *sql = "INSERT INTO transaction_logs (log_id, type, user_id, details, timestamp) "
                      "VALUES (?, ?, ?, ?, ?)";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, log->log_id);
    sqlite3_bind_int(stmt, 2, log->type);
    sqlite3_bind_int(stmt, 3, log->user_id);
    sqlite3_bind_text(stmt, 4, log->details, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, log->timestamp);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
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
        strncpy(name, (char *)sqlite3_column_text(stmt, 0), MAX_ITEM_NAME_LEN);
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

    const char *sql = "SELECT name, base_price, rarity FROM skin_definitions WHERE definition_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, definition_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        strncpy(name, (char *)sqlite3_column_text(stmt, 0), MAX_ITEM_NAME_LEN);
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

    sqlite3_finalize(stmt);
    return -1;
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

    const char *sql = "INSERT INTO skin_instances (definition_id, rarity, wear, pattern_seed, is_stattrak, owner_id, acquired_at, is_tradable) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, 0)";
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
    typedef struct {
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

// ==================== MARKET LISTINGS V2 OPERATIONS ====================

int db_save_listing_v2(int seller_id, int instance_id, float price, int *out_listing_id)
{
    if (!out_listing_id)
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
        time_t lock_duration = 7 * 24 * 60 * 60; // 7 days in seconds

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
    time_t lock_duration = 7 * 24 * 60 * 60; // 7 days
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

    const char *sql = "SELECT c.case_id, c.name, c.price, "
                      "COUNT(cs.definition_id) as skin_count "
                      "FROM cases c "
                      "LEFT JOIN case_skins cs ON c.case_id = cs.case_id "
                      "GROUP BY c.case_id, c.name, c.price";
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
        c->case_id = sqlite3_column_int(stmt, 0);
        const char *name = (const char *)sqlite3_column_text(stmt, 1);
        strncpy(c->name, name ? name : "", sizeof(c->name));
        c->price = (float)sqlite3_column_double(stmt, 2);
        c->skin_count = sqlite3_column_int(stmt, 3);

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

    const char *sql = "SELECT case_id, name, price, possible_skins, probabilities, skin_count FROM cases WHERE case_id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, case_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        out_case->case_id = sqlite3_column_int(stmt, 0);
        const char *name = (const char *)sqlite3_column_text(stmt, 1);
        strncpy(out_case->name, name ? name : "", sizeof(out_case->name));
        out_case->price = (float)sqlite3_column_double(stmt, 2);

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
