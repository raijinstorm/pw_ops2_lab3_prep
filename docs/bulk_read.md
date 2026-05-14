# `bulk_read()`

## Purpose
Lab helper that keeps reading until it gets the requested byte count, EOF, or an error.

## Definition Shape
```c
ssize_t bulk_read(int fd, char *buf, size_t count);
```

## Behavior
- Repeats `read()` until `count` bytes are read.
- Stops early on EOF and returns bytes read so far.
- Returns negative value on read error.

## Used In These Labs
- Fixed-size messages in `task1`, `task3`, `task4`, `website_lab`.

## Good Use Case
- You know the exact number of bytes required right now.
- Example: reading a fixed 4-byte or 20-byte protocol frame.

## Bad Use Case
- Stage 1 of `last_year_lab`, where the whole point is to manually handle partial buffered input.

## Example
```c
char buf[4];
ssize_t n = bulk_read(fd, buf, 4);
```

## Common Mistakes
- Using it for line-oriented or variable-length streaming data.
- Assuming it works like a single `read()` call.

## Related
- `read()`
- `bulk_write()`
