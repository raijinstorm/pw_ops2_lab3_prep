#include "l7_common.h"

#define BUF_SIZE 16

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pid_t pid = getpid();
    printf("PID=%d\n", pid);

    int sock = connect_tcp_socket(argv[1], argv[2]);

    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%d", pid);

    if (bulk_write(sock, buf, sizeof(buf)) < (ssize_t)sizeof(buf))
        ERR("write");

    int16_t net_sum;
    ssize_t n = bulk_read(sock, (char *)&net_sum, sizeof(net_sum));
    if (n < (ssize_t)sizeof(net_sum)) {
        if (n < 0)
            ERR("read");
        fprintf(stderr, "Server closed connection unexpectedly\n");
        exit(EXIT_FAILURE);
    }

    int16_t sum = ntohs(net_sum);
    printf("SUM=%d\n", (int)sum);

    if (TEMP_FAILURE_RETRY(close(sock)) < 0)
        ERR("close");

    return EXIT_SUCCESS;
}
