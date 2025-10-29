#ifndef AUTH_H
#define AUTH_H

#include "types.h"

// Hash password using SHA256
void hash_password(const char *password, char *output);

// Verify password
int verify_password(const char *password, const char *hash);

// Generate session token (UUID)
void generate_session_token(char *token);

// Register new user
int register_user(const char *username, const char *password, User *out_user);

// Login user
int login_user(const char *username, const char *password, Session *out_session);

// Validate session
int validate_session(const char *token, Session *out_session);

// Logout user
void logout_user(const char *session_token);

#endif // AUTH_H
