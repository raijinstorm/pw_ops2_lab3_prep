# `unlink()`

## Purpose
Remove a filesystem entry.

## Prototype
```c
int unlink(const char *pathname);
```

## Main Use In These Labs
Delete UNIX-domain socket files before `bind()` and during cleanup.

## Example
```c
if (unlink("Laurenty") < 0 && errno != ENOENT)
    ERR("unlink");
```

## Why It Matters For UNIX Sockets
The socket path is a real filesystem entry. If you leave it behind, the next `bind()` can fail.

## Related
- `bind_local_socket()`
- `sockaddr_un`
