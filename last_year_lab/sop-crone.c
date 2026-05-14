#include "l7-common.h"

#define BACKLOG 5
#define MAX_EVENTS 8

typedef struct {
    uint8_t data[256];
    int received;
} msg_buf_t;

static int epoll_fd   = -1;
static int listen_fd  = -1;
static int maiden_fd  = -1;
static int mother_fd  = -1;
static int candidate_fd = -1;

static struct sockaddr_in maiden_addr;
static struct sockaddr_in candidate_addr;

static int maiden_setup_done  = 0;
static int accepting_candidates = 0;

static msg_buf_t maiden_buf;
static msg_buf_t mother_buf;
static msg_buf_t candidate_buf;

/* ---- epoll helpers ---- */

static void epoll_add(int fd)
{
    struct epoll_event ev;
    ev.events  = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0)
        ERR("epoll_ctl");
}

static void drop(int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    TEMP_FAILURE_RETRY(close(fd));
}

/* ---- message I/O ---- */

/* Read one byte at a time until the current message is complete.
   Returns 1 = complete, 0 = partial, -1 = disconnect/error. */
static int recv_msg(int fd, msg_buf_t *buf)
{
    int to_read = (buf->received == 0) ? 1
                                       : (1 + (int)buf->data[0]) - buf->received;
    if (to_read <= 0) return 1;

    ssize_t n = TEMP_FAILURE_RETRY(read(fd, buf->data + buf->received, to_read));
    if (n <= 0) return -1;
    buf->received += (int)n;

    if (buf->received >= 1 && buf->received == 1 + (int)buf->data[0])
        return 1;
    return 0;
}

static void send_msg(int fd, const uint8_t *body, uint8_t len)
{
    uint8_t frame[256];
    frame[0] = len;
    if (len > 0) memcpy(frame + 1, body, len);
    bulk_write(fd, (char *)frame, 1 + len);
}

static void send_zeroes(void)
{
    uint8_t body[4] = {0};
    send_msg(maiden_fd, body, 4);
}

/* ---- outgoing connect ---- */

static int tcp_connect(struct sockaddr_in *addr)
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) ERR("socket");
    if (TEMP_FAILURE_RETRY(connect(sock, (struct sockaddr *)addr, sizeof(*addr))) < 0)
        ERR("connect");
    return sock;
}

/* ---- event handlers ---- */

static void on_listen(void)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int cfd = TEMP_FAILURE_RETRY(accept(listen_fd, (struct sockaddr *)&addr, &len));
    if (cfd < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return;
        ERR("accept");
    }

    if (maiden_fd == -1)
    {
        maiden_fd  = cfd;
        maiden_addr = addr;
        memset(&maiden_buf, 0, sizeof(maiden_buf));
        epoll_add(maiden_fd);
    }
    else if (accepting_candidates && candidate_fd == -1)
    {
        candidate_fd   = cfd;
        candidate_addr = addr;
        memset(&candidate_buf, 0, sizeof(candidate_buf));
        epoll_add(candidate_fd);
        accepting_candidates = 0;
    }
    else
    {
        TEMP_FAILURE_RETRY(close(cfd));
    }
}

static void on_maiden(void)
{
    if (!maiden_setup_done)
    {
        /* Waiting for initial [2][port] from first maiden */
        int r = recv_msg(maiden_fd, &maiden_buf);
        if (r < 0)
        {
            printf("The maiden witch left the coven, we are hopeless\n");
            exit(EXIT_SUCCESS);
        }
        if (r == 1)
        {
            uint16_t port_net;
            memcpy(&port_net, maiden_buf.data + 1, 2);

            struct sockaddr_in addr = maiden_addr;
            addr.sin_port = port_net; /* already network byte order */

            mother_fd = tcp_connect(&addr);
            memset(&mother_buf, 0, sizeof(mother_buf));
            epoll_add(mother_fd);

            send_zeroes();
            maiden_setup_done    = 1;
            accepting_candidates = 1;
            memset(&maiden_buf, 0, sizeof(maiden_buf));
        }
    }
    else
    {
        /* Maiden should not be sending us data; watch for disconnect */
        char discard[256];
        ssize_t n = TEMP_FAILURE_RETRY(read(maiden_fd, discard, sizeof(discard)));
        if (n <= 0)
        {
            printf("The maiden witch left the coven, we are hopeless\n");
            exit(EXIT_SUCCESS);
        }
    }
}

