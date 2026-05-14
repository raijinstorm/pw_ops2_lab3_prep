#include "l7-common.h"
#include <time.h>

void usage(char *name)
{
    printf("%s <timeout>\n", name);
    printf("  timeout - max waiting time after receiving the last message/connection (in seconds)\n");
    exit(EXIT_FAILURE);
}

#define SWAP(a, b)                      \
    do                                  \
    {                                   \
        __typeof__(a) __a = (a);        \
        __typeof__(b) __b = (b);        \
        __typeof__(*__a) __tmp = *__a;  \
        *__a = *__b;                    \
        *__b = __tmp;                   \
    } while (0)

#define MAX_CLIENTS 10
#define MAX_PAIRS 3
#define UNIX_SK_NAME "Laurenty"
#define MAX_MSG_LEN 63
#define MAX_EVENTS 16
#define BACKLOG 5

typedef enum { STATE_UNNAMED, STATE_NAMED, STATE_COMPLETE, STATE_PAIRED } client_state_t;

typedef struct {
    int fd;
    client_state_t state;
    int partner;
    char name[MAX_MSG_LEN + 1];
    char beloved[MAX_MSG_LEN + 1];
    char buf[MAX_MSG_LEN + 1];
    int buf_len;
} client_t;

static void set_deadline(struct timespec *deadline, int timeout_sec)
{
    clock_gettime(CLOCK_MONOTONIC, deadline);
    deadline->tv_sec += timeout_sec;
}

static int remaining_ms(struct timespec *deadline)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    long ms = (deadline->tv_sec - now.tv_sec) * 1000
            + (deadline->tv_nsec - now.tv_nsec) / 1000000;
    return ms > 0 ? (int)ms : 0;
}

static int find_free(client_t *clients)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].fd == -1)
            return i;
    return -1;
}

static int find_client(client_t *clients, int fd)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].fd == fd)
            return i;
    return -1;
}

static void close_client(client_t *clients, int idx, int epoll_fd)
{
    client_t *c = &clients[idx];
    if (c->state == STATE_PAIRED && c->partner >= 0)
        clients[c->partner].partner = -1;
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c->fd, NULL);
    TEMP_FAILURE_RETRY(close(c->fd));
    memset(c, 0, sizeof(client_t));
    c->fd = -1;
    c->partner = -1;
}

static void disconnect_client(client_t *clients, int idx, int epoll_fd)
{
    printf("I lost contact with %s\n", clients[idx].name[0] ? clients[idx].name : "??");
    close_client(clients, idx, epoll_fd);
}

static void marry_clients(client_t *clients, int i, int j, int epoll_fd)
{
    (void)epoll_fd;
    printf("%s and %s got married!\n", clients[i].name, clients[i].beloved);

    char msg[MAX_MSG_LEN + 2];
    int len = snprintf(msg, sizeof(msg), "Congratulations, %s and %s!\n",
                       clients[i].name, clients[i].beloved);
    bulk_write(clients[i].fd, msg, len);
    bulk_write(clients[j].fd, msg, len);

    clients[i].state = STATE_PAIRED;
    clients[i].partner = j;
    clients[j].state = STATE_PAIRED;
    clients[j].partner = i;
}

static void try_match(client_t *clients, int idx, int epoll_fd)
{
    client_t *a = &clients[idx];
    for (int j = 0; j < MAX_CLIENTS; j++)
    {
        if (j == idx || clients[j].state != STATE_COMPLETE) continue;
        client_t *b = &clients[j];
        if (strcmp(a->beloved, b->name) == 0 && strcmp(b->beloved, a->name) == 0)
        {
            marry_clients(clients, idx, j, epoll_fd);
            return;
        }
    }
}

static void handle_line(client_t *clients, int idx, char *line, int epoll_fd)
{
    client_t *c = &clients[idx];
    switch (c->state)
    {
        case STATE_UNNAMED:
            strncpy(c->name, line, MAX_MSG_LEN);
            c->state = STATE_NAMED;
            break;
        case STATE_NAMED:
            strncpy(c->beloved, line, MAX_MSG_LEN);
            c->state = STATE_COMPLETE;
            printf("%s wants to marry %s\n", c->name, c->beloved);
            try_match(clients, idx, epoll_fd);
            break;
        case STATE_COMPLETE:
            break;
        case STATE_PAIRED:
            if (c->partner >= 0 && clients[c->partner].fd != -1)
            {
                char msg[MAX_MSG_LEN + 2];
                int len = snprintf(msg, sizeof(msg), "%s\n", line);
                bulk_write(clients[c->partner].fd, msg, len);
            }
            break;
    }
}

