#define _GNU_SOURCE
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

void decompress_folder(char *file) {

    char cmd[1024];
    char path_copy[512];

    snprintf(path_copy, sizeof(path_copy), "%s", file);

    char *dir = dirname(path_copy);

    snprintf(cmd, sizeof(cmd),
             "tar xf \"%s\" -C \"%s\"",
             file, dir);

    printf("CMD: %s\n", cmd);

    int ret = system(cmd);
    if (ret != 0) {
        printf("[ERROR] tar extraction failed\n");
    }else{
        if (remove(file) == 0) {
            printf("Compressed file deleted successfully.\n");
        } else {
            printf("Error: Unable to delete the compressed file.\n");
        }
    }
}
