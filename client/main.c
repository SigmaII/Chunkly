
/*

uint32_t name_len = strlen(filename); //mette la lunghezza del nome del file in name_len, che è di tipo int a 32 bit (4 byte) 
uint32_t nlen = htonl(name_len);  // prende la lunghezza del nome del file (es. 1 byte) e la converte da int little endian (formato macchina) a int big endian (formato rete)

memcpy(buffer + offset, &nlen, 4); //mette in una variabile buffer i 4 byte di nlen, che contengono la dimensione del nome del file a partire dalla posizione 0 (offset) 
offset += 4;                       // avanza di 4 byte

memcpy(buffer + offset, filename, name_len); //mette il nome effettivo del file nella variabile buffer
offset += name_len;                          // avanza di n byte (n=name_len)

uint64_t fsize = htobe64(size);             // crea una variabile di tipo int a 64 bit e converte la int size litte endian in int size big endian

memcpy(buffer + offset, &fsize, 8);         // mette la dimensione del file nel buffer dedicandogli 8 byte (es. "3.5Gb" occuperà 2 byte)
offset += 8;                                // avanza di 8 byte

*/

// I primi 4 byte del messaggio sono predisposti per la lunghezza del nome del file
// altri <lunghezza_nome_file_in_byte> byte sono predisposti per il nome del file
// altri 8 byte sono predisposti per la dimensione del payload
//gli ultimi <dimensione_payload_in_byte> byte sono predisposti per il payload
//NB: è necessario convertire in notazione big endian solo il primo e il terzo elemento tra quelli appena elencati

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
/* 
L'Obiettivo è abilitare il protocollo al resume, passando il numero di segments uploaded (cosi il server impostera il file pointer a payload size * segments_uploaded)
e la lunghezza del file per fare il controllo finale e restituire al client una risposta di completezza del trasferimento.
N.B: per il resume va gestito bene l'ultimo segmento (magari per il resume facciamo ripartire il client a caricare sempre da segments_uploaded-1 cosi evitiamo il rischio) 
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

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>

struct stat st = {0};

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

void debug_payloads(struct message m){
    printf("Contenuto del file:\n");
    for (int j = 0; j < m.payload_size; j++) {
            unsigned char c = m.payload[j];
            if (c >= 32 && c <= 126) // caratteri stampabili ASCII
                printf("%c", c);
            else
                printf(".");         // per byte non stampabili
    }
    printf("\n");
}

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

    //imposto la grandezza del nome del file in formato big endian
    uint32_t nlen = htonl(data->fileName_len);
    //metto la grandezza del nome del file nel buffer ( primi 4 byte )
    memcpy(*buffer + data->offset, &nlen, 4);
    data->offset += 4;

    //metto il nome del file nel buffer
    memcpy(*buffer + data->offset, data->fileName, data->fileName_len);
    data->offset+=data->fileName_len;

    //metto il valore di compressed nel buffer
    uint32_t compressed = htonl(data->compressed);
    memcpy(*buffer + data->offset, &compressed, 4);
    data->offset += 4;

    //imposto la grandezza del file in formato big endian
    uint64_t file_size = htobe64(data->file_size);
    //metto la grandezza del file nel buffer ( 8 byte )
    memcpy(*buffer + data->offset, &file_size, 8);
    data->offset+=8;

    //imposto la grandezza del segmento in big endian
    uint64_t chunk_size = htobe64(data->payload_size);
    //metto la grandezza del segmento nel buffer (b byte)
    memcpy(*buffer + data->offset, &chunk_size, 8);
    data->offset+=8;
    return 1;

}

int main (int argc, char *argv[])
{

    if (argc < 4) {
    fprintf(stderr, "Use: %s <filename> <address> <path>\n", argv[0]);
    return 1;
    }

    int i;
    uint64_t chunk_size;
    size_t buff_size;
    uint8_t *buffer; // gli uint corrispondono agli unsigned, ad esempio uint8 = unsigned da 8 bit = unsigned char
    uint8_t *buffPath;
    char *addr=argv[2];
    char *path=argv[3];
    char *tfile= argv[1];
    char file[512];
    char prog[512];
    //char *prog = basename (argv[1]); // trasforma il path passato (nel caso in cui sia un path) nel file finale
    snprintf(prog, sizeof(prog), "%s", basename(tfile));
    snprintf(file, sizeof(file), "%s", tfile);

    FILE *fd;
    long segments=0;
    int last_segment;

    struct message data;
    //struct message m;
    data.payload=NULL;
    data.offset=0;
    data.file_size=0;
    data.payload_size=0;
    uint64_t uploaded_bytes;
    uint64_t total_bytes;

    if (isDirectory(prog)){
        printf("progprima:%s\n",prog);
        printf("addressprima:%s\n",addr);
        printf("fileprima:%s\n",file);
        compress_folder(file,prog,sizeof(prog));
        snprintf(file, sizeof(file), "%s", prog);
        printf("filedopo:%s\n",file);
        printf("progdopo:%s\n",prog);
        printf("addressdopo:%s\n",addr);
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

    int sockfd;
    // inizializzo il socket
    sockfd=OpenSocket(addr);

    buff_size=4+data.fileName_len+20;
    if((buffPath=malloc(buff_size)) == NULL){
        fprintf (stderr, "error: virtual memory exhausted.\n");
        return 1;
    }
    bzero(buffPath,buff_size);
    protocol_builder(&buffPath, &data);
    //Invio un primo messaggio, senza payload, per controllare che il server abbia il file

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
    //calcolo i segments affinché non superino i 2gb di ram massima

    if (total_bytes>10000000000){

        segments= (long)((total_bytes/((total_bytes/2000000000)+1))); //se il file supera i 10 gb calcolo i segments affinché non superino i 2gb
    }else{
        segments=5; // altrimenti divido sempre per 5 segmenti
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

    bzero(buffer,buff_size);

    fseek (fd, data.uploaded_bytes, SEEK_SET);
    
    protocol_builder(&buffer, &data);
    
    printf("Size of file name: %d byte\n", data.fileName_len);
    printf("Name of file: %s\n", data.fileName);
    printf("Chunk size: %ld byte\n", data.payload_size);



    //ad ogni ciclo cambio il payload mantenendo gli stessi metadati (per <segments> volte)


    for(i=0; i<segments; i++){

        //write_metadata(&data, i, prog, segments);
        file_splitter(fd, &data, i, segments, last_segment);
        if (i==segments-1){
            buff_size=4 + data.fileName_len + 4 + 8+ 8 + data.payload_size;
            printf("Last Buffer Size: %zd\n",buff_size);
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
        printf("%ld\n",data.payload_size);
        //debug_payloads(m);
            
        SendMessage(sockfd, buffer, buff_size);
    }                
    CloseSocket(sockfd);


    fclose (fd);
    free (buffer);
    free(data.payload);
    free (buffPath);
    return 0;
}