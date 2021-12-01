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

char* portName;
int baudrate = DEFAULT_BAUD;
unsigned int sequenceNumber;
unsigned int timeout = 1;
unsigned int maxTimeouts = 5;

unsigned int s = 0;
unsigned int r = 1;

char frame[MAX_PACKET_SIZE];

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
        portName = "/dev/ttyS0";
        break;
    
    case 1:
        portName = "/dev/ttyS1";
        break;

    case 2:
        portName = "/dev/ttyS2";
        break;

    case 10:
        portName = "/dev/ttyS10";
        break;

    case 11:
        portName = "/dev/ttyS11";
        break;
    
    default:
        perror("Invalid port number");
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

    char buffer[255];
    int state;
    timeoutCount = 0;
    sequenceNumber = 0;
    flag = 0;

    (void) signal(SIGALRM, timeout_handler);

    while(timeoutCount < maxTimeouts && flag < 2){
        flag = 0;
        alarm(timeout);

        write(fd, set, 5);   
            
        state = OTHER_RCV;

        while (flag == 0) {   
                
            read(fd, buffer, 1);   
                            
            switch(state){
                case OTHER_RCV:
                    if(buffer[0] == FLAG) { state = FLAG_RCV; }
                    break;
                    
                case FLAG_RCV:
                    if(buffer[0] == FLAG) { state = FLAG_RCV; }
                    else if(buffer[0] == A_RE) { state = A_RCV; }
                    else { state = OTHER_RCV; }
                    break;
                    
                case A_RCV:
                    if(buffer[0] == FLAG) { state = FLAG_RCV; }
                    else if(buffer[0] == UA) { state = C_RCV; }
                    else { state = OTHER_RCV; }
                    break;
                    
                case C_RCV:
                    if(buffer[0] == FLAG) { state = FLAG_RCV; }
                    else if(buffer[0] == (A_RE ^ UA)) { state = BCC_RCV; }
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
        perror("Too many timeouts ocurred, couldn't acnowledge port");
        return -1;
    }

    return 0;
}

int setup_receiver(int fd){
    char ua[5] = {FLAG, A_RE, UA, A_RE ^ UA, FLAG};

    char buffer[255];
    int state;
    int stop = FALSE;

    while (stop == FALSE) {       
      read(fd, &buffer[0], 1);   
                   
      switch(state){
        case OTHER_RCV:
            if(buffer[0] == FLAG) { state = FLAG_RCV; }
            break;
        
        case FLAG_RCV:
            if(buffer[0] == FLAG) { state = FLAG_RCV; }
            else if(buffer[0] == A_TR) { state = A_RCV; }
            else { state = OTHER_RCV; }
            break;
        
        case A_RCV:
            if(buffer[0] == FLAG) { state = FLAG_RCV; }
            else if(buffer[0] == SET) { state = C_RCV; }
            else { state = OTHER_RCV; }
            break;
        
        case C_RCV:
            if(buffer[0] == FLAG) { state = FLAG_RCV; }
            else if(buffer[0] == (A_TR ^ SET)) { state = BCC_RCV; }
            else { state = OTHER_RCV; }
            break;
        
        case BCC_RCV:
            if(buffer[0] == FLAG) { stop = TRUE; }
            else { state = OTHER_RCV; }
            break;
      }
        
    }

    write(fd, ua, 5); 

    return 0;
}

int send_disconect_message(int fd){
    char disc[5] = {FLAG, A_TR, DISC, A_TR ^ DISC, FLAG};
    char ua[5] = {FLAG, A_TR, UA, A_TR ^ UA, FLAG};

    char buffer[255];
    int state;
    timeoutCount = 0;
    flag = 0;
        
    (void) signal(SIGALRM, timeout_handler);

    while(timeoutCount < maxTimeouts && flag < 2){
        flag = 0;
        alarm(timeout);

        write(fd, disc, 5);   
                
        state = OTHER_RCV;

        while (flag == 0) {   
                
            read(fd, buffer, 1);   
                                
            switch(state){
                case OTHER_RCV:
                    if(buffer[0] == FLAG) { state = FLAG_RCV; }
                    break;
                        
                case FLAG_RCV:
                    if(buffer[0] == FLAG) { state = FLAG_RCV; }
                    else if(buffer[0] == A_RE) { state = A_RCV; }
                    else { state = OTHER_RCV; }
                    break;
                        
                case A_RCV:
                    if(buffer[0] == FLAG) { state = FLAG_RCV; }
                    else if(buffer[0] == DISC) { state = C_RCV; }
                    else { state = OTHER_RCV; }
                    break;
                        
                case C_RCV:
                    if(buffer[0] == FLAG) { state = FLAG_RCV; }
                    else if(buffer[0] == (A_RE ^ DISC)) { state = BCC_RCV; }
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

    if(flag == 2){
        write(fd, ua, 5);
    }

    else{
        perror("Too many timeouts ocurred, couldn't send acnowledgement message on close");
        return -1;
    }

    return 0;
}

int send_disconect_answer(int fd){
    char disc[5] = {FLAG, A_RE, DISC, A_RE ^ DISC, FLAG};

    char buffer[255];
    int state;
    timeoutCount = 0;
    int stop = FALSE;

    while(stop == FALSE){
        read(fd, &buffer[0], 1);   
                   
        switch(state){
            case OTHER_RCV:
                if(buffer[0] == FLAG) { state = FLAG_RCV; }
                break;
            
            case FLAG_RCV:
                if(buffer[0] == FLAG) { state = FLAG_RCV; }
                else if(buffer[0] == A_TR) { state = A_RCV; }
                else { state = OTHER_RCV; }
                break;
            
            case A_RCV:
                if(buffer[0] == FLAG) { state = FLAG_RCV; }
                else if(buffer[0] == DISC) { state = C_RCV; }
                else { state = OTHER_RCV; }
                break;
            
            case C_RCV:
                if(buffer[0] == FLAG) { state = FLAG_RCV; }
                else if(buffer[0] == (A_TR ^ DISC)) { state = BCC_RCV; }
                else { state = OTHER_RCV; }
                break;
            
            case BCC_RCV:
                if(buffer[0] == FLAG) { stop = TRUE; }
                else { state = OTHER_RCV; }
                break;
        }
    }

    write(fd, disc, 5);
    flag = 0;

    (void) signal(SIGALRM, timeout_handler);
    alarm(timeout);

    while(flag == 0){
        read(fd, &buffer[0], 1);   
                   
        switch(state){
            case OTHER_RCV:
                if(buffer[0] == FLAG) { state = FLAG_RCV; }
                break;
            
            case FLAG_RCV:
                if(buffer[0] == FLAG) { state = FLAG_RCV; }
                else if(buffer[0] == A_TR) { state = A_RCV; }
                else { state = OTHER_RCV; }
                break;
            
            case A_RCV:
                if(buffer[0] == FLAG) { state = FLAG_RCV; }
                else if(buffer[0] == UA) { state = C_RCV; }
                else { state = OTHER_RCV; }
                break;
            
            case C_RCV:
                if(buffer[0] == FLAG) { state = FLAG_RCV; }
                else if(buffer[0] == (A_TR ^ UA)) { state = BCC_RCV; }
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

    if(flag == 1){
        perror("UA acnowledgement not received");
        return -1;
    }

    return 0;
}

byte_stuffing(char *frame, int *length){
    int extra_length = 0;
    int prev_length = *length;

    for(int i = 0; i < prev_length - FRAME_INFO_SIZE; i++){
        if(frame[i + FRAME_INFO_SIZE] == FLAG || frame[i + FRAME_INFO_SIZE] == ESCAPE){
            extra_length++;
        }
    }

    *length += extra_length;
    char tempFrame[*length];
    int j = 0;
    
    for(int i = 0; i < prev_length - FRAME_INFO_SIZE; i++){
        if(frame[i + FRAME_INFO_SIZE] == FLAG){
            tempFrame[j] = ESCAPE;
            tempFrame[j+1] = FLAG ^ XOR_BYTE;
            j += 2;
        }
        else if(frame[i + FRAME_INFO_SIZE] == ESCAPE){
            tempFrame[j] = ESCAPE;
            tempFrame[j+1] = ESCAPE ^ XOR_BYTE;
            j += 2;
        }
        else{
            tempFrame[j] = frame[i];
            j++;
        }   
    }

    for(int i = 0; i < *length; i++){
        frame[i + FRAME_INFO_SIZE] = tempFrame[i];
    }

    frame[*length - 1] = FLAG;
}

int llopen(int port, int mode){
    int fd;

    fd = setup_port(port);

    if(fd < 0){
        return -1;
    }

    if(mode == TRANSMITER){
        if(setup_transmitter(fd) < 0) { return -1; }
    }

    else if(mode == RECEIVER){
        if(setup_receiver(fd) < 0) { return -1; }
    }

    else{
        perror("Invalid mode");
        return -1;
    }

    printf("Port sucessfully opened\n");

    return fd;
}

int llwrite(int fd, char* buffer, int length){

    int frame_length = length + FRAME_INFO_SIZE;
    char bcc2;
    
    frame[0] = FLAG;
    frame[1] = A_TR;
    
    if(r == 1){
        frame[2] = CTR_I_FRAME1;
        frame[3] = A_TR ^ CTR_I_FRAME1;
    }

    else if(r == 0){
        frame[2] = CTR_I_FRAME0;
        frame[3] = A_TR ^ CTR_I_FRAME0;
    }

    for (int i = 0; i < length; i++)
    {
        frame[i + INITIAL_FRAME_BITS] = buffer[i];
        if(i == 0){
            bcc2 = buffer[i];
        }
        else{
            bcc2 ^= buffer[i];
        }
    }
    
    frame[frame_length - 2] = bcc2;
    frame[frame_length - 1] = FLAG;

    byte_stuffing(frame, &frame_length);

    else{
        perror("r has an erroneous value, it should only take values of 1 or 0");
    }

    return -1;
}

int llread(int fd, char* buffer){
    printf("Not implemented yet\n");
    return -1;
}

int llclose(int fd){

    if(fcntl(fd, F_GETFD) < 0){
        perror("Invalid fd");
        return -1;
    }
    
    if(sequenceNumber == 0){
        if(send_disconect_message(fd) < 0){
            return -1;
        }
    }

    else if(sequenceNumber == 1){
        if(send_disconect_answer(fd) < 0){
            return -1;
        }
    }

    else{
        perror("Invalid sequence number!");
        return -1;
    }

    if(tcsetattr(fd, TCSANOW, &oldtio) == -1) {
        perror("Error while restoring port settings");
        return -1;
    }

    if(close(fd) != 0){
        perror("Error while closing port");
        return -1;
    }

    return 1;
}