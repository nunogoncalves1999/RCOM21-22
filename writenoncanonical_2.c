/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define F 0x7E
#define A 0x01
#define C 0x03
#define BCC1 A ^ C
#define OTHER_RCV 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_RCV 4
#define MAX_TIMEOUTS 3

int timeout_count = 0;

//flag controls the timeout mechanism; 0 if waiting, 1 if timeout and 2 if received
volatile int flag = 0;

void atende()                   
{
	printf("alarme # %d\n", timeout_count);
	flag = 1;
	timeout_count++;
}

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int sum = 0, speed = 0;
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) &&
          (strcmp("/dev/ttyS10", argv[1])!=0) && 
          (strcmp("/dev/ttyS11", argv[1])!=0))) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0.1;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */


  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */


    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

    unsigned char set[5] = {F,A,C,BCC1,F};
    
    int state;

    (void) signal(SIGALRM, atende);
    
    while(timeout_count < MAX_TIMEOUTS && flag < 2){
        flag = 0;
        alarm(3);

        res = write(fd,set,5);   
        
        state = OTHER_RCV;

        while (flag == 0) {   
             
          res = read(fd,buf,1);   
                       
          switch(state){
            case OTHER_RCV:
                if(buf[0] == F) { state = FLAG_RCV; }
                break;
            
            case FLAG_RCV:
                if(buf[0] == F) { state = FLAG_RCV; }
                else if(buf[0] == A) { state = A_RCV; }
                else { state = OTHER_RCV; }
                break;
            
            case A_RCV:
                if(buf[0] == F) { state = FLAG_RCV; }
                else if(buf[0] == C) { state = C_RCV; }
                else { state = OTHER_RCV; }
                break;
            
            case C_RCV:
                if(buf[0] == F) { state = FLAG_RCV; }
                else if(buf[0] == BCC1) { state = BCC_RCV; }
                else { state = OTHER_RCV; }
                break;
            
            case BCC_RCV:
                if(buf[0] == F) {  
                    alarm(0);
                    flag = 2;
                }
                else { state = OTHER_RCV; }
                break;
          }
            
        }
    }

    if(timeout_count >= MAX_TIMEOUTS){
        printf("Max number of timeouts reached\n");
    }
    
    else{
        printf("Message received back\n");
    }

   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }


    close(fd);
    return 0;
}
