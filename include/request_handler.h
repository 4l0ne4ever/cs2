#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "protocol.h"
#include "types.h"

// Handle client request
// This function is called by worker threads to process incoming messages
int handle_client_request(int client_fd, Message *request);

// Send response to client
int send_response(int client_fd, Message *response);

// Receive message from client
int receive_message(int client_fd, Message *message);

// Calculate CRC32 checksum (simple implementation) - internal use only
// Note: External implementation may be in protocol.c

// Validate message header
int validate_message_header(MessageHeader *header);

#endif // REQUEST_HANDLER_H

