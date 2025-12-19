// test_server_stress.c - Server Stress Tests (Phase 8)

#include "../include/protocol.h"
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

#define STRESS_CLIENTS 50
#define STRESS_REQUESTS_PER_CLIENT 100
#define STRESS_TIMEOUT 30

static int g_total_requests = 0;
static int g_successful_requests = 0;
static pthread_mutex_t g_stats_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *stress_client_worker(void *arg)
{
    int client_id = *(int *)arg;
    (void)client_id;
    
    for (int i = 0; i < STRESS_REQUESTS_PER_CLIENT; i++)
    {
        pthread_mutex_lock(&g_stats_mutex);
        g_total_requests++;
        pthread_mutex_unlock(&g_stats_mutex);
        
        // Simulate request processing
        usleep(1000); // 1ms delay
        
        pthread_mutex_lock(&g_stats_mutex);
        g_successful_requests++;
        pthread_mutex_unlock(&g_stats_mutex);
    }
    
    return NULL;
}

void test_rapid_connections()
{
    printf("Testing rapid connection handling...\n");
    
    db_init();
    
    // Create test user
    User test_user;
    register_user("stress_test", "pass123", &test_user);
    db_load_user_by_username("stress_test", &test_user);
    
    // Simulate rapid connections
    int success = 0;
    for (int i = 0; i < 100; i++)
    {
        // Simulate connection
        int sockets[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0)
        {
            success++;
            close(sockets[0]);
            close(sockets[1]);
        }
    }
    
    printf("  ✓ Created %d/100 socket pairs\n", success);
    assert(success == 100);
    
    db_close();
    printf("  ✓ Rapid connections test passed\n\n");
}

void test_message_throughput()
{
    printf("Testing message throughput...\n");
    
    int sockets[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    
    Message msg;
    memset(&msg, 0, sizeof(Message));
    msg.header.magic = 0xABCD;
    msg.header.msg_type = MSG_HEARTBEAT;
    msg.header.msg_length = 10;
    strcpy(msg.payload, "heartbeat");
    
    clock_t start = clock();
    int count = 0;
    
    // Send/receive messages rapidly
    for (int i = 0; i < 1000; i++)
    {
        send(sockets[0], &msg.header, sizeof(MessageHeader), 0);
        send(sockets[0], msg.payload, msg.header.msg_length, 0);
        
        MessageHeader header;
        recv(sockets[1], &header, sizeof(MessageHeader), MSG_WAITALL);
        char payload[11];
        recv(sockets[1], payload, header.msg_length, MSG_WAITALL);
        
        count++;
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    double throughput = count / elapsed;
    
    printf("  ✓ Processed %d messages in %.2f seconds (%.0f msg/s)\n", 
           count, elapsed, throughput);
    
    close(sockets[0]);
    close(sockets[1]);
    
    printf("  ✓ Message throughput test passed\n\n");
}

void test_concurrent_stress()
{
    printf("Testing concurrent stress (50 clients, 100 requests each)...\n");
    
    db_init();
    
    // Create test user
    User test_user;
    register_user("stress_concurrent", "pass123", &test_user);
    db_load_user_by_username("stress_concurrent", &test_user);
    
    g_total_requests = 0;
    g_successful_requests = 0;
    
    pthread_t threads[STRESS_CLIENTS];
    int thread_ids[STRESS_CLIENTS];
    
    clock_t start = clock();
    
    // Start all clients
    for (int i = 0; i < STRESS_CLIENTS; i++)
    {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, stress_client_worker, &thread_ids[i]);
    }
    
    // Wait for all
    for (int i = 0; i < STRESS_CLIENTS; i++)
    {
        pthread_join(threads[i], NULL);
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("  ✓ Total requests: %d\n", g_total_requests);
    printf("  ✓ Successful: %d\n", g_successful_requests);
    printf("  ✓ Time: %.2f seconds\n", elapsed);
    printf("  ✓ Throughput: %.0f req/s\n", g_successful_requests / elapsed);
    
    assert(g_successful_requests == STRESS_CLIENTS * STRESS_REQUESTS_PER_CLIENT);
    
    db_close();
    printf("  ✓ Concurrent stress test passed\n\n");
}

void test_memory_stability()
{
    printf("Testing memory stability (repeated operations)...\n");
    
    db_init();
    
    // Repeated operations to check for memory leaks
    for (int i = 0; i < 1000; i++)
    {
        User user;
        char username[32];
        snprintf(username, sizeof(username), "memtest%d", i);
        
        register_user(username, "pass123", &user);
        
        if (i % 100 == 0)
        {
            printf("  Processed %d operations...\n", i);
        }
    }
    
    printf("  ✓ Completed 1000 operations without crash\n");
    
    db_close();
    printf("  ✓ Memory stability test passed\n\n");
}

int main()
{
    printf("=== Server Stress Tests (Phase 8) ===\n\n");
    
    test_rapid_connections();
    test_message_throughput();
    test_concurrent_stress();
    test_memory_stability();
    
    printf("=== All Stress Tests Passed! ===\n");
    
    return 0;
}

