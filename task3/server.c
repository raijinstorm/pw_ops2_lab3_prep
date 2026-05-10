#include "l7_common.h"

#define BUF_SIZE 5
#define BACKLOG 10
#define MAX_CLIENTS 4
#define MAX_EVENTS 16

typedef struct {
    int clients_fd[MAX_CLIENTS];
    int is_filled[MAX_CLIENTS];
} clients_struct_t;


static void handle_client(int cfd)
{   
    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));

    ssize_t n = bulk_read(cfd, buf, sizeof(buf)-1);
    if (n < 0) {
        perror("read");
        return;
    }
    if (n == 0) {
        fprintf(stderr, "Client disconnected before sending data\n");
        return;
    }

    buf[BUF_SIZE - 1] = '\0';
    printf("Buffer content: %s", buf);
}

void doServer(int tcp_listen_socket)
{   
    clients_struct_t clients_struct;
    memset(&clients_struct, 0, sizeof(clients_struct_t));

    int epoll_descriptor;
    if ((epoll_descriptor = epoll_create1(0)) < 0)
    {
        ERR("epoll_create:");
    }
    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN;

    event.data.fd = tcp_listen_socket;
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, tcp_listen_socket, &event) == -1)
    {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    int nfds;
    // sigset_t mask, oldmask;
    // sigemptyset(&mask);
    // sigaddset(&mask, SIGINT);
    // sigprocmask(SIG_BLOCK, &mask, &oldmask);
    while (1)
    {   
        if ((nfds = epoll_wait(epoll_descriptor, events, MAX_EVENTS, -1)) > 0)
        {   
            for (int n = 0; n < nfds; n++)
            {   
                if (events[n].data.fd == tcp_listen_socket) {
                    int free_index = -1;
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (0 == clients_struct.is_filled[i]) {
                            free_index = i;
                            break;
                        }
                    }

                    int cfd;
                    if(( cfd = add_new_client(tcp_listen_socket))<0)
                    {
                        continue;
                    }
                    

                    if (free_index == -1) {
                        printf("All pidoras places are taken\n");
                        if (TEMP_FAILURE_RETRY(close(cfd)) < 0)
                            perror("close client");
                        continue;
                    }

                    clients_struct.is_filled[free_index] = 1;
                    clients_struct.clients_fd[free_index] = cfd;

                    printf("Added pidoras at index %d\n", free_index);

                    struct epoll_event cfd_event;
                    cfd_event.events = EPOLLIN;

                    cfd_event.data.fd = cfd;
                    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, cfd, &cfd_event) == -1)
                    {
                        perror("epoll_ctl: listen_sock");
                        exit(EXIT_FAILURE);
                    }
                }
                else {
                    handle_client(events[n].data.fd);
                }
            }
        }
        else
        {
            if (errno == EINTR)
                continue;
            ERR("epoll_pwait");
        }
    }
    if (TEMP_FAILURE_RETRY(close(epoll_descriptor)) < 0)
        ERR("close");
    // sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

int main(int argc, char *argv[])
{   
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    uint16_t port = (uint16_t)atoi(argv[1]);
    int server_fd = bind_tcp_socket(port, BACKLOG);

    doServer(server_fd);

    if (TEMP_FAILURE_RETRY(close(server_fd)) < 0)
        ERR("close server");

    return EXIT_SUCCESS;
}