static void handle_client_data(client_t *clients, int idx, int epoll_fd)
{
    client_t *c = &clients[idx];
    int space = MAX_MSG_LEN - c->buf_len;
    if (space <= 0)
    {
        disconnect_client(clients, idx, epoll_fd);
        return;
    }

    ssize_t n = read(c->fd, c->buf + c->buf_len, space);
    if (n <= 0)
    {
        disconnect_client(clients, idx, epoll_fd);
        return;
    }
    c->buf_len += (int)n;

    char *start = c->buf;
    int remaining = c->buf_len;
    while (remaining > 0)
    {
        char *nl = memchr(start, '\n', remaining);
        if (!nl) break;
        *nl = '\0';

        handle_line(clients, idx, start, epoll_fd);
        if (c->fd == -1) return; // client was closed inside handle_line

        int consumed = (nl - start) + 1;
        remaining -= consumed;
        start = nl + 1;
    }

    // Move leftover bytes to front of buffer
    if (remaining > 0 && start != c->buf)
        memmove(c->buf, start, remaining);
    c->buf_len = remaining;
}

static void handle_stdin(client_t *clients)
{
    char *line = NULL;
    size_t size = 0;
    ssize_t n = getline(&line, &size, stdin);
    if (n <= 0) { free(line); return; }
    if (line[n - 1] == '\n') line[n - 1] = '\0';

    char *colon = strchr(line, ':');
    if (!colon)
    {
        printf("Invalid stdin format (expected <addressee>:<message>)\n");
        free(line);
        return;
    }
    *colon = '\0';
    char *addressee = line;
    char *message   = colon + 1;

    int found = -1;
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].fd != -1 && strcmp(clients[i].name, addressee) == 0)
            { found = i; break; }

    if (found == -1)
    {
        printf("Unknown addressee: %s\n", addressee);
        free(line);
        return;
    }

    char msg[MAX_MSG_LEN + 2];
    int len = snprintf(msg, sizeof(msg), "%s\n", message);
    bulk_write(clients[found].fd, msg, len);
    free(line);
}

void doServer(int listen_fd, int timeout_sec)
{
    int epoll_fd;
    if ((epoll_fd = epoll_create1(0)) < 0)
        ERR("epoll_create1");

    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) < 0)
        ERR("epoll_ctl");

    event.events = EPOLLIN;
    event.data.fd = STDIN_FILENO;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &event) < 0 && errno != EPERM)
        ERR("epoll_ctl");

    client_t clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        memset(&clients[i], 0, sizeof(client_t));
        clients[i].fd = -1;
        clients[i].partner = -1;
    }

    struct timespec deadline;
    set_deadline(&deadline, timeout_sec);

    int do_work = 1;
    while (do_work)
    {
        int nfds;
        if ((nfds = epoll_pwait(epoll_fd, events, MAX_EVENTS, remaining_ms(&deadline), NULL)) > 0)
        {
            set_deadline(&deadline, timeout_sec);
            for (int i = 0; i < nfds; i++)
            {
                int fd = events[i].data.fd;
                if (fd == STDIN_FILENO)
                {
                    handle_stdin(clients);
                }
                else if (fd == listen_fd)
                {
                    int cfd = add_new_client(listen_fd);
                    if (cfd < 0) continue;

                    int slot = find_free(clients);
                    if (slot == -1)
                    {
                        TEMP_FAILURE_RETRY(close(cfd));
                        continue;
                    }

                    clients[slot].fd = cfd;
                    event.events = EPOLLIN;
                    event.data.fd = cfd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cfd, &event) < 0)
                        ERR("epoll_ctl");

                    printf("Another young person (%d) needs my help!\n", cfd);
                }
                else
                {
                    int idx = find_client(clients, fd);
                    if (idx >= 0)
                        handle_client_data(clients, idx, epoll_fd);
                }
            }
        }
        else
        {
            if (nfds == 0)
            {
                printf("No one needs my help anymore!\n");
                do_work = 0;
            }
            else if (errno == EINTR)
                continue;
            else
                ERR("epoll_pwait");
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].fd != -1)
            close_client(clients, i, epoll_fd);

    if (TEMP_FAILURE_RETRY(close(epoll_fd)) < 0)
        ERR("close");
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    int timeout = atoi(argv[1]);
    if (timeout < 1)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    sethandler(SIG_IGN, SIGPIPE);

    int listen_fd = bind_local_socket(UNIX_SK_NAME, BACKLOG);
    int flags = fcntl(listen_fd, F_GETFL) | O_NONBLOCK;
    fcntl(listen_fd, F_SETFL, flags);

    doServer(listen_fd, timeout);

    if (TEMP_FAILURE_RETRY(close(listen_fd)) < 0)
        ERR("close");
    if (unlink(UNIX_SK_NAME) < 0)
        ERR("unlink");

    return EXIT_SUCCESS;
}
