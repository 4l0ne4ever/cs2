// test_auth.c - Authentication Tests

#include "../include/auth.h"
#include "../include/database.h"
#include "../include/protocol.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

void test_password_hashing()
{
    printf("Testing password hashing...\n");

    char hash[65];
    hash_password("testpass123", hash);

    assert(strlen(hash) > 0);
    printf("✅ password hashing passed\n");
}

void test_password_verification()
{
    printf("\nTesting password verification...\n");

    char hash[65];
    hash_password("correctpass", hash);

    assert(verify_password("correctpass", hash) == 1);
    assert(verify_password("wrongpass", hash) == 0);

    printf("✅ password verification passed\n");
}

void test_session_token_generation()
{
    printf("\nTesting session token generation...\n");

    char token[37];
    generate_session_token(token);

    assert(strlen(token) == 32);
    printf("✅ session token generation passed\n");
}

void test_registration()
{
    printf("\nTesting user registration...\n");

    // Cleanup previous test data
    system("rm -rf data");
    db_init();

    User new_user;
    int result = register_user("alice", "password123", &new_user);

    assert(result == ERR_SUCCESS);
    assert(new_user.user_id > 0);
    assert(strcmp(new_user.username, "alice") == 0);
    assert(new_user.balance == 100.0f);

    printf("✅ user registration passed\n");
}

void test_registration_duplicate()
{
    printf("\nTesting duplicate registration...\n");

    User duplicate_user;
    int result = register_user("alice", "anotherpass", &duplicate_user);

    assert(result == ERR_USER_EXISTS);
    printf("✅ duplicate registration rejected\n");
}

void test_login()
{
    printf("\nTesting user login...\n");

    Session session;
    int result = login_user("alice", "password123", &session);

    assert(result == ERR_SUCCESS);
    assert(session.user_id > 0);
    assert(strlen(session.session_token) > 0);

    printf("✅ user login passed\n");
}

void test_login_wrong_password()
{
    printf("\nTesting wrong password login...\n");

    Session session;
    int result = login_user("alice", "wrongpass", &session);

    assert(result == ERR_INVALID_CREDENTIALS);
    printf("✅ wrong password rejected\n");
}

void test_session_validation()
{
    printf("\nTesting session validation...\n");

    Session session;
    login_user("alice", "password123", &session);

    Session validated_session;
    int result = validate_session(session.session_token, &validated_session);

    assert(result == ERR_SUCCESS);
    printf("✅ session validation passed\n");
}

int main()
{
    printf("=== Authentication Tests ===\n\n");

    test_password_hashing();
    test_password_verification();
    test_session_token_generation();
    test_registration();
    test_registration_duplicate();
    test_login();
    test_login_wrong_password();
    test_session_validation();

    printf("\n=== All authentication tests passed! ===\n");

    // Cleanup
    system("rm -rf data");

    return 0;
}
