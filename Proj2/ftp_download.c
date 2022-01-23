#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdarg.h>

#include <string.h>

#include "defs.h"

// DATA STRUCTURE MANAGEMENT 

int process_args(const char* url, conection_info *info){
    char user[256] = {0};
    char password[256] = {0};
    char hostname[256] = {0};
    char path[256] = {0};
    char* address;

    if(sscanf(url, "ftp://%256[^:]:%256[^@]@%256[^/]%256s", user, password, hostname, path) != 4){
        //If the url isn't in the above format, it may be in anonymous mode
        memset(user, 0, 256);
        memset(password, 0, 256);
        memset(hostname, 0, 256);
        memset(path, 0, 256);

        if(sscanf(url, "ftp://%256[^/]%256s", hostname, path) != 2){
            printf("Invalid url format!\n The format should be ftp://[<user>:<password>@]<host>/<url-path>\n");
            return 1;
        }
    
        strncpy(user, "anonymous", 9);
        strncpy(password, "anonymous", 9);
    }

    struct hostent *host_info;
    if((host_info = gethostbyname(hostname)) == NULL){
        printf("Couldn't conect to host %s\n", hostname);
        return 1;
    }

    size_t param_length = strlen(user);
    info->user = (char*) malloc(param_length);
    memcpy(info->user, user, param_length + 1);

    param_length = strlen(password);
    info->password = (char*) malloc(param_length);
    memcpy(info->password, password, param_length + 1);

    param_length = strlen(hostname);
    info->hostname = (char*) malloc(param_length);
    memcpy(info->hostname, hostname, param_length + 1);

    param_length = strlen(path);
    info->path = (char*) malloc(param_length);
    memcpy(info->path, path, param_length);

    char* filename = strrchr(path, '/');
	if(filename == NULL) {
		printf("Invalid format!");
		return 1;
	}

    size_t filename_length = strlen(filename);
    info->directory_path = (char*) malloc(param_length - filename_length);
    memcpy(info->directory_path, path, param_length - filename_length);

    info->filename = (char*) malloc(filename_length);
    memcpy(info->filename, &path[param_length - filename_length + 1], filename_length);

    address = inet_ntoa(*((struct in_addr *) host_info->h_addr));
    param_length = strlen(address);
    info->address = (char*) malloc(param_length);
    memcpy(info->address, address, 20);

    return 0;
}

void debug_print(conection_info info){
    printf("Username: %s\n", info.user);
    printf("Password: %s\n", info.password);
    printf("Host name: %s\n", info.hostname);
    printf("File path: %s\n", info.path);
    printf("File location path: %s\n", info.directory_path);
    printf("File name: %s\n", info.filename);
    printf("Host address: %s\n", info.address);
}

void free_buffers(conection_info* info){
    free(info->user);
    free(info->password);
    free(info->hostname);
    free(info->path);
    free(info->directory_path);
    free(info->filename);
    free(info->address);
}

// SOCKET OPENING AND CLOSURE

