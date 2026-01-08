// protocol.c - Message Protocol Implementation

#include "../include/protocol.h"
#include "../include/logger.h"
#include <stdio.h>
#include <string.h>

// Calculate CRC32 checksum
uint32_t calculate_checksum(const char *data, int length)
{
    if (!data || length < 0)
    {
        LOG_WARNING("[PROTOCOL] Invalid checksum calculation: data=%p, length=%d", data, length);
        return 0;
    }
    
    uint32_t crc = 0xFFFFFFFF;
    // Simple CRC32 implementation
    for (int i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ 0xEDB88320;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    uint32_t result = ~crc;
    LOG_DEBUG("[PROTOCOL] CRC32 checksum calculated: 0x%08X (data_length=%d)", result, length);
    return result;
}

// Validate message
int validate_message(Message *msg)
{
    if (!msg)
    {
        LOG_WARNING("[PROTOCOL] Message validation failed: null message");
        return 0;
    }
    
    if (msg->header.magic != 0xABCD)
    {
        LOG_WARNING("[PROTOCOL] Message validation failed: invalid magic number 0x%04X (expected 0xABCD)", 
                    msg->header.magic);
        return 0; // Invalid magic number
    }
    
    if (msg->header.msg_length > MAX_PAYLOAD_SIZE)
    {
        LOG_WARNING("[PROTOCOL] Message validation failed: payload too large (%d > %d)", 
                    msg->header.msg_length, MAX_PAYLOAD_SIZE);
        return 0;
    }
    
    LOG_DEBUG("[PROTOCOL] Message validation passed: magic=0x%04X, type=0x%04X, length=%d", 
              msg->header.magic, msg->header.msg_type, msg->header.msg_length);
    return 1;
}

// Note: send_message and receive_message are implemented in:
// - Server side: src/server/request_handler.c (send_response, receive_message)
// - Client side: src/client/network_client.c (send_message_to_server, receive_message_from_server)
// These are network I/O functions, not protocol-level functions.
