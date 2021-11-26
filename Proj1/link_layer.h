#ifndef __LINK_LAYER_H__
#define __LINK_LAYER_H__

void timeout_handler();

int setup_port(int port);

int setup_transmiter(int fd);

int send_disconect_message(int fd);

int send_disconect_answer(int fd);

int llopen(int port, int mode);

int llwrite(int fd, char* buffer, int length);

int llread(int fd, char* buffer);

int llclose(int fd);