int open_socket(int *socket_fd, char* address, FILE** socket_file, size_t port){
    struct sockaddr_in server_addr;
    
    bzero((char*) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);
    server_addr.sin_port = htons(port);

    if ((*socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error while opening socket");
        return 1;
    }

    *socket_file = fdopen(*socket_fd, "r");

    if(connect(*socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){
        perror("Error while conecting to socket");
        return 1;
    }

    printf("Opened and conected socket %i successfully\n", *socket_fd);

    return 0;
}

int close_socket(int socket_fd){

    if (close(socket_fd) < 0) {
        printf("Error while closing socket\n");
        return 1;
    }

    printf("Socket has been closed\n");

    return 0;
}

// READ AND WRITE OPERATIONS

int read_msg_aux(int socket_fd, char* code, char** reply){

    char msg[256] = "";
    char buffer[256] = "";
    int msg_length = 0;

    char code_comp[4];

    while(msg_length < 256){
        read(socket_fd, &buffer, 1);

        msg[msg_length] = buffer[0];
        msg_length++;

        if((msg[msg_length - 2] == '\r' && msg[msg_length - 1] == '\n')){
            break;
        }
    }

    strncpy(code_comp, msg, 3);

    if(strncmp(code_comp, code, 3) != 0){
        printf("Code doesn't match!\n");
        printf("Got %s, expected %s\n", code_comp, code);
        return 1;
    }

    if(reply != NULL){
        *reply = malloc(strlen(msg));
        strncpy(*reply, msg, strlen(msg));
    }

    return 0;
}

int write_msg(int socket_fd, char* cmd, ...){
    char msg[256];

    va_list va;
    va_start(va, cmd);
    vsnprintf(msg, 256, cmd, va);
    va_end(va);

    write(socket_fd, msg, strlen(msg));

    return 0;
}

int read_multiple_msgs(int socket_fd, char* code){

    fd_set set;
    FD_ZERO(&set);
    FD_SET(socket_fd ,&set);
    struct timeval timeout;

    int timeout_nr = 0;

    while (timeout_nr < MAX_TIMEOUTS)
    {
        timeout.tv_sec = 0;
        timeout.tv_usec = TIMEOUT_USEC;

        int res = select(socket_fd + 1, &set, NULL, NULL, &timeout);

        if(res == 0 || res == -1 || !FD_ISSET(socket_fd, &set)){
            timeout_nr++;
            continue;
        }

        if(read_msg_aux(socket_fd, code, NULL) != 0){
            return 1;
        }

        timeout_nr = 0;
    }

    return 0;
}

int read_msg(int socket_fd, char* code, char** reply){
    fd_set set;
    FD_ZERO(&set);
    FD_SET(socket_fd ,&set);
    struct timeval timeout;

    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;

    int res = select(socket_fd + 1, &set, NULL, NULL, &timeout);

    if(res == 0 || res == -1 || !FD_ISSET(socket_fd, &set)){
        printf("Timeout while reading from socket with return %i \n", res);
        return 1;
    }

    return read_msg_aux(socket_fd, code, reply);
}

int download_file(int fd, FILE* downloaded, size_t file_size){
    
    size_t block_size = 1024;
    uint8_t* data = malloc(block_size);
    size_t size = file_size;
	size_t print_timer = 1000;
	size_t timer = 0;
	int value = 0;

    while(size > 0){
        value = read(fd, data, block_size);
        size -= value;

        if(value < 0){
            printf("Error while downloading\n");
            return 1;
        }

        if(fwrite(data, value, 1, downloaded) < 0){
            printf("Saving failed download\n");
            return 1;
        }

        if(timer >= print_timer){
            double j = (1 - ((double)size / file_size)) * 100;
			int k = (int) j / 5;

			printf("Download: %.1f%% [%.*s>%.*s]\r", j, k, "====================", (20 - k), "                    ");
			fflush(stdout);
			timer = 0;
        }
    }

    free(data);

    fflush(stdout);
	printf("Download: 100.0%% [====================]\n");

    return 0;
}

// MAIN FUNCTION

int main(int argc, char const *argv[])
{
	if(argc != 2)
	{
        printf("Usage: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        return 1;
    }

    conection_info info;

    if(process_args(argv[1], &info) != 0){
        return 1;
    }

    debug_print(info);

    //Opening and conecting to sockets
    int socket_fd;
    FILE* socket_file;

    if(open_socket(&socket_fd, info.address, &socket_file, SERVER_PORT) != 0){
        return 1;
    }

    //Initial munication with socket
    if(read_multiple_msgs(socket_fd, "220") != 0){
        printf("Failed in first read from socket\n");
        close_socket(socket_fd);
        free_buffers(&info);
        return 1;
    }

    printf("Received mesage from socket\n");

    char* line_endings = "\r\n";
    char* reply;

    //Inserting username and password
    write_msg(socket_fd, "USER %s%s", info.user, line_endings);

    if(read_msg(socket_fd, "331", NULL) != 0){
        printf("Failed on username\n");
        close_socket(socket_fd);
        free_buffers(&info);
        return 1;
    }

    printf("User found\n");

    write_msg(socket_fd, "PASS %s%s", info.password, line_endings);

    if(read_msg(socket_fd, "230", NULL) != 0){
        printf("Failed on password\n");
        close_socket(socket_fd);
        free_buffers(&info);
        return 1;
    }

    printf("User authenticated\n");

    //Setting type
    write_msg(socket_fd, "TYPE I%s", line_endings);

    if(read_msg(socket_fd, "200", NULL) != 0){
        printf("Failed on setting type\n");
        close_socket(socket_fd);
        free_buffers(&info);
        return 1;
    }
    printf("Type set\n");

    //Getting file size
    write_msg(socket_fd, "SIZE %s%s", info.path, line_endings);

    int size_retreived = 0;
	size_t file_size = 0;

    if(read_msg(socket_fd, "213", &reply) == 0){
        file_size = strtol(&reply[4], NULL, 10);
		size_retreived = 1;
		free(reply);
    }

    printf("Size retreived\n");

    //Going to the desired directory
    write_msg(socket_fd, "CWD /%s%s", info.directory_path, line_endings);

    if(read_multiple_msgs(socket_fd, "250") != 0){
        printf("Failed while changing repository\n");
        close_socket(socket_fd);
        free_buffers(&info);
        return 1;
    }

    printf("Navigated to repository\n");

    //Starting passive mode
    write_msg(socket_fd, "PASV%s", line_endings);

    if(read_msg(socket_fd, "227", &reply) != 0){
        printf("Failed while setting passive mode\n");
        close_socket(socket_fd);
        free_buffers(&info);
        return 1;
    }

    uint8_t ip1;
	uint8_t ip2;
	uint8_t ip3;
	uint8_t ip4;
	uint8_t port_h;
	uint8_t port_l;

	if(sscanf(reply, "%*[^(](%hhu,%hhu,%hhu,%hhu,%hhu,%hhu).", &ip1, &ip2, &ip3, &ip4, &port_h, &port_l) != 6){
        printf("Erroneuos reply received");
        free(reply);
        return 1;
    }

    free(reply);

    uint16_t retrieved_port = port_h * 256 + port_l;
    char retrieved_ip[15];
    snprintf(retrieved_ip, 15, "%u.%u.%u.%u", ip1, ip2, ip3, ip4);

    printf("IP: %s\n", retrieved_ip);
	printf("PORT: %u\n", retrieved_port);

    //Opening new socket in server where the file will be downloaded from
    int retrieved_fd = 0;
	FILE* retrieved_file;
	if(open_socket(&retrieved_fd, retrieved_ip, &retrieved_file, retrieved_port) != 0){
        printf("Error while opening the retrieval socket");
        close_socket(socket_fd);
        free_buffers(&info);
        return 1;
    }

    printf("Passive mode set up\n");

    //Downloading file
    write_msg(socket_fd, "RETR %s%s", info.filename, line_endings);

    if(read_msg(socket_fd, "150", &reply) != 0){
        printf("Failed sending retrieval message\n");
        close_socket(retrieved_fd);
        close_socket(socket_fd);
        free(reply);
        free_buffers(&info);
        return 1;
    }

    FILE* downloaded;
    char discard[1024];

    if((downloaded = fopen(info.filename, "wb")) == NULL){
        perror("Error in fopen: ");
        close_socket(retrieved_fd);
        close_socket(socket_fd);
        free(reply);
        free_buffers(&info);
        return 1;
    }

    if(size_retreived == 0) {
		sscanf(reply, "%[^(](%ld", discard, &file_size);
		printf("File size: %ld bytes\n", file_size);
	}

	download_file(retrieved_fd, downloaded, file_size);

	fseek(downloaded, 0L, SEEK_END);
	long received_size = ftell(downloaded);

	fclose(downloaded);

    if(read_msg(socket_fd, "226", &reply) != 0){
        printf("Failed while recieving final message\n");
    }

    //Closing stuff at the end of the program
    free(reply);
    free_buffers(&info);
    close_socket(retrieved_fd);
    close_socket(socket_fd);

    printf("Program executed sucessfully!\n");

    return 0;
}