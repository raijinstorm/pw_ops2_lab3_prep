#include "l7_common.h"

#define BUF_SIZE 16
#define BACKLOG 10

volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig)
{
    (void)sig;
    do_work = 0;
}

static int16_t digit_sum(const char *s)
{
    int16_t sum = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] >= '0' && s[i] <= '9')
            sum += s[i] - '0';
    }
    return sum;
}

static void handle_client(int cfd, int16_t *max_sum)
{
    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));

    ssize_t n = bulk_read(cfd, buf, sizeof(buf));
    if (n < 0) {
        perror("read");
        return;
    }
    if (n == 0) {
        fprintf(stderr, "Client disconnected before sending data\n");
        return;
    }

    buf[BUF_SIZE - 1] = '\0';
    printf("PID=%s\n", buf);

    int16_t sum = digit_sum(buf);
    if (sum > *max_sum)
        *max_sum = sum;

    int16_t net_sum = htons(sum);
    if (bulk_write(cfd, (char *)&net_sum, sizeof(net_sum)) < (ssize_t)sizeof(net_sum))
        perror("write");
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (sethandler(sigint_handler, SIGINT) < 0)
        ERR("sethandler SIGINT");
    if (sethandler(SIG_IGN, SIGPIPE) < 0)
        ERR("sethandler SIGPIPE");

    uint16_t port = (uint16_t)atoi(argv[1]);
    int sfd = bind_tcp_socket(port, BACKLOG);

    int16_t max_sum = -1;

    while (do_work) {
        int cfd = accept(sfd, NULL, NULL);
        if (cfd < 0) {
            if (errno == EINTR)
                break;
            ERR("accept");
        }

        handle_client(cfd, &max_sum);

        if (TEMP_FAILURE_RETRY(close(cfd)) < 0)
            perror("close client");
    }

    printf("HIGH SUM=%d\n", (int)max_sum);

    if (TEMP_FAILURE_RETRY(close(sfd)) < 0)
        ERR("close server");

    return EXIT_SUCCESS;
}
