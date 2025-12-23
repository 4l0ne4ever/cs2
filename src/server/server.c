// server.c - Main Server Implementation (Phase 8)

#include "../include/types.h"
#include "../include/protocol.h"
#include "../include/database.h"
#include "../include/thread_pool.h"
#include "../include/request_handler.h"

// Forward declaration for calculate_checksum (from protocol.c)
extern uint32_t calculate_checksum(const char *data, int length);
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>


#define MAX_CLIENTS 1000
#define DEFAULT_PORT 8888

static int server_running = 1;
static ThreadPool g_thread_pool;

// Global client tracking for broadcasting
static int g_client_fds[MAX_CLIENTS];
static int g_client_count = 0;
static pthread_mutex_t g_client_mutex = PTHREAD_MUTEX_INITIALIZER;

// Signal handler for graceful shutdown
void signal_handler(int sig)
{
    (void)sig;
    server_running = 0;
    printf("\nShutting down server...\n");
}

// Setup server socket
static int setup_server_socket(int port)
{
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    
    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        return -1;
    }
    
    // Set socket options
    // SO_REUSEADDR: Allow reuse of local addresses (works on all platforms)
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt SO_REUSEADDR");
        close(server_fd);
        return -1;
    }
    
    // SO_REUSEPORT: Allow multiple sockets to bind to same port (Linux only, not supported on macOS)
    #ifdef __linux__
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt SO_REUSEPORT");
        close(server_fd);
        return -1;
    }
    #endif
    
    // Configure address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        close(server_fd);
        return -1;
    }
    
    // Listen
    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        close(server_fd);
        return -1;
    }
    
    return server_fd;
}

// Accept new connection
static int accept_connection(int server_fd)
{
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int client_fd;
    
    client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (client_fd < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            perror("accept");
        }
        return -1;
    }
    
    // Set non-blocking mode (optional, for better performance)
    // fcntl(client_fd, F_SETFL, O_NONBLOCK);
    
    return client_fd;
}

int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;
    
    if (argc > 1)
    {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535)
        {
            fprintf(stderr, "Invalid port number: %d\n", port);
            return 1;
        }
    }
    
    printf("=== CS2 Skin Trading Server ===\n");
    printf("Starting on port %d\n", port);
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize database
    if (db_init() != 0)
    {
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }
    printf("✓ Database initialized\n");
    
    // Initialize thread pool
    if (thread_pool_init(&g_thread_pool) != 0)
    {
        fprintf(stderr, "Failed to initialize thread pool\n");
        db_close();
        return 1;
    }
    printf("✓ Thread pool initialized (%d workers)\n", NUM_WORKER_THREADS);
    
    // Setup server socket
    int server_fd = setup_server_socket(port);
    if (server_fd < 0)
    {
        fprintf(stderr, "Failed to setup server socket\n");
        thread_pool_shutdown(&g_thread_pool);
        db_close();
        return 1;
    }
    printf("✓ Server socket listening on port %d\n", port);
    printf("Server ready to accept connections\n\n");
    
    // Main server loop
    fd_set read_fds;
    int max_fd = server_fd;
    
    while (server_running)
    {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        
        // Add all client sockets to select set
        pthread_mutex_lock(&g_client_mutex);
        for (int i = 0; i < g_client_count; i++)
        {
            if (g_client_fds[i] >= 0)
            {
                FD_SET(g_client_fds[i], &read_fds);
                if (g_client_fds[i] > max_fd)
                    max_fd = g_client_fds[i];
            }
        }
        pthread_mutex_unlock(&g_client_mutex);
        
        // Use select with timeout for responsiveness
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0 && errno != EINTR)
        {
            perror("select");
            break;
        }
        
        // Check if server socket has new connection
        if (FD_ISSET(server_fd, &read_fds))
        {
            int client_fd = accept_connection(server_fd);
            
            if (client_fd >= 0)
            {
                pthread_mutex_lock(&g_client_mutex);
                if (g_client_count < MAX_CLIENTS)
                {
                    // Add to client list
                    g_client_fds[g_client_count++] = client_fd;
                }
                else
                {
                    // Too many clients, close connection
                    close(client_fd);
                }
                pthread_mutex_unlock(&g_client_mutex);
            }
        }
        
        // Check for data on client sockets
        pthread_mutex_lock(&g_client_mutex);
        for (int i = 0; i < g_client_count; i++)
        {
            if (g_client_fds[i] >= 0 && FD_ISSET(g_client_fds[i], &read_fds))
            {
                int client_fd = g_client_fds[i];
                
                // Receive message from client
                Message request;
                int recv_result = receive_message(client_fd, &request);
                
                if (recv_result == 0)
                {
                    // Add job to thread pool (keep client in list for more requests)
                    int add_result = thread_pool_add_job(&g_thread_pool, client_fd, &request);
                    
                    if (add_result != 0)
                    {
                        // Queue full or shutdown, close connection
                        close(client_fd);
                        g_client_fds[i] = -1; // Mark as closed
                    }
                    // Keep client in list for more requests - don't remove it
                }
                else
                {
                    // Failed to receive message (error or connection closed)
                    close(client_fd);
                    g_client_fds[i] = -1; // Mark as closed
                }
            }
        }
        
        // Clean up closed client sockets
        int write_idx = 0;
        for (int i = 0; i < g_client_count; i++)
        {
            if (g_client_fds[i] >= 0)
            {
                g_client_fds[write_idx++] = g_client_fds[i];
            }
        }
        g_client_count = write_idx;
        pthread_mutex_unlock(&g_client_mutex);
    }
    
    // Cleanup
    printf("Shutting down...\n");
    thread_pool_shutdown(&g_thread_pool);
    close(server_fd);
    db_close();
    printf("Server stopped\n");
    
    return 0;
}

// Broadcast message to all connected clients
void broadcast_to_all_clients(const char *username, const char *message)
{
    if (!username || !message)
        return;
    
    // Create broadcast message
    Message broadcast;
    memset(&broadcast, 0, sizeof(Message));
    broadcast.header.magic = 0xABCD;
    broadcast.header.msg_type = MSG_CHAT_BROADCAST;
    
    // Format: "username: message"
    snprintf(broadcast.payload, MAX_PAYLOAD_SIZE, "%s: %s", username, message);
    broadcast.header.msg_length = strlen(broadcast.payload);
    
    // Calculate checksum
    broadcast.header.checksum = calculate_checksum(broadcast.payload, (int)broadcast.header.msg_length);
    
    // Send to all connected clients
    pthread_mutex_lock(&g_client_mutex);
    for (int i = 0; i < g_client_count; i++)
    {
        if (g_client_fds[i] >= 0)
        {
            // Send header
            ssize_t sent = send(g_client_fds[i], &broadcast.header, sizeof(MessageHeader), MSG_NOSIGNAL);
            if (sent > 0 && broadcast.header.msg_length > 0)
            {
                // Send payload
                send(g_client_fds[i], broadcast.payload, broadcast.header.msg_length, MSG_NOSIGNAL);
            }
        }
    }
    pthread_mutex_unlock(&g_client_mutex);
}
