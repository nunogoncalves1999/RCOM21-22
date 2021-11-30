#ifndef __DEFS_H__
#define __DEFS_H__

#define FALSE 0
#define TRUE 1

#define DATA_PACKET 1
#define START_PACKET 2
#define END_PACKET 3

#define CTR_I_FRAME0 0x00
#define CTR_I_FRAME1 0x40
#define SET 0x03
#define DISC 0x0B
#define UA 0x07
#define RR0 0x05
#define RR1 0x85
#define REJ0 0x01
#define REJ1 0x81

#define FLAG 0x7E
#define A_TR 0x03
#define A_RE 0x01

#define OTHER_RCV 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_RCV 4

#define ESCAPE 0x7D
#define XOR_BYTE 0x20

#define MAX_SIZE 255
#define MAX_PACKET_SIZE 64000

#define TRANSMITER 0
#define RECEIVER 1

#define DEFAULT_BAUD B9600

#endif
