# `bind_local_socket()`

## Purpose
Lab helper that creates and starts a listening UNIX-domain stream socket.

## Definition Shape
```c
int bind_local_socket(char *name, int backlog_size);
```

## What It Does
- Removes the old socket path with `unlink()`.
- Creates the socket via `make_local_socket()`.
- Calls `bind()`.
- Calls `listen()`.

## Used In These Labs
- `website_lab` local socket server.
- `polish_lab/sop-werona.c`.

## Why It Is Useful
UNIX sockets require filesystem cleanup. The helper centralizes that logic.

## Example
```c
int listen_fd = bind_local_socket("Laurenty", 5);
```

## Common Mistakes
- Forgetting to `unlink()` the socket path on shutdown.
- Using a too-long `sun_path`.

## Related
- `make_local_socket()`
- `unlink()`
- `bind()`
- `listen()`
