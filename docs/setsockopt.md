# `setsockopt()`

## Purpose
Change socket options.

## Prototype
```c
int setsockopt(int sockfd, int level, int optname,
               const void *optval, socklen_t optlen);
```

## Main Use In These Labs
Enable address reuse for TCP servers:
```c
int opt = 1;
setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

## Why `SO_REUSEADDR` Matters
It helps when restarting a server quickly after it exits.

## Common Mistakes
- Forgetting to set it before `bind()`.
- Mixing up `SOL_SOCKET` and the option name.

## Related
- `bind()`
- `socket()`
