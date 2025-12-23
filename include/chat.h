#ifndef CHAT_H
#define CHAT_H

#include "types.h"

#define MAX_CHAT_MESSAGES 100
#define MAX_CHAT_HISTORY 50

// Send global chat message
int send_chat_message(int user_id, const char *username, const char *message);

// Get recent chat messages
int get_recent_chat_messages(ChatMessage *out_messages, int *count, int limit);

// Broadcast message to all connected users
void broadcast_chat_message(const char *username, const char *message);

#endif // CHAT_H

