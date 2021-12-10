#ifndef __DEFS_H__
#define __DEFS_H__

typedef unsigned char uint8_t;

#define FALSE 0
#define TRUE 1

#define DATA_PACKET 1
#define START_PACKET 2
#define END_PACKET 3
#define SIZE_PARAM 0
#define NAME_PARAM 1

#define CTR_I_FRAME_0 0x00
#define CTR_I_FRAME_1 0x40
#define SET 0x03
#define DISC 0x0B
#define UA 0x07
#define RR_0 0x05
#define RR_1 0x85
#define REJ_0 0x01
#define REJ_1 0x81

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

#define FRAME_INFO_SIZE 6
#define INITIAL_FRAME_BITS 4
#define FINAL_FRAME_BITS 2
#define MAX_PACKET_SIZE 64000

#define DISC_RETURN 4
#define DISC_ERROR 3

#define TRANSMITER 0
#define RECEIVER 1

#define DEFAULT_BAUD B9600

#endif
