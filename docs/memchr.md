# `memchr()`

## Purpose
Find the first occurrence of a byte in a memory block.

## Prototype
```c
void *memchr(const void *s, int c, size_t n);
```

## Why It Matters In These Labs
Useful for buffered protocol parsing when data may contain only part of a line.

## Used In These Labs
- `polish_lab/sop-werona.c` to find `'\n'` inside partially received input.

## Example
```c
char *nl = memchr(buf, '\n', len);
```

## Difference From `strchr()`
- `memchr()` works on raw memory with explicit length.
- `strchr()` expects a null-terminated string.

## Related
- `memmove()`
- `read()`
