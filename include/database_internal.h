#ifndef DATABASE_INTERNAL_H
#define DATABASE_INTERNAL_H

#include <sqlite3.h>

// Shared database connection (defined in database_common.c)
extern sqlite3 *db;

// Helper functions for JSON parsing (used in database_cases.c)
int parse_int_array(const char *json, int *out_array, int *out_count, int max_count);
int parse_float_array(const char *json, float *out_array, int *out_count, int max_count);

#endif // DATABASE_INTERNAL_H
