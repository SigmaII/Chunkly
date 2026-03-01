#define _GNU_SOURCE
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

void compress_folder(char *file, char *prog, size_t size) {

    char cmd[1024];
    char path_copy1[512];
    char path_copy2[512];


    // Copy of data because dirname and basename change the original string
    snprintf(path_copy1, sizeof(path_copy1), "%s", file);
    snprintf(path_copy2, sizeof(path_copy2), "%s", file);

    char *dir  = dirname(path_copy1);
    char *base = basename(path_copy2);
    char finalprog[size];

    //temporary variable for if condition
    snprintf(finalprog, size, "%s.tar.gz", base);

    if (access(finalprog, F_OK) != 0){

        snprintf(cmd, sizeof(cmd),
                "tar -zcf \"%s.tar.gz\" -C \"%s\" \"%s\"",
                base, dir, base);

        printf("CMD: %s\n", cmd);

        int ret = system(cmd);
        if (ret != 0) {
            printf("[ERROR] tar failed\n");
            return;
        }

        printf("[INFO] Directory compressed successfully\n");

        //new name for prog
        snprintf(prog, size, "%s.tar.gz", base);
    }else{
        printf("cached tar detected\n");
        //new name for prog
        snprintf(prog, size, "%s.tar.gz", base);
    }

}