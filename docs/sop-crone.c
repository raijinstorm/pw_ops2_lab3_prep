#include "l7-common.h"

#define BACKLOG 8
#define MAX_EVENTS 8
#define MAX_MSG_SIZE 256

#define MSG_PENDING 0
#define MSG_COMPLETE 1
#define MSG_INVALID -1
#define MSG_CLOSED -2

typedef struct {
    unsigned char header;
    unsigned char body[255];
    size_t received;
    int reading_header;
} message_t;

static volatile sig_atomic_t do_work = 1;

static void sigint_handler(int sig)
{
    (void)sig;
    do_work = 0;
}

static void usage(char *name)
{
    fprintf(stderr, "Usage: %s <port>\n", name);
    exit(EXIT_FAILURE);
}

static void resetMessage(message_t *msg)
{
    msg->header = 0;
    msg->received = 0;
    msg->reading_header = 1;
}

static void closeConnection(int fd, int epoll_fd)
{
    if (fd < 0)
        return;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0)
        perror("epoll_ctl DEL");
    if (TEMP_FAILURE_RETRY(close(fd)) < 0)
        ERR("close");
}

static int connectMother(struct sockaddr_in maiden_addr, uint16_t port)
{
    int mother_fd = make_tcp_socket();
    maiden_addr.sin_port = port;
    if (connect(mother_fd, (struct sockaddr *)&maiden_addr, sizeof(maiden_addr)) < 0)
        ERR("connect");
    return mother_fd;
}

static int handleNewConnection(int listen_fd, int *maiden_fd, struct sockaddr_in *maiden_addr)
{
    int new_fd;
    socklen_t addr_len = sizeof(*maiden_addr);

    if ((new_fd = TEMP_FAILURE_RETRY(accept(listen_fd, (struct sockaddr *)maiden_addr, &addr_len))) < 0)
    {
        if (EAGAIN == errno || EWOULDBLOCK == errno)
            return 0;
        ERR("accept");
    }

    if (*maiden_fd == -1) {
        *maiden_fd = new_fd;
        return 1;
    }

    if (TEMP_FAILURE_RETRY(close(new_fd)) < 0)
        ERR("close");
    return 0;
}

static int handleCandidateConnection(int listen_fd, int *candidate_fd, struct sockaddr_in *candidate_addr)
{
    int new_fd;
    socklen_t addr_len = sizeof(*candidate_addr);

    if ((new_fd = TEMP_FAILURE_RETRY(accept(listen_fd, (struct sockaddr *)candidate_addr, &addr_len))) < 0)
    {
        if (EAGAIN == errno || EWOULDBLOCK == errno)
            return 0;
        ERR("accept");
    }

    if (*candidate_fd == -1) {
        *candidate_fd = new_fd;
        return 1;
    }

    if (TEMP_FAILURE_RETRY(close(new_fd)) < 0)
        ERR("close");
    return 0;
}

static int handleFirstMessage(int maiden_fd, message_t *msg, uint16_t *mother_port)
{
    ssize_t n;

    if (msg->reading_header)
        n = TEMP_FAILURE_RETRY(read(maiden_fd, &msg->header, 1));
    else
        n = TEMP_FAILURE_RETRY(read(maiden_fd, msg->body + msg->received, (size_t)(msg->header) - msg->received));

    if (n < 0)
        ERR("read");

    if (n == 0) {
        return MSG_CLOSED;
    }

    if (msg->reading_header) {
        msg->reading_header = 0;
        msg->received = 0;
        if (msg->header == 0)
            return MSG_INVALID;
        return MSG_PENDING;
    }

    msg->received += (size_t)n;
    if (msg->received == msg->header) {
        if (msg->header != 2)
            return MSG_INVALID;
        memcpy(mother_port, msg->body, sizeof(uint16_t));
        return MSG_COMPLETE;
    }
    return MSG_PENDING;
}

static void sendStartMessage(int maiden_fd)
{
    unsigned char msg[5] = {4, 0, 0, 0, 0};
    if (bulk_write(maiden_fd, (char *)msg, sizeof(msg)) < (ssize_t)sizeof(msg))
        ERR("write");
}

static void sendCandidateMessage(int maiden_fd, struct sockaddr_in candidate_addr, uint16_t port)
{
    unsigned char msg[7];

    msg[0] = 6;
    memcpy(msg + 1, &candidate_addr.sin_addr.s_addr, sizeof(candidate_addr.sin_addr.s_addr));
    memcpy(msg + 5, &port, sizeof(port));

    if (bulk_write(maiden_fd, (char *)msg, sizeof(msg)) < (ssize_t)sizeof(msg))
        ERR("write");
}

static int handleMother(int mother_fd, message_t *msg)
{
    ssize_t n;

    if (msg->reading_header)
        n = TEMP_FAILURE_RETRY(read(mother_fd, &msg->header, 1));
    else
        n = TEMP_FAILURE_RETRY(read(mother_fd, msg->body + msg->received, (size_t)(msg->header) - msg->received));

    if (n < 0)
        ERR("read");

    if (n == 0) {
        printf("The mother witch left the coven, we are hopeless\n");
        return 1;
    }

    if (msg->reading_header) {
        msg->reading_header = 0;
        msg->received = 0;
        if (msg->header == 0) {
            resetMessage(msg);
            return 0;
        }
        return 0;
    }

    msg->received += (size_t)n;
    if (msg->received == msg->header) {
        if (msg->header == 4) {
            int32_t value;
            memcpy(&value, msg->body, sizeof(int32_t));
            printf("%d\n", ntohl(value));
        }
        else if (msg->header > 6) {
            printf("%.*s\n", msg->header, msg->body);
        }
        resetMessage(msg);
    }

    return 0;
}

