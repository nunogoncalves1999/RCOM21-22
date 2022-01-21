#ifndef __DEFS_H__
#define __DEFS_H__

typedef struct{
    char* user;
    char* password;
    char* hostname;
    char* path;
    char* directory_path;
    char* filename;
    char* address;
} conection_info;

#define SERVER_PORT 21
#define MAX_TIMEOUTS 3
#define TIMEOUT_USEC 100000
#define TIMEOUT_SEC 5

#endif


