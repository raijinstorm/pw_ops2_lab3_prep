# `write()`

## Purpose
Write bytes to a file descriptor.

## Prototype
```c
ssize_t write(int fd, const void *buf, size_t count);
```

## Returns
- Number of bytes written.
- `-1` on error.

## Key Socket Idea
One `write()` call may send fewer bytes than requested.

## Used In These Labs
- Directly inside helper `bulk_write()`.
- Sometimes direct write-like behavior is needed when discussing partial sends.

## Common Mistakes
- Assuming `write(fd, buf, count) == count`.
- Ignoring `SIGPIPE` / `EPIPE` on disconnected sockets.

## Related
- `bulk_write()`
- `read()`
