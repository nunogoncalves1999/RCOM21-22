
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
int portNr;

char* path; 
size_t packet_size;
FILE* file;

int main(int argc, char *argv[])
{

    //Process args for transmitter and receiver modes
    if(argc < 3){
        printf("Usage: file_transfer <port 0|1|2|11|12> <mode 0|1> <file path> <packet size>\n");
        printf("port: /dev/ttySn, where n is the number entered\n");
        printf("mode: 0 for transmiter and 1 for receiver\n");
        printf("file path and packet size should only be entered if mode is 0 (transmiter)\n");
        
        return -1;
    }

    portNr = strtol(argv[1], NULL, 10);
    mode = strtol(argv[2], NULL, 10);

    if(mode != 0 && mode != 1){
        printf("Invalid mode; mode can only be 0 (transmiter) or 1 (receiver)\n");
        return -1;
    }

    if(mode == 0){
        if(argc < 5){
            printf("Usage: file_transfer <port 0|1|2|11|12> <mode 0|1> <file path> <packet size>\n");
            printf("port: /dev/ttySn, where n is the number entered\n");
            printf("mode: 0 for transmiter and 1 for receiver\n");
            printf("file path and packet size should only be entered if mode is 0 (transmiter)\n");
            return -1;
        }

        path = argv[3];
        file = fopen(path, "rb");
        
        if(file == NULL){
            perror("Couldn't open the file indicated in <file path>");
            return -1;
        }

        packet_size = strtol(argv[4], NULL, 10);

        if(packet_size > MAX_PACKET_SIZE){
            perror("Packet size too big");
            return -1;
        }
    }

    fd = llopen(portNr, mode);

    if(fd < 0){
        return -1;
    }

    //Read loop if receiver
    if(mode == 1){

    }

    //Write loop if transmitter
    if(mode == 0){

    }

    //Closing file, cleaning up used space and printing debugs to check errors
    fclose(file);

    if(llclose(fd) != 1){
        return -1;
    }

    return 0;
}