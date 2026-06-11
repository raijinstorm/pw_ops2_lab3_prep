#include "l7-common.h"

void usage(char *name)
{
    printf("%s <connect_port> <bind_port>\n", name);
    printf("  connect_port - port to receive messages from\n");
    printf("  bind_port - port to accept subscribers\n");
    exit(EXIT_FAILURE);
}

#define SWAP(a, b)                      \
    ({                                  \
        char c[sizeof(*(a))];           \
        memcpy(c, (a), sizeof(*(a)));   \
        memcpy((a), (b), sizeof(*(a))); \
        memcpy((b), (c), sizeof(*(a))); \
    })

#define MAX_CLIENTS 10
#define READ_BUFF_SIZE 64
#define GREP_LEN 5

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
