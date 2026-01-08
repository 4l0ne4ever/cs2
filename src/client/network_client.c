// network_client.c - Network Client Implementation (Phase 9)

#include "../include/protocol.h"
#include "../include/network_client.h"
#include "../include/logger.h"
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
    LOG_INFO("[NETWORK] Starting connection to server %s:%d", server_ip, port);
    
    if (g_client_fd >= 0)
    {
        LOG_DEBUG("[NETWORK] Closing existing connection (fd=%d)", g_client_fd);
        close(g_client_fd);
        g_client_fd = -1;
    }
    
    // Create socket
    LOG_DEBUG("[NETWORK] Creating TCP socket (AF_INET, SOCK_STREAM)");
    g_client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_client_fd < 0)
    {
        LOG_ERROR("[NETWORK] socket() failed: %s", strerror(errno));
        perror("socket");
        return -1;
    }
    LOG_DEBUG("[NETWORK] Socket created successfully (fd=%d)", g_client_fd);
    
    // Setup server address - support both IP addresses and hostnames
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    LOG_DEBUG("[NETWORK] Setting up server address: family=AF_INET, port=%d (network byte order: %d)", 
              port, ntohs(server_addr.sin_port));
    
    // Try to parse as IP address first
    char resolved_ip[INET_ADDRSTRLEN];
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        LOG_DEBUG("[NETWORK] '%s' is not a valid IP address, attempting DNS resolution...", server_ip);
        // If not an IP, try to resolve as hostname
        struct hostent *host_entry = gethostbyname(server_ip);
        if (host_entry == NULL)
        {
            LOG_ERROR("[NETWORK] DNS resolution failed for '%s': %s", server_ip, hstrerror(h_errno));
            fprintf(stderr, "Invalid address or hostname: %s\n", server_ip);
            close(g_client_fd);
            g_client_fd = -1;
            return -1;
        }
        
        // Copy resolved IP address
        memcpy(&server_addr.sin_addr, host_entry->h_addr_list[0], host_entry->h_length);
        inet_ntop(AF_INET, &server_addr.sin_addr, resolved_ip, INET_ADDRSTRLEN);
        LOG_INFO("[NETWORK] DNS resolution successful: '%s' -> %s", server_ip, resolved_ip);
    }
    else
    {
        inet_ntop(AF_INET, &server_addr.sin_addr, resolved_ip, INET_ADDRSTRLEN);
        LOG_DEBUG("[NETWORK] Using IP address directly: %s", resolved_ip);
    }
    
    // Connect
    LOG_INFO("[NETWORK] Attempting TCP connection to %s:%d...", resolved_ip, port);
    if (connect(g_client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        LOG_ERROR("[NETWORK] connect() failed: %s", strerror(errno));
        perror("connect");
        close(g_client_fd);
        g_client_fd = -1;
        return -1;
    }
    
    LOG_INFO("[NETWORK] TCP connection established successfully (fd=%d, server=%s:%d)", 
             g_client_fd, resolved_ip, port);
    return 0;
}

// Disconnect from server
void disconnect_from_server()
{
    if (g_client_fd >= 0)
    {
        LOG_INFO("[NETWORK] Closing connection (fd=%d)", g_client_fd);
        close(g_client_fd);
        g_client_fd = -1;
        LOG_DEBUG("[NETWORK] Connection closed");
    }
    else
    {
        LOG_DEBUG("[NETWORK] No active connection to close");
    }
}

// Send message to server
int send_message_to_server(Message *message)
{
    if (g_client_fd < 0 || !message)
    {
        LOG_ERROR("[NETWORK] Cannot send message: invalid socket (fd=%d) or null message", g_client_fd);
        return -1;
    }
    
    // Calculate checksum (from protocol.c)
    extern uint32_t calculate_checksum(const char *data, int length);
    message->header.checksum = calculate_checksum((const char *)message->payload, (int)message->header.msg_length);
    LOG_DEBUG("[NETWORK] Message checksum calculated: 0x%08X (payload_length=%d)", 
              message->header.checksum, message->header.msg_length);
    
    // Send header
    LOG_DEBUG("[NETWORK] Sending message header: magic=0x%04X, type=0x%04X, length=%d, checksum=0x%08X, seq=%d",
              message->header.magic, message->header.msg_type, message->header.msg_length,
              message->header.checksum, message->header.sequence_num);
    ssize_t sent = send(g_client_fd, &message->header, sizeof(MessageHeader), 0);
    if (sent != sizeof(MessageHeader))
    {
        LOG_ERROR("[NETWORK] Failed to send message header: sent=%zd, expected=%zu, error=%s", 
                  sent, sizeof(MessageHeader), strerror(errno));
        return -1;
    }
    LOG_DEBUG("[NETWORK] Message header sent successfully (%zd bytes)", sent);
    
    // Send payload if present
    if (message->header.msg_length > 0)
    {
        LOG_DEBUG("[NETWORK] Sending message payload (%d bytes)...", message->header.msg_length);
        sent = send(g_client_fd, message->payload, message->header.msg_length, 0);
        if (sent != (ssize_t)message->header.msg_length)
        {
            LOG_ERROR("[NETWORK] Failed to send message payload: sent=%zd, expected=%d, error=%s", 
                      sent, message->header.msg_length, strerror(errno));
            return -1;
        }
        LOG_DEBUG("[NETWORK] Message payload sent successfully (%zd bytes)", sent);
        
        // Log payload preview (first 100 chars for debugging)
        if (message->header.msg_length < 100)
        {
            LOG_DEBUG("[NETWORK] Payload preview: %.*s", (int)message->header.msg_length, message->payload);
        }
        else
        {
            LOG_DEBUG("[NETWORK] Payload preview: %.100s...", message->payload);
        }
    }
    else
    {
        LOG_DEBUG("[NETWORK] No payload to send (msg_length=0)");
    }
    
    LOG_INFO("[NETWORK] Message sent successfully: type=0x%04X, total_bytes=%zu", 
             message->header.msg_type, sizeof(MessageHeader) + message->header.msg_length);
    return 0;
}

