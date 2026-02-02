#define _GNU_SOURCE
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

void compress_folder(char *file,char *prog) {
    char buffer[512];
    char cprog[512];
    snprintf(buffer, sizeof(buffer), "tar -zcf %s.tar.gz -C \"$(dirname %s)\" %s", prog, file, prog);
    if(system("tar --version > /dev/null 2>&1") == 0){
        if(system(buffer)==0){
            printf("[INFO] Directory has been compressed\n");
            snprintf(cprog, sizeof(cprog), "%s.tar.gz", prog);
            //questo genera il problema di sovrascrittura di addr
            strcpy(prog,cprog);
        }
    }else{
        printf("[ERROR] tar is not installed\n");
    }
}