// =============================================================================
// TEMPLATE: LENGTH-PREFIXED BINARY SERVER  (variable size, reassembled)
// -----------------------------------------------------------------------------
// The pattern when binary messages VARY in size: each message is a 1-byte
// header giving the body length (0..255) followed by that many body bytes. This
// is the genuinely tricky case students fail, because TCP is a byte stream:
//   * the header and body can arrive in separate reads,
//   * a single read can deliver a partial body,
//   * two messages can arrive glued together.
// So each client carries a small STATE MACHINE (reading_header? how many body
// bytes so far?) and we do exactly ONE read() per epoll wakeup — never blocking,
// letting level-triggered epoll re-fire while bytes remain.
//
//   Covers: the witches' coven protocol (sop-codex).
//
// Wire format:  [1 byte: len N]  [N bytes: body]
// Multi-byte integers inside the body are still your responsibility to
// htonl/ntohl (e.g. a 4-byte body carrying an int — see handle_message).
//
// Test with the matching client:  ./server_binary_framed 9000
//                                 ./client_binary_framed localhost 9000 hello hi
// =============================================================================
#include "common.h"

#define BACKLOG 8
#define MAX_EVENTS 16
#define MAX_CLIENTS 16
#define MAX_BODY 255

typedef struct
{
    int fd;                       // -1 == free
    unsigned char header;         // body length of the message in flight
    unsigned char body[MAX_BODY];
    size_t received;              // body bytes collected so far
    int reading_header;           // 1 = next read fills header, 0 = filling body
}
client_t;

static client_t clients[MAX_CLIENTS];
volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig)
{
    (void)sig;
    do_work = 0;
}

static void reset_msg(client_t *c)
{
    c->header = 0;
    c->received = 0;
    c->reading_header = 1;
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

// Send one framed message: [len][body].
static int send_framed(int fd, const unsigned char *body, unsigned char len)
{
    unsigned char frame[1 + MAX_BODY];
    frame[0] = len;
    if (len)
        memcpy(frame + 1, body, len);
    return bulk_write(fd, (char *)frame, (size_t)1 + len) < 0 ? -1 : 0;
}

// ---- PROTOCOL: one complete message has arrived in clients[idx].body[0..len) -
static void handle_message(int idx, int epfd)
{
    client_t *c = &clients[idx];
    unsigned char len = c->header;

    // ---- TODO: dispatch on the body. Examples: ----
    if (len == 4)
    {
        int32_t v;
        memcpy(&v, c->body, 4);
        printf("fd=%d: int message = %d\n", c->fd, ntohl(v));  // 4-byte body => integer
    }
    else
    {
        printf("fd=%d: msg (len=%u): %.*s\n", c->fd, len, (int)len, c->body);
    }

    // demo: echo the sameframed message back to the sender
    if (send_framed(c->fd, c->body, len) < 0)
    {
        if (errno == EPIPE)
            disconnect_client(idx, epfd);
        else
            ERR("send_framed");
    }
}

// Exactly ONE read() per call; advance the per-client state machine.
static void handle_client(int idx, int epfd)
{
    client_t *c = &clients[idx];
    ssize_t n;
    if (c->reading_header)
        n = TEMP_FAILURE_RETRY(read(c->fd, &c->header, 1));
    else
        n = TEMP_FAILURE_RETRY(read(c->fd, c->body + c->received, (size_t)c->header - c->received));

    if (n < 0)
    {
        perror("read");
        disconnect_client(idx, epfd);
        return;
    }
    if (n == 0)  // peer closed
    {
        disconnect_client(idx, epfd);
        return;
    }

    if (c->reading_header)
    {
        c->reading_header = 0;
        c->received = 0;
        if (c->header == 0)  // empty body => message is already complete
        {
            handle_message(idx, epfd);
            if (c->fd != -1)
                reset_msg(c);
        }
        return;
    }

    c->received += (size_t)n;
    if (c->received == c->header)  // body fully assembled
    {
        handle_message(idx, epfd);
        if (c->fd != -1)
            reset_msg(c);
    }
}

void doServer(int listen_fd)
{
    int epfd;
    if ((epfd = epoll_create1(0)) < 0)
        ERR("epoll_create1");
    epoll_add(epfd, listen_fd, EPOLLIN);
    clients_init();

    struct epoll_event events[MAX_EVENTS];
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    while (do_work)
    {
        int nfds = epoll_pwait(epfd, events, MAX_EVENTS, -1, &oldmask);
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
    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("sethandler SIGPIPE");
    if (sethandler(sigint_handler, SIGINT))
        ERR("sethandler SIGINT");

    uint16_t port = (uint16_t)atoi(argv[1]);
    int listen_fd = bind_tcp_socket(port, BACKLOG);
    set_nonblock(listen_fd);

    doServer(listen_fd);

    if (TEMP_FAILURE_RETRY(close(listen_fd)) < 0)
        ERR("close");
    return EXIT_SUCCESS;
}
