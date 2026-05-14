# `memset()`

## Purpose
Fill memory with a byte value.

## Prototype
```c
void *memset(void *s, int c, size_t n);
```

## Main Uses In These Labs
- Zeroing `sockaddr_in`, `sockaddr_un`, `sigaction`, and `epoll_event`.
- Filling arrays with a specific marker value.

## Example
```c
struct sockaddr_in addr;
memset(&addr, 0, sizeof(addr));
```

## Common Mistakes
- Forgetting that it fills bytes, not typed values.
- Using nonzero fill on structs without understanding the memory layout.

## Related
- `strncpy()`
- `sockaddr_in`
