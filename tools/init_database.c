// init_database.c - Initialize SQLite Database with Sample Data

#include "../include/database.h"
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    
    printf("=== CS2 Skin Trading - Database Initializer ===\n\n");

    // Remove old database
    system("rm -f data/database.db");
    system("mkdir -p data");

    // Initialize database using db_init() from database_sqlite.c
    if (db_init() != 0)
    {
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }

    printf("✓ Database initialized\n");

    // Verify data
    printf("\n=== Verifying data ===\n");

    sqlite3 *db;
    int rc = sqlite3_open("data/database.db", &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database for verification: %s\n", sqlite3_errmsg(db));
        db_close();
        return 1;
    }

    int rows = 0;
    sqlite3_stmt *stmt;
    
    // Count users
    const char *sql_count = "SELECT COUNT(*) FROM users";
    rc = sqlite3_prepare_v2(db, sql_count, -1, &stmt, 0);
    if (rc == SQLITE_OK)
    {
        sqlite3_step(stmt);
        rows = sqlite3_column_int(stmt, 0);
        printf("Users: %d\n", rows);
    }
    sqlite3_finalize(stmt);

    // Count cases
    sql_count = "SELECT COUNT(*) FROM cases";
    rc = sqlite3_prepare_v2(db, sql_count, -1, &stmt, 0);
    if (rc == SQLITE_OK)
    {
        sqlite3_step(stmt);
        rows = sqlite3_column_int(stmt, 0);
        printf("Cases: %d\n", rows);
    }
    sqlite3_finalize(stmt);

    // Count skin definitions
    sql_count = "SELECT COUNT(*) FROM skin_definitions";
    rc = sqlite3_prepare_v2(db, sql_count, -1, &stmt, 0);
    if (rc == SQLITE_OK)
    {
        sqlite3_step(stmt);
        rows = sqlite3_column_int(stmt, 0);
        printf("Skin definitions: %d\n", rows);
    }
    sqlite3_finalize(stmt);

    // Count case_skins
    sql_count = "SELECT COUNT(*) FROM case_skins";
    rc = sqlite3_prepare_v2(db, sql_count, -1, &stmt, 0);
    if (rc == SQLITE_OK)
    {
        sqlite3_step(stmt);
        rows = sqlite3_column_int(stmt, 0);
        printf("Case-skin mappings: %d\n", rows);
    }
    sqlite3_finalize(stmt);

    sqlite3_close(db);
    db_close();

    printf("\n=== Database initialized successfully! ===\n");
    printf("\nYou can now view the database with:\n");
    printf("  sqlite3 data/database.db\n");
    printf("  .tables\n");
    printf("  SELECT * FROM cases;\n");
    printf("  SELECT * FROM skin_definitions LIMIT 10;\n");

    return 0;
}
