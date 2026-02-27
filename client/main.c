
/*
 Protocollo L7:
 ---------------------------------------------|
 [4 byte ]  filename length (big endian)      |
 ---------------------------------------------|
 [N byte ]  filename                          |
 ---------------------------------------------|
 [4 byte]   compressed                        |
 ---------------------------------------------|
 [8 byte]   file size (big endian)            |
 ---------------------------------------------|
 [8 byte ]  payload size (big endian)         |
 ---------------------------------------------|
 [M byte ]  payload                           |
 ---------------------------------------------|
*/


#include <arpa/inet.h>
#define _GNU_SOURCE
#include <string.h>
#include <libgen.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "socket_client.h"
#include "compression.h"
#define _DEFAULT_SOURCE
#include <endian.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>

struct stat st = {0};

void print_banner() {                                                         
    printf("\n");
    printf(" ██████╗██╗  ██╗██╗   ██╗███╗   ██╗██╗  ██╗██╗  ██╗   ██╗\n");
    printf("██╔════╝██║  ██║██║   ██║████╗  ██║██║ ██╔╝██║  ╚██╗ ██╔╝\n");
    printf("██║     ███████║██║   ██║██╔██╗ ██║█████╔╝ ██║   ╚████╔╝ \n");
    printf("██║     ██╔══██║██║   ██║██║╚██╗██║██╔═██╗ ██║    ╚██╔╝ \n");
    printf("╚██████╗██║  ██║╚██████╔╝██║ ╚████║██║  ██╗███████╗██║\n");
    printf(" ╚═════╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝╚═╝\n");
    printf("\n");
}

//helper function
void help(){

    print_banner();
    printf("Chunkly is a tool for transfering files with resume functions\n\n");
    printf("Examples:\n\n");
    printf("    ./chunkly <source_file> <server_address> <server_path>\n");

}



//function for check if filename passed by command end with slash or not. This is necessary for the fusion of data.filename and basename of client path
int EndsWithSlash(const char *path)
{
    if (!path || *path == '\0') return 0;
    return path[strlen(path) - 1] == '/';
}

int isDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

//debug function for check the data processed by splitter
void debug_payloads(struct message m){
    printf("Contenuto del file:\n");
    for (uint64_t j = 0; j < m.payload_size; j++) {
            unsigned char c = m.payload[j];
            if (c >= 32 && c <= 126)
                printf("%c", c);
            else
                printf(".");
    }
    printf("\n");
}

/*
#####################
# SPLITTER FUNCTION #
#####################

1. Read the uploaded_bytes received by server
2. Move file pointer to (uploaded_bytes) bytes
3. Build the protocol
4. Move file pointer to (payload_size)*i bytes
5. Read bytes of file from pointer to payload_size
(out of fuction)
6. save bytes to buffer
7. Send buffer to server

*/
int file_splitter(FILE *fd, struct message *data, int i, long segments, int last_segment){

    if (i<segments-1){
        fseek (fd, data->payload_size*i, SEEK_SET);
        fread (data->payload,1, data->payload_size, fd);
    }else if (i==segments-1)
    {
        fseek (fd, 0, SEEK_END);
        rewind (fd);
        fseek (fd, data->payload_size*i, SEEK_SET);
        fread (data->payload,1,last_segment, fd);
        data->payload_size = last_segment;
    }
    return 1;
}

int protocol_builder(uint8_t **buffer, struct message *data){

    //file name size in big endian format
    uint32_t nlen = htonl(data->fileName_len);
    //put file name size in the first 4 bytes of buffer
    memcpy(*buffer + data->offset, &nlen, 4);
    //increse offset of offset(0)+4
    data->offset += 4;

    //put the file name in the (filename size) bytes of buffer
    memcpy(*buffer + data->offset, data->fileName, data->fileName_len);
    //increase offset of (filename size): offset=4+(filename size)
    data->offset+=data->fileName_len;

    //compressed in big endian format
    uint32_t compressed = htonl(data->compressed);
    //put comrepssed value in the 4 bytes of buffer
    memcpy(*buffer + data->offset, &compressed, 4);
    //increase offset of 4: offset = 4+(filename size)+4
    data->offset += 4;

    //file size in big endian
    uint64_t file_size = htobe64(data->file_size);
    //put file size value in the 8 bytes of buffer
    memcpy(*buffer + data->offset, &file_size, 8);
    //increase offset of 8: offset = 4+(filename size)+4+8
    data->offset+=8;

    //chunk size in big endian
    uint64_t chunk_size = htobe64(data->payload_size);
    //put the chunk size in the (chunk size) bytes of buffer
    memcpy(*buffer + data->offset, &chunk_size, 8);
    //increase offset of (chunk size): offset = 4+(filename size)+4+8+(chunk size)
    data->offset+=8;
    return 1;

}

