# `bulk_write()`

## Purpose
Lab helper that keeps writing until all requested bytes are sent or an error occurs.

## Definition Shape
```c
ssize_t bulk_write(int fd, char *buf, size_t count);
```

## Behavior
- Repeats `write()` until `count` bytes are written.
- Returns total bytes written.
- Returns negative value on write error.

## Used In These Labs
- Sending fixed protocol packets in `task1`, `task3`, `task4`, `website_lab`, `last_year_lab`.

## Why It Matters
`write()` on sockets is allowed to write fewer bytes than you asked for.

## Example
```c
if (bulk_write(fd, msg, 5) < 0)
    ERR("write");
```

## Common Mistakes
- Treating one `write()` as guaranteed full send.
- Ignoring `EPIPE` when writing to a disconnected peer.

## Related
- `write()`
- `bulk_read()`