// Receive message from server
int receive_message_from_server(Message *message)
{
    if (g_client_fd < 0 || !message)
    {
        LOG_ERROR("[NETWORK] Cannot receive message: invalid socket (fd=%d) or null message", g_client_fd);
        return -1;
    }
    
    LOG_DEBUG("[NETWORK] Waiting to receive message header (%zu bytes)...", sizeof(MessageHeader));
    
    // Receive header (handle partial receives, like server does)
    size_t total_received = 0;
    char *header_ptr = (char *)&message->header;
    
    while (total_received < sizeof(MessageHeader))
    {
        ssize_t received = recv(g_client_fd, header_ptr + total_received, 
                               sizeof(MessageHeader) - total_received, 0);
        if (received <= 0)
        {
            if (received == 0)
            {
                LOG_WARNING("[NETWORK] Connection closed by server while receiving header");
                return -1; // Connection closed
            }
            if (errno == EINTR)
            {
                LOG_DEBUG("[NETWORK] recv() interrupted, retrying...");
                continue; // Interrupted, retry
            }
            LOG_ERROR("[NETWORK] recv() error while receiving header: %s", strerror(errno));
            return -1; // Error
        }
        total_received += received;
        if (total_received < sizeof(MessageHeader))
        {
            LOG_DEBUG("[NETWORK] Partial header received: %zu/%zu bytes", total_received, sizeof(MessageHeader));
        }
    }
    
    LOG_DEBUG("[NETWORK] Message header received: magic=0x%04X, type=0x%04X, length=%d, checksum=0x%08X, seq=%d",
              message->header.magic, message->header.msg_type, message->header.msg_length,
              message->header.checksum, message->header.sequence_num);
    
    // Validate header
    if (message->header.magic != 0xABCD)
    {
        LOG_ERROR("[NETWORK] Invalid magic number: 0x%04X (expected 0xABCD)", message->header.magic);
        return -1;
    }
    
    // Receive payload if present (handle partial receives)
    if (message->header.msg_length > 0)
    {
        if (message->header.msg_length > MAX_PAYLOAD_SIZE)
        {
            LOG_ERROR("[NETWORK] Message payload too large: %d bytes (max=%d)", 
                      message->header.msg_length, MAX_PAYLOAD_SIZE);
            return -1;
        }
        
        LOG_DEBUG("[NETWORK] Receiving message payload (%d bytes)...", message->header.msg_length);
        total_received = 0;
        while (total_received < message->header.msg_length)
        {
            ssize_t received = recv(g_client_fd, message->payload + total_received,
                                   message->header.msg_length - total_received, 0);
            if (received <= 0)
            {
                if (received == 0)
                {
                    LOG_WARNING("[NETWORK] Connection closed by server while receiving payload");
                    return -1; // Connection closed
                }
                if (errno == EINTR)
                {
                    LOG_DEBUG("[NETWORK] recv() interrupted, retrying...");
                    continue; // Interrupted, retry
                }
                LOG_ERROR("[NETWORK] recv() error while receiving payload: %s", strerror(errno));
                return -1; // Error
            }
            total_received += received;
            if (total_received < message->header.msg_length)
            {
                LOG_DEBUG("[NETWORK] Partial payload received: %zu/%d bytes", 
                          total_received, message->header.msg_length);
            }
        }
        
        message->payload[message->header.msg_length] = '\0';
        LOG_DEBUG("[NETWORK] Message payload received successfully (%zu bytes)", total_received);
        
        // Log payload preview (first 100 chars for debugging)
        if (message->header.msg_length < 100)
        {
            LOG_DEBUG("[NETWORK] Payload preview: %.*s", (int)message->header.msg_length, message->payload);
        }
        else
        {
            LOG_DEBUG("[NETWORK] Payload preview: %.100s...", message->payload);
        }
        
        // Validate checksum
        extern uint32_t calculate_checksum(const char *data, int length);
        uint32_t calculated_checksum = calculate_checksum((const char *)message->payload, 
                                                           (int)message->header.msg_length);
        if (calculated_checksum != message->header.checksum)
        {
            LOG_ERROR("[NETWORK] Checksum mismatch: received=0x%08X, calculated=0x%08X", 
                      message->header.checksum, calculated_checksum);
            return -1;
        }
        LOG_DEBUG("[NETWORK] Checksum validation passed: 0x%08X", calculated_checksum);
    }
    else
    {
        message->payload[0] = '\0';
        LOG_DEBUG("[NETWORK] No payload in message (msg_length=0)");
    }
    
    LOG_INFO("[NETWORK] Message received successfully: type=0x%04X, total_bytes=%zu", 
             message->header.msg_type, sizeof(MessageHeader) + message->header.msg_length);
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

