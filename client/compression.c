#define _GNU_SOURCE
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

void compress_folder(char *file,char *prog,size_t bufsize) {
    char buffer[512];
    char cprog[512];
    // lo script funziona solo passando la cartella senza "/" finale mentre ci si trova nella ., bisogna modificare dirname con qualcosa di piu sofisticato
    snprintf(buffer, sizeof(buffer), "tar -zcf %s.tar.gz -C \"$(dirname %s)\" %s", prog, file, prog);
    if(system("tar --version > /dev/null 2>&1") == 0){
        if(system(buffer)==0){
            printf("[INFO] Directory has been compressed\n");
            snprintf(cprog, sizeof(cprog), "%s.tar.gz", prog);
            snprintf(prog, bufsize, "%s", cprog);
        }
    }else{
        printf("[ERROR] tar is not installed\n");
    }
}