#ifndef __LINK_LAYER_H__
#define __LINK_LAYER_H__

void timeout_handler();

void flipSequenceNumber();

int setupPort(int port);

int checkAcknowledgement(char ack);

int checkControlField(char c);

int checkBcc2(char bcc2, int msgLength);

void byteStuffing(int *length);

void byteDestuffing(int *length);

int setupTransmiter(int fd);

int setupReceiver(int fd);

int sendDisconectMessage(int fd);

int sendDisconectAnswer(int fd);

int sendInfoPacket(int fd, int frameLength, char* buffer);

int readPacket(int fd, char* buffer);

int llopen(int port, int mode);

int llwrite(int fd, char* buffer, int length);

int llread(int fd, char* buffer);

int llclose(int fd);

#endif