static void on_mother(void)
{
    int r = recv_msg(mother_fd, &mother_buf);
    if (r < 0)
    {
        printf("The mother witch left the coven, we are hopeless\n");
        exit(EXIT_SUCCESS);
    }
    if (r == 1)
    {
        int   body_len = (int)mother_buf.data[0];
        uint8_t *body = mother_buf.data + 1;

        if (body_len == 4)
        {
            uint32_t n;
            memcpy(&n, body, 4);
            printf("%d\n", (int32_t)ntohl(n));
        }
        else if (body_len > 6)
        {
            printf("%.*s\n", body_len, (char *)body);
        }

        memset(&mother_buf, 0, sizeof(mother_buf));
    }
}

static void on_candidate(void)
{
    int r = recv_msg(candidate_fd, &candidate_buf);
    if (r < 0)
    {
        drop(candidate_fd);
        candidate_fd = -1;
        printf("Another young one lost to the shadows\n");
        accepting_candidates = 1;
        return;
    }
    if (r == 1)
    {
        uint16_t port_net;
        memcpy(&port_net, candidate_buf.data + 1, 2);

        if (mother_fd == -1)
        {
            /* No mother yet — connect to candidate's address:port */
            struct sockaddr_in addr = candidate_addr;
            addr.sin_port = port_net;
            mother_fd = tcp_connect(&addr);
            memset(&mother_buf, 0, sizeof(mother_buf));
            epoll_add(mother_fd);
        }
        else
        {
            /* Mother exists — tell old maiden to reconnect to candidate */
            uint8_t body[6];
            memcpy(body,     &candidate_addr.sin_addr.s_addr, 4); /* network order */
            memcpy(body + 4, &port_net,                       2); /* network order */
            send_msg(maiden_fd, body, 6);
            drop(maiden_fd);
        }

        /* Relabel candidate as new maiden */
        maiden_fd    = candidate_fd;
        maiden_addr  = candidate_addr;
        candidate_fd = -1;
        maiden_setup_done = 1;

        send_zeroes();
        accepting_candidates = 1;
    }
}

/* ---- main ---- */

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("Setting SIGPIPE");

    uint16_t port = (uint16_t)atoi(argv[1]);
    listen_fd = bind_tcp_socket(port, BACKLOG);
    int flags = fcntl(listen_fd, F_GETFL) | O_NONBLOCK;
    fcntl(listen_fd, F_SETFL, flags);

    if ((epoll_fd = epoll_create1(0)) < 0)
        ERR("epoll_create1");

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events  = EPOLLIN;
    ev.data.fd = listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) < 0)
        ERR("epoll_ctl");

    int do_work = 1;
    while (do_work)
    {
        int nfds;
        if ((nfds = epoll_pwait(epoll_fd, events, MAX_EVENTS, -1, NULL)) > 0)
        {
            for (int i = 0; i < nfds; i++)
            {
                int fd = events[i].data.fd;
                if      (fd == listen_fd)    on_listen();
                else if (fd == maiden_fd)    on_maiden();
                else if (fd == mother_fd)    on_mother();
                else if (fd == candidate_fd) on_candidate();
            }
        }
        else
        {
            if (errno == EINTR) continue;
            ERR("epoll_pwait");
        }
    }

    if (maiden_fd    != -1) drop(maiden_fd);
    if (mother_fd    != -1) drop(mother_fd);
    if (candidate_fd != -1) drop(candidate_fd);
    drop(listen_fd);
    TEMP_FAILURE_RETRY(close(epoll_fd));

    return EXIT_SUCCESS;
}
