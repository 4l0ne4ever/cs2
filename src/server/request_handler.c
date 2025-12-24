// request_handler.c - Request Handler Implementation (Phase 8)

#include "../include/request_handler.h"
#include "../include/auth.h"
#include "../include/market.h"
#include "../include/trading.h"
#include "../include/unbox.h"
#include "../include/database.h"
#include "../include/protocol.h"
#include "../include/quests.h"
#include "../include/achievements.h"
#include "../include/login_rewards.h"
#include "../include/chat.h"
#include "../include/price_tracking.h"
#include "../include/leaderboards.h"
#include "../include/trade_analytics.h"
#include "../include/trading_challenges.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#define MAGIC_NUMBER 0xABCD


// Validate message header
int validate_message_header(MessageHeader *header)
{
    if (!header)
        return 0;
    
    if (header->magic != MAGIC_NUMBER)
        return 0;
    
    if (header->msg_length > MAX_PAYLOAD_SIZE)
        return 0;
    
    return 1;
}

// Receive message from client (handles partial receives)
int receive_message(int client_fd, Message *message)
{
    if (!message || client_fd < 0)
        return -1;
    
    // Receive header (handle partial receives)
    size_t total_received = 0;
    char *header_ptr = (char *)&message->header;
    
    while (total_received < sizeof(MessageHeader))
    {
        ssize_t received = recv(client_fd, header_ptr + total_received, 
                                sizeof(MessageHeader) - total_received, 0);
        if (received <= 0)
        {
            if (received == 0) return -1; // Connection closed
            if (errno == EAGAIN || errno == EWOULDBLOCK) return -1; // Timeout
            if (errno == EINTR) continue; // Interrupted, retry
            return -1; // Error
        }
        total_received += received;
    }
    
    // Validate header
    if (!validate_message_header(&message->header))
        return -1;
    
    // Receive payload if present (handle partial receives)
    if (message->header.msg_length > 0)
    {
        if (message->header.msg_length > MAX_PAYLOAD_SIZE)
            return -1;
        
        total_received = 0;
        while (total_received < message->header.msg_length)
        {
            ssize_t received = recv(client_fd, message->payload + total_received,
                                    message->header.msg_length - total_received, 0);
            if (received <= 0)
            {
                if (received == 0) return -1; // Connection closed
                if (errno == EAGAIN || errno == EWOULDBLOCK) return -1; // Timeout
                if (errno == EINTR) continue; // Interrupted, retry
                return -1; // Error
            }
            total_received += received;
        }
        
        // Ensure null termination (safety)
        if (message->header.msg_length < MAX_PAYLOAD_SIZE)
        {
            message->payload[message->header.msg_length] = '\0';
        }
        else
        {
            message->payload[MAX_PAYLOAD_SIZE - 1] = '\0';
        }
    }
    else
    {
        message->payload[0] = '\0';
    }
    
    return 0;
}

// Send response to client (handles partial sends)
int send_response(int client_fd, Message *response)
{
    if (!response || client_fd < 0)
    {
        return -1;
    }
    
    // Calculate checksum (from protocol.c - declared in protocol.h)
    response->header.checksum = calculate_checksum((const char *)response->payload, (int)response->header.msg_length);
    
    // Send header (handle partial sends)
    size_t total_sent = 0;
    const char *header_ptr = (const char *)&response->header;
    
    while (total_sent < sizeof(MessageHeader))
    {
        ssize_t sent = send(client_fd, header_ptr + total_sent, 
                           sizeof(MessageHeader) - total_sent, 0);
        if (sent < 0)
        {
            if (errno == EINTR) continue; // Interrupted, retry
            if (errno == EAGAIN || errno == EWOULDBLOCK) return -1; // Would block
            return -1; // Error
        }
        if (sent == 0)
        {
            return -1; // Connection closed
        }
        total_sent += sent;
    }
    
    // Send payload if present (handle partial sends)
    if (response->header.msg_length > 0)
    {
        total_sent = 0;
        while (total_sent < response->header.msg_length)
        {
            ssize_t sent = send(client_fd, response->payload + total_sent,
                               response->header.msg_length - total_sent, 0);
            if (sent < 0)
            {
                if (errno == EINTR) continue; // Interrupted, retry
                if (errno == EAGAIN || errno == EWOULDBLOCK) return -1; // Would block
                return -1; // Error
            }
            if (sent == 0)
            {
                return -1; // Connection closed
            }
            total_sent += sent;
        }
    }
    
    return 0;
}

// Helper: Create error response
static void create_error_response(Message *response, uint16_t request_type, uint32_t error_code)
{
    memset(response, 0, sizeof(Message));
    response->header.magic = MAGIC_NUMBER;
    response->header.msg_type = MSG_ERROR;
    response->header.msg_length = sizeof(uint16_t) + sizeof(uint32_t);
    response->header.sequence_num = 0;
    
    // Pack error info into payload
    memcpy(response->payload, &request_type, sizeof(uint16_t));
    memcpy(response->payload + sizeof(uint16_t), &error_code, sizeof(uint32_t));
}

// Helper: Create success response
static void create_success_response(Message *response, uint16_t msg_type, const void *data, size_t data_len)
{
    memset(response, 0, sizeof(Message));
    response->header.magic = MAGIC_NUMBER;
    response->header.msg_type = msg_type;
    response->header.msg_length = data_len;
    response->header.sequence_num = 0;
    
    if (data && data_len > 0 && data_len <= MAX_PAYLOAD_SIZE)
    {
        memcpy(response->payload, data, data_len);
    }
}

