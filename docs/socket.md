# `socket()`

## Purpose
Create a socket endpoint.

## Prototype
```c
int socket(int domain, int type, int protocol);
```

## Common Arguments In These Labs
- `domain`: `PF_INET` / `AF_INET` for TCP over IPv4, `PF_UNIX` for local sockets.
- `type`: `SOCK_STREAM`.
- `protocol`: `0`.

## Returns
- New socket fd on success.
- `-1` on error.

## Example
```c
int fd = socket(AF_INET, SOCK_STREAM, 0);
```

## Typical Lifecycles
- Server: `socket -> setsockopt -> bind -> listen -> accept`
- Client: `socket -> connect`

## Related
- `bind()`
- `listen()`
- `accept()`
- `connect()`
