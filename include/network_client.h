#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include "protocol.h"

// Connect to server
int connect_to_server(const char *server_ip, int port);

// Disconnect from server
void disconnect_from_server();

// Send message to server
int send_message_to_server(Message *message);

// Receive message from server
int receive_message_from_server(Message *message);

// Check if connected
int is_connected();

// Get client file descriptor
int get_client_fd();

#endif // NETWORK_CLIENT_H

