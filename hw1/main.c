#include "server.h"

// Constants
#define N_ORDER 20
#define ID_PREFIX 902001

// Phases
#define GET_ORDER 1
#define SET_ORDER 2

// Operation stats
#define OPERATION_FAILED -1
#define LOCKED -2
#define SUCCESSED 0

// Global variables
server svr;
request *requestP = NULL;
int maxfd;
int preorder_record;
int id_table[N_ORDER] = {}; // The lock for same process

//========================================================

// Locks functions
struct flock genearateLock(int type, int id)
{
    struct flock fl;
    fl.l_type = type;
    fl.l_whence = SEEK_SET;
    fl.l_start = id * sizeof(Order);
    fl.l_len = sizeof(Order);
    fl.l_pid = getpid();
    return fl;
}

void unlock(int fd, int id)
{
    struct flock fl = genearateLock(F_UNLCK, id);
    fcntl(fd, F_SETLK, &fl);
    id_table[id] = 0;
    return;
}

int setWriteLock(int fd, int id)
{
    struct flock fl = genearateLock(F_WRLCK, id);
    return fcntl(fd, F_SETLK, &fl);
}

int setReadLock(int fd, int id)
{
    struct flock fl = genearateLock(F_RDLCK, id);
    return fcntl(fd, F_SETLK, &fl);
}

//========================================================

// Other functions
void parseAmount(request *request, int *amount)
{
    char *str_amount = strtok(NULL, " ");
    if (str_amount == NULL)
    {
        request->status = OPERATION_FAILED;
        return;
    }
    *amount = atoi(str_amount);
    if (*amount <= 0)
    {
        request->status = OPERATION_FAILED;
        return;
    }
}

//========================================================

// Get/set order functions
void getOrder(request *request)
{
    // Parse id
    int id = atoi(request->buf) - ID_PREFIX;
    if (id < 0 || id >= N_ORDER)
    {
        request->status = OPERATION_FAILED;
        return;
    }

    // Set lock
#ifdef READ_SERVER
    if (setReadLock(preorder_record, id) < 0)
    {
        request->status = LOCKED;
        return;
    }
#else
    if (id_table[id] || setWriteLock(preorder_record, id) < 0)
    {
        request->status = LOCKED;
        return;
    }
#endif

    // Get order successed
    id_table[id] = 1;
    lseek(preorder_record, sizeof(Order) * id, SEEK_SET);
    read(preorder_record, request->order, sizeof(Order));
    request->status = SUCCESSED;

    return;
};

void setOrder(request *request, char *type, int amount)
{
    if (request->status == OPERATION_FAILED)
        return;

    // Set order
    if (strcmp(type, "adult") == 0 && request->order->adultMask - amount >= 0)
    {
        request->order->adultMask -= amount;
    }
    else if (strcmp(type, "children") == 0 && request->order->childrenMask - amount >= 0)
    {
        request->order->childrenMask -= amount;
    }
    else
    {
        request->status = OPERATION_FAILED;
        return;
    }

    // Set order successed
    int id = request->order->id - ID_PREFIX;
    lseek(preorder_record, sizeof(Order) * id, SEEK_SET);
    write(preorder_record, request->order, sizeof(Order));
    request->status = SUCCESSED;

    return;
};

//========================================================

