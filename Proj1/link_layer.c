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

char* port[20];
int baudrate;
unsigned int sequenceNumber;
unsigned int timeout;
unsigned int maxTimeouts;

char* frame[MAX_SIZE];

int llopen(int port, int mode){
    printf("Not implemented yet\n");
    return -1;
}

int llwrite(int fd, char* buffer, int length){
    printf("Not implemented yet\n");
    return -1;
}

int llread(int fd, char* buffer){
    printf("Not implemented yet\n");
    return -1;
}

int llclose(int fd){
    printf("Not implemented yet\n");
    return -1;
}