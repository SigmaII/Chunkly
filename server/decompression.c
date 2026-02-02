#define _GNU_SOURCE
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

void decompress_folder(char *file) {
    char buffer[512];
    snprintf(buffer, sizeof(buffer),"tar xf %s -C \"$(dirname %s)\"",file, file);
    if(system("tar --version > /dev/null 2>&1") == 0){
        if(system(buffer)==0){
            printf("[INFO] Directory has been decompressed\n");
        }
    }else{
        printf("[ERROR] tar is not installed\n");
    }
}
