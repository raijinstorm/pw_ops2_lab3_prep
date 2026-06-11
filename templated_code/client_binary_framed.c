// =============================================================================
// TEMPLATE: LENGTH-PREFIXED BINARY CLIENT
// -----------------------------------------------------------------------------
// Sends each command-line message as one framed packet [len][body], then reads
// the framed reply and prints it. Because the client is sequential (send, wait,
// read) it can use blocking bulk_read for the reply — read the 1-byte header,
// then read exactly that many body bytes.
//
// Test:  ./server_binary_framed 9000
//        ./client_binary_framed localhost 9000 hello "a longer chapter" hi
// =============================================================================
#include "common.h"

#define MAX_BODY 255

static int send_framed(int fd, const unsigned char *body, unsigned char len)
{
    unsigned char frame[1 + MAX_BODY];
    frame[0] = len;
    if (len)
        memcpy(frame + 1, body, len);
    return bulk_write(fd, (char *)frame, (size_t)1 + len) < 0 ? -1 : 0;
}

// Read one framed message into body (capacity MAX_BODY). Returns body length,
// or -1 on EOF/error.
static int read_framed(int fd, unsigned char *body)
{
    unsigned char len;
    ssize_t n = bulk_read(fd, (char *)&len, 1);
    if (n != 1)
        return -1;
    if (len == 0)
        return 0;
    n = bulk_read(fd, (char *)body, len);
    if (n != (ssize_t)len)
        return -1;
    return len;
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s host port msg [msg ...]\n", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc < 4)
        usage(argv[0]);
    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("sethandler SIGPIPE");

    int sock = connect_tcp_socket(argv[1], argv[2]);

    for (int i = 3; i < argc; i++)
    {
        size_t len = strlen(argv[i]);
        if (len > MAX_BODY)
            len = MAX_BODY;  // a single frame's body maxes out at 255 bytes
        if (send_framed(sock, (const unsigned char *)argv[i], (unsigned char)len) < 0)
            ERR("send_framed");

        unsigned char body[MAX_BODY];
        int rlen = read_framed(sock, body);
        if (rlen < 0)
        {
            fprintf(stderr, "server closed the connection\n");
            break;
        }
        printf("reply (len=%d): %.*s\n", rlen, rlen, body);
    }

    if (TEMP_FAILURE_RETRY(close(sock)) < 0)
        ERR("close");
    return EXIT_SUCCESS;
}
