# `strncpy()`

## Purpose
Copy up to `n` bytes from one character array to another.

## Prototype
```c
char *strncpy(char *dest, const char *src, size_t n);
```

## Used In These Labs
- Filling `sun_path` in `sockaddr_un`.
- Storing names in `polish_lab`.

## Important Gotcha
`strncpy()` does not always add a trailing `'\0'` if the source is too long.

## Safe Pattern
```c
strncpy(addr.sun_path, name, sizeof(addr.sun_path) - 1);
addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
```

## Why It Is Acceptable Here
The code often zeroes the destination first with `memset()`, so the trailing byte is already `0`.

## Related
- `memset()`
