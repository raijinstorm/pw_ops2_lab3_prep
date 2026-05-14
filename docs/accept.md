# `accept()`

## Purpose
Take one pending connection from a listening socket and return a new connected file descriptor.

## Prototype
```c
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

## Parameters
- `sockfd`: listening socket created by `socket()`, `bind()`, `listen()`.
- `addr`: optional output buffer for the peer address. Use `NULL` if you do not care.
- `addrlen`: input/output length of `addr`.

## Returns
- Connected socket fd on success.
- `-1` on error, with `errno` set.

## Used In These Labs
- `task1/server.c`: simple blocking server.
- `task3`, `website_lab`, `last_year_lab`: usually wrapped by `add_new_client()`.
- `last_year_lab/sop-crone.c`: direct `accept()` is needed to capture peer `sockaddr_in`.

## Common Pattern
```c
struct sockaddr_in peer;
socklen_t len = sizeof(peer);
int cfd = accept(listen_fd, (struct sockaddr *)&peer, &len);
```

## Common Mistakes
- Calling it on a socket that was not put into listening mode.
- Forgetting that the returned fd is different from the listening fd.
- Treating `EAGAIN` on a nonblocking socket as fatal.
- Forgetting to close rejected or disconnected clients.

## Related
- `listen()`
- `bind()`
- `socket()`
- `connect()`
