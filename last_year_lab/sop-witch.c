#include "l7-common.h"

#define BACKLOG 4
#define MAX_EVENTS 8
#define MAX_MSG_BODY 255

typedef struct {
    unsigned char header;
    unsigned char body[MAX_MSG_BODY];
    size_t have;
    size_t need;
    int have_header;
} msg_reader_t;

typedef struct {
    int is_string;
    int32_t int_value;
    unsigned char str[MAX_MSG_BODY];
    uint8_t str_len;
} response_t;

static void usage(char *name)
{
    fprintf(stderr, "Usage: %s <listen_port> <crone_host> <crone_port> [i:<num>|s:<text>]\n", name);
    exit(EXIT_FAILURE);
}

static void reset_reader(msg_reader_t *reader)
{
    reader->header = 0;
    reader->have = 0;
    reader->need = 0;
    reader->have_header = 0;
}

static int send_message(int fd, uint8_t body_len, const void *body)
{
    unsigned char msg[1 + MAX_MSG_BODY];

    msg[0] = body_len;
    if (body_len > 0 && body != NULL)
        memcpy(msg + 1, body, body_len);

    return bulk_write(fd, (char *)msg, body_len + 1) == body_len + 1 ? 0 : -1;
}

static int parse_int32(const char *text, int32_t *value)
{
    char *end = NULL;
    long parsed;

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0')
        return -1;
    if (parsed < INT32_MIN || parsed > INT32_MAX)
        return -1;

    *value = (int32_t)parsed;
    return 0;
}

static void parse_response(int argc, char **argv, uint16_t listen_port, response_t *response)
{
    memset(response, 0, sizeof(*response));
    response->int_value = listen_port;

    if (argc == 4)
        return;

    if (argc != 5 || strlen(argv[4]) < 3 || argv[4][1] != ':')
        usage(argv[0]);

    if (argv[4][0] == 'i') {
        if (parse_int32(argv[4] + 2, &response->int_value) < 0)
            usage(argv[0]);
        return;
    }

    if (argv[4][0] == 's') {
        size_t len = strlen(argv[4] + 2);
        if (len > MAX_MSG_BODY)
            usage(argv[0]);
        response->is_string = 1;
        response->str_len = (uint8_t)len;
        memcpy(response->str, argv[4] + 2, len);
        return;
    }

    usage(argv[0]);
}

static int connect_to_ipv4(uint32_t addr_net, uint16_t port_net)
{
    int fd = make_tcp_socket();
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = addr_net;
    addr.sin_port = port_net;

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        ERR("connect");

    return fd;
}

static int connect_initial(char *host, char *port)
{
    return connect_tcp_socket(host, port);
}

static int accept_one(int listen_fd)
{
    int fd = TEMP_FAILURE_RETRY(accept(listen_fd, NULL, NULL));
    if (fd < 0)
        ERR("accept");
    return fd;
}

static int receive_message(int fd, msg_reader_t *reader, uint8_t *body_len, unsigned char body[MAX_MSG_BODY])
{
    ssize_t n;

    if (!reader->have_header) {
        n = TEMP_FAILURE_RETRY(read(fd, &reader->header, 1));
        if (n == 0)
            return 0;
        if (n < 0)
            ERR("read");
        reader->have_header = 1;
        reader->need = reader->header;
        reader->have = 0;
    }

    while (reader->have < reader->need) {
        n = TEMP_FAILURE_RETRY(read(fd, reader->body + reader->have, reader->need - reader->have));
        if (n == 0)
            return 0;
        if (n < 0)
            ERR("read");
        reader->have += (size_t)n;
    }

    *body_len = reader->header;
    if (reader->need > 0)
        memcpy(body, reader->body, reader->need);
    reset_reader(reader);
    return 1;
}

static void send_configured_response(int return_fd, const response_t *response)
{
    if (return_fd < 0)
        return;

    if (response->is_string) {
        if (send_message(return_fd, response->str_len, response->str) < 0)
            ERR("write");
        return;
    }

    int32_t value_net = htonl(response->int_value);
    if (send_message(return_fd, 4, &value_net) < 0)
        ERR("write");
}

