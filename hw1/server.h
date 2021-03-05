#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <netdb.h>

#define ERR_EXIT(a) \
    do              \
    {               \
        perror(a);  \
        exit(1);    \
    } while (0)
typedef struct
{
    char hostname[512];  // server's hostname
    unsigned short port; // port to listen
    int listen_fd;       // fd to wait for a new connection
} server;

typedef struct
{
    int id;
    int adultMask;
    int childrenMask;
} Order;

typedef struct
{
    char host[512]; // client's host
    int conn_fd;    // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len; // bytes used by buf
    // you don't need to change this.

    int status; 
    char phase;
    Order* order;

} request;

extern server svr;               // server
extern request *requestP; // point to a list of requests
extern int maxfd; 

void init_server(unsigned short port);
// initailize a server, exit for error

void init_request(request *reqP);
// initailize a request instance

void close_connection(request *reqP);
// free resources used by a request instance
void read_from_client(request *request);
void write_to_client(request *request, char string[]);