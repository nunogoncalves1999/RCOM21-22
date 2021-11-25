#ifndef __LINK_LAYER_H__
#define __LINK_LAYER_H__

int setup_port(int port);

int llopen(int port, int mode);

int llwrite(int fd, char* buffer, int length);

int llread(int fd, char* buffer);

int llclose(int fd);