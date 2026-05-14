# `connect()`

## Purpose
Initiate an outgoing connection to a remote socket.

## Prototype
```c
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

## Parameters
- `sockfd`: socket created by `socket()`.
- `addr`: remote address.
- `addrlen`: size of that address.

## Returns
- `0` on success for blocking sockets.
- `-1` on error.

## Used In These Labs
- `task1/client.c`
- `website_lab` clients
- `last_year_lab/sop-crone.c` for mother connection
- `last_year_lab/sop-witch.c` for initial and redirected control connections

## Example
```c
int fd = socket(PF_INET, SOCK_STREAM, 0);
connect(fd, (struct sockaddr *)&addr, sizeof(addr));
```

## Common Mistakes
- Wrong byte order in `sin_port`.
- Confusing local bind address with remote connect address.
- Using an unresolved hostname without `getaddrinfo()`.

## Related
- `socket()`
- `getaddrinfo()`
- `sockaddr_in`
