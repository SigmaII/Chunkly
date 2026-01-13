#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include "socket_client.h"
#include <unistd.h> // read(), write(), close()
#define MAX 80
#define PORT 5000
#define SA struct sockaddr

int OpenSocket(char *addr)
{
    int sockfd;
    struct sockaddr_in servaddr;
    printf("address: %s\n", addr);

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);   
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(1);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, addr, &servaddr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(1);
    }

    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))!= 0) {
        printf("connection with the server failed...\n");
        exit(1);
    }
    else
        printf("connected to the server..\n");

    return sockfd;
}

void CloseSocket(int sockfd){
    close(sockfd);
}

void SendMessage(int sockfd, uint8_t *buffer, size_t length)
{
    //char *buff;
    //int n;
    //bzero(buffer, sizeof(buffer));
    //n = 0;

    size_t total = 0;
    while (total < length) {
        ssize_t n = write(sockfd, buffer + total, length - total);
        if (n <= 0) {
            perror("write failed");
            break;
        }
        total += n;
    }

    /*bzero(buffer, sizeof(buffer));
    read(sockfd, buffer, sizeof(buffer));
    printf("From Server : %s", buffer);
    if ((strncmp(buffer, "exit", 4)) == 0) {
        printf("Client Exit...\n");
        
    }*/
}

ssize_t read_exact(int sockfd, void *buf, size_t len)
{
    size_t total = 0;
    uint8_t *p = buf;

    while (total < len) {
        ssize_t n = read(sockfd, p + total, len - total);
        if (n <= 0)
            return -1; // errore o connessione chiusa
        total += n;
    }
    return total;
}