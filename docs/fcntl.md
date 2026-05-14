# `fcntl()`

## Purpose
Control fd flags and behavior.

## Prototype
```c
int fcntl(int fd, int cmd, ...);
```

## Common Commands In These Labs
- `F_GETFL`: read current file status flags.
- `F_SETFL`: write file status flags.

## Main Use Here
Turn listening sockets into nonblocking sockets:
```c
int flags = fcntl(listen_fd, F_GETFL) | O_NONBLOCK;
fcntl(listen_fd, F_SETFL, flags);
```

## Why It Matters
With `epoll`, the listening socket is commonly nonblocking so `accept()` can safely return `EAGAIN` instead of blocking.

## Common Mistakes
- Using `F_SETFL` without preserving old flags.
- Forgetting error checks.

## Related
- `accept()`
- `epoll_ctl()`
