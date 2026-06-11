// =============================================================================
// TEMPLATE: TEXT CLIENT  (line-oriented, epoll on stdin + socket)
// -----------------------------------------------------------------------------
// Reads lines from stdin and sends them (with '\n') to the server; prints
// whatever the server sends back. Both stdin and the socket sit in one epoll so
// the client stays responsive in both directions — i.e. a tiny netcat.
//
// In practice the labs often just use `nc localhost PORT` as the text client;
// this template is here for when you need custom client behavior.
//
// Test:  ./server_text 9000   then   ./client_text localhost 9000
// =============================================================================
#include "common.h"

#define MAX_EVENTS 8
#define BUF_SIZE 512

volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig)
{
    (void)sig;
    do_work = 0;
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s host port\n", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc != 3)
        usage(argv[0]);
    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("sethandler SIGPIPE");
    if (sethandler(sigint_handler, SIGINT))
        ERR("sethandler SIGINT");

    int sock = connect_tcp_socket(argv[1], argv[2]);

    int epfd;
    if ((epfd = epoll_create1(0)) < 0)
        ERR("epoll_create1");
    epoll_add(epfd, sock, EPOLLIN);
    // epoll can't watch an un-pollable stdin (regular-file redirect) -> EPERM;
    // tolerate it and run receive-only in that case.
    {
        struct epoll_event ev = {.events = EPOLLIN, .data.fd = STDIN_FILENO};
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1 && errno != EPERM)
            ERR("epoll_ctl ADD stdin");
    }

    struct epoll_event events[MAX_EVENTS];
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    char buf[BUF_SIZE];
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
            if (events[i].data.fd == STDIN_FILENO)
            {
                ssize_t n = TEMP_FAILURE_RETRY(read(STDIN_FILENO, buf, sizeof(buf)));
                if (n <= 0)  // EOF on stdin -> we're done sending
                {
                    do_work = 0;
                    break;
                }
                // ---- TODO: shape the outgoing line(s) here if needed ----
                if (bulk_write(sock, buf, n) < 0)
                {
                    if (errno == EPIPE)
                    {
                        printf("server closed the connection\n");
                        do_work = 0;
                    }
                    else
                        ERR("bulk_write");
                }
            }
            else  // data from the server
            {
                ssize_t n = TEMP_FAILURE_RETRY(read(sock, buf, sizeof(buf)));
                if (n <= 0)
                {
                    printf("server disconnected\n");
                    do_work = 0;
                    break;
                }
                // ---- TODO: parse the server's reply; demo just echoes it ----
                if (bulk_write(STDOUT_FILENO, buf, n) < 0)
                    ERR("bulk_write stdout");
            }
        }
    }

    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    if (TEMP_FAILURE_RETRY(close(epfd)) < 0)
        ERR("close");
    if (TEMP_FAILURE_RETRY(close(sock)) < 0)
        ERR("close");
    return EXIT_SUCCESS;
}
