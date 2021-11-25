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

char* portName[20];
int baudrate = DEFAULT_BAUD;
unsigned int sequenceNumber;
unsigned int timeout;
unsigned int maxTimeouts;

char* frame[MAX_SIZE];

struct termios oldtio;

//Opens the port identified by the port number
//Returns fd on sucess and -1 on failure
int setup_port(int port){

    int fd;

    switch (port)
    {
    case 0:
        portName = "/dev/ttyS0"
        break;
    
    case 1:
        portName = "/dev/ttyS1"
        break;

    case 2:
        portName = "/dev/ttyS2"
        break;

    case 10:
        portName = "/dev/ttyS10"
        break;

    case 11:
        portName = "/dev/ttyS11"
        break;
    
    default:
        perror("Invalid port number\n");
        return -1;
    }

    fd = open(portName, O_RDWR);

    if(fd < 0){
        perror("Failed to open specified port");
        return -1;
    }

    if (tcgetattr(fd, &oldtio) == -1) { /* save current port settings */
      perror("Error on tcgetattr");
      return -1;
    }

    struct termios newtio;

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = baudrate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0.1;   
    newtio.c_cc[VMIN]     = 0;  

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
      perror("Error on tcsetattr");
      return -1;
    }

    return fd;
}

int llopen(int port, int mode){
    int fd;
    int errorValue;

    fd = setup_port(port);

    if(fd < 0){
        return -1;
    }

    if(mode == TRANSMITER){
        char set[5] = {FLAG, A_TR, C_TR, BCC1_TR, FLAG};

        tcflush(fd, TCIOFLUSH);
    }

    else if(mode == RECEIVER){
        char ua[5] = {FLAG, A_RE, C_RE, BCC1_RE, FLAG};
    }

    else{
        printf("Error! Invalid mode\n");
        return -1;
    }

    return fd;
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