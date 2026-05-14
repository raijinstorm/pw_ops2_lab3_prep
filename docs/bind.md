# `bind()`

## Purpose
Attach a socket to a local address.

## Prototype
```c
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

## Parameters
- `sockfd`: socket from `socket()`.
- `addr`: local address.
- `addrlen`: size of that address structure.

## Returns
- `0` on success.
- `-1` on error.

## Used In These Labs
- TCP servers bind `sockaddr_in`.
- UNIX-domain servers bind `sockaddr_un`.
- Wrapped by `bind_tcp_socket()` and `bind_local_socket()`.

## Common Pattern
```c
struct sockaddr_in addr = {};
addr.sin_family = AF_INET;
addr.sin_port = htons(port);
addr.sin_addr.s_addr = htonl(INADDR_ANY);
bind(fd, (struct sockaddr *)&addr, sizeof(addr));
```

## Common Mistakes
- Using host byte order for `sin_port`.
- Forgetting `setsockopt(... SO_REUSEADDR ...)` for restart-friendly TCP servers.
- Rebinding a UNIX socket path without removing the old file first.

## Related
- `socket()`
- `listen()`
- `sockaddr_in`
- `sockaddr_un`
