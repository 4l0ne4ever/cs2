// client_auth.c - Client Authentication Module

#include "../include/ui.h"
#include "../include/network_client.h"
#include "../include/protocol.h"
#include "../include/types.h"
#include "../include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global state (shared with main client.c)
extern int g_user_id;
extern char g_session_token[37];

int handle_login(const char *username, const char *password)
{
    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_LOGIN_REQUEST;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%s:%s", username, password);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) != 0)
    {
        print_error("Failed to send login request");
        return -1;
    }

    if (receive_message_from_server(&response) != 0)
    {
        print_error("Failed to receive login response");
        return -1;
    }

    if (response.header.msg_type == MSG_LOGIN_RESPONSE)
    {
        // Parse: "session_token:user_id"
        char payload_copy[MAX_PAYLOAD_SIZE + 1];
        size_t payload_len = response.header.msg_length;
        if (payload_len >= sizeof(payload_copy))
            payload_len = sizeof(payload_copy) - 1;
        memcpy(payload_copy, response.payload, payload_len);
        payload_copy[payload_len] = '\0';
        
        char *colon = strchr(payload_copy, ':');
        if (colon)
        {
            *colon = '\0';
            strncpy(g_session_token, payload_copy, sizeof(g_session_token) - 1);
            g_session_token[sizeof(g_session_token) - 1] = '\0';
            g_user_id = atoi(colon + 1);
        }
        else
        {
            // Fallback: old format (just session token)
            strncpy(g_session_token, (char *)response.payload, sizeof(g_session_token) - 1);
            g_session_token[sizeof(g_session_token) - 1] = '\0';
        }
        LOG_INFO("User logged in: user_id=%d", g_user_id);
        return 0;
    }
    else if (response.header.msg_type == MSG_ERROR)
    {
        uint32_t error_code;
        memcpy(&error_code, response.payload + sizeof(uint16_t), sizeof(uint32_t));
        if (error_code == ERR_INVALID_CREDENTIALS)
            print_error("Invalid username or password");
        else
            print_error("Login failed");
        return -1;
    }

    return -1;
}

int handle_register(const char *username, const char *password)
{
    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_REGISTER_REQUEST;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%s:%s", username, password);
    request.header.msg_length = strlen(request.payload);

    int send_result = send_message_to_server(&request);

    if (send_result != 0)
    {
        print_error("Failed to send register request");
        return -1;
    }

    int recv_result = receive_message_from_server(&response);

    if (recv_result != 0)
    {
        print_error("Failed to receive register response");
        return -1;
    }

    if (response.header.msg_type == MSG_REGISTER_RESPONSE)
    {
        memcpy(&g_user_id, response.payload, sizeof(uint32_t));
        print_success("Registration successful!");
        LOG_INFO("User registered: user_id=%d", g_user_id);
        return 0;
    }
    else if (response.header.msg_type == MSG_ERROR)
    {
        uint32_t error_code;
        memcpy(&error_code, response.payload + sizeof(uint16_t), sizeof(uint32_t));
        if (error_code == ERR_USER_EXISTS)
            print_error("Username already exists");
        else
            print_error("Registration failed");
        return -1;
    }

    return -1;
}

