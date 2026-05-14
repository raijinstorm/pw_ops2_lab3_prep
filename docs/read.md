# `read()`

## Purpose
Read bytes from a file descriptor.

## Prototype
```c
ssize_t read(int fd, void *buf, size_t count);
```

## Returns
- Number of bytes read.
- `0` on EOF / orderly close.
- `-1` on error.

## Key Socket Idea
On a stream socket, one `read()` does not correspond to one sent message.

## Used In These Labs
- Buffered parsing in `polish_lab`.
- Stage 1+ framing in `last_year_lab`.
- Wrapped by `bulk_read()` in simpler fixed-size protocols.

## Common Mistakes
- Expecting a full logical message in one call.
- Ignoring `0`, which means the peer closed the connection.

## Related
- `write()`
- `bulk_read()`
