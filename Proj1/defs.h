#ifndef __DEFS_H__
#define __DEFS_H__

#define FALSE 0
#define TRUE 1

#define CTR_I_FRAME 0x40 | 0x00
#define SET 0x03
#define DISC 0x0B
#define UA 0x07
#define RR 0x85 | 0x05
#define REJ 0x81 | 0x01

#define FLAG 0x7E
#define A_TR 0x03
#define C_TR 0x07
#define BCC1_TR A_TR ^ C_TR
#define A_RE 0x01
#define C_RE 0x03
#define BCC1_RE A_RE ^ C_RE

#define OTHER_RCV 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_RCV 4

#define ESCAPE 0x7D
#define XOR_BYTE 0x20

#define MAX_SIZE 255

#define TRANSMITER 0
#define RECEIVER 1

#define DEFAULT_BAUD B9600
