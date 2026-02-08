// common.c
#include "../include/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

/**
 * @brief Ensures all requested bytes are read from the socket (Handles Half-Packets)
 * * This function addresses the TCP stream nature where a single recv() 
 * might not return all requested data.
 */
int recv_all(int sockfd, void *buffer, size_t length) {
    size_t total_received = 0;
    char *buf = (char *)buffer;

    while (total_received < length) {
        ssize_t bytes = recv(sockfd, buf + total_received, length - total_received, 0);
        if (bytes <= 0) {
            if (bytes < 0 && errno == EINTR) continue; // Interrupted system call
            return -1; // Error or Connection Closed
        }
        total_received += bytes;
    }
    return 0; // Success
}

/**
 * @brief Ensures all bytes are sent to the socket
 */
int send_all(int sockfd, const void *buffer, size_t length) {
    size_t total_sent = 0;
    const char *buf = (const char *)buffer;

    while (total_sent < length) {
        ssize_t bytes = send(sockfd, buf + total_sent, length - total_sent, 0);
        if (bytes < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        total_sent += bytes;
    }
    return 0; // Success
}

/**
 * @brief Utility to send a TLV packet
 */
void send_packet(int sockfd, uint8_t type, const void *payload, uint32_t payload_len) {
    PacketHeader header;
    header.type = type;
    header.length = htonl(payload_len); // Convert to Network Byte Order (Big Endian)

    // 1. Send Header
    if (send_all(sockfd, &header, sizeof(PacketHeader)) < 0) {
        perror("Failed to send header");
        return;
    }

    // 2. Send Payload (if exists)
    if (payload_len > 0 && payload != NULL) {
        if (send_all(sockfd, payload, payload_len) < 0) {
            perror("Failed to send payload");
        }
    }
}