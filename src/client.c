#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "tcp_client.h"

/*
 Protocol L7:
 ----------------------------------------------------------------|
 [1 byte ]  msg_type (0x01 = INFO_REQ, 0x02 = RES, 0x03 = CHUNK) |
 ----------------------------------------------------------------|
 [1 byte ]  chunk_id                                             |
 ----------------------------------------------------------------|
 [4 byte ]  filename length (big endian)                         |
 ----------------------------------------------------------------|
 [N byte ]  filename                                             |
 ----------------------------------------------------------------|
 [8 byte]   file size (big endian)                               |
 ----------------------------------------------------------------|
 [8 byte ]  payload size (big endian)                            |
 ----------------------------------------------------------------|
 [M byte ]  payload                                              |
 ----------------------------------------------------------------|
*/


// gcc -Ilib/socket_client/include lib/socket_client/src/tcp_client.c src/client.c

typedef enum {
    MSG_INFO_REQ = 0x01,  // File state request (1 byte)
    MSG_INFO_RES = 0x02,  // Server response (1 byte)
    MSG_CHUNK    = 0x03   // Send chunk of file (1 byte)
} msg_type_t;

typedef struct file_data{
    uint8_t type; //message type
    uint8_t id; //chunk ID
    uint32_t fileName_len; //length of filename in big endian
    char *fileName; //name of file in little endian
    uint64_t file_size; //size of file in big endian
    uint64_t payload_size; //size of payload
    unsigned char *payload; //content of payload
}file_data;

/**
     * Get size of file in bytes from local filesystem.
     * @param path path of file
     * @return size of file or 0 if there are errors
     */
uint64_t get_lc_filesize(char *path){
    FILE* fd;
    uint64_t filesize;
    fd = fopen (path, "rb");
    if (fd == NULL) {
        ferror (fd);
        return 0;
    }
    fseek (fd, 0, SEEK_END);
    filesize=ftell(fd);
    fclose(fd);
    return filesize;
}

/**
     * Get size of file in bytes from destination server filesystem.
     * @param sockfd socket descriptor
     * @param data metadata of file
     * @return size of file
     */
uint64_t get_sv_filesize(int sockfd, file_data *data){
    uint64_t filesize;
    data->type=MSG_INFO_REQ;
    send_data(sockfd,data,sizeof(data));
    receive_data(sockfd,&filesize);
    return filesize;
}

/**
     * Split file into segments.
     * @param fd file to split
     * @param data 
     * @param segments_num
     * @return 
     */
int file_splitter(char *path, file_data *data,uint64_t filesize, int segments_num, int segments_len){


    //NB: fread ritorna il numero di byte letti: se proviamo a leggere 1 MB anche se rimangono solo 0.5 Mb luio li legge senza andare in errore!!
    int segments_read;
    FILE* fd;

    fd = fopen (path, "rb");
    if (fd == NULL) {
        ferror (fd);
        return 0;
    }

    segments_read=filesize/segments_len*1000000; //get number of segments already read by server
    for(int i=segments_read; i<segments_num-segments_read; i++){
        fseek(fd,filesize,SEEK_SET);
        fread(data->payload,1,)

    }

    if (i<segments-1){
        fseek(fd, data->uploaded_bytes + (data->payload_size * i), SEEK_SET);
        fread (data->payload,1, data->payload_size, fd);
    }else if (i==segments-1)
    {
        fseek (fd, 0, SEEK_END);
        rewind (fd);
        fseek(fd, data->uploaded_bytes + (data->payload_size * i), SEEK_SET);
        fread (data->payload,1,last_segment, fd);
        data->payload_size = last_segment;
    }
    return 1;
}


//helper function
void help(){

    printf("Chunkly is a tool for transfering files with resume functions\n\n");
    printf("Flags:\n\n");
    printf("    -f  file to transfer\n");
    printf("    -d  hostname or ip of destination server\n");
    printf("    -p  destination path of server\n");
    printf("    -s  number of segments (default is 5)\n\n");

    printf("Examples:\n\n");
    printf("    ./chunkly -f <source_file> -d <server_address> -p <server_path>\n");

}

int main(int argc, char* argv[]){

    //network vars
    int sockfd;

    //vars for files management
    file_data file;
    char base_name[512];
    int segments_len=1*1000000; //length of segments (1 000 000 bytes = 1Mb for segment)
    int segments_num; //total number of segments
    uint64_t srv_filesize; //filesize returned by server if file is already present

//flags definition
    //vars for flags
    char *hostname = NULL; //destionation server
    char *server_path = NULL; // destionation path of server
    char *client_path = NULL; // file
    int ram_limit; //ram limit for segment partition (4 = 4Gb)
    int index;
    int c;

    opterr = 0;
    while ((c = getopt (argc, argv, "f:d:p:s:h")) != -1)
        switch (c)
        {
        case 'f':
            client_path = optarg;
            break;
        case 'd':
            hostname = optarg;
            break;
        case 'p':
            server_path = optarg;
            break;
        case 'r':
            ram_limit = atoi(optarg);
            break;
        case 'h':
            help();
            return 1;
            break;
        case '?':
            if (optopt == 'c')
            fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
            fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
            fprintf (stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
            return 1;
        default:
            abort ();
        }

    for (index = optind; index < argc; index++)
        printf ("Non-option argument %s\n", argv[index]);
//end of flags


    //path basename (eg. "/home/test/hello.txt" to "hello.txt")
    snprintf(base_name, sizeof(base_name), "%s", basename(client_path));
    //Add final slash to path if not present
    size_t len = strlen(server_path);
    if (len > 0 && server_path[len - 1] != '/') {
        snprintf(server_path+len, 2, "/");
    }

    
    //set number of segments
    segments_num=((ram_limit*1000)/(segments_len/1000000));

    //build protocol
    file.fileName=base_name;
    file.fileName_len=strlen(base_name);
    file.file_size=get_lc_filesize(client_path);
    
    file_splitter(client_path,&file,get_sv_filesize(sockfd,&file));



    //Open TCP socket
    sockfd=open_tcp_socket(hostname);

    
    if(close(sockfd)<0){
        printf("[ERROR] Failed to close connection\n");
    }

    return 0;
    
}