// Main function
int main(int argc, char **argv)
{
    // Parse args
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    // Initialization
    init_server((unsigned short)atoi(argv[1])); // Init server
    fd_set read_set;                            // Init fdset
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    // Open the preorder record
    char pathname[32] = "preorderRecord";
#ifdef READ_SERVER
    preorder_record = open(pathname, O_RDONLY);
#else
    preorder_record = open(pathname, O_RDWR);
#endif

    // Loop for handling connections
    while (1)
    {
        // IO multiplexing
        FD_ZERO(&read_set);               // Reset the fd set
        FD_SET(svr.listen_fd, &read_set); // Turn on the bit of listening port
        for (int i = 0; i < maxfd; i++)
            if (requestP[i].conn_fd > 0)
                FD_SET(requestP[i].conn_fd, &read_set); // Turn on the bit of request connections
        select(maxfd, &read_set, NULL, NULL, NULL);     // Check if there is any ready connection

        // Check new connection
        if (FD_ISSET(svr.listen_fd, &read_set))
        {
            struct sockaddr_in cliaddr;
            int clilen = sizeof(cliaddr);
            int conn_fd = accept(svr.listen_fd, (struct sockaddr *)&cliaddr, (socklen_t *)&clilen);

            // Connection failed
            if (conn_fd < 0)
            {
                if (errno == EINTR || errno == EAGAIN)
                    continue; // try again
                if (errno == ENFILE)
                {
                    (void)fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                    continue;
                }
                ERR_EXIT("accept");
            }

            // Connection accepted
            requestP[conn_fd].conn_fd = conn_fd;
            strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
            requestP[conn_fd].order = (Order *)malloc(sizeof(Order));
            printf("getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);

            // Ask the client to input an order id
            write_to_client(&requestP[conn_fd], "Please enter the id (to check how many masks can be ordered):\n");
            requestP[conn_fd].phase = GET_ORDER;
        }

        // Check if fd is ready for reading
        for (int i = 0; i < maxfd; i++)
        {
            request *cur_request = &requestP[i];
            if (FD_ISSET(cur_request->conn_fd, &read_set))
            {
                // Ready for reading
                if (cur_request->phase == GET_ORDER)
                {
                    // Get order
                    read_from_client(cur_request);
                    getOrder(cur_request);

                    // Check result
                    if (cur_request->status == SUCCESSED)
                    {
                        // Reading order successed
                        char output_msg[512];
                        sprintf(output_msg,
                                "You can order %d adult mask(s) and %d children mask(s).\n",
                                cur_request->order->adultMask, cur_request->order->childrenMask);
                        write_to_client(cur_request, output_msg);
#ifdef READ_SERVER
                        // If it is a read server, close the connection
                        unlock(preorder_record, cur_request->order->id - ID_PREFIX);
                        close_connection(cur_request);
#else
                        // If it is a write server, set phase to SET_ORDER
                        write_to_client(cur_request,
                                        "Please enter the mask type (adult or children) and number of mask you would like to order:\n");
                        cur_request->phase = SET_ORDER;
#endif
                    }
                    else
                    {
                        // Reading order failed
                        if (cur_request->status == LOCKED) // Locked
                            write_to_client(cur_request, "Locked.\n");
                        else if (cur_request->status == OPERATION_FAILED) // Operation failed
                            write_to_client(cur_request, "Operation failed.\n");

                        close_connection(cur_request); // Close connection
                    }
                }
                else if (cur_request->phase == SET_ORDER)
                {
                    // Get modified info and parse them
                    read_from_client(cur_request);
                    char *type = strtok(cur_request->buf, " "); // Get mask type
                    int amount;
                    parseAmount(cur_request, &amount);          // Get amount
                    
                    // Set order
                    setOrder(cur_request, type, amount);

                    if (cur_request->status == SUCCESSED)
                    {
                        // Set order successed
                        char output_msg[512];
                        sprintf(output_msg,
                                "Pre-order for %d successed, %d %s mask(s) ordered.\n",
                                cur_request->order->id, amount, type);
                        write_to_client(cur_request, output_msg);
                    }
                    else if (cur_request->status == OPERATION_FAILED)
                    {
                        // Set order failed
                        write_to_client(cur_request, "Operation failed.\n");
                    }

                    // Close connection
                    unlock(preorder_record, cur_request->order->id - ID_PREFIX);
                    close_connection(cur_request);
                }
            }
        }
    }
    close(preorder_record);
    free(requestP);
    return 0;
}