static int handleMaidenAfterStart(int maiden_fd)
{
    unsigned char c;
    ssize_t n = TEMP_FAILURE_RETRY(recv(maiden_fd, &c, sizeof(c), MSG_PEEK));

    if (n < 0)
        ERR("recv");
    if (n == 0) {
        printf("The maiden witch left the coven, we are hopeless\n");
        return 1;
    }
    return 0;
}

void doServer(int listen_fd)
{
    int epoll_fd;
    struct epoll_event event, events[MAX_EVENTS];
    int maiden_fd = -1;
    int mother_fd = -1;
    int candidate_fd = -1;
    struct sockaddr_in maiden_addr;
    struct sockaddr_in candidate_addr;
    message_t maiden_msg;
    message_t mother_msg;
    message_t candidate_msg;
    uint16_t mother_port = 0;
    int ritual_started = 0;

    if ((epoll_fd = epoll_create1(0)) < 0)
        ERR("epoll_create1");

    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN;
    event.data.fd = listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) < 0)
        ERR("epoll_ctl listen");

    resetMessage(&maiden_msg);
    resetMessage(&mother_msg);
    resetMessage(&candidate_msg);

    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    while (do_work) {
        int nfds = epoll_pwait(epoll_fd, events, MAX_EVENTS, -1, &oldmask);
        if (nfds < 0) {
            if (errno == EINTR)
                continue;
            ERR("epoll_pwait");
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == listen_fd) {
                if (!ritual_started) {
                    if (handleNewConnection(listen_fd, &maiden_fd, &maiden_addr)) {
                        memset(&event, 0, sizeof(event));
                        event.events = EPOLLIN | EPOLLRDHUP;
                        event.data.fd = maiden_fd;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, maiden_fd, &event) < 0)
                            ERR("epoll_ctl client");
                    }
                }
                else {
                    if (handleCandidateConnection(listen_fd, &candidate_fd, &candidate_addr)) {
                        resetMessage(&candidate_msg);
                        memset(&event, 0, sizeof(event));
                        event.events = EPOLLIN | EPOLLRDHUP;
                        event.data.fd = candidate_fd;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, candidate_fd, &event) < 0)
                            ERR("epoll_ctl candidate");
                    }
                }
                continue;
            }

            if (!ritual_started && maiden_fd >= 0 && events[i].data.fd == maiden_fd) {
                int status = handleFirstMessage(maiden_fd, &maiden_msg, &mother_port);
                if (status == MSG_INVALID)
                    do_work = 0;
                else if (status == MSG_CLOSED) {
                    printf("No!  The ritual...\n");
                    do_work = 0;
                }
                else if (status == MSG_COMPLETE) {
                    mother_fd = connectMother(maiden_addr, mother_port);
                    memset(&event, 0, sizeof(event));
                    event.events = EPOLLIN | EPOLLRDHUP;
                    event.data.fd = mother_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, mother_fd, &event) < 0)
                        ERR("epoll_ctl mother");
                    sendStartMessage(maiden_fd);
                    ritual_started = 1;
                }
            }
            else if (ritual_started && maiden_fd >= 0 && events[i].data.fd == maiden_fd) {
                if (handleMaidenAfterStart(maiden_fd))
                    do_work = 0;
            }
            else if (ritual_started && mother_fd >= 0 && events[i].data.fd == mother_fd) {
                if (handleMother(mother_fd, &mother_msg))
                    do_work = 0;
            }
            else if (ritual_started && candidate_fd >= 0 && events[i].data.fd == candidate_fd) {
                int status = handleFirstMessage(candidate_fd, &candidate_msg, &mother_port);
                if (status == MSG_COMPLETE) {
                    sendCandidateMessage(maiden_fd, candidate_addr, mother_port);
                    closeConnection(maiden_fd, epoll_fd);
                    maiden_fd = candidate_fd;
                    maiden_addr = candidate_addr;
                    candidate_fd = -1;
                    resetMessage(&candidate_msg);
                    resetMessage(&maiden_msg);
                    sendStartMessage(maiden_fd);
                }
                else if (status != MSG_PENDING) {
                    printf("Another young one lost to the shadows\n");
                    closeConnection(candidate_fd, epoll_fd);
                    candidate_fd = -1;
                    resetMessage(&candidate_msg);
                }
            }
        }
    }

    if (maiden_fd >= 0) {
        closeConnection(maiden_fd, epoll_fd);
    }
    if (candidate_fd >= 0) {
        closeConnection(candidate_fd, epoll_fd);
    }
    if (mother_fd >= 0) {
        closeConnection(mother_fd, epoll_fd);
    }
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    if (TEMP_FAILURE_RETRY(close(epoll_fd)) < 0)
        ERR("close");
}

int main(int argc, char **argv)
{
    uint16_t port;
    int listen_fd;

    if (argc != 2)
        usage(argv[0]);

    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("sethandler SIGPIPE");
    if (sethandler(sigint_handler, SIGINT))
        ERR("sethandler SIGINT");

    port = (uint16_t)atoi(argv[1]);
    if (port == 0)
        usage(argv[0]);

    listen_fd = bind_tcp_socket(port, BACKLOG);
    int flags = fcntl(listen_fd, F_GETFL) | O_NONBLOCK;
    fcntl(listen_fd, F_SETFL, flags);

    doServer(listen_fd);

    if (TEMP_FAILURE_RETRY(close(listen_fd)) < 0)
        ERR("close");

    return EXIT_SUCCESS;
}
