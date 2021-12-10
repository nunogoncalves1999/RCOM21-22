#ifndef __LINK_LAYER_H__
#define __LINK_LAYER_H__

void timeout_handler();

void flipSequenceNumber();

int setupPort(int port);

int checkAcknowledgement(uint8_t ack);

int checkControlField(uint8_t c);

int checkBcc2(uint8_t bcc2, int msgLength);

void byteStuffing(int *length);

void byteDestuffing(int *length);

int setupTransmiter(int fd);

int setupReceiver(int fd);

int sendDisconectMessage(int fd);

int sendDisconectAnswer(int fd);

int sendInfoPacket(int fd, int frameLength, uint8_t* buffer);

int readPacket(int fd, uint8_t* buffer);

int llopen(int port, int mode);

int llwrite(int fd, uint8_t* buffer, int length);

int llread(int fd, uint8_t* buffer);

int llclose(int fd);

#endif
