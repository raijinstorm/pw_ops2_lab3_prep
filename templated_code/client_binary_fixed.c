// =============================================================================
// TEMPLATE: FIXED-SIZE BINARY CLIENT  (request/response, htonl/ntohl)
// -----------------------------------------------------------------------------
// Connect, send ONE fixed-size request struct (network byte order), read the
// fixed-size reply, print it, close. The canonical connection-per-request
// client (the calculator client).
//
// Test:  ./server_binary_fixed 9000
//        ./client_binary_fixed localhost 9000 6 7 '*'   ->  6 * 7 = 42
// =============================================================================
#include "common.h"

typedef struct
{
    uint32_t op1;
    uint32_t op2;
    uint32_t result;
    uint32_t op;
    uint32_t status;
}
msg_t;

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s host port op1 op2 operator\n", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc != 6)
        usage(argv[0]);
    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("sethandler SIGPIPE");

    int sock = connect_tcp_socket(argv[1], argv[2]);

    // ---- build the request in network byte order ----
    msg_t m;
    m.op1 = htonl((uint32_t)atoi(argv[3]));
    m.op2 = htonl((uint32_t)atoi(argv[4]));
    m.result = htonl(0);
    m.op = htonl((uint32_t)argv[5][0]);
    m.status = htonl(1);

    if (bulk_write(sock, (char *)&m, sizeof(m)) < 0)
        ERR("bulk_write");

    ssize_t n = bulk_read(sock, (char *)&m, sizeof(m));
    if (n != (ssize_t)sizeof(m))
    {
        fprintf(stderr, "server closed before sending a full reply\n");
        TEMP_FAILURE_RETRY(close(sock));
        return EXIT_FAILURE;
    }

    // ---- decode the reply (network -> host byte order) ----
    if (ntohl(m.status))
        printf("%d %c %d = %d\n", (int32_t)ntohl(m.op1), (char)ntohl(m.op),
               (int32_t)ntohl(m.op2), (int32_t)ntohl(m.result));
    else
        printf("operation impossible\n");

    if (TEMP_FAILURE_RETRY(close(sock)) < 0)
        ERR("close");
    return EXIT_SUCCESS;
}
