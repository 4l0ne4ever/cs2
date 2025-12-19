// protocol.c - Message Protocol Implementation

#include "../include/protocol.h"
#include <stdio.h>
#include <string.h>

// Calculate CRC32 checksum
uint32_t calculate_checksum(const char *data, int length)
{
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
    return ~crc;
}

// Validate message
int validate_message(Message *msg)
{
    if (msg->header.magic != 0xABCD)
    {
        return 0; // Invalid magic number
    }
    return 1;
}

// Note: send_message and receive_message are implemented in:
// - Server side: src/server/request_handler.c (send_response, receive_message)
// - Client side: src/client/network_client.c (send_message_to_server, receive_message_from_server)
// These are network I/O functions, not protocol-level functions.
