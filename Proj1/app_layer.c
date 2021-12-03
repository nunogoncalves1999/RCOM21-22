
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
int mode;
int portNr;

char* path; 
unsigned int packet_size;
FILE* file;
long file_size;
int file_packets;

int buildControlPacket(char** packet){
    unsigned int packet_size;
    char* filename;

    char* marker = strrchr(path, '/');
	if(marker != NULL){
		filename = &(marker[1]);
    }

    char fs_oct = 0; 
    long fs_temp = file_size;

    while(fs_temp > 0){
        fs_temp >>= 8;
        fs_oct++;
    }

    int fn_length = strlen(filename);
    packet_size = 5 + fs_oct + fn_length;
    *packet = (char*) malloc(packet_size);

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

int buildDataPacket(char** packet, int packet_nr, char* data, int data_size){
    unsigned int packet_size = data_size + 4;
    *packet = malloc(packet_size);

    (*packet)[0] = DATA_PACKET;
    (*packet)[1] = packet_nr;
    (*packet)[2] = data_size / 256;
    (*packet)[3] = data_size % 256;

    for(int i = 0; i < data_size; i++){
        (*packet)[i + 4] = data[i];
    }

    return packet_size;
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
    char* buffer;

    if(mode != 0 && mode != 1){
        printf("Invalid mode; mode can only be 0 (transmiter) or 1 (receiver)\n");
        return -1;
    }

    if(mode == 0){
        if(argc < 5){
            printf("Usage: file_transfer <port 0|1|2|10|11> <mode 0|1> <file path> <packet size>\n");
            printf("port: /dev/ttySn, where n is the number entered\n");
            printf("mode: 0 for transmiter and 1 for receiver\n");
            printf("file path and packet size should only be entered if mode is 0 (transmiter)\n");
            return -1;
        }

        path = argv[3];
        file = fopen(path, "rb");

        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        
        if(file == NULL){
            perror("Couldn't open the file indicated in <file path>");
            return -1;
        }

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

    if(fd < 0){
        return -1;
    }

    char* control_packet = NULL;
    char* data_packet = NULL;
    int fread_return;

    //Read loop if receiver
    if(mode == 1){
        int bytesRead = 0;

        while(bytesRead != 3 && bytesRead != 4 && bytesRead >= 0){
            bytesRead = llread(fd, buffer);
        }

        if(bytesRead < 0){
            perror("Error on reading from port; aborting");
        }
    }

    //Write loop if transmitter
    if(mode == 0){
        int cp_length = buildControlPacket(&control_packet);
        int bytes_writen = llwrite(fd, control_packet, cp_length);

        if(bytes_writen > 0){
            int data_packets_writen = 0;
            int bytes_to_read = packet_size;

            while(data_packets_writen < file_packets){
                if((file_size / packet_size) == data_packets_writen){
                    bytes_to_read = file_size % packet_size;
                }

                fread_return = fread(buffer, bytes_to_read, 1, file);
                if(fread_return != 1 || feof(file) > 0){
                    if(ferror(file)){
                        perror("Error while reading file");
                        continue;
                    }
                }

                int dp_length = buildDataPacket(&data_packet, data_packets_writen, buffer, bytes_to_read);
                bytes_writen = llwrite(fd, data_packet, dp_length);

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

    //Closing file, cleaning up used space and printing debugs to check errors
    fclose(file);

    free(buffer);
    if(control_packet){
        free(control_packet);
    }
    if(data_packet){
        free(data_packet);
    }

    if(llclose(fd) != 1){
        return -1;
    }

    return 0;
}