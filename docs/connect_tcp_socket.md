# `connect_tcp_socket()`

## Purpose
Lab helper that resolves an address and connects a TCP socket.

## Definition Shape
```c
int connect_tcp_socket(char *name, char *port);
```

## What It Does
- Creates a TCP socket.
- Resolves `name:port` through `make_address()` / `getaddrinfo()`.
- Calls `connect()`.

## Used In These Labs
- `task4/client.c`
- `website_lab/l7-1_client_tcp.c`
- `last_year_lab/sop-witch.c`

## Example
```c
int fd = connect_tcp_socket("localhost", "8080");
```

## Related
- `make_address()`
- `getaddrinfo()`
- `connect()`
