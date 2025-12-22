// network_client.c - Network Client Implementation (Phase 9)

#include "../include/protocol.h"
#include "../include/network_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>


static int g_client_fd = -1;

// Connect to server
int connect_to_server(const char *server_ip, int port)
{
    if (g_client_fd >= 0)
    {
        close(g_client_fd);
        g_client_fd = -1;
    }
    
    // Create socket
    g_client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_client_fd < 0)
    {
        perror("socket");
        return -1;
    }
    
    // Setup server address - support both IP addresses and hostnames
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // Try to parse as IP address first
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        // If not an IP, try to resolve as hostname
        struct hostent *host_entry = gethostbyname(server_ip);
        if (host_entry == NULL)
        {
            fprintf(stderr, "Invalid address or hostname: %s\n", server_ip);
            close(g_client_fd);
            g_client_fd = -1;
            return -1;
        }
        
        // Copy resolved IP address
        memcpy(&server_addr.sin_addr, host_entry->h_addr_list[0], host_entry->h_length);
    }
    
    // Connect
    if (connect(g_client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(g_client_fd);
        g_client_fd = -1;
        return -1;
    }
    
    return 0;
}

// Disconnect from server
void disconnect_from_server()
{
    if (g_client_fd >= 0)
    {
        close(g_client_fd);
        g_client_fd = -1;
    }
}

// Send message to server
int send_message_to_server(Message *message)
{
    if (g_client_fd < 0 || !message)
        return -1;
    
    // Calculate checksum (from protocol.c)
    extern uint32_t calculate_checksum(const char *data, int length);
    message->header.checksum = calculate_checksum((const char *)message->payload, (int)message->header.msg_length);
    
    // Send header
    ssize_t sent = send(g_client_fd, &message->header, sizeof(MessageHeader), 0);
    if (sent != sizeof(MessageHeader))
        return -1;
    
    // Send payload if present
    if (message->header.msg_length > 0)
    {
        sent = send(g_client_fd, message->payload, message->header.msg_length, 0);
        if (sent != (ssize_t)message->header.msg_length)
            return -1;
    }
    
    return 0;
}

// Receive message from server
int receive_message_from_server(Message *message)
{
    if (g_client_fd < 0 || !message)
    {
        return -1;
    }
    
    // Receive header (handle partial receives, like server does)
    size_t total_received = 0;
    char *header_ptr = (char *)&message->header;
    
    while (total_received < sizeof(MessageHeader))
    {
        ssize_t received = recv(g_client_fd, header_ptr + total_received, 
                               sizeof(MessageHeader) - total_received, 0);
        if (received <= 0)
        {
            if (received == 0) return -1; // Connection closed
            if (errno == EINTR) continue; // Interrupted, retry
            return -1; // Error
        }
        total_received += received;
    }
    
    // Validate header
    if (message->header.magic != 0xABCD)
    {
        return -1;
    }
    
    // Receive payload if present (handle partial receives)
    if (message->header.msg_length > 0)
    {
        if (message->header.msg_length > MAX_PAYLOAD_SIZE)
        {
            return -1;
        }
        
        total_received = 0;
        while (total_received < message->header.msg_length)
        {
            ssize_t received = recv(g_client_fd, message->payload + total_received,
                                   message->header.msg_length - total_received, 0);
            if (received <= 0)
            {
                if (received == 0) return -1; // Connection closed
                if (errno == EINTR) continue; // Interrupted, retry
                return -1; // Error
            }
            total_received += received;
        }
        
        message->payload[message->header.msg_length] = '\0';
    }
    else
    {
        message->payload[0] = '\0';
    }
    
    return 0;
}

// Check if connected
int is_connected()
{
    return (g_client_fd >= 0);
}

// Get client file descriptor
int get_client_fd()
{
    return g_client_fd;
}

