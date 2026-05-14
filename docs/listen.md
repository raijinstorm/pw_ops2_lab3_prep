# `listen()`

## Purpose
Turn a bound stream socket into a listening socket.

## Prototype
```c
int listen(int sockfd, int backlog);
```

## Parameters
- `sockfd`: socket already bound to a local address.
- `backlog`: queue size hint for pending connections.

## Returns
- `0` on success.
- `-1` on error.

## Used In These Labs
Every server lab, directly or inside helper wrappers.

## Example
```c
if (listen(server_fd, 10) < 0)
    ERR("listen");
```

## Common Mistakes
- Calling `accept()` before `listen()`.
- Using it on a datagram socket.

## Related
- `socket()`
- `bind()`
- `accept()`
