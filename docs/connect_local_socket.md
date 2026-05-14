# `connect_local_socket()`

## Purpose
Lab helper that connects to a UNIX-domain stream socket by pathname.

## Definition Shape
```c
int connect_local_socket(char *name);
```

## What It Does
- Builds a `sockaddr_un`.
- Creates a `PF_UNIX` stream socket.
- Calls `connect()`.

## Used In These Labs
- Available in the common headers.
- Similar logic appears in `website_lab/l7-1_client_local.c`.

## Example
```c
int fd = connect_local_socket("Laurenty");
```

## Related
- `sockaddr_un`
- `connect()`
- `make_local_socket()`
