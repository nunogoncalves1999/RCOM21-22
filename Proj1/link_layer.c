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

char* port_name;
int baudrate = DEFAULT_BAUD;
unsigned int sequence_number;
unsigned int timeout = 1;
unsigned int max_timeouts = 5;

//0 if transmiter and 1 if receiver
unsigned int mode;

uint8_t frame[MAX_PACKET_SIZE];

struct termios oldtio;

int timeout_count = 0;

//flag controls the timeout mechanism; 0 if waiting, 1 if timeout and 2 if received
volatile int flag = 0;

void timeout_handler()                   
{
	printf("timeout nº%d\n", timeout_count);
	flag = 1;
	timeout_count++;
}

void flipSequenceNumber(){
    if(sequence_number == 1){
        sequence_number = 0;
    }
    else if(sequence_number == 0){
        sequence_number = 1;
    }
}

//Opens the port identified by the port number
//Returns fd on sucess and -1 on failure
int setupPort(int port){

    int fd;

    switch (port)
    {
    case 0:
        port_name = "/dev/ttyS0";
        break;
    
    case 1:
        port_name = "/dev/ttyS1";
        break;

    case 2:
        port_name = "/dev/ttyS2";
        break;

    case 10:
        port_name = "/dev/ttyS10";
        break;

    case 11:
        port_name = "/dev/ttyS11";
        break;
    
    default:
        perror("Invalid port number");
        return -1;
    }

    fd = open(port_name, O_RDWR);

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

    newtio.c_cc[VTIME] =0.1;    
    newtio.c_cc[VMIN] = 0;  

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
      perror("Error on tcsetattr");
      return -1;
    }

    return fd;
}

int checkAcknowledgement(uint8_t ack){
    
    if(ack == RR_1 || ack == REJ_1){
        if(sequence_number == 0){
            return 1;
        }
        else{
            perror("Wrong sequence number value in acknowledgement");
            return 0;
        }
    }

    if(ack == RR_0 || ack == REJ_0){
        if(sequence_number == 1){
            return 1;
        }
        else{
            perror("Wrong sequence number value in acknowledgement");
            return 0;
        }
    }

    return 0;
}

int checkControlField(uint8_t c){
    if(c == CTR_I_FRAME_1 || c == CTR_I_FRAME_0 || c == SET || c == DISC){
        return 1;
    }

    return 0;
}

int checkBcc2(uint8_t bcc2, int msg_length){
    uint8_t test_bcc2;

    for (int i = 0; i < msg_length; i++)
    {
        if(i == 0){
            test_bcc2 = frame[i];
        }
        else{
            test_bcc2 ^= frame[i];
        }
    }

    if(test_bcc2 == bcc2){
        return 1;
    }

    return 0;
}

void byteStuffing(int *length){
    int extra_length = 0;
    int prev_length = *length;

    for(int i = INITIAL_FRAME_BITS; i < prev_length - 1; i++){
        if(frame[i] == FLAG || frame[i] == ESCAPE){
            extra_length++;
        }
    }

    *length += extra_length;
    uint8_t temp_frame[*length];
    int j = 0;
    
    for(int i = INITIAL_FRAME_BITS; i < prev_length - 1; i++){
        if(frame[i] == FLAG){
            temp_frame[j] = ESCAPE;
            temp_frame[j+1] = FLAG ^ XOR_BYTE;
            j += 2;
        }
        else if(frame[i] == ESCAPE){
            temp_frame[j] = ESCAPE;
            temp_frame[j+1] = ESCAPE ^ XOR_BYTE;
            j += 2;
        }
        else{
            temp_frame[j] = frame[i];
            j++;
        }   
    }

    for(int i = 0; i < *length - FRAME_INFO_SIZE + 1; i++){
        frame[i + INITIAL_FRAME_BITS] = temp_frame[i];
    }

    frame[*length - 1] = FLAG;
}

void byteDestuffing(int *length){
    int extra_length = 0;
    int prev_length = *length;

    for(int i = INITIAL_FRAME_BITS; i < prev_length - 1; i++){
        if(frame[i] == ESCAPE && (frame[i + 1] == (FLAG ^ XOR_BYTE) || frame[i + 1] == (ESCAPE ^ XOR_BYTE))){
            extra_length++;
        }
    }

    *length -= extra_length;
    uint8_t temp_frame[*length];
    int j = 0;   

    for(int i = INITIAL_FRAME_BITS; i < prev_length - 1; i++){
        if(frame[i] == ESCAPE){
            if(frame[i + 1] == (FLAG ^ XOR_BYTE)){
                temp_frame[j] = FLAG;
            }
            else if(frame[i + 1] == (ESCAPE ^ XOR_BYTE)){
                temp_frame[j] = ESCAPE;
            }
            i++;
        }
        else{
            temp_frame[j] = frame[i];
        }   
        j++;
    }

    for(int i = 0; i < *length - FRAME_INFO_SIZE + 1; i++){
        frame[i + INITIAL_FRAME_BITS] = temp_frame[i];
    }

    frame[*length - 1] = FLAG;
}

