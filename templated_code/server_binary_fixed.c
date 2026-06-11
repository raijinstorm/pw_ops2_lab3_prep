// =============================================================================
// TEMPLATE: FIXED-SIZE BINARY SERVER  (struct on the wire, htonl/ntohl)
// -----------------------------------------------------------------------------
// The pattern when every message is the SAME known size: a fixed struct of
// integers in network byte order. Because the size is known, framing is trivial
// — bulk_read EXACTLY sizeof(msg) and bulk_write EXACTLY sizeof(msg). The only
// real work is byte-order conversion (ntohl on the way in, htonl on the way out).
//
//   Covers: the calculator (int32_t[5]), the cities 5-byte protocol, electors.
//
// IMPORTANT byte-order rule: convert EVERY multi-byte integer field. A single
// byte (or a char carried in a uint32) needs no swap, but keep it consistent.
//
// NOTE on bulk_read + epoll: reading the exact size can BLOCK if a malicious
// client sends half a message and then stalls. For well-behaved fixed-size
// protocols (the usual lab case) this is the standard, accepted approach. If you
// must tolerate dribbled/partial messages, use server_binary_framed.c, which
// keeps per-client reassembly state and never blocks.
//
// Test with the matching client:  ./server_binary_fixed 9000
//                                 ./client_binary_fixed localhost 9000 6 7 '*'
// =============================================================================
#include "common.h"

#define BACKLOG 8
#define MAX_EVENTS 16
#define MAX_CLIENTS 16

// Wire format: 5 x uint32_t, ALL in network byte order. Fixed-width types with
// no padding keep sizeof predictable across machines (== 20 bytes here).
typedef struct
{
    uint32_t op1;
    uint32_t op2;
    uint32_t result;
    uint32_t op;      // an operator char carried in 4 bytes
    uint32_t status;  // 1 = ok, 0 = invalid request
}
msg_t;

static int client_fds[MAX_CLIENTS];
volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig)
{
    (void)sig;
    do_work = 0;
}

static void clients_init(void)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        client_fds[i] = -1;
}
static int find_free(void)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (client_fds[i] == -1)
            return i;
    return -1;
}
static int find_client(int fd)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (client_fds[i] == fd)
            return i;
    return -1;
}
static void disconnect_client(int idx, int epfd)
{
    epoll_del(epfd, client_fds[idx]);
    TEMP_FAILURE_RETRY(close(client_fds[idx]));
    client_fds[idx] = -1;
}

// ---- PROTOCOL: fill in result/status from op1/op2/op (all host order here) --
static void compute(msg_t *m)
{
    uint32_t a = ntohl(m->op1), b = ntohl(m->op2);
    int32_t result = 0, status = 1;
    switch ((char)ntohl(m->op))
    {
        case '+': result = (int32_t)a + (int32_t)b; break;
        case '-': result = (int32_t)a - (int32_t)b; break;
        case '*': result = (int32_t)a * (int32_t)b; break;
        case '/': if (b == 0) status = 0; else result = (int32_t)a / (int32_t)b; break;
        default:  status = 0;
    }
    m->result = htonl((uint32_t)result);
    m->status = htonl((uint32_t)status);
}

static void handle_client(int idx, int epfd)
{
    msg_t m;
    ssize_t n = bulk_read(client_fds[idx], (char *)&m, sizeof(m));
    if (n == 0)  // clean EOF: client closed
    {
        disconnect_client(idx, epfd);
        return;
    }
    if (n != (ssize_t)sizeof(m))  // short read => closed mid-message / error
    {
        disconnect_client(idx, epfd);
        return;
    }

    compute(&m);  // request and reply share the struct

    if (bulk_write(client_fds[idx], (char *)&m, sizeof(m)) < 0)
    {
        if (errno == EPIPE)
            disconnect_client(idx, epfd);
        else
            ERR("bulk_write");
        return;
    }
    // request/response done; we keep the connection open for the next request.
    // (For a calc-style "one request then close" server, disconnect here.)
    // To BROADCAST instead of reply, loop over client_fds[] and write to each.
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
                client_fds[slot] = cfd;
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
        if (client_fds[i] != -1)
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
