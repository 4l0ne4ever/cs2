// auth.c - Authentication Module

#include "../include/auth.h"
#include "../include/database.h"
#include "../include/protocol.h"
#include "../include/types.h"
#include "../include/quests.h"
#include "../include/login_rewards.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sqlite3.h>

// Simple hash function (for testing without OpenSSL)
void hash_password(const char *password, char *output)
{
    // Simple hash for testing (should be SHA256 in production)
    unsigned long hash = 5381;
    int c;
    const char *str = password;

    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    sprintf(output, "%016lx", hash);
}

int verify_password(const char *password, const char *hash)
{
    char computed_hash[65];
    hash_password(password, computed_hash);
    return strcmp(computed_hash, hash) == 0;
}

void generate_session_token(char *token)
{
    // Generate simple session token
    srand(time(NULL));
    for (int i = 0; i < 16; i++)
    {
        sprintf(token + i * 2, "%02x", rand() % 256);
    }
    token[32] = '\0';
}

int register_user(const char *username, const char *password, User *out_user)
{
    if (!username || !password || !out_user)
    {
        return ERR_INVALID_CREDENTIALS;
    }

    // Validate username length
    size_t username_len = strlen(username);
    if (username_len < 3 || username_len >= MAX_USERNAME_LEN)
    {
        return ERR_INVALID_CREDENTIALS;
    }

    // Validate password length (minimum 6 characters)
    size_t password_len = strlen(password);
    if (password_len < 6 || password_len > 64)
    {
        return ERR_INVALID_CREDENTIALS;
    }

    // Check if user exists
    if (db_user_exists(username))
    {
        return ERR_USER_EXISTS;
    }

    // Create new user
    User new_user = {0};
    new_user.user_id = 0; // Will be assigned by db_save_user
    strncpy(new_user.username, username, MAX_USERNAME_LEN - 1);
    hash_password(password, new_user.password_hash);
    new_user.balance = 100.0f; // Starting balance
    new_user.created_at = time(NULL);
    new_user.is_banned = 0;

    // Save to database (db_save_user will assign user_id if it's 0)
    if (db_save_user(&new_user) != 0)
    {
        return ERR_DATABASE_ERROR;
    }

    // Give free Consumer skin to new user
    // Find a Consumer rarity skin definition
    int consumer_def_id = 0;
    sqlite3 *db;
    if (sqlite3_open("data/database.db", &db) == SQLITE_OK)
    {
        const char *sql = "SELECT definition_id FROM skin_definitions WHERE rarity = 0 LIMIT 1";
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK)
        {
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                consumer_def_id = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
        sqlite3_close(db);
    }
    
    if (consumer_def_id > 0)
    {
        // Create skin instance with Consumer rarity
        int instance_id = 0;
        WearCondition wear = 0.05f; // Factory New
        int pattern_seed = 0;
        int is_stattrak = 0;
        
        if (db_create_skin_instance(consumer_def_id, RARITY_CONSUMER, wear, pattern_seed, is_stattrak, new_user.user_id, &instance_id) == 0)
        {
            // Add to inventory
            db_add_to_inventory(new_user.user_id, instance_id);
        }
    }

    // Initialize daily quests for new user
    init_daily_quests(new_user.user_id);

    // new_user.user_id should now be set by db_save_user
    *out_user = new_user;
    return ERR_SUCCESS;
}

int login_user(const char *username, const char *password, Session *out_session)
{
    if (!username || !password || !out_session)
    {
        return ERR_INVALID_CREDENTIALS;
    }

    // Load user
    User user;
    if (db_load_user_by_username(username, &user) != 0)
    {
        return ERR_INVALID_CREDENTIALS;
    }

    // Check banned
    if (user.is_banned)
    {
        return ERR_BANNED;
    }

    // Verify password
    if (!verify_password(password, user.password_hash))
    {
        return ERR_INVALID_CREDENTIALS;
    }

    // Create session
    Session session = {0};
    generate_session_token(session.session_token);
    session.user_id = user.user_id;
    session.socket_fd = -1; // Set by caller
    session.login_time = time(NULL);
    session.last_activity = time(NULL);
    session.is_active = 1;

    // Save session
    if (db_save_session(&session) != 0)
    {
        return ERR_DATABASE_ERROR;
    }

    // Update user last_login
    user.last_login = time(NULL);
    db_update_user(&user);
    
    // Update login streak
    update_login_streak(user.user_id);
    
    // Check and reset daily quests if needed (after 24 hours)
    // This will reset quests that are older than 24 hours
    check_and_reset_daily_quests(user.user_id);

    *out_session = session;
    return ERR_SUCCESS;
}

int validate_session(const char *token, Session *out_session)
{
    if (!token || !out_session)
    {
        return ERR_SESSION_EXPIRED;
    }

    Session session;
    if (db_load_session(token, &session) != 0)
    {
        return ERR_SESSION_EXPIRED;
    }

    // Check if session is active
    if (!session.is_active)
    {
        return ERR_SESSION_EXPIRED;
    }

    // Check if session expired (1 hour)
    if (time(NULL) - session.last_activity > 3600)
    {
        return ERR_SESSION_EXPIRED;
    }

    *out_session = session;
    return ERR_SUCCESS;
}

void logout_user(const char *session_token)
{
    if (!session_token)
        return;

    db_delete_session(session_token);
}
