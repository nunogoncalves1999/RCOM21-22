
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "defs.h"
#include "link_layer.h"

int fd;
int mode;

char* path;
size_t packet_size;
FILE* file;

int main(int argc, char *argv[])
{

    //Process args for transmitter and receiver modes
    if(argc < 3){
        printf("Usage: file_transfer <port 0|1|2|11|12> <mode 0|1> <file path> <packet size>\n
                port: /dev/ttySn, where n is the number entered\n
                mode: 0 for transmiter and 1 for receiver\n
                file path and packet size should only be entered if mode is 0 (transmiter)\n");
        
        return -1;
    }

    mode = argv[2];
    if(mode != 0 && mode != 1){
        printf("Invalid mode; mode can only be 0 (transmiter) or 1 (receiver)\n");
        return -1;
    }

    if(mode == 0){
        if(argc < 5){
            printf("Usage: file_transfer <port 0|1|2|11|12> <mode 0|1> <file path> <packet size>\n
                port: /dev/ttySn, where n is the number entered\n
                mode: 0 for transmiter and 1 for receiver\n
                file path and packet size should only be entered if mode is 0 (transmiter)\n");
                return -1;
        }

        path = argv[3];
        file = fopen(path, "r");
        
        if(file == NULL){
            perror("Couldn't open the file indicated in <file path>");
            return -1;
        }

        packet_size = argv[4];
    }

    fd = llopen(argv[1], mode);

    if(fd < 0){
        printf("Error while opening port\n");
        return -1;
    }

    //Read loop if receiver

    //Write loop if transmitter

    //Closing file, cleaning up used space and printing debugs to check errors
}