#include "l7_common.h"

int main(int argc, char **argv) {

    //argv = ip, port. For examples: "localhost 8080"
    char *ip = argv[1];
    int port = atoi(argv[2]);

    int client_fd;

    struct sockaddr_in server_addr;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (client_fd < 0)
        ERR("socket");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        ERR("connect");

    int client_pid = getpid();
    
    printf("PID=%d\n", client_pid);
    printf("[DEBUG] Sending PID to server...\n");
    if (bulk_write(client_fd, (char *)&client_pid, sizeof(client_pid)) < 0) {
        perror("write to server");
        if (TEMP_FAILURE_RETRY(close(client_fd)) < 0) ERR("close");
        return EXIT_FAILURE;
    }

    int result;

    ssize_t count = bulk_read(client_fd, (char *)&result, sizeof(result));
    if (count <= 0) {
        printf("Server disconnected before sending result\n");
        if (TEMP_FAILURE_RETRY(close(client_fd)) < 0) ERR("close");
        return EXIT_FAILURE;
    }

    printf("SUM=%d\n", result);


    if (TEMP_FAILURE_RETRY(close(client_fd)) < 0)
        ERR("close");

    return EXIT_SUCCESS;
}