static void forward_message(int return_fd, uint8_t body_len, const unsigned char body[MAX_MSG_BODY])
{
    if (return_fd < 0)
        return;
    if (send_message(return_fd, body_len, body) < 0)
        ERR("write");
}

static void do_witch(int listen_fd, int control_fd, const response_t *response)
{
    int epoll_fd;
    struct epoll_event event, events[MAX_EVENTS];
    int return_fd = -1;
    msg_reader_t reader;
    int pending_zero_msgs = 0;

    reset_reader(&reader);

    if ((epoll_fd = epoll_create1(0)) < 0)
        ERR("epoll_create1");

    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN;
    event.data.fd = listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) < 0)
        ERR("epoll_ctl listen");

    event.data.fd = control_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, control_fd, &event) < 0)
        ERR("epoll_ctl control");

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            if (errno == EINTR)
                continue;
            ERR("epoll_wait");
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == listen_fd) {
                int fd = accept_one(listen_fd);
                if (return_fd >= 0) {
                    if (TEMP_FAILURE_RETRY(close(fd)) < 0)
                        ERR("close");
                    continue;
                }

                return_fd = fd;
                while (pending_zero_msgs-- > 0)
                    send_configured_response(return_fd, response);
                pending_zero_msgs = 0;
                continue;
            }

            if (events[i].data.fd == control_fd) {
                uint8_t body_len;
                unsigned char body[MAX_MSG_BODY];
                int status = receive_message(control_fd, &reader, &body_len, body);

                if (status == 0)
                    goto cleanup;

                if (body_len == 4 && memcmp(body, "\0\0\0\0", 4) == 0) {
                    if (return_fd >= 0)
                        send_configured_response(return_fd, response);
                    else
                        pending_zero_msgs++;
                    continue;
                }

                if (body_len == 6) {
                    uint32_t addr_net;
                    uint16_t port_net_msg;
                    int new_control_fd;

                    memcpy(&addr_net, body, 4);
                    memcpy(&port_net_msg, body + 4, 2);

                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, control_fd, NULL) < 0)
                        ERR("epoll_ctl del control");
                    if (TEMP_FAILURE_RETRY(close(control_fd)) < 0)
                        ERR("close control");

                    new_control_fd = connect_to_ipv4(addr_net, port_net_msg);
                    control_fd = new_control_fd;
                    reset_reader(&reader);

                    event.events = EPOLLIN;
                    event.data.fd = control_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, control_fd, &event) < 0)
                        ERR("epoll_ctl add redirected control");
                    continue;
                }

                forward_message(return_fd, body_len, body);
            }
        }
    }

cleanup:
    if (return_fd >= 0 && TEMP_FAILURE_RETRY(close(return_fd)) < 0)
        ERR("close return");
    if (TEMP_FAILURE_RETRY(close(control_fd)) < 0)
        ERR("close control");
    if (TEMP_FAILURE_RETRY(close(epoll_fd)) < 0)
        ERR("close epoll");
}

int main(int argc, char **argv)
{
    uint16_t listen_port;
    int listen_fd;
    int control_fd;
    response_t response;
    uint16_t port_body;

    if (argc != 4 && argc != 5)
        usage(argv[0]);

    listen_port = (uint16_t)atoi(argv[1]);
    if (listen_port == 0)
        usage(argv[0]);

    parse_response(argc, argv, listen_port, &response);

    listen_fd = bind_tcp_socket(listen_port, BACKLOG);

    control_fd = connect_initial(argv[2], argv[3]);
    port_body = htons(listen_port);
    if (send_message(control_fd, 2, &port_body) < 0)
        ERR("write");

    do_witch(listen_fd, control_fd, &response);

    if (TEMP_FAILURE_RETRY(close(listen_fd)) < 0)
        ERR("close listen");
    return EXIT_SUCCESS;
}
