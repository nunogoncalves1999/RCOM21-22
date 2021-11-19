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


volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    unsigned char buf[255];

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

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */



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
    
    int i = 0;
    int state = OTHER_RCV;

    while (STOP==FALSE) {       
      res = read(fd,&buf[i],1);   
                   
      switch(state){
        case OTHER_RCV:
            if(buf[i] == F) { state = FLAG_RCV; }
            break;
        
        case FLAG_RCV:
            if(buf[i] == F) { state = FLAG_RCV; }
            else if(buf[i] == A) { state = A_RCV; }
            else { state = OTHER_RCV; }
            break;
        
        case A_RCV:
            if(buf[i] == F) { state = FLAG_RCV; }
            else if(buf[i] == C) { state = C_RCV; }
            else { state = OTHER_RCV; }
            break;
        
        case C_RCV:
            if(buf[i] == F) { state = FLAG_RCV; }
            else if(buf[i] == BCC1) { state = BCC_RCV; }
            else { state = OTHER_RCV; }
            break;
        
        case BCC_RCV:
            if(buf[i] == F) { STOP=TRUE; }
            else { state = OTHER_RCV; }
            break;
      }
      
      i++;    
    }
    
    printf("Message received\n");


    char ua[5] = {F,A,C,BCC1,F};

    write(fd,ua,5); 



  /* 
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o 
  */



    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
