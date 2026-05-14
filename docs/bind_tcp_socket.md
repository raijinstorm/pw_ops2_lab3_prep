# `bind_tcp_socket()`

## Purpose
Lab helper that creates, configures, binds, and listens on a TCP socket.

## Definition Shape
```c
int bind_tcp_socket(uint16_t port, int backlog_size);
```

## What It Does
- Calls `socket(PF_INET, SOCK_STREAM, 0)`.
- Enables `SO_REUSEADDR`.
- Binds to `INADDR_ANY` on the given port.
- Calls `listen()`.

## Used In These Labs
- `task3/server.c`
- `website_lab/l7-1_server.c`
- `last_year_lab/sop-crone.c`
- `last_year_lab/sop-witch.c`

## Example
```c
int listen_fd = bind_tcp_socket(8080, 10);
```

## Common Mistakes
- Passing a host-order port into a raw `sockaddr_in`; this helper avoids that by using `htons()`.
- Forgetting to switch the listening socket to nonblocking mode when using `epoll`.

## Related
- `setsockopt()`
- `bind()`
- `listen()`
- `fcntl()`
