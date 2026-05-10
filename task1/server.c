#include "l7_common.h" 

int max_sum = 0;

void sigint_handler(int sig) {
    printf("HGIH SUM=%d\n", max_sum);
    exit(EXIT_SUCCESS);
}

int sum_digits(int n) {
    int sum = 0;
    while (n > 0) {
        sum += n % 10;
        n /= 10;
    }
    return sum;
}

int main(int argc, char **argv) {

    if (sethandler(sigint_handler, SIGINT))
        ERR("sethandler");

    int port = atoi(argv[1]);
    int server_fd, client_fd;

    struct sockaddr_in server_addr;

    //create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
        ERR("socket");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    //allow reuse of address (after quick restart of the server)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        ERR("setsockopt");

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        ERR("bind"); 

    if (listen(server_fd, 10) < 0)
        ERR("listen");

    printf("[DEBUG] Server waiting on port %d...\n", port);

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0)
            ERR("accept");

        int client_pid;
        ssize_t count = bulk_read(client_fd, (char *)&client_pid, sizeof(client_pid));
        if (count <= 0) {
            // Connection closed or error
            if (TEMP_FAILURE_RETRY(close(client_fd)) < 0) ERR("close");
            continue;
        }
        printf("[DEBUG] received client PID: %d\n", client_pid);

        int result = sum_digits(client_pid);

        if (result > max_sum)
            max_sum = result;

        if (bulk_write(client_fd, (char *)&result, sizeof(result)) < 0) {
            perror("write to client");
            if (TEMP_FAILURE_RETRY(close(client_fd)) < 0) ERR("close");
            continue;
        }
        printf("[DEBUG] sent result to client: %d\n", result);

        if (TEMP_FAILURE_RETRY(close(client_fd)) < 0)
            ERR("close");
    }
    if (TEMP_FAILURE_RETRY(close(server_fd)) < 0)
        ERR("close");

    return EXIT_SUCCESS;
}