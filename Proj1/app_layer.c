
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "defs.h"
#include "link_layer.h"

int fd;
unsigned int mode;
int portNr;

char* path; 
unsigned int packet_size;
FILE* file;
long file_size;
int file_packets;

int buildControlPacket(uint8_t** packet){
    unsigned int packet_size;
    char* filename;

    char* marker = strrchr(path, '/');
	if(marker != NULL){
		filename = &(marker[1]);
    }

    int fs_oct = 0; 
    long fs_temp = file_size;

    while(fs_temp > 0){
        fs_temp >>= 8;
        fs_oct++;
    }

    int fn_length = strlen(filename);
    packet_size = 6 + fs_oct + fn_length;
    *packet = (uint8_t*) malloc(packet_size);

    (*packet)[0] = START_PACKET;
    (*packet)[1] = SIZE_PARAM;
    (*packet)[2] = fs_oct;

    for(int i = 0; i < fs_oct; i++){
        int size_byte = file_size >> 8 * (fs_oct - i - 1);
        (*packet)[3 + i] = size_byte;
    }

    (*packet)[3 + fs_oct] = NAME_PARAM;
    (*packet)[4 + fs_oct] = fn_length;

    for(int i = 0; i < fn_length; i++){
        (*packet)[5 + fs_oct + i] = filename[i];
    }

    return packet_size;
}

int buildDataPacket(uint8_t** packet, int packet_nr, uint8_t* data, int data_size){
    unsigned int packet_size = data_size + 5;
    *packet = malloc(packet_size);

    (*packet)[0] = DATA_PACKET;
    (*packet)[1] = packet_nr;
    (*packet)[2] = data_size / 256;
    (*packet)[3] = data_size % 256;

    for(int i = 0; i < data_size; i++){
        (*packet)[i + 4] = data[i];
    }
    printf("Packet size: %i\n", packet_size);
    return packet_size;
}

void unpackControlPacket(uint8_t* packet, char** filename){
    //printf("Unpack entered\n");
    int pointer = 2;
    int fs_oct = packet[pointer];
    pointer++;

    file_size = 0;
    for(int i = 0; i < fs_oct; i++){
        file_size += (packet[pointer] << 8 *(fs_oct - i - 1));
        pointer++;
    }

    pointer++;

    int fn_length = packet[pointer];
    pointer++;
    *filename = malloc(fn_length + 1);

    for(int i = 0; i < fn_length; i++){
        (*filename)[i] = packet[pointer];
        pointer++;
    }

    (*filename)[fn_length] = '\0';
    printf("Filename: %s\n", *filename);
}

int main(int argc, char *argv[])
{

    //Process args for transmitter and receiver modes
    if(argc < 3){
        printf("Usage: file_transfer <port 0|1|2|10|11> <mode 0|1> <file path> <packet size>\n");
        printf("port: /dev/ttySn, where n is the number entered\n");
        printf("mode: 0 for transmiter and 1 for receiver\n");
        printf("file path and packet size should only be entered if mode is 0 (transmiter)\n");
        
        return -1;
    }

    portNr = strtol(argv[1], NULL, 10);
    mode = strtol(argv[2], NULL, 10);
    uint8_t* buffer;

    if(mode != TRANSMITER && mode != RECEIVER){
        printf("Invalid mode; mode can only be 0 (transmiter) or 1 (receiver)\n");
        return -1;
    }

    if(mode == TRANSMITER){
        if(argc < 5){
            printf("Usage: file_transfer <port 0|1|2|10|11> <mode 0|1> <file path> <packet size>\n");
            printf("port: /dev/ttySn, where n is the number entered\n");
            printf("mode: 0 for transmiter and 1 for receiver\n");
            printf("file path and packet size should only be entered if mode is 0 (transmiter)\n");
            return -1;
        }

        path = argv[3];
        file = fopen(path, "rb");
        //file_fd = fileno(file);
        printf("Opened file\n");

        if(file == NULL){
            perror("Couldn't open the file indicated in <file path>");
            return -1;
        }

        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        packet_size = strtol(argv[4], NULL, 10);
        buffer = malloc(packet_size);

        file_packets = file_size / packet_size;
        if(file_size % packet_size > 0){
            file_packets++;
        }

        if(packet_size > (MAX_PACKET_SIZE - FRAME_INFO_SIZE)){
            perror("Packet size too big");
            return -1;
        }
    }

    fd = llopen(portNr, mode);
    printf("Comunication port opened\n");

    if(fd < 0){
        return -1;
    }

    uint8_t* control_packet = NULL;
    uint8_t* data_packet = NULL;
    int file_op_return;

    //Read loop if receiver
    if(mode == RECEIVER){
        int bytes_read = 0;
        char* filename;
        buffer = malloc(256);

        while(1){
            bytes_read = llread(fd, buffer);
            printf("llread finished with a value of %i\n", bytes_read);

            if(bytes_read == DISC_RETURN || bytes_read == DISC_ERROR || bytes_read < 0){
                break;
            }

            if(buffer[0] == START_PACKET){
                unpackControlPacket(buffer, &filename);

                if(file_size > 256){
                    buffer = realloc(buffer, file_size);
                }

                file = fopen(filename, "wb");
                printf("Opened file\n");
                //file_fd = fileno(file);
                //printf("Fileno\n");
            }

            else if(buffer[0] == DATA_PACKET && bytes_read != BCC2_ERROR){

                file_op_return = fwrite(&buffer[4], 1, bytes_read - 10, file);
                printf("Wrote %i bytes\n", bytes_read - 9);

                if(file_op_return < 0 || feof(file) > 0){
                    perror("Error while writing in file");
                    continue;
                }
            }

            else if(buffer[0] == END_PACKET){
                printf("End packet received\n");
            }
        }

        if(bytes_read < 0){
            perror("Error on reading from port; aborting");
        }
    }

    //Write loop if transmitter
    if(mode == TRANSMITER){
        int cp_length = buildControlPacket(&control_packet);
        printf("Control packet built\n");
        int bytes_writen = llwrite(fd, control_packet, cp_length);

        if(bytes_writen > 0){
            printf("Control packet sent, sending data\n");
            int data_packets_writen = 0;
            int bytes_to_read = packet_size;

            while(data_packets_writen < file_packets){
                if((file_size / packet_size) == data_packets_writen){
                    bytes_to_read = file_size % packet_size;
                }

                file_op_return = fread(buffer, 1, bytes_to_read, file);

                if(file_op_return != 1 || feof(file) > 0){
                    if(ferror(file)){
                        perror("Error while reading file");
                        continue;
                    }
                }

                int dp_length = buildDataPacket(&data_packet, data_packets_writen, buffer, bytes_to_read);
                //printf("Sending data\n");
                bytes_writen = llwrite(fd, data_packet, dp_length);
                //printf("Data packet sent\n");

                if(bytes_writen == -2){
                    break;
                }

                data_packets_writen++;
            }
        }

        if(bytes_writen != -2){
            control_packet[0] = END_PACKET;
            llwrite(fd, control_packet, cp_length);
        }
    }

    fclose(file);

    free(buffer);
    if(control_packet){
        free(control_packet);
    }
    if(data_packet){
        free(data_packet);
    }

    printf("Closing file\n");
    if(llclose(fd) != 1){
        return -1;
    }

    return 0;
}