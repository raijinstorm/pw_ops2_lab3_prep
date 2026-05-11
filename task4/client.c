#include "l7_common.h"

#define BUF_SIZE 5
#define CITY_COUNT 20

char cities[CITY_COUNT];

void print_cities() {
    for (int i = 0; i < CITY_COUNT; i++)
        printf("Citi %d belongs to %c\n" ,i, cities[i]);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < CITY_COUNT; i++)
        cities[i] = '?';

    int sock = connect_tcp_socket(argv[1], argv[2]);

    char *buf=NULL;
    size_t size=0;
    srand(getpid());

    printf("connected to server\n");

    while (1) {
        size_t read_c = getline(&buf,&size,stdin);
        printf("Size of buf read: %ld\n", read_c);
        if (read_c == 2) {
            if (buf[0] == 'e') {
                break;
            }
            if (buf[0] == 'o') {
                print_cities();
                continue;
            }
            printf("Invalid input\n");
            continue;
        }
        if (read_c == 5) {
            if (buf[0] != 't') {
                printf("Invalid input\n");
                continue;
            }
            int rand_c = rand() % 2;
            char buffer_to_send[BUF_SIZE-1];
            if (rand_c)
                buffer_to_send[0] = 'g';
            else
                buffer_to_send[0] = 'p';
            buffer_to_send[1] = buf[2];
            buffer_to_send[2] = buf[3];
            buffer_to_send[3] = '\n';

            int city_index = (buffer_to_send[1]-'0') * 10 + (buffer_to_send[2]-'0') - 1;
            cities[city_index] = buffer_to_send[0];

            if (TEMP_FAILURE_RETRY(bulk_write(sock,buf,BUF_SIZE-1)) < 0)
                ERR("bulk_write");
        }
        if (read_c == 6) {
            if (buf[0] != 'm') {
                printf("Invalid input\n");
                continue;
            }
            char buffer_to_send[BUF_SIZE-1];
            buffer_to_send[0] = buf[2];
            buffer_to_send[1] = buf[3];
            buffer_to_send[2] = buf[4];
            buffer_to_send[3] = '\n';

            int city_index = (buffer_to_send[1]-'0') * 10 + (buffer_to_send[2]-'0') - 1;
            cities[city_index] = buffer_to_send[0];

            if (TEMP_FAILURE_RETRY(bulk_write(sock,buf,BUF_SIZE-1)) < 0)
                ERR("bulk_write");
        }

    }
    
    

    if (TEMP_FAILURE_RETRY(close(sock)) < 0)
    ERR("close");

    return EXIT_SUCCESS;

}