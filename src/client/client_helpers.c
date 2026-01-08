// client_helpers.c - Shared helper functions for client modules

#include "../include/client_common.h"
#include "../include/ui.h"
#include "../include/network_client.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global state
int g_user_id = -1;
char g_session_token[37] = {0};

// Helper function to get definition_id from instance_id
int get_definition_id_from_instance(int instance_id, int *out_definition_id)
{
    if (!out_definition_id || instance_id <= 0)
        return -1;

    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_DEFINITION_ID;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", instance_id);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) != 0)
        return -1;

    if (receive_message_from_server(&response) != 0)
        return -1;

    if (response.header.msg_type == MSG_DEFINITION_ID_DATA)
    {
        memcpy(out_definition_id, response.payload, sizeof(int));
        return 0;
    }
    else if (response.header.msg_type == MSG_ERROR)
    {
        return -1;
    }

    return -1;
}

// Helper function to get price trend for a skin definition
int get_price_trend(int definition_id, PriceTrend *out_trend)
{
    if (!out_trend || definition_id <= 0)
        return -1;

    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_PRICE_TREND;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", definition_id);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) != 0)
        return -1;

    if (receive_message_from_server(&response) != 0)
        return -1;

    if (response.header.msg_type == MSG_PRICE_TREND_DATA)
    {
        memcpy(out_trend, response.payload, sizeof(PriceTrend));
        return 0;
    }
    else if (response.header.msg_type == MSG_ERROR)
    {
        return -1;
    }

    return -1;
}

// Helper function to get price history for a skin definition
int get_price_history(int definition_id, PriceHistoryEntry *out_history, int *count)
{
    if (!out_history || !count || definition_id <= 0)
        return -1;

    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_PRICE_HISTORY;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", definition_id);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) != 0)
        return -1;

    if (receive_message_from_server(&response) != 0)
        return -1;

    if (response.header.msg_type == MSG_PRICE_HISTORY_DATA)
    {
        int history_count = response.header.msg_length / sizeof(PriceHistoryEntry);
        if (history_count > 100)
            history_count = 100;

        memcpy(out_history, response.payload, sizeof(PriceHistoryEntry) * history_count);
        *count = history_count;
        return 0;
    }
    else if (response.header.msg_type == MSG_ERROR)
    {
        *count = 0;
        return -1;
    }

    *count = 0;
    return -1;
}

// Helper function to load full skin details from server
int load_skin_details(int instance_id, Skin *out_skin)
{
    if (!out_skin)
        return -1;

    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_SKIN_DETAILS;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", instance_id);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) != 0)
        return -1;

    if (receive_message_from_server(&response) != 0)
        return -1;

    if (response.header.msg_type == MSG_SKIN_DETAILS_DATA)
    {
        if (response.header.msg_length >= sizeof(Skin))
        {
            memcpy(out_skin, response.payload, sizeof(Skin));
            return 0;
        }
    }

    return -1;
}

// Helper function to get user balance
float get_user_balance(void)
{
    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_USER_PROFILE;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", g_user_id);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
    {
        if (response.header.msg_type == MSG_USER_PROFILE_DATA)
        {
            User user;
            memcpy(&user, response.payload, sizeof(User));
            return user.balance;
        }
    }
    return 0.0f;
}

// Helper function to calculate total inventory value
float calculate_inventory_value(void)
{
    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_INVENTORY;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", g_user_id);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) != 0)
        return 0.0f;

    if (receive_message_from_server(&response) != 0)
        return 0.0f;

    if (response.header.msg_type == MSG_INVENTORY_DATA)
    {
        Inventory inv;
        memcpy(&inv, response.payload, sizeof(Inventory));

        float total_value = 0.0f;
        for (int i = 0; i < inv.count && i < MAX_INVENTORY_SIZE; i++)
        {
            Skin skin;
            if (load_skin_details(inv.skin_ids[i], &skin) == 0)
            {
                total_value += skin.current_price;
            }
        }
        return total_value;
    }
    return 0.0f;
}

// Helper function to display balance info at top of screen
void display_balance_info(void)
{
    float balance = get_user_balance();
    float inv_value = calculate_inventory_value();
    float total = balance + inv_value;

    move_cursor(1, 1);
    printf("Balance: %s$%.2f%s | Inventory Value: %s$%.2f%s | Total: %s$%.2f%s\n",
           COLOR_GREEN, balance, COLOR_RESET,
           COLOR_YELLOW, inv_value, COLOR_RESET,
           COLOR_BRIGHT_GREEN, total, COLOR_RESET);
    fflush(stdout);
}

// Helper function to search user by username
int search_user_by_username(const char *username, User *out_user)
{
    if (!username || !out_user)
        return -1;

    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_SEARCH_USER_BY_USERNAME;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%s", username);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) != 0)
        return -1;

    if (receive_message_from_server(&response) != 0)
        return -1;

    if (response.header.msg_type == MSG_SEARCH_USER_RESPONSE)
    {
        if (response.header.msg_length >= sizeof(User))
        {
            memcpy(out_user, response.payload, sizeof(User));
            return 0;
        }
    }
    else if (response.header.msg_type == MSG_ERROR)
    {
        uint32_t error_code;
        memcpy(&error_code, response.payload + sizeof(uint16_t), sizeof(uint32_t));
        return (int)error_code;
    }

    return -1;
}

