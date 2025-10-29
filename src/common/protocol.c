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

// TODO: Implement send_message, receive_message
