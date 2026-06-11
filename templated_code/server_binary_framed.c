// =============================================================================
// TEMPLATE: LINE-BASED TEXT SERVER (TCP + epoll)
// -----------------------------------------------------------------------------
// Protocol:
//      COMMAND arg1 arg2 ...\n
//
// Examples:
//      HELLO\n
//      LAUNCH 123\n
//      MSG hello world\n
//
// Why this template exists:
//
// TCP is a BYTE STREAM.
//
// One write():
//      "LAUNCH 123\n"
//
// may arrive as:
//
//      "LAUN"
//      "CH 123\n"
//
// or:
//
//      "LAUNCH 123\nHELLO\n"
//
// in a single read().
//
// Therefore each client owns an accumulator buffer.
// We append bytes to it until '\n' appears.
// Every complete line becomes one message.
// The unfinished tail remains for the next read.
//
// Exactly ONE read() per epoll wakeup.
// Level-triggered epoll will wake us again if more bytes remain.
//
// Covers:
//      - text commands
//      - chat protocols
//      - FIFO-like command streams
//      - telnet-style protocols
//      - "LAUNCH <id>" assignments
// =============================================================================

#include "common.h"

#define BACKLOG 8
#define MAX_EVENTS 16
#define MAX_CLIENTS 16
#define MAX_LINE 255

typedef struct
{
    int fd;                     // -1 == free
    char buf[MAX_LINE + 1];     // accumulated line
    size_t len;                 // bytes currently stored
} client_t;

static client_t clients[MAX_CLIENTS];
volatile sig_atomic_t do_work = 1;

// =============================================================================
// SIGNALS
// =============================================================================

void sigint_handler(int sig)
{
    (void)sig;
    do_work = 0;
}

// =============================================================================
// CLIENT HELPERS
// =============================================================================

static void reset_msg(client_t *c)
{
    c->len = 0;
    c->buf[0] = '\0';
}

static void clients_init(void)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].fd = -1;
        reset_msg(&clients[i]);
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
    reset_msg(&clients[idx]);
}

// =============================================================================
// WRITING
// =============================================================================

static int send_line(int fd, const char *line)
{
    char out[MAX_LINE + 2];

    int n = snprintf(out, sizeof(out), "%s\n", line);

    if (n < 0)
        return -1;

    return bulk_write(fd, out, (size_t)n) < 0 ? -1 : 0;
}

// =============================================================================
// APPLICATION LOGIC
// =============================================================================

static void handle_message(int idx, int epfd, char *line)
{
    client_t *c = &clients[idx];

    printf("fd=%d: [%s]\n", c->fd, line);

    // ------------------------------------------------------------
    // Example command parser:
    // ------------------------------------------------------------

    unsigned int id;

    if (sscanf(line, "LAUNCH %u", &id) == 1)
    {
        printf("LAUNCH received, id=%u\n", id);
    }
    else if (strcmp(line, "HELLO") == 0)
    {
        printf("HELLO received\n");
    }

    // ------------------------------------------------------------
    // Demo: echo back
    // ------------------------------------------------------------

    if (send_line(c->fd, line) < 0)
    {
        if (errno == EPIPE)
            disconnect_client(idx, epfd);
        else
            ERR("send_line");
    }
}

// =============================================================================
// ONE READ PER EPOLL WAKEUP
// =============================================================================

static void handle_client(int idx, int epfd)
{
    client_t *c = &clients[idx];

    char tmp[128];

    ssize_t n = TEMP_FAILURE_RETRY(read(c->fd, tmp, sizeof(tmp)));

    if (n < 0)
    {
        perror("read");
        disconnect_client(idx, epfd);
        return;
    }

    if (n == 0)
    {
        disconnect_client(idx, epfd);
        return;
    }

    for (ssize_t i = 0; i < n; i++)
    {
        char ch = tmp[i];

        // --------------------------------------------------------
        // complete line received
        // --------------------------------------------------------

        if (ch == '\n')
        {
            c->buf[c->len] = '\0';

            handle_message(idx, epfd, c->buf);

            if (c->fd == -1)
                return;

            reset_msg(c);
        }
        else
        {
            // ----------------------------------------------------
            // line too long
            // ----------------------------------------------------

            if (c->len >= MAX_LINE)
            {
                fprintf(stderr,
                        "fd=%d: line exceeds MAX_LINE\n",
                        c->fd);

                disconnect_client(idx, epfd);
                return;
            }

            c->buf[c->len++] = ch;
        }
    }
}

// =============================================================================
// SERVER LOOP
// =============================================================================

static void doServer(int listen_fd)
{
    int epfd;

    if ((epfd = epoll_create1(0)) < 0)
        ERR("epoll_create1");

    epoll_add(epfd, listen_fd, EPOLLIN);

    clients_init();

    struct epoll_event events[MAX_EVENTS];

    sigset_t mask;
    sigset_t oldmask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);

    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    while (do_work)
    {
        int nfds = epoll_pwait(
            epfd,
            events,
            MAX_EVENTS,
            -1,
            &oldmask);

        if (nfds < 0)
        {
            if (errno == EINTR)
                continue;

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
                    TEMP_FAILURE_RETRY(close(cfd));
                    continue;
                }

                clients[slot].fd = cfd;

                reset_msg(&clients[slot]);

                epoll_add(epfd, cfd, EPOLLIN);
            }
            else
            {
                int idx = find_client(fd);

                if (idx >= 0)
                    handle_client(idx, epfd);
            }
        }
    }

    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].fd != -1)
            disconnect_client(i, epfd);
    }

    TEMP_FAILURE_RETRY(close(epfd));

    printf("server: shutting down\n");
}

// =============================================================================
// MAIN
// =============================================================================

static void usage(char *name)
{
    fprintf(stderr, "USAGE: %s port\n", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc != 2)
        usage(argv[0]);

    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("sethandler SIGPIPE");

    if (sethandler(sigint_handler, SIGINT))
        ERR("sethandler SIGINT");

    uint16_t port = (uint16_t)atoi(argv[1]);

    int listen_fd = bind_tcp_socket(port, BACKLOG);

    set_nonblock(listen_fd);

    doServer(listen_fd);

    TEMP_FAILURE_RETRY(close(listen_fd));

    return EXIT_SUCCESS;
}