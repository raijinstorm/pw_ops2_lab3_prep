#include "l7_common.h"

#define BUF_SIZE 5
#define CITY_COUNT 20
#define MAX_EVENTS 16

char cities[CITY_COUNT];

volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig) { (void)sig; do_work = 0; }

void print_cities(void) {
    for (int i = 0; i < CITY_COUNT; i++)
        printf("City %d belongs to %c\n", i + 1, cities[i]);
}

void handle_server(int tcp_socket) {
    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));

    ssize_t n = bulk_read(tcp_socket, buf, BUF_SIZE - 1);
    if (n != BUF_SIZE - 1) {
        do_work = 0;
        return;
    }

    if ((buf[0] != 'g' && buf[0] != 'p') || buf[3] != '\n')
        return;

    int city_index = (buf[1] - '0') * 10 + (buf[2] - '0') - 1;
    if (city_index < 0 || city_index >= CITY_COUNT)
        return;

    cities[city_index] = buf[0];
}

void handle_stdin(int tcp_socket) {
    char *buf = NULL;
    size_t size = 0;

    ssize_t read_c = getline(&buf, &size, stdin);
    if (read_c < 0) {
        free(buf);
        do_work = 0;
        return;
    }

    if (read_c == 2 && buf[0] == 'e') {
        free(buf);
        do_work = 0;
        return;
    }

    if (read_c == 2 && buf[0] == 'o') {
        print_cities();
        free(buf);
        return;
    }

    if (read_c == 5 && buf[0] == 't' && buf[1] == ' ') {
        int city_num = (buf[2] - '0') * 10 + (buf[3] - '0');
        if (city_num < 1 || city_num > CITY_COUNT) {
            printf("City number out of range [1, 20]\n");
            free(buf);
            return;
        }
        char msg[BUF_SIZE - 1];
        msg[0] = (rand() % 2) ? 'g' : 'p';
        msg[1] = buf[2];
        msg[2] = buf[3];
        msg[3] = '\n';
        cities[city_num - 1] = msg[0];
        if (bulk_write(tcp_socket, msg, BUF_SIZE - 1) < 0)
            ERR("bulk_write");
        free(buf);
        return;
    }

    if (read_c == 6 && buf[0] == 'm' && buf[1] == ' ') {
        char msg[BUF_SIZE - 1];
        msg[0] = buf[2];
        msg[1] = buf[3];
        msg[2] = buf[4];
        msg[3] = '\n';
        if (bulk_write(tcp_socket, msg, BUF_SIZE - 1) < 0)
            ERR("bulk_write");
        free(buf);
        return;
    }

    printf("Invalid command\n");
    free(buf);
}

void doClient(int tcp_socket) {
    int epoll_descriptor;
    if ((epoll_descriptor = epoll_create1(0)) < 0)
        ERR("epoll_create");

    struct epoll_event event, events[MAX_EVENTS];

    event.events = EPOLLIN;
    event.data.fd = tcp_socket;
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, tcp_socket, &event) == -1)
        ERR("epoll_ctl");

    event.events = EPOLLIN;
    event.data.fd = STDIN_FILENO;
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, STDIN_FILENO, &event) == -1)
        ERR("epoll_ctl");

    int nfds;
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    while (do_work) {
        if ((nfds = epoll_pwait(epoll_descriptor, events, MAX_EVENTS, -1, &oldmask)) > 0) {
            for (int n = 0; n < nfds; n++) {
                if (events[n].data.fd == tcp_socket)
                    handle_server(tcp_socket);
                else
                    handle_stdin(tcp_socket);
            }
        } else {
            if (errno == EINTR)
                continue;
            ERR("epoll_pwait");
        }
    }

    if (TEMP_FAILURE_RETRY(close(epoll_descriptor)) < 0)
        ERR("close");
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    print_cities();
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("Setting SIGPIPE");
    if (sethandler(sigint_handler, SIGINT))
        ERR("Setting SIGINT");

    srand(getpid());

    for (int i = 0; i < CITY_COUNT; i++)
        cities[i] = '?';

    int sock = connect_tcp_socket(argv[1], argv[2]);
    doClient(sock);

    if (TEMP_FAILURE_RETRY(close(sock)) < 0)
        ERR("close");

    return EXIT_SUCCESS;
}
