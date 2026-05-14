#include "l7_common.h"

#define BUF_SIZE 5
#define BACKLOG 10
#define MAX_CLIENTS 4
#define MAX_EVENTS 16
#define CITY_COUNT 20

typedef struct {
    int clients_fd[MAX_CLIENTS];
    int is_filled[MAX_CLIENTS];
} clients_struct_t;

char cities[CITY_COUNT];

volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig) { (void) sig; do_work = 0; }

static void disconnect_client(int cfd, clients_struct_t *clients_struct, int epoll_fd)
{   
    printf("[DEBUG] disconnecting client with fd = %d\n", cfd);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cfd, NULL);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients_struct->clients_fd[i] == cfd) {
            clients_struct->is_filled[i] = 0;
            clients_struct->clients_fd[i] = -1;
            break;
        }
    }
    TEMP_FAILURE_RETRY(close(cfd));
}

static void handle_client(int cfd, clients_struct_t* clients_struct, int epoll_fd)
{   
    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));

    ssize_t n = bulk_read(cfd, buf, sizeof(buf)-1);
    if (n < 0) {
        perror("read");
        disconnect_client(cfd, clients_struct, epoll_fd);
        return;
    }
    if (n != BUF_SIZE - 1) {
        disconnect_client(cfd, clients_struct, epoll_fd);
        return;
    }

    buf[BUF_SIZE - 1] = '\0';
    printf("[DEBUG] Buffer content: %s", buf);

    char belongs_to = buf[0];
    int belongs_to_correct_format = (belongs_to == 'g' || belongs_to == 'p');

    int city_index = (buf[1]-'0') * 10 + (buf[2]-'0') - 1;

    if (city_index < 0 || city_index >= CITY_COUNT || !belongs_to_correct_format)
    {
        printf("Daun skazal hujnu. Poszel nahuj: %s", buf);
        disconnect_client(cfd, clients_struct, epoll_fd);
        return;
    }

    if (cities[city_index] == belongs_to) {
        printf("No new information\n");
        return;
    }
        

    cities[city_index] = belongs_to;

    printf("Notify everyone\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients_struct->is_filled[i] == 0 || clients_struct->clients_fd[i] == cfd)
            continue;
        ssize_t n = bulk_write(clients_struct->clients_fd[i], buf, sizeof(buf)-1);

        if (n == -1 &&  errno == EPIPE) {
            disconnect_client(clients_struct->clients_fd[i], clients_struct, epoll_fd);
            continue;
        }

        printf("[DEBUG] Bulk write to client %d   -   %ld bytes\n", i, n);
        if (n != BUF_SIZE - 1)
        {   
            printf("Unexpected error, disconnecting client at index %d\n", i);
            disconnect_client(clients_struct->clients_fd[i], clients_struct, epoll_fd);
            continue;
        }
    }
}

void doServer(int tcp_listen_socket)
{   
    clients_struct_t clients_struct;
    memset(&clients_struct, 0, sizeof(clients_struct_t));
    memset(cities, 'g', CITY_COUNT);

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
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    while (do_work)
    {   
        if ((nfds = epoll_pwait(epoll_descriptor, events, MAX_EVENTS, -1, &oldmask)) > 0)
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
                    handle_client(events[n].data.fd, &clients_struct, epoll_descriptor);
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
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    printf("Gracefully stopping\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients_struct.is_filled[i] == 0)
            continue;
        printf("    [DEBUG] Closing client at index %d with fd = %d\n", i, clients_struct.clients_fd[i]);
        if (TEMP_FAILURE_RETRY(close(clients_struct.clients_fd[i])) < 0)
            ERR("close");
    }

    for (int i = 0; i < CITY_COUNT; i++) {
        printf("City at index: %d belongs to %c\n", i+1, cities[i]);
    }
}

int main(int argc, char *argv[])
{   
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("Seting SIGPIPE:");
    if (sethandler(sigint_handler, SIGINT))
        ERR("Seting SIGINT:");

    uint16_t port = (uint16_t)atoi(argv[1]);
    int server_fd = bind_tcp_socket(port, BACKLOG);

    int flags = fcntl(server_fd, F_GETFL) | O_NONBLOCK;
    fcntl(server_fd, F_SETFL, flags);

    doServer(server_fd);

    if (TEMP_FAILURE_RETRY(close(server_fd)) < 0)
        ERR("close server");

    return EXIT_SUCCESS;
}