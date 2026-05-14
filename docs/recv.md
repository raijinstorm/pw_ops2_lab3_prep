# `recv()`

## Purpose
Receive data from a socket, optionally with flags.

## Prototype
```c
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
```

## Why Use It Instead Of `read()`?
The `flags` argument gives extra control.

## Used In These Labs
- `last_year_lab/sop-crone.c` uses `recv(..., MSG_PEEK)` to check whether the maiden socket has closed without consuming data.

## Useful Flag Here
- `MSG_PEEK`: inspect incoming bytes without removing them from the receive queue.

## Example
```c
unsigned char c;
ssize_t n = recv(fd, &c, 1, MSG_PEEK);
```

## Related
- `read()`
