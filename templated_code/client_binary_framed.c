// =============================================================================
// TEMPLATE: LINE-BASED TEXT CLIENT
// -----------------------------------------------------------------------------
// Sends each command-line message as:
//
//      message\n
//
// Then reads one newline-terminated reply and prints it.
//
// Protocol examples:
//      HELLO\n
//      LAUNCH 123\n
//      MSG hello world\n
//
// Unlike length-prefixed binary messages, here the message ends when '\n'
// is received.
//
// The client is sequential:
//      send one line
//      wait for one reply line
//      print reply
//
// Test:
//      ./server_line 9000
//      ./client_line localhost 9000 HELLO "LAUNCH 123" "MSG hello world"
// =============================================================================

#include "common.h"

#define MAX_LINE 255

static int send_line(int fd, const char *line)
{
    char out[MAX_LINE + 2];

    int n = snprintf(out, sizeof(out), "%s\n", line);

    if (n < 0)
        return -1;

    return bulk_write(fd, out, (size_t)n) < 0 ? -1 : 0;
}

// Read one newline-terminated line.
// Returns line length, or -1 on EOF/error.
// The returned line does NOT include '\n'.
static int read_line(int fd, char *line)
{
    size_t len = 0;

    while (len < MAX_LINE)
    {
        char ch;

        ssize_t n = TEMP_FAILURE_RETRY(read(fd, &ch, 1));

        if (n < 0)
            return -1;

        if (n == 0)
            return -1;

        if (ch == '\n')
        {
            line[len] = '\0';
            return (int)len;
        }

        line[len++] = ch;
    }

    // line too long
    line[MAX_LINE] = '\0';
    return -1;
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
        char msg[MAX_LINE + 1];

        snprintf(msg, sizeof(msg), "%s", argv[i]);

        if (send_line(sock, msg) < 0)
            ERR("send_line");

        char reply[MAX_LINE + 1];

        int rlen = read_line(sock, reply);

        if (rlen < 0)
        {
            fprintf(stderr, "server closed the connection or sent invalid line\n");
            break;
        }

        printf("reply (len=%d): %s\n", rlen, reply);
    }

    if (TEMP_FAILURE_RETRY(close(sock)) < 0)
        ERR("close");

    return EXIT_SUCCESS;
}