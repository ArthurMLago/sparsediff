#define _GNU_SOURCE 1

#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BYTES_PER_LINE 16

struct popts{
    char print_data_header;
    char use_colors;
    unsigned context_lines;
} program_options = {0,0,3};

int min(int v1, int v2){
    if (v1 < v2)
        return v1;
    return v2;
}

int max(int v1, int v2){
    if (v1 > v2)
        return v1;
    return v2;
}

int main(int argc, char **argv){
    // Allowing more than 2 files someday?
    unsigned nfilenames = 0;
    char* filenames[8];
    for (int i = 1; i < argc; i++){
        if (argv[i][0] == '-'){
            if (!strcmp(argv[i] + 1, "-color-diffs") || !strcmp(argv[i] + 1, "c")){
                program_options.use_colors = 1;
            }else if (!strcmp(argv[i] + 1, "-data-marker") || !strcmp(argv[i] + 1, "m")){
                program_options.print_data_header = 1;
            }else if (!strcmp(argv[i] + 1, "-context-lines") || !strcmp(argv[i] + 1, "l")){
                program_options.context_lines = atoi(argv[i + 1]);
                i++;
            }
        }else{
            filenames[nfilenames++] = argv[i];
        }
    }

    int fd1, fd2;
    fd1 = open(filenames[0], O_RDONLY);
    if (fd1 < 0){
        fprintf(stderr, "Error opening file %s(errno %d: %s)\n", filenames[0], errno, strerror(errno));
        return errno;
    }
    fd2 = open(filenames[1], O_RDONLY);
    if (fd2 < 0){
        fprintf(stderr, "Error opening file %s(errno %d: %s)\n", filenames[1], errno, strerror(errno));
    }

    uint8_t buff1[4096];
    uint8_t buff2[4096];
    off_t next_data = 0;
    off_t next_hole = 0;
    while(next_data != -1){
        unsigned stored_lines = program_options.context_lines * 2 + 5;
        char lines[stored_lines][512];
        int lastDifference = -1000000;
        int lastStop = -1;
        next_data = lseek(fd1, next_hole, SEEK_DATA);
        next_hole = lseek(fd1, next_data, SEEK_HOLE);

        lseek(fd1, next_data, SEEK_SET);
        lseek(fd2, next_data, SEEK_SET);

        unsigned nread = 0;
        unsigned target = next_hole - next_data;
        unsigned scanfret;
        if (program_options.print_data_header){
            printf("***New Page*** %010lX:\n", next_data);
        }
        while(nread < target){
            int ret1 = read(fd1, buff1, min(target - nread, 4096));
            int ret2 = read(fd2, buff2, min(target - nread, 4096));
            if (ret1 != ret2){
                return -7;
            }
            nread += ret1;
            for(int i = 0; i < ret1/BYTES_PER_LINE; i++){
                unsigned currentRow = (nread- ret1)/BYTES_PER_LINE + i;
                scanfret = sprintf(lines[currentRow % stored_lines], "%10lX | ", next_data + currentRow*BYTES_PER_LINE);
                char colored = 0;
                for (int j = 0; j < BYTES_PER_LINE; j++){
                    if (program_options.use_colors){
                        if (!colored && buff1[i * BYTES_PER_LINE + j] != buff2[i * BYTES_PER_LINE + j]){
                            scanfret += sprintf(lines[currentRow % stored_lines] + scanfret, "\e[31m");
                            colored = 1;
                        }else if(colored && buff1[i * BYTES_PER_LINE + j] == buff2[i * BYTES_PER_LINE + j]){
                            scanfret += sprintf(lines[currentRow % stored_lines] + scanfret, "\e[0m");
                            colored = 0;
                        }
                    }
                    scanfret += sprintf(lines[currentRow % stored_lines] + scanfret, "%02X ", buff1[i * BYTES_PER_LINE + j]);
                }
                if (colored){
                    scanfret += sprintf(lines[currentRow % stored_lines] + scanfret, "\e[0m");
                    colored = 0;
                }
                scanfret += sprintf(lines[currentRow % stored_lines] + scanfret, "| ");
                for (int j = 0; j < BYTES_PER_LINE; j++){
                    if (buff1[i * BYTES_PER_LINE + j] != buff2[i * BYTES_PER_LINE + j]){
                        if (program_options.use_colors && !colored){
                            scanfret += sprintf(lines[currentRow % stored_lines] + scanfret, "\e[31m");
                            colored = 1;
                        }
                        lastDifference = currentRow;
                    }else{
                        if (program_options.use_colors && colored){
                            scanfret += sprintf(lines[currentRow % stored_lines] + scanfret, "\e[0m");
                            colored = 0;
                        }
                    }
                    scanfret += sprintf(lines[currentRow % stored_lines] + scanfret, "%02X ", buff2[i * BYTES_PER_LINE + j]);
                }
                if (colored){
                    scanfret += sprintf(lines[currentRow % stored_lines] + scanfret, "\e[0m");
                    colored = 0;
                }
                scanfret += sprintf(lines[currentRow % stored_lines] + scanfret, "\n");
                if ( currentRow - lastDifference > program_options.context_lines){
                    // NOP
                }else if(currentRow == lastDifference){
                    if (currentRow > lastStop + 1 || (currentRow == 0 && !program_options.print_data_header )){
                        printf("...\n");
                    }
                    for (int j = max(lastStop + 1, currentRow - program_options.context_lines); j <= currentRow; j++){
                        printf("%s", lines[j % stored_lines]);
                    }
                    lastStop = currentRow;
                }else{
                    printf("%s", lines[currentRow % stored_lines]);
                    lastStop = currentRow;
                }
            }
        }
    }
}

