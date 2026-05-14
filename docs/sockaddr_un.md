# `struct sockaddr_un`

## Purpose
UNIX-domain socket address structure.

## Typical Fields
```c
struct sockaddr_un addr = {};
addr.sun_family = AF_UNIX;
strncpy(addr.sun_path, name, sizeof(addr.sun_path) - 1);
```

## Important Fields
- `sun_family`: `AF_UNIX`.
- `sun_path`: filesystem path to the socket.

## Used In These Labs
- `website_lab` local-socket client/server path.
- `polish_lab` local server.

## Common Mistakes
- Path too long for `sun_path`.
- Forgetting to remove the old path with `unlink()`.

## Related
- `bind_local_socket()`
- `connect_local_socket()`
- `unlink()`