// Handle authentication messages
static int handle_auth_request(int client_fd, Message *request, Message *response)
{
    switch (request->header.msg_type)
    {
    case MSG_REGISTER_REQUEST:
    {
        // Parse: username:password
        // Make a copy to avoid modifying original payload
        char payload_copy[MAX_PAYLOAD_SIZE + 1];
        size_t payload_len = request->header.msg_length;
        if (payload_len >= sizeof(payload_copy))
            payload_len = sizeof(payload_copy) - 1;
        memcpy(payload_copy, request->payload, payload_len);
        payload_copy[payload_len] = '\0';
        
        char *colon = strchr(payload_copy, ':');
        if (!colon)
        {
            create_error_response(response, MSG_REGISTER_REQUEST, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        *colon = '\0';
        char *username = payload_copy;
        char *password = colon + 1;
        
        User new_user;
        int result = register_user(username, password, &new_user);
        
        if (result == ERR_SUCCESS)
        {
            // Success response with user_id
            uint32_t user_id = new_user.user_id;
            create_success_response(response, MSG_REGISTER_RESPONSE, &user_id, sizeof(uint32_t));
        }
        else
        {
            create_error_response(response, MSG_REGISTER_REQUEST, result);
        }
        break;
    }
    
    case MSG_LOGIN_REQUEST:
    {
        // Parse: username:password
        // Make a copy to avoid modifying original payload
        char payload_copy[MAX_PAYLOAD_SIZE + 1];
        size_t payload_len = request->header.msg_length;
        if (payload_len >= sizeof(payload_copy))
            payload_len = sizeof(payload_copy) - 1;
        memcpy(payload_copy, request->payload, payload_len);
        payload_copy[payload_len] = '\0';
        
        char *colon = strchr(payload_copy, ':');
        if (!colon)
        {
            create_error_response(response, MSG_LOGIN_REQUEST, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        *colon = '\0';
        char *username = payload_copy;
        char *password = colon + 1;
        
        Session session;
        int result = login_user(username, password, &session);
        
        if (result == ERR_SUCCESS)
        {
            // Success response with session token
            create_success_response(response, MSG_LOGIN_RESPONSE, session.session_token, strlen(session.session_token));
        }
        else
        {
            create_error_response(response, MSG_LOGIN_REQUEST, result);
        }
        break;
    }
    
    case MSG_LOGOUT:
    {
        // Parse: session_token
        char *token = (char *)request->payload;
        logout_user(token);
        create_success_response(response, MSG_LOGOUT, NULL, 0);
        break;
    }
    
    default:
        create_error_response(response, request->header.msg_type, ERR_INVALID_REQUEST);
        break;
    }
    
    return send_response(client_fd, response);
}

// Handle market messages
static int handle_market_request(int client_fd, Message *request, Message *response)
{
    switch (request->header.msg_type)
    {
    case MSG_GET_MARKET_LISTINGS:
    {
        MarketListing listings[100];
        int count = 0;
        int result = get_market_listings(listings, &count);
        
        if (result == 0 && count > 0)
        {
            // Send listings data
            create_success_response(response, MSG_MARKET_DATA, listings, sizeof(MarketListing) * count);
            response->header.msg_length = sizeof(MarketListing) * count;
        }
        else
        {
            create_success_response(response, MSG_MARKET_DATA, NULL, 0);
        }
        break;
    }
    
    case MSG_BUY_FROM_MARKET:
    {
        // Parse: user_id:listing_id
        uint32_t user_id, listing_id;
        if (sscanf((char *)request->payload, "%u:%u", &user_id, &listing_id) != 2)
        {
            create_error_response(response, MSG_BUY_FROM_MARKET, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        int result = buy_from_market((int)user_id, (int)listing_id);
        if (result == 0)
        {
            create_success_response(response, MSG_BUY_FROM_MARKET, NULL, 0);
        }
        else
        {
            // Map error codes
            uint32_t error_code = ERR_INVALID_REQUEST;
            if (result == -1)
                error_code = ERR_ITEM_NOT_FOUND;
            else if (result == -2)
                error_code = ERR_ITEM_NOT_FOUND; // Already sold
            else if (result == -3)
                error_code = ERR_PERMISSION_DENIED; // Cannot buy own listing
            else if (result == -4 || result == -6)
                error_code = ERR_ITEM_NOT_FOUND; // User not found
            else if (result == -5)
                error_code = ERR_INSUFFICIENT_FUNDS;
            else if (result == -6)
                error_code = ERR_DATABASE_ERROR;
            else
                error_code = ERR_DATABASE_ERROR;
            
            create_error_response(response, MSG_BUY_FROM_MARKET, error_code);
        }
        break;
    }
    
    case MSG_SELL_TO_MARKET:
    {
        // Parse: user_id:instance_id:price
        uint32_t user_id, instance_id;
        float price;
        if (sscanf((char *)request->payload, "%u:%u:%f", &user_id, &instance_id, &price) != 3)
        {
            create_error_response(response, MSG_SELL_TO_MARKET, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        int result = list_skin_on_market((int)user_id, (int)instance_id, price);
        if (result == 0)
        {
            create_success_response(response, MSG_SELL_TO_MARKET, NULL, 0);
        }
        else
        {
            // Map error codes
            uint32_t error_code = ERR_INVALID_REQUEST;
            if (result == -1)
                error_code = ERR_ITEM_NOT_FOUND;
            else if (result == -2)
                error_code = ERR_PERMISSION_DENIED;
            else if (result == -3)
                error_code = ERR_DATABASE_ERROR;
            else if (result == -4)
                error_code = ERR_ITEM_NOT_FOUND; // User not found
            else if (result == -5)
                error_code = ERR_INSUFFICIENT_FUNDS; // Insufficient funds for listing fee
            else if (result == -6)
                error_code = ERR_DATABASE_ERROR; // Failed to update balance
            else if (result == -7)
                error_code = ERR_INVALID_TRADE; // Item is in a pending trade
            
            create_error_response(response, MSG_SELL_TO_MARKET, error_code);
        }
        break;
    }
    
    case MSG_REMOVE_FROM_MARKET:
    {
        // Parse: user_id:listing_id
        uint32_t user_id, listing_id;
        if (sscanf((char *)request->payload, "%u:%u", &user_id, &listing_id) != 2)
        {
            create_error_response(response, MSG_REMOVE_FROM_MARKET, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        // Verify listing belongs to user
        int seller_id, instance_id, is_sold;
        float price;
        if (db_get_listing_v2((int)listing_id, &seller_id, &instance_id, &price, &is_sold) != 0)
        {
            create_error_response(response, MSG_REMOVE_FROM_MARKET, ERR_ITEM_NOT_FOUND);
            return send_response(client_fd, response);
        }
        
        if (seller_id != (int)user_id)
        {
            create_error_response(response, MSG_REMOVE_FROM_MARKET, ERR_PERMISSION_DENIED);
            return send_response(client_fd, response);
        }
        
        if (is_sold)
        {
            create_error_response(response, MSG_REMOVE_FROM_MARKET, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        int result = remove_listing((int)listing_id);
        if (result == 0)
        {
            create_success_response(response, MSG_REMOVE_FROM_MARKET, NULL, 0);
        }
        else
        {
            create_error_response(response, MSG_REMOVE_FROM_MARKET, result);
        }
        break;
    }
    
    case MSG_SEARCH_MARKET_BY_NAME:
    {
        // Parse: search_term (skin name)
        char search_term[256];
        if (sscanf((char *)request->payload, "%255s", search_term) != 1)
        {
            create_error_response(response, MSG_SEARCH_MARKET_BY_NAME, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        MarketListing listings[100];
        int count = 0;
        int result = search_market_listings_by_name(search_term, listings, &count);
        
        if (result == 0 && count > 0)
        {
            // Send search results
            create_success_response(response, MSG_MARKET_DATA, listings, sizeof(MarketListing) * count);
            response->header.msg_length = sizeof(MarketListing) * count;
        }
        else
        {
            // No results found
            create_success_response(response, MSG_MARKET_DATA, NULL, 0);
        }
        break;
    }
    
    case MSG_GET_PRICE_HISTORY:
    {
        // Parse: definition_id
        uint32_t definition_id;
        if (sscanf((char *)request->payload, "%u", &definition_id) != 1)
        {
            create_error_response(response, MSG_GET_PRICE_HISTORY, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        PriceHistoryEntry history[100];
        int count = 0;
        int result = get_price_history_24h((int)definition_id, history, &count);
        
        if (result == 0 && count > 0)
        {
            // Send price history data
            create_success_response(response, MSG_PRICE_HISTORY_DATA, history, sizeof(PriceHistoryEntry) * count);
            response->header.msg_length = sizeof(PriceHistoryEntry) * count;
        }
        else
        {
            // No history found
            create_success_response(response, MSG_PRICE_HISTORY_DATA, NULL, 0);
        }
        break;
    }
    
    case MSG_GET_PRICE_TREND:
    {
        // Parse: definition_id
        uint32_t definition_id;
        if (sscanf((char *)request->payload, "%u", &definition_id) != 1)
        {
            create_error_response(response, MSG_GET_PRICE_TREND, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        PriceTrend trend;
        int result = get_price_trend((int)definition_id, &trend);
        
        if (result == 0)
        {
            // Send price trend data
            create_success_response(response, MSG_PRICE_TREND_DATA, &trend, sizeof(PriceTrend));
            response->header.msg_length = sizeof(PriceTrend);
        }
        else
        {
            // No trend data available
            create_error_response(response, MSG_GET_PRICE_TREND, ERR_ITEM_NOT_FOUND);
        }
        break;
    }
    
    default:
        create_error_response(response, request->header.msg_type, ERR_INVALID_REQUEST);
        break;
    }
    
    return send_response(client_fd, response);
}

// Handle trading messages
static int handle_trading_request(int client_fd, Message *request, Message *response)
{
    switch (request->header.msg_type)
    {
    case MSG_SEND_TRADE_OFFER:
    {
        // Parse TradeOffer struct directly from payload
        // Client sends the entire TradeOffer struct as binary data
        if (request->header.msg_length < sizeof(TradeOffer))
        {
            create_error_response(response, MSG_SEND_TRADE_OFFER, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        TradeOffer offer;
        memcpy(&offer, request->payload, sizeof(TradeOffer));
        
        // Validate offer structure
        if (offer.from_user_id <= 0 || offer.to_user_id <= 0 || 
            offer.from_user_id == offer.to_user_id ||
            offer.offered_count < 0 || offer.offered_count > 10 ||
            offer.requested_count < 0 || offer.requested_count > 10 ||
            offer.offered_cash < 0 || offer.requested_cash < 0)
        {
            create_error_response(response, MSG_SEND_TRADE_OFFER, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        // Send trade offer
        int result = send_trade_offer(offer.from_user_id, offer.to_user_id, &offer);
        if (result == 0)
        {
            // Return the created trade offer with trade_id
            create_success_response(response, MSG_SEND_TRADE_OFFER, &offer, sizeof(TradeOffer));
            response->header.msg_length = sizeof(TradeOffer);
        }
        else
        {
            // Map error codes
            uint32_t error_code = ERR_INVALID_REQUEST;
            if (result == -1)
                error_code = ERR_INVALID_REQUEST;
            else if (result == -2)
                error_code = ERR_INVALID_TRADE;
            else if (result == -3)
                error_code = ERR_DATABASE_ERROR;
            else if (result == -4 || result == -7)
                error_code = ERR_TRADE_LOCKED;
            else if (result == -5 || result == -6)
                error_code = ERR_PERMISSION_DENIED;
            else if (result == -8 || result == -9 || result == -10 || result == -11)
                error_code = ERR_INSUFFICIENT_FUNDS;
            else if (result == -12)
                error_code = ERR_INVALID_TRADE; // Empty trade
            else if (result == -13 || result == -14)
                error_code = ERR_INVALID_TRADE; // Item already in pending trade
            
            create_error_response(response, MSG_SEND_TRADE_OFFER, error_code);
        }
        break;
    }
    
    case MSG_ACCEPT_TRADE:
    {
        uint32_t user_id, trade_id;
        if (sscanf((char *)request->payload, "%u:%u", &user_id, &trade_id) != 2)
        {
            create_error_response(response, MSG_ACCEPT_TRADE, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        int result = accept_trade((int)user_id, (int)trade_id);
        if (result == 0)
        {
            create_success_response(response, MSG_TRADE_COMPLETED, NULL, 0);
        }
        else
        {
            create_error_response(response, MSG_ACCEPT_TRADE, result);
        }
        break;
    }
    
    case MSG_DECLINE_TRADE:
    {
        uint32_t user_id, trade_id;
        if (sscanf((char *)request->payload, "%u:%u", &user_id, &trade_id) != 2)
        {
            create_error_response(response, MSG_DECLINE_TRADE, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        decline_trade((int)user_id, (int)trade_id);
        create_success_response(response, MSG_DECLINE_TRADE, NULL, 0);
        break;
    }
    
    case MSG_CANCEL_TRADE:
    {
        uint32_t user_id, trade_id;
        if (sscanf((char *)request->payload, "%u:%u", &user_id, &trade_id) != 2)
        {
            create_error_response(response, MSG_CANCEL_TRADE, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        cancel_trade((int)user_id, (int)trade_id);
        create_success_response(response, MSG_CANCEL_TRADE, NULL, 0);
        break;
    }
    
    case MSG_GET_TRADES:
    {
        uint32_t user_id;
        if (sscanf((char *)request->payload, "%u", &user_id) != 1)
        {
            create_error_response(response, MSG_GET_TRADES, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        TradeOffer trades[50];
        int count = 0;
        int result = get_user_trades((int)user_id, trades, &count);
        
        if (result == 0 && count > 0)
        {
            if (count > 50) count = 50;
            create_success_response(response, MSG_TRADES_DATA, trades, sizeof(TradeOffer) * count);
            response->header.msg_length = sizeof(TradeOffer) * count;
        }
        else
        {
            create_success_response(response, MSG_TRADES_DATA, NULL, 0);
        }
        break;
    }
    
    default:
        create_error_response(response, request->header.msg_type, ERR_INVALID_REQUEST);
        break;
    }
    
    return send_response(client_fd, response);
}

// Map unbox_case() error codes (negative) to protocol error codes (positive)
static uint32_t map_unbox_error(int unbox_result)
{
    switch (unbox_result)
    {
    case 0:
        return ERR_SUCCESS;
    case -1:
        return ERR_INVALID_REQUEST; // Invalid params
    case -2:
        return ERR_ITEM_NOT_FOUND; // Case not found
    case -3:
        return ERR_ITEM_NOT_FOUND; // User not found
    case -4:
        return ERR_DATABASE_ERROR; // Failed to update balance
    case -5:
        return ERR_DATABASE_ERROR; // No skins with this rarity available
    case -6:
        return ERR_DATABASE_ERROR; // Failed to load skin definition or create instance
    case -7:
        return ERR_DATABASE_ERROR; // Failed to add to inventory
    default:
        // For known protocol error codes (positive), return as-is
        if (unbox_result > 0 && unbox_result <= 13)
            return (uint32_t)unbox_result;
        // For unknown negative codes, map to generic database error
        return ERR_DATABASE_ERROR;
    }
}

// Handle unbox messages
static int handle_unbox_request(int client_fd, Message *request, Message *response)
{
    switch (request->header.msg_type)
    {
    case MSG_GET_CASES:
    {
        Case cases[50];
        int count = 0;
        int result = get_available_cases(cases, &count);
        
        if (result == 0 && count > 0)
        {
            create_success_response(response, MSG_CASES_DATA, cases, sizeof(Case) * count);
            response->header.msg_length = sizeof(Case) * count;
        }
        else
        {
            create_success_response(response, MSG_CASES_DATA, NULL, 0);
        }
        break;
    }
    
    case MSG_UNBOX_CASE:
    {
        // Parse: user_id:case_id
        uint32_t user_id, case_id;
        int parse_result = sscanf((char *)request->payload, "%u:%u", &user_id, &case_id);
        
        if (parse_result != 2)
        {
            create_error_response(response, MSG_UNBOX_CASE, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        Skin unboxed;
        int result = unbox_case((int)user_id, (int)case_id, &unboxed);
        
        if (result == 0)
        {
            create_success_response(response, MSG_UNBOX_RESULT, &unboxed, sizeof(Skin));
            response->header.msg_length = sizeof(Skin);
        }
        else
        {
            uint32_t mapped_error = map_unbox_error(result);
            create_error_response(response, MSG_UNBOX_CASE, mapped_error);
        }
        break;
    }
    
    default:
        create_error_response(response, request->header.msg_type, ERR_INVALID_REQUEST);
        break;
    }
    
    return send_response(client_fd, response);
}

// Handle inventory messages
static int handle_inventory_request(int client_fd, Message *request, Message *response)
{
    switch (request->header.msg_type)
    {
    case MSG_GET_INVENTORY:
    {
        // Parse: user_id
        uint32_t user_id;
        if (sscanf((char *)request->payload, "%u", &user_id) != 1)
        {
            create_error_response(response, MSG_GET_INVENTORY, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        Inventory inv;
        int result = db_load_inventory((int)user_id, &inv);
        
        if (result == 0)
        {
            create_success_response(response, MSG_INVENTORY_DATA, &inv, sizeof(Inventory));
            response->header.msg_length = sizeof(Inventory);
        }
        else
        {
            create_error_response(response, MSG_GET_INVENTORY, ERR_ITEM_NOT_FOUND);
        }
        break;
    }
    
    case MSG_GET_USER_PROFILE:
    {
        // Parse: user_id
        uint32_t user_id;
        if (sscanf((char *)request->payload, "%u", &user_id) != 1)
        {
            create_error_response(response, MSG_GET_USER_PROFILE, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        User user;
        int result = db_load_user((int)user_id, &user);
        
        if (result == 0)
        {
            create_success_response(response, MSG_USER_PROFILE_DATA, &user, sizeof(User));
            response->header.msg_length = sizeof(User);
        }
        else
        {
            create_error_response(response, MSG_GET_USER_PROFILE, ERR_ITEM_NOT_FOUND);
        }
        break;
    }
    
    case MSG_SEARCH_USER_BY_USERNAME:
    {
        // Parse: username
        char username[MAX_USERNAME_LEN];
        if (sscanf((char *)request->payload, "%31s", username) != 1)
        {
            create_error_response(response, MSG_SEARCH_USER_BY_USERNAME, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        User user;
        int result = db_load_user_by_username(username, &user);
        
        if (result == 0)
        {
            create_success_response(response, MSG_SEARCH_USER_RESPONSE, &user, sizeof(User));
            response->header.msg_length = sizeof(User);
        }
        else
        {
            create_error_response(response, MSG_SEARCH_USER_BY_USERNAME, ERR_ITEM_NOT_FOUND);
        }
        break;
    }
    
    case MSG_GET_SKIN_DETAILS:
    {
        // Parse: instance_id
        uint32_t instance_id;
        if (sscanf((char *)request->payload, "%u", &instance_id) != 1)
        {
            create_error_response(response, MSG_GET_SKIN_DETAILS, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        // Load skin instance
        int definition_id;
        SkinRarity rarity;
        WearCondition wear;
        int pattern_seed;
        int is_stattrak;
        int owner_id;
        time_t acquired_at;
        int is_tradable;
        
        if (db_load_skin_instance((int)instance_id, &definition_id, &rarity, &wear, &pattern_seed, &is_stattrak, &owner_id, &acquired_at, &is_tradable) != 0)
        {
            create_error_response(response, MSG_GET_SKIN_DETAILS, ERR_ITEM_NOT_FOUND);
            return send_response(client_fd, response);
        }
        
        // Load skin definition
        char def_name[MAX_ITEM_NAME_LEN];
        float def_base_price;
        if (db_load_skin_definition_with_rarity(definition_id, def_name, &def_base_price, &rarity) != 0)
        {
            create_error_response(response, MSG_GET_SKIN_DETAILS, ERR_ITEM_NOT_FOUND);
            return send_response(client_fd, response);
        }
        
        // Calculate price
        float current_price = db_calculate_skin_price(definition_id, rarity, wear);
        
        // Construct full Skin object
        Skin skin;
        memset(&skin, 0, sizeof(Skin));
        skin.skin_id = (int)instance_id;
        strncpy(skin.name, def_name, MAX_ITEM_NAME_LEN);
        skin.rarity = rarity;
        skin.wear = wear;
        skin.pattern_seed = pattern_seed;
        skin.is_stattrak = is_stattrak;
        
        float rarity_mult = 1.0f;
        db_get_rarity_multiplier(rarity, &rarity_mult);
        skin.base_price = def_base_price * rarity_mult;
        
        skin.current_price = current_price;
        skin.owner_id = owner_id;
        skin.acquired_at = acquired_at;
        skin.is_tradable = is_tradable;
        
        create_success_response(response, MSG_SKIN_DETAILS_DATA, &skin, sizeof(Skin));
        response->header.msg_length = sizeof(Skin);
        break;
    }
    
    default:
        create_error_response(response, request->header.msg_type, ERR_INVALID_REQUEST);
        break;
    }
    
    return send_response(client_fd, response);
}

// Handle quests and achievements messages
static int handle_quests_request(int client_fd, Message *request, Message *response)
{
    switch (request->header.msg_type)
    {
    case MSG_GET_QUESTS:
    {
        uint32_t user_id;
        if (sscanf((char *)request->payload, "%u", &user_id) != 1)
        {
            create_error_response(response, MSG_GET_QUESTS, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        Quest quests[10];
        int count = 0;
        if (get_user_quests((int)user_id, quests, &count) == 0)
        {
            create_success_response(response, MSG_QUESTS_DATA, quests, sizeof(Quest) * count);
            response->header.msg_length = sizeof(Quest) * count;
        }
        else
        {
            create_success_response(response, MSG_QUESTS_DATA, NULL, 0);
        }
        break;
    }
    
    case MSG_CLAIM_QUEST_REWARD:
    {
        uint32_t user_id, quest_id;
        if (sscanf((char *)request->payload, "%u:%u", &user_id, &quest_id) != 2)
        {
            create_error_response(response, MSG_CLAIM_QUEST_REWARD, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        if (claim_quest_reward((int)user_id, (int)quest_id) == 0)
        {
            create_success_response(response, MSG_CLAIM_QUEST_REWARD, NULL, 0);
        }
        else
        {
            create_error_response(response, MSG_CLAIM_QUEST_REWARD, ERR_INVALID_REQUEST);
        }
        break;
    }
    
    case MSG_GET_ACHIEVEMENTS:
    {
        uint32_t user_id;
        if (sscanf((char *)request->payload, "%u", &user_id) != 1)
        {
            create_error_response(response, MSG_GET_ACHIEVEMENTS, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        Achievement achievements[10];
        int count = 0;
        if (get_user_achievements((int)user_id, achievements, &count) == 0)
        {
            create_success_response(response, MSG_ACHIEVEMENTS_DATA, achievements, sizeof(Achievement) * count);
            response->header.msg_length = sizeof(Achievement) * count;
        }
        else
        {
            create_success_response(response, MSG_ACHIEVEMENTS_DATA, NULL, 0);
        }
        break;
    }
    
    case MSG_CLAIM_ACHIEVEMENT:
    {
        uint32_t user_id, achievement_id;
        if (sscanf((char *)request->payload, "%u:%u", &user_id, &achievement_id) != 2)
        {
            create_error_response(response, MSG_CLAIM_ACHIEVEMENT, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        if (claim_achievement_reward((int)user_id, (int)achievement_id) == 0)
        {
            create_success_response(response, MSG_CLAIM_ACHIEVEMENT, NULL, 0);
        }
        else
        {
            create_error_response(response, MSG_CLAIM_ACHIEVEMENT, ERR_INVALID_REQUEST);
        }
        break;
    }
    
    case MSG_GET_LOGIN_REWARD:
    {
        uint32_t user_id;
        if (sscanf((char *)request->payload, "%u", &user_id) != 1)
        {
            create_error_response(response, MSG_GET_LOGIN_REWARD, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        float reward_amount = 0.0f;
        int streak_day = 0;
        int result = claim_daily_reward((int)user_id, &reward_amount, &streak_day);
        
        if (result == 0)
        {
            char reward_data[64];
            snprintf(reward_data, sizeof(reward_data), "%.2f:%d", reward_amount, streak_day);
            create_success_response(response, MSG_LOGIN_REWARD_DATA, reward_data, strlen(reward_data));
        }
        else if (result == -2)
        {
            // Already claimed today
            create_error_response(response, MSG_GET_LOGIN_REWARD, ERR_INVALID_REQUEST);
        }
        else
        {
            create_error_response(response, MSG_GET_LOGIN_REWARD, ERR_DATABASE_ERROR);
        }
        break;
    }
    
    default:
        create_error_response(response, request->header.msg_type, ERR_INVALID_REQUEST);
        break;
    }
    
    return send_response(client_fd, response);
}

// Handle chat messages
static int handle_chat_request(int client_fd, Message *request, Message *response)
{
    switch (request->header.msg_type)
    {
    case MSG_CHAT_GLOBAL:
    {
        // Parse: user_id:username:message
        uint32_t user_id;
        char username[MAX_USERNAME_LEN];
        char message[256];
        int parse_result = sscanf((char *)request->payload, "%u:%31[^:]:%255[^\n]", &user_id, username, message);
        
        if (parse_result != 3)
        {
            create_error_response(response, MSG_CHAT_GLOBAL, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        if (send_chat_message((int)user_id, username, message) == 0)
        {
            create_success_response(response, MSG_CHAT_GLOBAL, NULL, 0);
        }
        else
        {
            create_error_response(response, MSG_CHAT_GLOBAL, ERR_DATABASE_ERROR);
        }
        break;
    }
    
    case MSG_GET_CHAT_HISTORY:
    {
        // Parse: limit (optional, default 50)
        int limit = 50;
        if (request->header.msg_length > 0)
        {
            limit = atoi((char *)request->payload);
            if (limit <= 0 || limit > 100)
                limit = 50;
        }
        
        ChatMessage messages[100];
        int count = 0;
        if (get_recent_chat_messages(messages, &count, limit) == 0)
        {
            create_success_response(response, MSG_CHAT_HISTORY_DATA, messages, sizeof(ChatMessage) * count);
            response->header.msg_length = sizeof(ChatMessage) * count;
        }
        else
        {
            create_error_response(response, MSG_GET_CHAT_HISTORY, ERR_DATABASE_ERROR);
        }
        break;
    }
    
    default:
        create_error_response(response, request->header.msg_type, ERR_INVALID_REQUEST);
        break;
    }
    
    return send_response(client_fd, response);
}

// Handle leaderboards messages
static int handle_leaderboards_request(int client_fd, Message *request, Message *response)
{
    switch (request->header.msg_type)
    {
    case MSG_GET_TOP_TRADERS:
    {
        // Parse: limit (optional, default 10)
        int limit = 10;
        if (request->header.msg_length > 0)
        {
            sscanf((char *)request->payload, "%d", &limit);
        }
        if (limit <= 0 || limit > 100)
            limit = 10;
        
        LeaderboardEntry entries[100];
        int count = 0;
        int result = get_top_traders(entries, &count, limit);
        
        if (result == 0 && count > 0)
        {
            create_success_response(response, MSG_TOP_TRADERS_DATA, entries, sizeof(LeaderboardEntry) * count);
            response->header.msg_length = sizeof(LeaderboardEntry) * count;
        }
        else
        {
            create_success_response(response, MSG_TOP_TRADERS_DATA, NULL, 0);
        }
        break;
    }
    
    case MSG_GET_LUCKIEST_UNBOXERS:
    {
        // Parse: limit (optional, default 10)
        int limit = 10;
        if (request->header.msg_length > 0)
        {
            sscanf((char *)request->payload, "%d", &limit);
        }
        if (limit <= 0 || limit > 100)
            limit = 10;
        
        LeaderboardEntry entries[100];
        int count = 0;
        int result = get_luckiest_unboxers(entries, &count, limit);
        
        if (result == 0 && count > 0)
        {
            create_success_response(response, MSG_LUCKIEST_UNBOXERS_DATA, entries, sizeof(LeaderboardEntry) * count);
            response->header.msg_length = sizeof(LeaderboardEntry) * count;
        }
        else
        {
            create_success_response(response, MSG_LUCKIEST_UNBOXERS_DATA, NULL, 0);
        }
        break;
    }
    
    case MSG_GET_MOST_PROFITABLE:
    {
        // Parse: limit (optional, default 10)
        int limit = 10;
        if (request->header.msg_length > 0)
        {
            sscanf((char *)request->payload, "%d", &limit);
        }
        if (limit <= 0 || limit > 100)
            limit = 10;
        
        LeaderboardEntry entries[100];
        int count = 0;
        int result = get_most_profitable(entries, &count, limit);
        
        if (result == 0 && count > 0)
        {
            create_success_response(response, MSG_MOST_PROFITABLE_DATA, entries, sizeof(LeaderboardEntry) * count);
            response->header.msg_length = sizeof(LeaderboardEntry) * count;
        }
        else
        {
            create_success_response(response, MSG_MOST_PROFITABLE_DATA, NULL, 0);
        }
        break;
    }
    
    default:
        create_error_response(response, request->header.msg_type, ERR_INVALID_REQUEST);
        break;
    }
    
    return send_response(client_fd, response);
}

// Handle trade analytics messages
static int handle_trade_analytics_request(int client_fd, Message *request, Message *response)
{
    switch (request->header.msg_type)
    {
    case MSG_GET_TRADE_HISTORY:
    {
        // Parse: user_id:limit
        uint32_t user_id;
        int limit = 50;
        if (sscanf((char *)request->payload, "%u:%d", &user_id, &limit) < 1)
        {
            create_error_response(response, MSG_GET_TRADE_HISTORY, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        if (limit <= 0 || limit > 100)
            limit = 50;
        
        TransactionLog logs[100];
        int count = 0;
        int result = get_trade_history((int)user_id, logs, &count, limit);
        
        if (result == 0 && count > 0)
        {
            create_success_response(response, MSG_TRADE_HISTORY_DATA, logs, sizeof(TransactionLog) * count);
            response->header.msg_length = sizeof(TransactionLog) * count;
        }
        else
        {
            create_success_response(response, MSG_TRADE_HISTORY_DATA, NULL, 0);
        }
        break;
    }
    
    case MSG_GET_TRADE_STATS:
    {
        // Parse: user_id
        uint32_t user_id;
        if (sscanf((char *)request->payload, "%u", &user_id) != 1)
        {
            create_error_response(response, MSG_GET_TRADE_STATS, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        TradeStats stats;
        int result = calculate_trade_stats((int)user_id, &stats);
        
        if (result == 0)
        {
            create_success_response(response, MSG_TRADE_STATS_DATA, &stats, sizeof(TradeStats));
            response->header.msg_length = sizeof(TradeStats);
        }
        else
        {
            create_error_response(response, MSG_GET_TRADE_STATS, ERR_DATABASE_ERROR);
        }
        break;
    }
    
    case MSG_GET_BALANCE_HISTORY:
    {
        // Parse: user_id:days
        uint32_t user_id;
        int days = 7;
        if (sscanf((char *)request->payload, "%u:%d", &user_id, &days) < 1)
        {
            create_error_response(response, MSG_GET_BALANCE_HISTORY, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        if (days <= 0 || days > 30)
            days = 7;
        
        BalanceHistoryEntry history[30];
        int count = 0;
        int result = get_balance_history((int)user_id, history, &count, days);
        
        if (result == 0 && count > 0)
        {
            create_success_response(response, MSG_BALANCE_HISTORY_DATA, history, sizeof(BalanceHistoryEntry) * count);
            response->header.msg_length = sizeof(BalanceHistoryEntry) * count;
        }
        else
        {
            create_success_response(response, MSG_BALANCE_HISTORY_DATA, NULL, 0);
        }
        break;
    }
    
    default:
        create_error_response(response, request->header.msg_type, ERR_INVALID_REQUEST);
        break;
    }
    
    return send_response(client_fd, response);
}

// Handle trading challenges messages
static int handle_trading_challenges_request(int client_fd, Message *request, Message *response)
{
    switch (request->header.msg_type)
    {
    case MSG_CREATE_CHALLENGE:
    {
        // Parse: challenger_id:opponent_id:duration_minutes
        uint32_t challenger_id, opponent_id;
        int duration_minutes;
        if (sscanf((char *)request->payload, "%u:%u:%d", &challenger_id, &opponent_id, &duration_minutes) != 3)
        {
            create_error_response(response, MSG_CREATE_CHALLENGE, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        int challenge_id = 0;
        int result = create_profit_race_challenge((int)challenger_id, (int)opponent_id, duration_minutes, &challenge_id);
        
        if (result == 0)
        {
            create_success_response(response, MSG_CREATE_CHALLENGE_RESPONSE, &challenge_id, sizeof(int));
            response->header.msg_length = sizeof(int);
        }
        else
        {
            uint32_t error_code = ERR_INVALID_REQUEST;
            if (result == -2)
                error_code = ERR_INVALID_REQUEST; // Cannot challenge yourself
            else if (result == -3 || result == -4)
                error_code = ERR_ITEM_NOT_FOUND; // User not found
            else
                error_code = ERR_DATABASE_ERROR;
            create_error_response(response, MSG_CREATE_CHALLENGE, error_code);
        }
        break;
    }
    
    case MSG_GET_USER_CHALLENGES:
    {
        // Parse: user_id
        uint32_t user_id;
        if (sscanf((char *)request->payload, "%u", &user_id) != 1)
        {
            create_error_response(response, MSG_GET_USER_CHALLENGES, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        TradingChallenge challenges[50];
        int count = 0;
        int result = get_user_challenges((int)user_id, challenges, &count);
        
        if (result == 0 && count > 0)
        {
            create_success_response(response, MSG_USER_CHALLENGES_DATA, challenges, sizeof(TradingChallenge) * count);
            response->header.msg_length = sizeof(TradingChallenge) * count;
        }
        else
        {
            create_success_response(response, MSG_USER_CHALLENGES_DATA, NULL, 0);
        }
        break;
    }
    
    case MSG_ACCEPT_CHALLENGE:
    {
        // Parse: challenge_id:user_id
        uint32_t challenge_id, user_id;
        if (sscanf((char *)request->payload, "%u:%u", &challenge_id, &user_id) != 2)
        {
            create_error_response(response, MSG_ACCEPT_CHALLENGE, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        int result = accept_challenge((int)challenge_id, (int)user_id);
        
        if (result == 0)
        {
            create_success_response(response, MSG_ACCEPT_CHALLENGE_RESPONSE, NULL, 0);
        }
        else
        {
            uint32_t error_code = ERR_INVALID_REQUEST;
            if (result == -1)
                error_code = ERR_ITEM_NOT_FOUND; // Challenge not found
            else if (result == -2)
                error_code = ERR_PERMISSION_DENIED; // Not authorized
            else if (result == -3)
                error_code = ERR_INVALID_REQUEST; // Challenge not pending
            else
                error_code = ERR_DATABASE_ERROR;
            create_error_response(response, MSG_ACCEPT_CHALLENGE, error_code);
        }
        break;
    }
    
    case MSG_UPDATE_CHALLENGE_PROGRESS:
    {
        // Parse: challenge_id
        uint32_t challenge_id;
        if (sscanf((char *)request->payload, "%u", &challenge_id) != 1)
        {
            create_error_response(response, MSG_UPDATE_CHALLENGE_PROGRESS, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        int result = update_challenge_progress((int)challenge_id);
        
        if (result == 0)
        {
            TradingChallenge challenge;
            if (get_challenge((int)challenge_id, &challenge) == 0)
            {
                create_success_response(response, MSG_UPDATE_CHALLENGE_PROGRESS_RESPONSE, &challenge, sizeof(TradingChallenge));
                response->header.msg_length = sizeof(TradingChallenge);
            }
            else
            {
                create_error_response(response, MSG_UPDATE_CHALLENGE_PROGRESS, ERR_DATABASE_ERROR);
            }
        }
        else
        {
            create_error_response(response, MSG_UPDATE_CHALLENGE_PROGRESS, ERR_DATABASE_ERROR);
        }
        break;
    }
    
    case MSG_COMPLETE_CHALLENGE:
    {
        // Parse: challenge_id
        uint32_t challenge_id;
        if (sscanf((char *)request->payload, "%u", &challenge_id) != 1)
        {
            create_error_response(response, MSG_COMPLETE_CHALLENGE, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        int winner_id = 0;
        int result = complete_challenge((int)challenge_id, &winner_id);
        
        if (result == 0)
        {
            create_success_response(response, MSG_COMPLETE_CHALLENGE_RESPONSE, &winner_id, sizeof(int));
            response->header.msg_length = sizeof(int);
        }
        else
        {
            create_error_response(response, MSG_COMPLETE_CHALLENGE, ERR_DATABASE_ERROR);
        }
        break;
    }
    
    case MSG_CANCEL_CHALLENGE:
    {
        // Parse: challenge_id:user_id
        uint32_t challenge_id, user_id;
        if (sscanf((char *)request->payload, "%u:%u", &challenge_id, &user_id) != 2)
        {
            create_error_response(response, MSG_CANCEL_CHALLENGE, ERR_INVALID_REQUEST);
            return send_response(client_fd, response);
        }
        
        int result = cancel_challenge((int)challenge_id, (int)user_id);
        
        if (result == 0)
        {
            create_success_response(response, MSG_CANCEL_CHALLENGE_RESPONSE, NULL, 0);
        }
        else
        {
            uint32_t error_code = ERR_INVALID_REQUEST;
            if (result == -1)
                error_code = ERR_ITEM_NOT_FOUND; // Challenge not found
            else if (result == -2)
                error_code = ERR_PERMISSION_DENIED; // Not authorized
            else if (result == -3)
                error_code = ERR_INVALID_REQUEST; // Cannot cancel completed challenge
            else
                error_code = ERR_DATABASE_ERROR;
            create_error_response(response, MSG_CANCEL_CHALLENGE, error_code);
        }
        break;
    }
    
    default:
        create_error_response(response, request->header.msg_type, ERR_INVALID_REQUEST);
        break;
    }
    
    return send_response(client_fd, response);
}

// Main request handler - routes to appropriate handler
int handle_client_request(int client_fd, Message *request)
{
    if (!request || client_fd < 0)
        return -1;
    
    Message response;
    
    // Route based on message type
    uint16_t msg_type = request->header.msg_type;
    
    if (msg_type >= MSG_REGISTER_REQUEST && msg_type <= MSG_LOGOUT)
    {
        handle_auth_request(client_fd, request, &response);
    }
    else if (msg_type >= MSG_GET_MARKET_LISTINGS && msg_type <= MSG_PRICE_TREND_DATA)
    {
        handle_market_request(client_fd, request, &response);
    }
    else if (msg_type >= MSG_SEND_TRADE_OFFER && msg_type <= MSG_TRADES_DATA)
    {
        handle_trading_request(client_fd, request, &response);
    }
    else if (msg_type >= MSG_GET_INVENTORY && msg_type <= MSG_DEFINITION_ID_DATA)
    {
        handle_inventory_request(client_fd, request, &response);
    }
    else if (msg_type >= MSG_UNBOX_CASE && msg_type <= MSG_CASES_DATA)
    {
        handle_unbox_request(client_fd, request, &response);
    }
    else if (msg_type >= MSG_CHAT_GLOBAL && msg_type <= MSG_CHAT_HISTORY_DATA)
    {
        handle_chat_request(client_fd, request, &response);
    }
    else if (msg_type >= MSG_GET_QUESTS && msg_type <= MSG_LOGIN_REWARD_DATA)
    {
        handle_quests_request(client_fd, request, &response);
    }
    else if (msg_type >= MSG_GET_TOP_TRADERS && msg_type <= MSG_MOST_PROFITABLE_DATA)
    {
        handle_leaderboards_request(client_fd, request, &response);
    }
    else if (msg_type >= MSG_GET_TRADE_HISTORY && msg_type <= MSG_BALANCE_HISTORY_DATA)
    {
        handle_trade_analytics_request(client_fd, request, &response);
    }
    else if (msg_type >= MSG_CREATE_CHALLENGE && msg_type <= MSG_CANCEL_CHALLENGE_RESPONSE)
    {
        handle_trading_challenges_request(client_fd, request, &response);
    }
    else if (msg_type == MSG_HEARTBEAT)
    {
        // Heartbeat response
        create_success_response(&response, MSG_HEARTBEAT, NULL, 0);
        send_response(client_fd, &response);
    }
    else
    {
        create_error_response(&response, msg_type, ERR_INVALID_REQUEST);
        send_response(client_fd, &response);
    }
    
    return 0;
}