int setupTransmiter(int fd){
    sequence_number = 0;
    mode = 0;

    uint8_t set[5] = {FLAG, A_TR, SET, A_TR ^ SET, FLAG};

    uint8_t buffer[255];
    int state;
    timeout_count = 0;
    flag = 0;

    (void) signal(SIGALRM, timeout_handler);

    while(timeout_count < max_timeouts && flag < 2){
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

    if(timeout_count >= max_timeouts){
        perror("Too many timeouts ocurred, couldn't acnowledge port");
        return -1;
    }

    return 0;
}

int setupReceiver(int fd){
    sequence_number = 1;
    mode = 1;

    uint8_t ua[5] = {FLAG, A_RE, UA, A_RE ^ UA, FLAG};

    uint8_t buffer[255];
    int state = OTHER_RCV;
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

int sendDisconectMessage(int fd){
    uint8_t disc[5] = {FLAG, A_TR, DISC, A_TR ^ DISC, FLAG};
    uint8_t ua[5] = {FLAG, A_TR, UA, A_TR ^ UA, FLAG};

    uint8_t buffer[255];
    int state;
    timeout_count = 0;
    flag = 0;
        
    (void) signal(SIGALRM, timeout_handler);

    while(timeout_count < max_timeouts && flag < 2){
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

int sendDisconectAnswer(int fd){
    uint8_t disc[5] = {FLAG, A_RE, DISC, A_RE ^ DISC, FLAG};

    uint8_t buffer[255];
    int state = OTHER_RCV;

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

int sendInfoPacket(int fd, int frame_length, uint8_t* buffer){

    int bytes_writen;
    int state;
    uint8_t ack;
    timeout_count = 0;
    flag = 0;

    (void) signal(SIGALRM, timeout_handler);

    while(timeout_count < max_timeouts && flag < 2){
        flag = 0;
        alarm(timeout);

        bytes_writen = write(fd, frame, frame_length);

        state = OTHER_RCV;

        while (flag == 0)
        {
            read(fd, buffer, 1);

            switch(state){
                case OTHER_RCV:
                    if(buffer[0] == FLAG) { 
                        state = FLAG_RCV; 
                        }
                    break;
                        
                case FLAG_RCV:
                    if(buffer[0] == FLAG) { state = FLAG_RCV; }
                    else if(buffer[0] == A_RE) { 
                        state = A_RCV; 
                    }
                    else { state = OTHER_RCV; }
                    break;
                        
                case A_RCV:
                    if(buffer[0] == FLAG) { state = FLAG_RCV; }
                    else if(checkAcknowledgement(buffer[0]) == 1) {
                        ack = buffer[0];
                        state = C_RCV; 
                    }
                    else { state = OTHER_RCV; }
                    break;
                        
                case C_RCV:
                    if(buffer[0] == FLAG) { state = FLAG_RCV; }
                    else if(buffer[0] == (A_RE ^ ack)) { 
                        state = BCC_RCV; 
                    }
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

        if(flag == 2){
            flipSequenceNumber();
        }

        else if(ack == REJ_0 || ack == REJ_1){
            //If a REJ acknowledgement is received, flag is reset to 0 so there's a retry without a timeout
            flag = 0;
        }
    }

    if(timeout_count >= max_timeouts){
        perror("Too many timeouts ocurred, didn't receive an acknowledgement");
        return -2;
    }

    //Here for debug
    else{
        //printf("Received response\n");
    }

    return bytes_writen;
}

int readPacket(int fd, uint8_t* buffer){
    uint8_t control_field;
    int bytes_read;
    int state = OTHER_RCV;
    int stop = FALSE;

    while (stop == FALSE) {    
      read(fd, &buffer[0], 1); 

      switch(state){
        case OTHER_RCV:
            if(buffer[0] == FLAG) { 
                bytes_read = 1;
                state = FLAG_RCV; 
            }
            break;
        
        case FLAG_RCV:
            if(buffer[0] == FLAG) { state = FLAG_RCV; }
            else if(buffer[0] == A_TR) { 
                bytes_read++;
                state = A_RCV; 
            }
            else { state = OTHER_RCV; }
            break;
        
        case A_RCV:
            if(buffer[0] == FLAG) { state = FLAG_RCV; }
            else if(checkControlField(buffer[0]) == 1) { 
                bytes_read++;
                control_field = buffer[0];
                state = C_RCV; 
            }
            else { state = OTHER_RCV; }
            break;
        
        case C_RCV:
            if(buffer[0] == FLAG) { state = FLAG_RCV; }
            else if(buffer[0] == (A_TR ^ control_field)) { 
                bytes_read++;
                state = BCC_RCV; 
            }
            else { state = OTHER_RCV; }
            break;
        
        case BCC_RCV:
            if(buffer[0] == FLAG) { 
                stop = TRUE; 
                //printf("packet fully received\n");
            }
            else { bytes_read++; }
            break;
      }

      frame[bytes_read - 1] = buffer[0];
    }

    if(control_field == CTR_I_FRAME_1 || control_field == CTR_I_FRAME_0){
        //printf("Control entered\n");
        uint8_t answer[5];
        byteDestuffing(&bytes_read);
        //printf("Destuffing done\n");

        uint8_t bcc2 = frame[bytes_read - 2];

        answer[0] = FLAG;
        answer[1] = A_RE;
        answer[4] = FLAG;

        if(checkBcc2(bcc2, bytes_read - FRAME_INFO_SIZE) == 1){
            //printf("Sending REJ\n");
            if(sequence_number == 1){
                answer[2] = REJ_1;
                answer[3] = A_RE ^ REJ_1;
            }
            else if (sequence_number == 0){
                answer[2] = REJ_0;
                answer[3] = A_RE ^ REJ_0;
            }

            write(fd, answer, 5);
        }

        else{
            if(sequence_number == 1){
                answer[2] = RR_1;
                answer[3] = A_RE ^ RR_1;
            }
            else if(sequence_number == 0){
                answer[2] = RR_0;
                answer[3] = A_RE ^ RR_0;
            }

            write(fd, answer, 5);
            //printf("Wrote RR\n");

            memcpy(buffer, &frame[INITIAL_FRAME_BITS], bytes_read - FRAME_INFO_SIZE);

            flipSequenceNumber();
        }
    }

    else if(control_field == SET){
        uint8_t ua[5] = {FLAG, A_RE, UA, A_RE ^ UA, FLAG};

        write(fd, ua, 5);
    }

    else if(control_field == DISC){

        if(sendDisconectAnswer(fd) < 0){
            bytes_read = DISC_ERROR;
        }
        else{
            bytes_read = DISC_RETURN;
        }
    }

    return bytes_read;
}

int llopen(int port, int mode){
    int fd;

    fd = setupPort(port);

    if(fd < 0){
        return -1;
    }

    if(mode == TRANSMITER){
        if(setupTransmiter(fd) < 0) { return -1; }
    }

    else if(mode == RECEIVER){
        if(setupReceiver(fd) < 0) { return -1; }
    }

    else{
        perror("Invalid mode");
        return -1;
    }

    //printf("Port sucessfully opened\n");

    return fd;
}

int llwrite(int fd, uint8_t* buffer, int msg_length){
    //printf("llwrite entered\n");

    if(fcntl(fd, F_GETFD) < 0){
        perror("Invalid fd");
        return -1;
    }

    int frame_length = msg_length + FRAME_INFO_SIZE;
    uint8_t bcc2;
    
    frame[0] = FLAG;
    frame[1] = A_TR;
    
    if(sequence_number == 1){
        frame[2] = CTR_I_FRAME_1;
        frame[3] = A_TR ^ CTR_I_FRAME_1;
    }

    else if(sequence_number == 0){
        frame[2] = CTR_I_FRAME_0;
        frame[3] = A_TR ^ CTR_I_FRAME_0;
    }

    else{
        perror("r has an erroneous value, it should only take values of 1 or 0");
    }

    for (int i = 0; i < msg_length; i++)
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

    byteStuffing(&frame_length);

    //printf("Bytes stuffed\n");

    int bytes_writen = sendInfoPacket(fd, frame_length, buffer);

    return bytes_writen;
}

int llread(int fd, uint8_t* buffer){
    int frame_length = 0;

    if(fcntl(fd, F_GETFD) < 0){
        perror("Invalid fd");
        return -1;
    }

    frame_length = readPacket(fd, buffer);

    return frame_length;
}

int llclose(int fd){

    if(fcntl(fd, F_GETFD) < 0){
        perror("Invalid fd");
        return -1;
    }
    
    if(mode == 0){
        if(sendDisconectMessage(fd) < 0){
            return -1;
        }
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