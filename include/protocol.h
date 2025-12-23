#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include "types.h"

// ==================== MESSAGE HEADER ====================

typedef struct
{
    uint16_t magic;        // 0xABCD (validate message)
    uint16_t msg_type;     // Loại message
    uint32_t msg_length;   // Độ dài payload
    uint32_t sequence_num; // Số thứ tự
    uint32_t checksum;     // CRC32
} MessageHeader;

#define MAX_PAYLOAD_SIZE 4096

typedef struct
{
    MessageHeader header;
    char payload[MAX_PAYLOAD_SIZE];
} Message;

// ==================== MESSAGE TYPES ====================

// AUTHENTICATION
#define MSG_REGISTER_REQUEST 0x0001
#define MSG_REGISTER_RESPONSE 0x0002
#define MSG_LOGIN_REQUEST 0x0003
#define MSG_LOGIN_RESPONSE 0x0004
#define MSG_LOGOUT 0x0005

// MARKET
#define MSG_GET_MARKET_LISTINGS 0x0010
#define MSG_MARKET_DATA 0x0011
#define MSG_BUY_FROM_MARKET 0x0012
#define MSG_SELL_TO_MARKET 0x0013
#define MSG_REMOVE_FROM_MARKET 0x0014
#define MSG_SEARCH_MARKET_BY_NAME 0x0015
#define MSG_PRICE_UPDATE 0x0016

// TRADING
#define MSG_SEND_TRADE_OFFER 0x0020
#define MSG_TRADE_OFFER_NOTIFY 0x0021
#define MSG_ACCEPT_TRADE 0x0022
#define MSG_DECLINE_TRADE 0x0023
#define MSG_CANCEL_TRADE 0x0024
#define MSG_TRADE_COMPLETED 0x0025
#define MSG_GET_TRADES 0x0026
#define MSG_TRADES_DATA 0x0027

// INVENTORY
#define MSG_GET_INVENTORY 0x0030
#define MSG_INVENTORY_DATA 0x0031
#define MSG_GET_USER_PROFILE 0x0032
#define MSG_USER_PROFILE_DATA 0x0033
#define MSG_GET_SKIN_DETAILS 0x0034
#define MSG_SKIN_DETAILS_DATA 0x0035
#define MSG_SEARCH_USER_BY_USERNAME 0x0036
#define MSG_SEARCH_USER_RESPONSE 0x0037

// UNBOXING
#define MSG_UNBOX_CASE 0x0040
#define MSG_UNBOX_RESULT 0x0041
#define MSG_GET_CASES 0x0042
#define MSG_CASES_DATA 0x0043

// CHAT
#define MSG_CHAT_GLOBAL 0x0060

// MISC
#define MSG_HEARTBEAT 0x0090
#define MSG_ERROR 0x00FF

// ==================== ERROR CODES ====================

#define ERR_SUCCESS 0
#define ERR_INVALID_CREDENTIALS 1
#define ERR_USER_EXISTS 2
#define ERR_INSUFFICIENT_FUNDS 3
#define ERR_ITEM_NOT_FOUND 4
#define ERR_PERMISSION_DENIED 5
#define ERR_TRADE_EXPIRED 6
#define ERR_INVALID_TRADE 7
#define ERR_SESSION_EXPIRED 8
#define ERR_SERVER_FULL 9
#define ERR_BANNED 10
#define ERR_TRADE_LOCKED 11
#define ERR_INVALID_REQUEST 12
#define ERR_DATABASE_ERROR 13

// ==================== PROTOCOL FUNCTIONS ====================

// Calculate CRC32 checksum (implemented in protocol.c)
uint32_t calculate_checksum(const char *data, int length);

#endif // PROTOCOL_H
