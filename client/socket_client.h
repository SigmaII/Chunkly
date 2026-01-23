#include <stdint.h>
#include <stddef.h>
#include <sys/types.h> 
#ifndef SOCKET_CLIENT_H
#define SOCKET_CLIENT_H

struct message{
    char fileName[512];
    uint32_t fileName_len;
    uint64_t file_size;
    uint32_t compressed;
    unsigned char *payload;
    uint64_t payload_size;
    uint64_t uploaded_bytes;
    long offset;
};

int OpenSocket(char *addr);
void CloseSocket(int sockfd);
void SendMessage(int sockfd, uint8_t *buffer, size_t lenght);
ssize_t read_exact(int sockfd, void *buf, size_t len);

#endif