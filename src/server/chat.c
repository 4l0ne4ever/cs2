// chat.c - Global Chat System

#include "../include/chat.h"
#include "../include/database.h"
#include "../include/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global list of connected client sockets (for broadcasting)
// This should be managed by the server's connection manager
// For now, we'll use a simple approach with a callback function
static void (*broadcast_callback)(int user_id, const char *message) = NULL;

void set_chat_broadcast_callback(void (*callback)(int user_id, const char *message))
{
    broadcast_callback = callback;
}

// Send global chat message
int send_chat_message(int user_id, const char *username, const char *message)
{
    if (user_id <= 0 || !username || !message)
        return -1;

    // Validate message length
    if (strlen(message) == 0 || strlen(message) > 255)
        return -2;

    // Save to database
    if (db_save_chat_message(user_id, username, message) != 0)
        return -3;

    // Broadcast to all connected users
    if (broadcast_callback)
    {
        char broadcast_msg[512];
        snprintf(broadcast_msg, sizeof(broadcast_msg), "%s: %s", username, message);
        broadcast_callback(user_id, broadcast_msg);
    }

    return 0;
}

// Get recent chat messages
int get_recent_chat_messages(ChatMessage *out_messages, int *count, int limit)
{
    if (!out_messages || !count)
        return -1;

    return db_load_recent_chat_messages(out_messages, count, limit);
}

// Broadcast message to all connected users (called by server)
void broadcast_chat_message(const char *username, const char *message)
{
    // This is a placeholder - actual broadcasting is handled by the server
    // through the broadcast_callback mechanism
    (void)username;
    (void)message;
}

