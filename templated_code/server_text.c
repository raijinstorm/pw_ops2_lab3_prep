// =============================================================================
// TEMPLATE: TEXT SERVER  (newline-delimited messages, epoll-multiplexed)
// -----------------------------------------------------------------------------
// The pattern for protocols where each message is a line of text ending in '\n'
// and the length is NOT known ahead of time. The hard part (pre-solved here):
//   * TCP is a byte stream, so one read() may return half a line, several
//     lines, or a line split across two reads. Each client therefore needs its
//     OWN accumulation buffer; we extract complete lines with memchr('\n') and
//     memmove the leftover partial line to the front.
//
//   Covers: Father Laurence (werona), the cities stdin client, any "send me a
//   line, I'll act on it / broadcast it" server.
//
// Skeleton shared with the binary templates: epoll + client table +
// sigprocmask/epoll_pwait + graceful shutdown. The only protocol-specific parts
// are client_t.buf and handle_line().
//
// Test with netcat:   ./server_text 9000
//                     nc localhost 9000     (type lines; see them broadcast)
// =============================================================================
#include "common.h"

#define BACKLOG 8
#define MAX_EVENTS 16
#define MAX_CLIENTS 16
#define MAX_MSG_LEN 256  // longest single line we accept (excluding '\n')

typedef struct
{
    int fd;                    // -1 == free slot
    char buf[MAX_MSG_LEN + 1]; // partial-line accumulator
    int buf_len;
}
client_t;

static client_t clients[MAX_CLIENTS];
volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig)
{
    (void)sig;
    do_work = 0;
}

static void clients_init(void)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].fd = -1;
        clients[i].buf_len = 0;
    }
}

static int find_free(void)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].fd == -1)
            return i;
    return -1;
}

static int find_client(int fd)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].fd == fd)
            return i;
    return -1;
}

static void disconnect_client(int idx, int epfd)
{
    epoll_del(epfd, clients[idx].fd);
    TEMP_FAILURE_RETRY(close(clients[idx].fd));
    clients[idx].fd = -1;
    clients[idx].buf_len = 0;
}

// ---- PROTOCOL: act on one complete line (newline already stripped) ----------
// Demo: broadcast the line to every other connected client, prefixed with the
// sender's fd. Replace with your task logic (parse a command, pair clients, …).
static void handle_line(int idx, char *line, int epfd)
{
    char out[MAX_MSG_LEN + 32];
    int len = snprintf(out, sizeof(out), "%d: %s\n", clients[idx].fd, line);
    if (len < 0)
        return;
    if (len > (int)sizeof(out))
        len = sizeof(out);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].fd == -1 || i == idx)
            continue;
        if (bulk_write(clients[i].fd, out, len) < 0)
        {
            if (errno == EPIPE)
                disconnect_client(i, epfd);  // reader gone
            else
                ERR("bulk_write");
        }
    }
}

// Read available bytes, append to this client's buffer, and peel off every
// complete line. Returns with the leftover partial line kept at buf[0..buf_len).
static void handle_client_data(int idx, int epfd)
{
    client_t *c = &clients[idx];
    int space = MAX_MSG_LEN - c->buf_len;
    if (space <= 0)
    {
        // a full buffer with no newline => line too long for the protocol
        disconnect_client(idx, epfd);
        return;
    }

    ssize_t n = TEMP_FAILURE_RETRY(read(c->fd, c->buf + c->buf_len, space));
    if (n <= 0)  // 0 == peer closed, <0 == error
    {
        disconnect_client(idx, epfd);
        return;
    }
    c->buf_len += (int)n;

    char *start = c->buf;
    int remaining = c->buf_len;
    char *nl;
    while ((nl = memchr(start, '\n', remaining)) != NULL)
    {
        *nl = '\0';
        handle_line(idx, start, epfd);
        if (c->fd == -1)
            return;  // client was disconnected inside handle_line
        int consumed = (int)(nl - start) + 1;
        remaining -= consumed;
        start = nl + 1;
    }

    if (remaining > 0 && start != c->buf)
        memmove(c->buf, start, remaining);  // keep the partial line
    c->buf_len = remaining;
}

// Optional: treat the server's own stdin as a control channel (one line in).
static void handle_stdin(int epfd)
{
    char line[MAX_MSG_LEN + 1];
    ssize_t n = TEMP_FAILURE_RETRY(read(STDIN_FILENO, line, MAX_MSG_LEN));
    if (n <= 0)
    {
        // EOF on stdin: drop it from epoll, else level-triggered epoll would
        // report it readable forever and spin the loop.
        epoll_del(epfd, STDIN_FILENO);
        return;
    }
    line[n] = '\0';
    // ---- TODO: parse server-operator commands here ----
    printf("[stdin] %.*s", (int)n, line);
    fflush(stdout);
}

void doServer(int listen_fd)
{
    int epfd;
    if ((epfd = epoll_create1(0)) < 0)
        ERR("epoll_create1");
    epoll_add(epfd, listen_fd, EPOLLIN);
    // stdin as a control channel. epoll rejects un-pollable fds (a regular file
    // redirect, e.g. `./server_text 9000 < file`) with EPERM -> just skip it.
    {
        struct epoll_event ev = {.events = EPOLLIN, .data.fd = STDIN_FILENO};
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1 && errno != EPERM)
            ERR("epoll_ctl ADD stdin");
    }

    clients_init();

    struct epoll_event events[MAX_EVENTS];
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);  // SIGINT delivered only inside epoll_pwait

    while (do_work)
    {
        int nfds = epoll_pwait(epfd, events, MAX_EVENTS, -1, &oldmask);
        if (nfds < 0)
        {
            if (errno == EINTR)
                continue;  // SIGINT -> do_work cleared -> loop exits
            ERR("epoll_pwait");
        }
        for (int i = 0; i < nfds; i++)
        {
            int fd = events[i].data.fd;
            if (fd == listen_fd)
            {
                int cfd = add_new_client(listen_fd);
                if (cfd < 0)
                    continue;
                int slot = find_free();
                if (slot == -1)
                {
                    TEMP_FAILURE_RETRY(close(cfd));  // no room -> politely drop
                    continue;
                }
                clients[slot].fd = cfd;
                clients[slot].buf_len = 0;
                epoll_add(epfd, cfd, EPOLLIN);
                printf("client connected: fd=%d slot=%d\n", cfd, slot);
            }
            else if (fd == STDIN_FILENO)
            {
                handle_stdin(epfd);
            }
            else
            {
                int idx = find_client(fd);
                if (idx >= 0)
                    handle_client_data(idx, epfd);
            }
        }
    }

    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].fd != -1)
            disconnect_client(i, epfd);
    if (TEMP_FAILURE_RETRY(close(epfd)) < 0)
        ERR("close");
    printf("server: shutting down\n");
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s port\n", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc != 2)
        usage(argv[0]);
    if (sethandler(SIG_IGN, SIGPIPE))  // writing to a gone client => EPIPE, not death
        ERR("sethandler SIGPIPE");
    if (sethandler(sigint_handler, SIGINT))
        ERR("sethandler SIGINT");

    uint16_t port = (uint16_t)atoi(argv[1]);
    int listen_fd = bind_tcp_socket(port, BACKLOG);  // swap for bind_local_socket for UNIX
    set_nonblock(listen_fd);

    doServer(listen_fd);

    if (TEMP_FAILURE_RETRY(close(listen_fd)) < 0)
        ERR("close");
    return EXIT_SUCCESS;
}
