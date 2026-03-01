#include "decompression.h"
#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <stddef.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h>
#define PORT 5000 
#define SA struct sockaddr
 

struct message{
    char *fileName;
    uint32_t fileName_len;
    uint32_t compressed;
    uint64_t file_size;
    unsigned char *payload;
    uint64_t payload_size;
    uint64_t uploaded_bytes;
    long offset;
};

int file_merger(struct message *data){

    FILE *fd;
    fd = fopen (data->fileName, "ab");
    if (fd == NULL) {
        ferror (fd);
        return -1;
    }
    fwrite (data->payload,1, data->payload_size, fd);
    fclose(fd);
    return 0;
    
}

// this function is necessary because tcp send segments fo data instead of integer message
ssize_t read_exact(int sockfd, void *buf, size_t len)
{
    size_t total = 0;
    uint8_t *p = buf;

    while (total < len) {
        ssize_t n = read(sockfd, p + total, len - total);
        if (n <= 0)
            return -1; //error
        total += n;
    }
    return total;
}



// Function designed for chat between client and server. 
int ReceiveMessage(int connfd, struct message *data) 
{ 
      
    uint32_t nlen_net;
    uint32_t compressed;
    uint64_t file_size;
    uint64_t payload_size_net;

    if (read_exact(connfd, &nlen_net, 4) < 0)
        return -1;

    //protocol re-builder
    data->fileName_len = ntohl(nlen_net);
    printf("Size of filename: %d byte\n", data->fileName_len);

    data->fileName = malloc(data->fileName_len + 1);
    if (!data->fileName)
    return -1;
    if (read_exact(connfd, data->fileName, data->fileName_len) < 0)
        return -1;
    data->fileName[data->fileName_len] = '\0';
    printf("Name of file: %s\n",data->fileName );


    if (read_exact(connfd, &compressed, 4) < 0)
        return -1;

    data->compressed = ntohl(compressed);
    printf("Value of compressed: %d \n", data->compressed);


    if (read_exact(connfd, &file_size, 8) < 0)
        return -1;
    data->file_size = be64toh(file_size);
    printf("File size: %ld byte\n", data->file_size);

    if (read_exact(connfd, &payload_size_net, 8) < 0)
        return -1;
    data->payload_size = be64toh(payload_size_net);
    printf("Chunk size: %ld byte\n", data->payload_size);

    data->payload = malloc(data->payload_size);
    if (!data->payload)
        return -1;
    if (read_exact(connfd, data->payload, data->payload_size) < 0)
        return -1;
    return 0;
} 

//this function return the size of existing file to client for changing file pointer position in client function and resume from the last byte received
int UploadedBytes(struct message *data){

    FILE *fd;
    if (access(data->fileName, F_OK) == 0) {
        fd = fopen (data->fileName, "rb");
        if (fd == NULL) {
            ferror (fd);
            return -1;
        }
        fseek (fd, 0, SEEK_END);
        data->uploaded_bytes=ftell(fd);
        return 1;
    } else {
        data->uploaded_bytes=0;
        return 0;
    }

}

uint64_t UploadedBytesForCompression(char *finalFileName){

    FILE *fd;
    uint64_t uploaded_bytes;
    if (access(finalFileName, F_OK) == 0) {
        fd = fopen (finalFileName, "rb");
        if (fd == NULL) {
            ferror (fd);
            return -1;
        }
        fseek (fd, 0, SEEK_END);
        uploaded_bytes=ftell(fd);
    } else {
        uploaded_bytes=0;
    }
    return uploaded_bytes;
}

//send message to client for verify that the upload was successful
void SendMessage(int sockfd, void *buffer, size_t length)
{

    size_t total = 0;
    while (total < length) {
        ssize_t n = write(sockfd, buffer + total, length - total);
        if (n <= 0) {
            perror("write failed");
            break;
        }
        total += n;
    }

}

int main() {

    char *finalFileName = NULL;
    int server_socket = socket(AF_INET, SOCK_STREAM, 0); 
    if (server_socket == -1) { 
        printf("socket creation failed...\n"); 
        exit(1); 
    } 
    else
        printf("Socket successfully created..\n"); 

    int res_accept;

    struct sockaddr_in adr = {0};
    adr.sin_family = AF_INET;
    adr.sin_port = htons(PORT);

    //bind
    if ((bind(server_socket, (SA*)&adr, sizeof(adr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(1); 
    } 
    else
        printf("Socket successfully binded..\n"); 

    //listen
    if ((listen(server_socket, SOMAXCONN)) != 0) { 
        printf("Listen failed...\n"); 
        exit(1); 
    } 
    else
        printf("Server listening..\n");
    
    for(;;) {
        int connfd = accept(server_socket, NULL, NULL);

        if (connfd < 0) { 
        printf("server accept failed...\n"); 
        exit(1); 
        } 
        else
            printf("server accept the client...\n"); 


        pid_t pid = fork();

        if (pid == 0) {
            //child (for multiple connections)
            close(server_socket);

            struct message data;
            uint64_t uploaded_bytes;
            ReceiveMessage(connfd,&data);
            printf("file name passato dal client: %s\n",data.fileName);
            if(UploadedBytes(&data)){
                printf("La dimensione del file caricato e': %ld\n",data.uploaded_bytes);
                uploaded_bytes = htobe64(data.uploaded_bytes);
                SendMessage(connfd,&uploaded_bytes,8);
            }else{
                uploaded_bytes = htobe64(data.uploaded_bytes);
                SendMessage(connfd,&uploaded_bytes,8);
            }

            while (ReceiveMessage(connfd, &data) == 0) {
                
                if (!finalFileName) {
                    finalFileName = strdup(data.fileName);
                }
                file_merger(&data);
                free(data.payload);
                free(data.fileName);
            }

            if(data.compressed && finalFileName && UploadedBytesForCompression(finalFileName)==data.file_size ){
                decompress_folder(finalFileName);
                free(finalFileName);
            }

            close(connfd);
            exit(0);
        }

        //dad
        close(connfd);
    }
}




int OpenSocketBck() 
{ 
    int sockfd, connfd, len; 
    struct sockaddr_in servaddr, cli; 
  
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n"); 
  
    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
    len = sizeof(cli); 
  
    // Accept the data packet from client and verification 
    connfd = accept(sockfd, (SA*)&cli, &len); 
    if (connfd < 0) { 
        printf("server accept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server accept the client...\n"); 
  
    // Function for chatting between client and server 
    return connfd;
}

void CloseSocket(int sockfd){
    close(sockfd); 
}