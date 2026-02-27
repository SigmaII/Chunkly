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

int dns_resolver(const char *hostname, char *addr, size_t addr_len){

    struct addrinfo hints, *result, *rp;
    int s;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  // AF_INET or AF_INET6 to specify IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    s = getaddrinfo(hostname, NULL, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return EXIT_FAILURE;
    }

    char ipstr[INET6_ADDRSTRLEN];
    void *addr_ptr;
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)result->ai_addr;
    addr_ptr = &(ipv4->sin_addr);
    inet_ntop(AF_INET, &(ipv4->sin_addr), addr, addr_len);
    //printf("IP Address: %s\n", ipstr);

    freeaddrinfo(result);
    return EXIT_SUCCESS;
}

int OpenSocket(char *hostname)
{

    int sockfd;
    char addr[INET6_ADDRSTRLEN];
    struct sockaddr_in servaddr;
    if (dns_resolver(hostname, addr, sizeof(addr)) == -1){
        printf("name resolution failed...\n");
        exit(1);
    }
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