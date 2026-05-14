#include "l7_common.h"

#define BACKLOG 3
#define MAX_CLIENTS 16
#define MAX_EVENTS 16
#define NUM_ELECTORS 7

static const char *ELECTOR_NAMES[NUM_ELECTORS + 1] = {
    NULL, "Mainz", "Trier", "Cologne", "Bohemia",
    "Palatinate", "Saxony", "Brandenburg"
};

typedef enum { STATE_UNIDENTIFIED, STATE_IDENTIFIED } client_state_t;

typedef struct {
    int fd;
    client_state_t state;
    int elector_id;
    int vote;
} client_t;

static client_t clients[MAX_CLIENTS];

static int find_free(void)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].fd == -1) return i;
    return -1;
}

static int find_client(int fd)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].fd == fd) return i;
    return -1;
}

static int find_elector(int elector_id)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].fd != -1 && clients[i].elector_id == elector_id) return i;
    return -1;
}

static void disconnect_client(int idx, int epoll_fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clients[idx].fd, NULL);
    TEMP_FAILURE_RETRY(close(clients[idx].fd));
    clients[idx].fd = -1;
    clients[idx].state = STATE_UNIDENTIFIED;
    clients[idx].elector_id = -1;
    clients[idx].vote = 0;
}

static void handle_client(int fd, int idx, int epoll_fd)
{
    char c;
    ssize_t n = read(fd, &c, 1);
    if (n <= 0)
    {
        disconnect_client(idx, epoll_fd);
        return;
    }

    if (clients[idx].state == STATE_UNIDENTIFIED)
    {
        if (c < '1' || c > '7')
        {
            disconnect_client(idx, epoll_fd);
            return;
        }
        int elector_id = c - '0';
        if (find_elector(elector_id) != -1)
        {
            disconnect_client(idx, epoll_fd);
            return;
        }
        clients[idx].elector_id = elector_id;
        clients[idx].state = STATE_IDENTIFIED;

        char msg[64];
        int len = snprintf(msg, sizeof(msg), "Welcome, elector of %s!\n",
                           ELECTOR_NAMES[elector_id]);
        bulk_write(fd, msg, len);
        printf("Elector %d (%s) connected\n", elector_id, ELECTOR_NAMES[elector_id]);
    }
    else
    {
        if (c < '1' || c > '3') return;
        int candidate = c - '0';
        clients[idx].vote = candidate;
        printf("Elector %d (%s) votes for candidate %d\n",
               clients[idx].elector_id,
               ELECTOR_NAMES[clients[idx].elector_id],
               candidate);
    }
}

void doServer(int server_fd)
{
    int epoll_fd;
    if ((epoll_fd = epoll_create1(0)) < 0)
        ERR("epoll_create1");

    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0)
        ERR("epoll_ctl");

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].fd = -1;
        clients[i].state = STATE_UNIDENTIFIED;
        clients[i].elector_id = -1;
        clients[i].vote = 0;
    }

    int do_work = 1;
    while (do_work)
    {
        int nfds;
        if ((nfds = epoll_pwait(epoll_fd, events, MAX_EVENTS, -1, NULL)) > 0)
        {
            for (int i = 0; i < nfds; i++)
            {
                int fd = events[i].data.fd;
                if (fd == server_fd)
                {
                    int cfd = add_new_client(server_fd);
                    if (cfd < 0) continue;

                    int slot = find_free();
                    if (slot == -1)
                    {
                        TEMP_FAILURE_RETRY(close(cfd));
                        continue;
                    }
                    clients[slot].fd = cfd;
                    clients[slot].state = STATE_UNIDENTIFIED;
                    clients[slot].elector_id = -1;
                    clients[slot].vote = 0;

                    event.events = EPOLLIN;
                    event.data.fd = cfd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cfd, &event) < 0)
                        ERR("epoll_ctl");
                }
                else
                {
                    int idx = find_client(fd);
                    if (idx >= 0)
                        handle_client(fd, idx, epoll_fd);
                }
            }
        }
        else
        {
            if (errno == EINTR) continue;
            ERR("epoll_pwait");
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].fd != -1)
            disconnect_client(i, epoll_fd);

    if (TEMP_FAILURE_RETRY(close(epoll_fd)) < 0)
        ERR("close");
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("Setting SIGPIPE");
    setbuf(stdout, NULL);

    uint16_t port = (uint16_t)atoi(argv[1]);
    int server_fd = bind_tcp_socket(port, BACKLOG);
    int flags = fcntl(server_fd, F_GETFL) | O_NONBLOCK;
    fcntl(server_fd, F_SETFL, flags);

    doServer(server_fd);

    if (TEMP_FAILURE_RETRY(close(server_fd)) < 0)
        ERR("close server");

    return EXIT_SUCCESS;
}
