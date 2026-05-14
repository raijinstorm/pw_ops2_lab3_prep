# `epoll_ctl()`

## Purpose
Register, modify, or remove a file descriptor from an epoll instance.

## Prototype
```c
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
```

## Parameters
- `epfd`: epoll instance from `epoll_create1()`.
- `op`: `EPOLL_CTL_ADD`, `EPOLL_CTL_MOD`, or `EPOLL_CTL_DEL`.
- `fd`: watched file descriptor.
- `event`: event configuration for `ADD`/`MOD`.

## Typical Event Flags In These Labs
- `EPOLLIN`: readable.
- `EPOLLRDHUP`: peer closed its half of the connection.

## Example
```c
struct epoll_event ev = {};
ev.events = EPOLLIN;
ev.data.fd = listen_fd;
epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);
```

## Common Mistakes
- Forgetting to initialize `ev.data.fd`.
- Adding the same fd twice.
- Forgetting to delete fds you no longer want to track.

## Related
- `epoll_create1()`
- `epoll_pwait()`
- `epoll_event`
