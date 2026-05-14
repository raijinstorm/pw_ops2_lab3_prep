-#include "l7-common.h"
      2 -
      3 -#define BACKLOG 8
      4 -#define MAX_EVENTS 8
      5 -#define MAX_MSG_SIZE 256
      6 -
      7 -#define MSG_PENDING 0
      8 -#define MSG_COMPLETE 1
      9 -#define MSG_INVALID -1
     10 -#define MSG_CLOSED -2
     11 -
     12 -typedef struct {
     13 -    unsigned char header;
     14 -    unsigned char body[255];
     15 -    size_t received;
     16 -    int reading_header;
     17 -} message_t;
     18 -
     19 -static volatile sig_atomic_t do_work = 1;
     20 -
     21 -static void sigint_handler(int sig)
     22 -{
     23 -    (void)sig;
     24 -    do_work = 0;
     25 -}
     26 -
     27 -static void usage(char *name)
     28 -{
     29 -    fprintf(stderr, "Usage: %s <port>\n", name);
     30 -    exit(EXIT_FAILURE);
     31 -}
     32 -
     33 -static void resetMessage(message_t *msg)
     34 -{
     35 -    msg->header = 0;
     36 -    msg->received = 0;
     37 -    msg->reading_header = 1;
     38 -}
     39 -
     40 -static void closeConnection(int fd, int epoll_fd)
     41 -{
     42 -    if (fd < 0)
     43 -        return;
     44 -    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0)
     45 -        perror("epoll_ctl DEL");
     46 -    if (TEMP_FAILURE_RETRY(close(fd)) < 0)
     47 -        ERR("close");
     48 -}
     49 -
     50 -static int connectMother(struct sockaddr_in maiden_addr, uint16_t port)
     51 -{
     52 -    int mother_fd = make_tcp_socket();
     53 -    maiden_addr.sin_port = port;
     54 -    if (connect(mother_fd, (struct sockaddr *)&maiden_addr, sizeof(maiden_addr)) < 0)
     55 -        ERR("connect");
     56 -    return mother_fd;
     57 -}
     58 -
     59 -static int handleNewConnection(int listen_fd, int *maiden_fd, struct sockaddr_in *maiden_addr)
     60 -{
     61 -    int new_fd;
     62 -    socklen_t addr_len = sizeof(*maiden_addr);
     63 -
     64 -    if ((new_fd = TEMP_FAILURE_RETRY(accept(listen_fd, (struct sockaddr *)maiden_addr, &addr_len))) < 0)
     65 -    {
     66 -        if (EAGAIN == errno || EWOULDBLOCK == errno)
     67 -            return 0;
     68 -        ERR("accept");
     69 -    }
     70 -
     71 -    if (*maiden_fd == -1) {
     72 -        *maiden_fd = new_fd;
     73 -        return 1;
     74 -    }
     75 -
     76 -    if (TEMP_FAILURE_RETRY(close(new_fd)) < 0)
     77 -        ERR("close");
     78 -    return 0;
     79 -}
     80 -
     81 -static int handleCandidateConnection(int listen_fd, int *candidate_fd, struct sockaddr_in *candidate_addr)
     82 -{
     83 -    int new_fd;
     84 -    socklen_t addr_len = sizeof(*candidate_addr);
     85 -
     86 -    if ((new_fd = TEMP_FAILURE_RETRY(accept(listen_fd, (struct sockaddr *)candidate_addr, &addr_len))) < 0)
     87 -    {
     88 -        if (EAGAIN == errno || EWOULDBLOCK == errno)
     89 -            return 0;
     90 -        ERR("accept");
     91 -    }
     92 -
     93 -    if (*candidate_fd == -1) {
     94 -        *candidate_fd = new_fd;
     95 -        return 1;
     96 -    }
     97 -
     98 -    if (TEMP_FAILURE_RETRY(close(new_fd)) < 0)
     99 -        ERR("close");
    100 -    return 0;
    101 -}
    102 -
    103 -static int handleFirstMessage(int maiden_fd, message_t *msg, uint16_t *mother_port)
    104 -{
    105 -    ssize_t n;
    106 -
    107 -    if (msg->reading_header)
    108 -        n = TEMP_FAILURE_RETRY(read(maiden_fd, &msg->header, 1));
    109 -    else
    110 -        n = TEMP_FAILURE_RETRY(read(maiden_fd, msg->body + msg->received, (size_t)(msg->header) - msg->received));
    111 -
    112 -    if (n < 0)
    113 -        ERR("read");
    114 -
    115 -    if (n == 0) {
    116 -        return MSG_CLOSED;
    117 -    }
    118 -
    119 -    if (msg->reading_header) {
    120 -        msg->reading_header = 0;
    121 -        msg->received = 0;
    122 -        if (msg->header == 0)
    123 -            return MSG_INVALID;
    124 -        return MSG_PENDING;
    125 -    }
    126 -
    127 -    msg->received += (size_t)n;
    128 -    if (msg->received == msg->header) {
    129 -        if (msg->header != 2)
    130 -            return MSG_INVALID;
    131 -        memcpy(mother_port, msg->body, sizeof(uint16_t));
    132 -        return MSG_COMPLETE;
    133 -    }
    134 -    return MSG_PENDING;
    135 -}
    136 -
    137 -static void sendStartMessage(int maiden_fd)
    138 -{
    139 -    unsigned char msg[5] = {4, 0, 0, 0, 0};
    140 -    if (bulk_write(maiden_fd, (char *)msg, sizeof(msg)) < (ssize_t)sizeof(msg))
    141 -        ERR("write");
    142 -}
    143 -
    144 -static void sendCandidateMessage(int maiden_fd, struct sockaddr_in candidate_addr, uint16_t port)
    145 -{
    146 -    unsigned char msg[7];
    147 -
    148 -    msg[0] = 6;
    149 -    memcpy(msg + 1, &candidate_addr.sin_addr.s_addr, sizeof(candidate_addr.sin_addr.s_addr));
    150 -    memcpy(msg + 5, &port, sizeof(port));
    151 -
    152 -    if (bulk_write(maiden_fd, (char *)msg, sizeof(msg)) < (ssize_t)sizeof(msg))
    153 -        ERR("write");
    154 -}
    155 -
    156 -static int handleMother(int mother_fd, message_t *msg)
    157 -{
    158 -    ssize_t n;
    159 -
    160 -    if (msg->reading_header)
    161 -        n = TEMP_FAILURE_RETRY(read(mother_fd, &msg->header, 1));
    162 -    else
    163 -        n = TEMP_FAILURE_RETRY(read(mother_fd, msg->body + msg->received, (size_t)(msg->header) - msg->received));
    164 -
    165 -    if (n < 0)
    166 -        ERR("read");
    167 -
    168 -    if (n == 0) {
    169 -        printf("The mother witch left the coven, we are hopeless\n");
    170 -        return 1;
    171 -    }
    172 -
    173 -    if (msg->reading_header) {
    174 -        msg->reading_header = 0;
    175 -        msg->received = 0;
    176 -        if (msg->header == 0) {
    177 -            resetMessage(msg);
    178 -            return 0;
    179 -        }
    180 -        return 0;
    181 -    }
    182 -
    183 -    msg->received += (size_t)n;
    184 -    if (msg->received == msg->header) {
    185 -        if (msg->header == 4) {
    186 -            int32_t value;
    187 -            memcpy(&value, msg->body, sizeof(int32_t));
    188 -            printf("%d\n", ntohl(value));
    189 -        }
    190 -        else if (msg->header > 6) {
    191 -            printf("%.*s\n", msg->header, msg->body);
    192 -        }
    193 -        resetMessage(msg);
    194 -    }
    195 -
    196 -    return 0;
    197 -}
    198 -
    199 -static int handleMaidenAfterStart(int maiden_fd)
    200 -{
    201 -    unsigned char c;
    202 -    ssize_t n = TEMP_FAILURE_RETRY(recv(maiden_fd, &c, sizeof(c), MSG_PEEK));
    203 -
    204 -    if (n < 0)
    205 -        ERR("recv");
    206 -    if (n == 0) {
    207 -        printf("The maiden witch left the coven, we are hopeless\n");
    208 -        return 1;
    209 -    }
    210 -    return 0;
    211 -}
    212 -
    213 -void doServer(int listen_fd)
    214 -{
    215 -    int epoll_fd;
    216 -    struct epoll_event event, events[MAX_EVENTS];
    217 -    int maiden_fd = -1;
    218 -    int mother_fd = -1;
    219 -    int candidate_fd = -1;
    220 -    struct sockaddr_in maiden_addr;
    221 -    struct sockaddr_in candidate_addr;
    222 -    message_t maiden_msg;
    223 -    message_t mother_msg;
    224 -    message_t candidate_msg;
    225 -    uint16_t mother_port = 0;
    226 -    int ritual_started = 0;
    227 -
    228 -    if ((epoll_fd = epoll_create1(0)) < 0)
    229 -        ERR("epoll_create1");
    230 -
    231 -    memset(&event, 0, sizeof(event));
    232 -    event.events = EPOLLIN;
    233 -    event.data.fd = listen_fd;
    234 -    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) < 0)
    235 -        ERR("epoll_ctl listen");
    236 -
    237 -    resetMessage(&maiden_msg);
    238 -    resetMessage(&mother_msg);
    239 -    resetMessage(&candidate_msg);
    240 -
    241 -    sigset_t mask, oldmask;
    242 -    sigemptyset(&mask);
    243 -    sigaddset(&mask, SIGINT);
    244 -    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    245 -
    246 -    while (do_work) {
    247 -        int nfds = epoll_pwait(epoll_fd, events, MAX_EVENTS, -1, &oldmask);
    248 -        if (nfds < 0) {
    249 -            if (errno == EINTR)
    250 -                continue;
    251 -            ERR("epoll_pwait");
    252 -        }
    253 -
    254 -        for (int i = 0; i < nfds; i++) {
    255 -            if (events[i].data.fd == listen_fd) {
    256 -                if (!ritual_started) {
    257 -                    if (handleNewConnection(listen_fd, &maiden_fd, &maiden_addr)) {
    258 -                        memset(&event, 0, sizeof(event));
    259 -                        event.events = EPOLLIN | EPOLLRDHUP;
    260 -                        event.data.fd = maiden_fd;
    261 -                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, maiden_fd, &event) < 0)
    262 -                            ERR("epoll_ctl client");
    263 -                    }
    264 -                }
    265 -                else {
    266 -                    if (handleCandidateConnection(listen_fd, &candidate_fd, &candidate_addr)) {
    267 -                        resetMessage(&candidate_msg);
    268 -                        memset(&event, 0, sizeof(event));
    269 -                        event.events = EPOLLIN | EPOLLRDHUP;
    270 -                        event.data.fd = candidate_fd;
    271 -                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, candidate_fd, &event) < 0)
    272 -                            ERR("epoll_ctl candidate");
    273 -                    }
    274 -                }
    275 -                continue;
    276 -            }
    277 -
    278 -            if (!ritual_started && maiden_fd >= 0 && events[i].data.fd == maiden_fd) {
    279 -                int status = handleFirstMessage(maiden_fd, &maiden_msg, &mother_port);
    280 -                if (status == MSG_INVALID)
    281 -                    do_work = 0;
    282 -                else if (status == MSG_CLOSED) {
    283 -                    printf("No!  The ritual...\n");
    284 -                    do_work = 0;
    285 -                }
    286 -                else if (status == MSG_COMPLETE) {
    287 -                    mother_fd = connectMother(maiden_addr, mother_port);
    288 -                    memset(&event, 0, sizeof(event));
    289 -                    event.events = EPOLLIN | EPOLLRDHUP;
    290 -                    event.data.fd = mother_fd;
    291 -                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, mother_fd, &event) < 0)
    292 -                        ERR("epoll_ctl mother");
    293 -                    sendStartMessage(maiden_fd);
    294 -                    ritual_started = 1;
    295 -                }
    296 -            }
    297 -            else if (ritual_started && maiden_fd >= 0 && events[i].data.fd == maiden_fd) {
    298 -                if (handleMaidenAfterStart(maiden_fd))
    299 -                    do_work = 0;
    300 -            }
    301 -            else if (ritual_started && mother_fd >= 0 && events[i].data.fd == mother_fd) {
    302 -                if (handleMother(mother_fd, &mother_msg))
    303 -                    do_work = 0;
    304 -            }
    305 -            else if (ritual_started && candidate_fd >= 0 && events[i].data.fd == candidate_fd) {
    306 -                int status = handleFirstMessage(candidate_fd, &candidate_msg, &mother_port);
    307 -                if (status == MSG_COMPLETE) {
    308 -                    sendCandidateMessage(maiden_fd, candidate_addr, mother_port);
    309 -                    closeConnection(maiden_fd, epoll_fd);
    310 -                    maiden_fd = candidate_fd;
    311 -                    maiden_addr = candidate_addr;
    312 -                    candidate_fd = -1;
    313 -                    resetMessage(&candidate_msg);
    314 -                    resetMessage(&maiden_msg);
    315 -                    sendStartMessage(maiden_fd);
    316 -                }
    317 -                else if (status != MSG_PENDING) {
    318 -                    printf("Another young one lost to the shadows\n");
    319 -                    closeConnection(candidate_fd, epoll_fd);
    320 -                    candidate_fd = -1;
    321 -                    resetMessage(&candidate_msg);
    322 -                }
    323 -            }
    324 -        }
    325 -    }
    326 -
    327 -    if (maiden_fd >= 0) {
    328 -        closeConnection(maiden_fd, epoll_fd);
    329 -    }
    330 -    if (candidate_fd >= 0) {
    331 -        closeConnection(candidate_fd, epoll_fd);
    332 -    }
    333 -    if (mother_fd >= 0) {
    334 -        closeConnection(mother_fd, epoll_fd);
    335 -    }
    336 -    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    337 -    if (TEMP_FAILURE_RETRY(close(epoll_fd)) < 0)
    338 -        ERR("close");
    339 -}
    340 -
    341 -int main(int argc, char **argv)
    342 -{
    343 -    uint16_t port;
    344 -    int listen_fd;
    345 -
    346 -    if (argc != 2)
    347 -        usage(argv[0]);
    348 -
    349 -    if (sethandler(SIG_IGN, SIGPIPE))
    350 -        ERR("sethandler SIGPIPE");
    351 -    if (sethandler(sigint_handler, SIGINT))
    352 -        ERR("sethandler SIGINT");
    353 -
    354 -    port = (uint16_t)atoi(argv[1]);
    355 -    if (port == 0)
    356 -        usage(argv[0]);
    357 -
    358 -    listen_fd = bind_tcp_socket(port, BACKLOG);
    359 -    int flags = fcntl(listen_fd, F_GETFL) | O_NONBLOCK;
    360 -    fcntl(listen_fd, F_SETFL, flags);
    361 -
    362 -    doServer(listen_fd);
    363 -
    364 -    if (TEMP_FAILURE_RETRY(close(listen_fd)) < 0)
    365 -        ERR("close");
    366 -
    367 -    return EXIT_SUCCESS;
    368 -}