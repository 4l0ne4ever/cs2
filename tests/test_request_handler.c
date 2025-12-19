// test_request_handler.c - Request Handler Tests (Phase 8)

#include "../include/request_handler.h"
#include "../include/protocol.h"
#include "../include/database.h"
#include "../include/auth.h"
#include "../include/market.h"
#include "../include/trading.h"
#include "../include/unbox.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void test_validate_message_header()
{
    printf("Testing validate_message_header...\n");
    
    MessageHeader header;
    
    // Test 1: Valid header
    header.magic = 0xABCD;
    header.msg_length = 10;
    assert(validate_message_header(&header) == 1);
    printf("  ✓ Valid header accepted\n");
    
    // Test 2: Invalid magic
    header.magic = 0x1234;
    assert(validate_message_header(&header) == 0);
    printf("  ✓ Invalid magic rejected\n");
    
    // Test 3: Payload too large
    header.magic = 0xABCD;
    header.msg_length = MAX_PAYLOAD_SIZE + 1;
    assert(validate_message_header(&header) == 0);
    printf("  ✓ Oversized payload rejected\n");
    
    // Test 4: NULL header
    assert(validate_message_header(NULL) == 0);
    printf("  ✓ NULL header rejected\n");
    
    printf("  ✓ validate_message_header test passed\n\n");
}

void test_receive_send_message()
{
    printf("Testing receive_message and send_response...\n");
    
    int sockets[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    
    // Test 1: Send and receive valid message
    Message sent_msg;
    memset(&sent_msg, 0, sizeof(Message));
    sent_msg.header.magic = 0xABCD;
    sent_msg.header.msg_type = MSG_HEARTBEAT;
    sent_msg.header.msg_length = 5;
    strcpy(sent_msg.payload, "test");
    
    // Send
    ssize_t sent = send(sockets[0], &sent_msg.header, sizeof(MessageHeader), 0);
    assert(sent == sizeof(MessageHeader));
    sent = send(sockets[0], sent_msg.payload, sent_msg.header.msg_length, 0);
    assert(sent == sent_msg.header.msg_length);
    
    // Receive
    Message received_msg;
    int result = receive_message(sockets[1], &received_msg);
    assert(result == 0);
    assert(received_msg.header.magic == 0xABCD);
    assert(received_msg.header.msg_type == MSG_HEARTBEAT);
    assert(strcmp(received_msg.payload, "test") == 0);
    printf("  ✓ Send/receive valid message\n");
    
    // Test 2: Invalid message (wrong magic)
    MessageHeader bad_header;
    bad_header.magic = 0x1234;
    bad_header.msg_length = 0;
    send(sockets[0], &bad_header, sizeof(MessageHeader), 0);
    
    result = receive_message(sockets[1], &received_msg);
    assert(result == -1);
    printf("  ✓ Invalid message rejected\n");
    
    // Test 3: Empty payload
    sent_msg.header.msg_length = 0;
    send(sockets[0], &sent_msg.header, sizeof(MessageHeader), 0);
    result = receive_message(sockets[1], &received_msg);
    assert(result == 0);
    assert(received_msg.header.msg_length == 0);
    printf("  ✓ Empty payload handled\n");
    
    // Test 4: Send response
    Message response;
    memset(&response, 0, sizeof(Message));
    response.header.magic = 0xABCD;
    response.header.msg_type = MSG_HEARTBEAT;
    response.header.msg_length = 4;
    strcpy(response.payload, "ok");
    
    result = send_response(sockets[1], &response);
    assert(result == 0);
    printf("  ✓ Send response successful\n");
    
    close(sockets[0]);
    close(sockets[1]);
    
    printf("  ✓ receive_message/send_response test passed\n\n");
}

void test_error_handling()
{
    printf("Testing error handling...\n");
    
    // Test invalid file descriptors
    Message msg;
    assert(receive_message(-1, &msg) == -1);
    assert(receive_message(-1, NULL) == -1);
    assert(send_response(-1, &msg) == -1);
    assert(send_response(-1, NULL) == -1);
    
    printf("  ✓ Invalid FDs rejected\n");
    
    // Test closed socket
    int sockets[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    close(sockets[0]);
    close(sockets[1]);
    
    assert(receive_message(sockets[1], &msg) == -1);
    printf("  ✓ Closed socket handled\n");
    
    printf("  ✓ Error handling test passed\n\n");
}

int main()
{
    printf("=== Request Handler Tests (Phase 8) ===\n\n");
    
    db_init();
    
    test_validate_message_header();
    test_receive_send_message();
    test_error_handling();
    
    db_close();
    
    printf("=== All Request Handler Tests Passed! ===\n");
    
    return 0;
}

