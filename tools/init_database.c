// init_database.c - Initialize SQLite Database with Sample Data

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>

int main(int argc, char *argv[])
{
    sqlite3 *db;
    char *err_msg = 0;
    int rc;

    printf("=== CS2 Skin Trading - Database Initializer ===\n\n");

    // Remove old database
    system("rm -f data/database.db");
    system("mkdir -p data");

    // Open database
    rc = sqlite3_open("data/database.db", &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    printf("✓ Database created\n");

    // Read and execute schema
    FILE *schema_file = fopen("data/schema.sql", "r");
    if (!schema_file)
    {
        fprintf(stderr, "Cannot read schema.sql\n");
        sqlite3_close(db);
        return 1;
    }

    char line[1024];
    char sql[10000] = "";
    while (fgets(line, sizeof(line), schema_file))
    {
        strcat(sql, line);
    }
    fclose(schema_file);

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Schema error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    printf("✓ Schema created\n");

    // Read and execute initial data
    FILE *data_file = fopen("data/init_data.sql", "r");
    if (!data_file)
    {
        fprintf(stderr, "Cannot read init_data.sql\n");
        sqlite3_close(db);
        return 1;
    }

    memset(sql, 0, sizeof(sql));
    while (fgets(line, sizeof(line), data_file))
    {
        strcat(sql, line);
    }
    fclose(data_file);

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Data error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    printf("✓ Initial data loaded\n");

    // Verify data
    printf("\n=== Verifying data ===\n");

    int rows = 0;
    char *sql_count = "SELECT COUNT(*) FROM users";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql_count, -1, &stmt, 0);
    if (rc == SQLITE_OK)
    {
        sqlite3_step(stmt);
        rows = sqlite3_column_int(stmt, 0);
        printf("Users: %d\n", rows);
    }
    sqlite3_finalize(stmt);

    sql_count = "SELECT COUNT(*) FROM skins";
    rc = sqlite3_prepare_v2(db, sql_count, -1, &stmt, 0);
    if (rc == SQLITE_OK)
    {
        sqlite3_step(stmt);
        rows = sqlite3_column_int(stmt, 0);
        printf("Skins: %d\n", rows);
    }
    sqlite3_finalize(stmt);

    sql_count = "SELECT COUNT(*) FROM cases";
    rc = sqlite3_prepare_v2(db, sql_count, -1, &stmt, 0);
    if (rc == SQLITE_OK)
    {
        sqlite3_step(stmt);
        rows = sqlite3_column_int(stmt, 0);
        printf("Cases: %d\n", rows);
    }
    sqlite3_finalize(stmt);

    sql_count = "SELECT COUNT(*) FROM market_listings WHERE is_sold = 0";
    rc = sqlite3_prepare_v2(db, sql_count, -1, &stmt, 0);
    if (rc == SQLITE_OK)
    {
        sqlite3_step(stmt);
        rows = sqlite3_column_int(stmt, 0);
        printf("Active market listings: %d\n", rows);
    }
    sqlite3_finalize(stmt);

    sqlite3_close(db);

    printf("\n=== Database initialized successfully! ===\n");
    printf("\nYou can now view the database with:\n");
    printf("  sqlite3 data/database.db\n");
    printf("  .tables\n");
    printf("  SELECT * FROM users;\n");
    printf("  SELECT * FROM skins;\n");

    return 0;
}
