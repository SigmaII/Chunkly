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

/*
Il buffer circolare funziona semplicemente con un array statico da n elementi:
il writer scrive sulla tail, mentre il reader legge dalla head, ma entrambi (tail e head) sono dei CONTATORI (non puntatori) che si muovono nella stessa direzione
Il writer, prima di sovrascrivere una cella, controlla la head (posizione del reader), se questo non è ancora arrivato a codesta cella si deve bloccare in attesa 
che ci arrivi
*/


// gcc -Ilib/socket_client/include lib/socket_client/src/tcp_client.c src/client.c

typedef enum {
    MSG_INFO_REQ = 0x01,  // File state request (1 byte)
    MSG_INFO_RES = 0x02,  // Server response (1 byte)
    MSG_CHUNK    = 0x03   // Send chunk of file (1 byte)
} msg_type_t;

typedef struct DataNode{
    uint8_t type; //message type
    uint8_t id; //chunk ID
    uint32_t fileName_len; //length of filename in big endian
    char *fileName; //name of file in little endian
    uint64_t file_size; //size of file in big endian
    uint64_t payload_size; //size of payload
    unsigned char *payload; //content of payload
}DataNode;

typedef struct CircularBuffer{
    DataNode buff[50];
    int head;
    int tail;
    int count; //used for ambiguity
}c_buff;

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
uint64_t get_sv_filesize(int sockfd, DataNode *data){
    uint64_t filesize;
    data->type=MSG_INFO_REQ;
    send_data(sockfd,data,sizeof(data));
    receive_data(sockfd,&filesize);
    return filesize;
}


int buff_writer(FILE *fd,int n_segments,int segment_len,uint64_t start_bytes,c_buff *c_buff){

    int *tail,*head;
    unsigned char* payload=NULL;
    uint64_t *payload_size=NULL;

    fd=fopen(fd,"rb");
    if(fd == NULL){
        ferror (fd);
        return 0;
    }

    fseek(fd,start_bytes,SEEK_SET); //init file pointer
    for(c_buff->tail=0; c_buff->tail<n_segments; c_buff->tail++){
        tail=c_buff->tail;
        head=c_buff->head;
        payload=c_buff->buff[*tail].payload;
        payload_size=c_buff->buff[*tail].payload_size;

        fread(payload,segment_len,1,fd);
        payload_size=sizeof(payload);
        if(tail > head){
            
        }

    }

}

void buff_reader(){

}

/**
     * Circular Buffer for Chunkly core. It reads, partitions and sends segmented file from clt to srv
     * @param segment_len length of segment
     * @param filesize size of clt file
     * @param start_bytes size of srv file (so the start point of buff_writer)
     */
void circular_buffer(int segment_len,uint64_t filesize,uint64_t start_bytes){

    int n_segments;
    c_buff *c_buff;

    if(filesize>segment_len){
        n_segments=(filesize/segment_len)+1; //calculate number of total file segments
    }else{
        printf("[INFO] File size is lower than segments lenght configured. Skipping segmentation...\n");
        n_segments=1;
    }

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
    DataNode file;
    char base_name[512];
    int segments_len=1*1000000; //length of segments in bytes (default: 1Mb)
    uint64_t srv_filesize; //filesize returned by server if file is already present

//flags definition
    //vars for flags
    char *hostname = NULL; //destionation server
    char *server_path = NULL; // destionation path of server
    char *client_path = NULL; // file
    int ram_limit = 4; //ram limit for segment partition (default: 4Gb)
    int index;
    int c;

    opterr = 0;
    while ((c = getopt (argc, argv, "i:d:o:r:s:h")) != -1)
        switch (c)
        {
        case 'i': //input file
            client_path = optarg;
            break;
        case 'd': //destination
            hostname = optarg;
            break;
        case 'o': //server output file
            server_path = optarg;
            break;
        case 'r': //ram limit
            ram_limit = atoi(optarg);
            break;
        case 's': //segments len in byte
            segments_len = atoi(optarg);
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

    //build protocol
    file.fileName=base_name;
    file.fileName_len=strlen(base_name);
    file.file_size=get_lc_filesize(client_path);

    //set buffer ring

    //utilizzare un ring buffer circolare
    if(segments_len > file.file_size){
        segments_num=file.file_size / segments_len;
        file_splitter(client_path,&file,get_sv_filesize(sockfd,&file));
    }else{
        printf("[INFO] File size is lower than segments lenght configured. Skipping segmentation...\n");
    }

    //Open TCP socket
    sockfd=open_tcp_socket(hostname);

    
    if(close(sockfd)<0){
        printf("[ERROR] Failed to close connection\n");
    }

    return 0;
    
}