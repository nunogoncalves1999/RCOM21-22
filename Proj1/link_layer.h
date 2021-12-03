#ifndef __LINK_LAYER_H__
#define __LINK_LAYER_H__

void timeout_handler();

void flip_sequence_number();

int setup_port(int port);

int check_acknowledgement(char ack);

int check_control_field(char c);

void byte_stuffing(int *length);

void byte_destuffing(int *length);

int setup_transmiter(int fd);

int setup_receiver(int fd);

int send_disconect_message(int fd);

int send_disconect_answer(int fd);

int send_info_packet(int fd, int frameLength);

int read_packet(int fd);

int llopen(int port, int mode);

int llwrite(int fd, char* buffer, int length);

int llread(int fd, char* buffer);

int llclose(int fd);

#endif
