# `close()`

## Purpose
Close a file descriptor.

## Prototype
```c
int close(int fd);
```

## Returns
- `0` on success.
- `-1` on error.

## Used In These Labs
Everywhere: disconnecting clients, closing servers, releasing epoll fds, rejecting connections.

## Typical Pattern Here
```c
if (TEMP_FAILURE_RETRY(close(fd)) < 0)
    ERR("close");
```

## Common Mistakes
- Closing the listening socket instead of the accepted client socket.
- Forgetting to remove the fd from `epoll` state first when your logic depends on that.
- Using an fd after closing it.

## Related
- `TEMP_FAILURE_RETRY`
- `epoll_ctl()`
