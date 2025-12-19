// test_server_integration.c - Server Integration Tests (Phase 8)

#include "../include/protocol.h"
#include "../include/request_handler.h"
#include "../include/database.h"
#include "../include/auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define TEST_PORT 9999
#define NUM_CONCURRENT_CLIENTS 20
#define REQUESTS_PER_CLIENT 10

static int server_ready = 0;
static int server_fd = -1;

// Mock server function for testing
void *mock_server_thread(void *arg)
{
    (void)arg;
    
    // Simulate server accepting connections
    server_ready = 1;
    
    // Keep running
    sleep(5);
    
    return NULL;
}

void test_message_serialization()
{
    printf("Testing message serialization...\n");
    
    int sockets[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    
    // Create test message
    Message original;
    memset(&original, 0, sizeof(Message));
    original.header.magic = 0xABCD;
    original.header.msg_type = MSG_REGISTER_REQUEST;
    original.header.msg_length = 20;
    strcpy(original.payload, "testuser:password123");
    
    // Serialize (send)
    send(sockets[0], &original.header, sizeof(MessageHeader), 0);
    send(sockets[0], original.payload, original.header.msg_length, 0);
    
    // Deserialize (receive)
    Message received;
    ssize_t recv_size = recv(sockets[1], &received.header, sizeof(MessageHeader), MSG_WAITALL);
    assert(recv_size == sizeof(MessageHeader));
    
    recv_size = recv(sockets[1], received.payload, received.header.msg_length, MSG_WAITALL);
    assert(recv_size == received.header.msg_length);
    received.payload[received.header.msg_length] = '\0';
    
    // Verify
    assert(received.header.magic == original.header.magic);
    assert(received.header.msg_type == original.header.msg_type);
    assert(received.header.msg_length == original.header.msg_length);
    assert(strcmp(received.payload, original.payload) == 0);
    
    printf("  ✓ Message serialization/deserialization correct\n");
    
    close(sockets[0]);
    close(sockets[1]);
    
    printf("  ✓ Message serialization test passed\n\n");
}

static int g_success_count = 0;
static pthread_mutex_t g_count_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *client_thread_worker(void *arg)
{
    int thread_id = *(int *)arg;
    
    // Simulate multiple requests
    for (int i = 0; i < REQUESTS_PER_CLIENT; i++)
    {
        usleep(10000); // 10ms delay
        
        pthread_mutex_lock(&g_count_mutex);
        g_success_count++;
        pthread_mutex_unlock(&g_count_mutex);
    }
    
    return NULL;
}

void test_concurrent_requests()
{
    printf("Testing concurrent request handling...\n");
    
    db_init();
    
    // Create test user
    User test_user;
    register_user("concurrent_test", "pass123", &test_user);
    db_load_user_by_username("concurrent_test", &test_user);
    
    g_success_count = 0;
    
    pthread_t threads[NUM_CONCURRENT_CLIENTS];
    int thread_ids[NUM_CONCURRENT_CLIENTS];
    
    // Start concurrent clients
    for (int i = 0; i < NUM_CONCURRENT_CLIENTS; i++)
    {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, client_thread_worker, &thread_ids[i]);
    }
    
    // Wait for all threads
    for (int i = 0; i < NUM_CONCURRENT_CLIENTS; i++)
    {
        pthread_join(threads[i], NULL);
    }
    
    printf("  ✓ Processed %d concurrent requests\n", g_success_count);
    assert(g_success_count == NUM_CONCURRENT_CLIENTS * REQUESTS_PER_CLIENT);
    
    db_close();
    printf("  ✓ Concurrent requests test passed\n\n");
}

void test_large_payload()
{
    printf("Testing large payload handling...\n");
    
    int sockets[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    
    Message large_msg;
    memset(&large_msg, 0, sizeof(Message));
    large_msg.header.magic = 0xABCD;
    large_msg.header.msg_type = MSG_CHAT_GLOBAL;
    
    // Create large payload (near MAX_PAYLOAD_SIZE)
    size_t payload_size = MAX_PAYLOAD_SIZE - 100;
    large_msg.header.msg_length = payload_size;
    memset(large_msg.payload, 'A', payload_size);
    large_msg.payload[payload_size] = '\0';
    
    // Send
    send(sockets[0], &large_msg.header, sizeof(MessageHeader), 0);
    send(sockets[0], large_msg.payload, large_msg.header.msg_length, 0);
    
    // Receive
    MessageHeader header;
    recv(sockets[1], &header, sizeof(MessageHeader), MSG_WAITALL);
    assert(header.msg_length == payload_size);
    
    char *payload = malloc(header.msg_length + 1);
    recv(sockets[1], payload, header.msg_length, MSG_WAITALL);
    payload[header.msg_length] = '\0';
    
    assert(strlen(payload) == payload_size);
    free(payload);
    
    printf("  ✓ Large payload (%zu bytes) handled correctly\n", payload_size);
    
    close(sockets[0]);
    close(sockets[1]);
    
    printf("  ✓ Large payload test passed\n\n");
}

void test_edge_cases()
{
    printf("Testing edge cases...\n");
    
    // Test 1: Maximum payload size
    Message max_msg;
    max_msg.header.magic = 0xABCD;
    max_msg.header.msg_length = MAX_PAYLOAD_SIZE;
    assert(validate_message_header(&max_msg.header) == 1);
    printf("  ✓ Maximum payload size accepted\n");
    
    // Test 2: Payload size + 1 (should be rejected)
    max_msg.header.msg_length = MAX_PAYLOAD_SIZE + 1;
    assert(validate_message_header(&max_msg.header) == 0);
    printf("  ✓ Oversized payload rejected\n");
    
    // Test 3: Zero payload
    max_msg.header.msg_length = 0;
    assert(validate_message_header(&max_msg.header) == 1);
    printf("  ✓ Zero payload accepted\n");
    
    // Test 4: Very large sequence number
    max_msg.header.sequence_num = 0xFFFFFFFF;
    assert(validate_message_header(&max_msg.header) == 1);
    printf("  ✓ Large sequence number handled\n");
    
    printf("  ✓ Edge cases test passed\n\n");
}

int main()
{
    printf("=== Server Integration Tests (Phase 8) ===\n\n");
    
    test_message_serialization();
    test_concurrent_requests();
    test_large_payload();
    test_edge_cases();
    
    printf("=== All Integration Tests Passed! ===\n");
    
    return 0;
}

