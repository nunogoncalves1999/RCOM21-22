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
unsigned int timeout = 1;
unsigned int maxTimeouts = 5;

char* frame[MAX_SIZE];

struct termios oldtio;

int timeoutCount = 0;

//flag controls the timeout mechanism; 0 if waiting, 1 if timeout and 2 if received
volatile int flag = 0;

void timeout_handler()                   
{
	printf("timeout nº%d\n", timeoutCount);
	flag = 1;
	timeoutCount++;
}

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

int setup_transmitter(int fd){
    char set[5] = {FLAG, A_TR, SET, A_TR ^ SET, FLAG};

    int state;
    timeoutCount = 0;
    sequenceNumber = 0;
    flag = 0;

    (void) signal(SIGALRM, timeout_handler);

    while(timeoutCount < maxTimeouts && flag < 2){
        flag = 0;
        alarm(timeout);

        write(fd,set,5);   
            
        state = OTHER_RCV;

        while (flag == 0) {   
                
            read(fd, buffer, 1);   
                            
            switch(state){
                case OTHER_RCV:
                    if(buffer[0] == FLAG) { state = FLAG_RCV; }
                    break;
                    
                case FLAG_RCV:
                    if(buffer[0] == FLAG) { state = FLAG_RCV; }
                    else if(buf[0] == A_RE) { state = A_RCV; }
                    else { state = OTHER_RCV; }
                    break;
                    
                case A_RCV:
                    if(buffer[0] == FLAG) { state = FLAG_RCV; }
                    else if(buf[0] == UA) { state = C_RCV; }
                    else { state = OTHER_RCV; }
                    break;
                    
                case C_RCV:
                    if(buffer[0] == FLAG) { state = FLAG_RCV; }
                    else if(buf[0] == A_RE ^ UA) { state = BCC_RCV; }
                    else { state = OTHER_RCV; }
                    break;
                    
                case BCC_RCV:
                    if(buffer[0] == FLAG) {  
                        alarm(0);
                        flag = 2;
                    }
                    else { state = OTHER_RCV; }
                    break;
            }
                    
        }
    }

    if(timeoutCount >= maxTimeouts){
        perror("Too many timeouts ocurred, couldn't acnowledge port\n");
        return -1;
    }

    return 0;
}

int llopen(int port, int mode){
    int fd;
    int errorValue;
    char* buffer[255];

    fd = setup_port(port);

    if(fd < 0){
        return -1;
    }

    if(mode == TRANSMITER){
        if(setup_transmitter(fd) < 0) { return -1; }
    }

    else if(mode == RECEIVER){
        sequenceNumber = 1;
    }

    else{
        printf("Error! Invalid mode\n");
        return -1;
    }

    printf("Port sucessfully opened\n");

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
    
    if(sequenceNumber == 0){
        char disc[5] = {FLAG, A_TR, DISC, A_TR ^ DISC, FLAG};

        if(fcntl(fd, GETFD) < 0){
            perror("Invalid fd\n");
            return -1;
        }

        int state;
        timeoutCount = 0;
        flag = 0;
        
        (void) signal(SIGALRM, timeout_handler);

        while(timeoutCount < maxTimeouts && flag < 2){

        }
    }

    else if(sequenceNumber == 1){

    }

    else{
        printf("Invalid sequence number!");
        return -1;
    }

    return 1;
}