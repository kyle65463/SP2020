#include "server.h"

void read_from_client(request *request)
{
    memset(request->buf, '\0', sizeof(request->buf));
    read(request->conn_fd, request->buf, sizeof(request->buf));
}

void write_to_client(request *request, char string[])
{
    write(request->conn_fd, string, strlen(string));
}

void init_request(request *reqP)
{
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->status = 1;
}

void close_connection(request *reqP)
{
    printf("Close connection fd %d\n", reqP->conn_fd);
    close(reqP->conn_fd);
    free(reqP->order);
    init_request(reqP);
}

void init_server(unsigned short port)
{
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0)
        ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&tmp, sizeof(tmp)) < 0)
    {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0)
    {
        ERR_EXIT("listen");
    }

    // Get file descripter table sbuf_lenize and initize request table
    maxfd = 1020;//getdtablesize();

    requestP = (request *)malloc(sizeof(request) * maxfd);
    if (requestP == NULL)
    {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++)
    {
        init_request(&requestP[i]);
    }

    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}