int main (int argc, char *argv[])
{

    if (argc < 4) {
    help();
    return 1;
    }

    int i;
    uint64_t chunk_size;
    size_t buff_size;
    uint8_t *buffer;
    uint8_t *buffPath;
    char *hostname=argv[2];
    char *path=argv[3];
    char *tfile= argv[1];
    char file[512];
    char prog[512];
    snprintf(prog, sizeof(prog), "%s", basename(tfile));
    snprintf(file, sizeof(file), "%s", tfile);

    FILE *fd;
    long segments=0;
    int last_segment;

    struct message data;
    data.payload=NULL;
    data.offset=0;
    data.file_size=0;
    data.payload_size=0;
    uint64_t uploaded_bytes;
    uint64_t total_bytes;

    int sockfd;
    sockfd=OpenSocket(hostname);
    if (isDirectory(prog)){
        compress_folder(file,prog,sizeof(prog));
        snprintf(file, sizeof(file), "%s", prog);
        data.compressed=1;
    }else{
        data.compressed=0;
    }

    if (EndsWithSlash(path)){
        snprintf(data.fileName, sizeof(data.fileName), "%s%s", path, prog);
    }else{
        snprintf(data.fileName, sizeof(data.fileName), "%s/%s", path, prog);
    }
    data.fileName_len=strlen(data.fileName);

    

    buff_size=4+data.fileName_len+20;
    if((buffPath=malloc(buff_size)) == NULL){
        fprintf (stderr, "error: virtual memory exhausted.\n");
        return 1;
    }
    memset(buffPath,0,buff_size);
    protocol_builder(&buffPath, &data);

    //first message to check if the server has the file

    SendMessage(sockfd, buffPath, buff_size);

    if (read_exact(sockfd, &uploaded_bytes, 8) < 0)
        return -1;
    data.uploaded_bytes = be64toh(uploaded_bytes);
    printf("Uploaded Bytes from server: %ld byte\n", data.uploaded_bytes);

    data.offset=0;

    fd = fopen (file, "rb");
    if (fd == NULL) {
        ferror (fd);
        return 1;
    }
    fseek (fd, 0, SEEK_END); 

    data.file_size=ftell(fd);
    total_bytes=data.file_size-data.uploaded_bytes;
    
    //calculate the size of segments so that it isn't more than 2Gb of Ram

    if (total_bytes>10000000000){

        segments= (long)((total_bytes/((total_bytes/2000000000)+1))); //if file is more than 10Gb
    }else{
        segments=5; // otherwise divide by 5 (default)
    }

    data.payload_size = total_bytes / segments;
    if ((data.payload = malloc (data.payload_size * sizeof *(data.payload))) == NULL) { 
        fprintf (stderr, "error: virtual memory exhausted.\n");
        return 1;
    }

    last_segment=(total_bytes)-((data.payload_size) * (segments-1));

    buff_size=4 + data.fileName_len + 4 + 8 + 8 + data.payload_size;
    printf("Buffer Size: %zd\n",buff_size);

    if((buffer=malloc(buff_size)) == NULL){
        fprintf (stderr, "error: virtual memory exhausted.\n");
        return 1;
    }

    memset(buffer,0,buff_size);

    fseek (fd, data.uploaded_bytes, SEEK_SET);
    
    protocol_builder(&buffer, &data);
    
    printf("Size of file name: %d byte\n", data.fileName_len);
    printf("Name of file: %s\n", data.fileName);
    printf("Chunk size: %ld byte\n", data.payload_size);

    //same metadata but differents payloads in every iteration
    for(i=0; i<segments; i++){

        file_splitter(fd, &data, i, segments, last_segment);
        if (i==segments-1){
            buff_size=4 + data.fileName_len + 4 + 8+ 8 + data.payload_size;
            
            uint8_t *tmp= realloc(buffer,buff_size);
            if (!tmp) {
            perror("realloc");
            break;
            }
            buffer=tmp;
            
            chunk_size = htobe64(data.payload_size);
            memcpy(buffer + 4 + data.fileName_len + 4 +8, &chunk_size, 8);
        }
        memcpy(buffer + data.offset, data.payload, data.payload_size);
        
        //debug_payloads(m);
            
        SendMessage(sockfd, buffer, buff_size);
    }                
    CloseSocket(sockfd);

    if (data.compressed==1){
        if (remove(file) == 0) {
            printf("Compressed file deleted successfully.\n");
        } else {
            printf("Error: Unable to delete the compressed file.\n");
    }
    }

    fclose (fd);
    free (buffer);
    free(data.payload);
    free (buffPath);
    return 0;
}