# `memmove()`

## Purpose
Copy bytes safely even when source and destination overlap.

## Prototype
```c
void *memmove(void *dest, const void *src, size_t n);
```

## Used In These Labs
- `polish_lab/sop-werona.c` to shift leftover unread bytes to the front of a buffer.

## Why Not `memcpy()`?
If memory regions overlap, `memcpy()` is undefined behavior. `memmove()` is safe.

## Example
```c
memmove(buf, start, remaining);
```

## Common Use Case
Buffered parsers:
- process one complete message,
- move leftover bytes to index `0`,
- continue reading.

## Related
- `memchr()`
