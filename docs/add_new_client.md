# `add_new_client()`

## Purpose
Lab helper wrapper around `accept()` for nonblocking servers.

## Definition Shape
```c
int add_new_client(int sfd);
```

## What It Does
- Calls `accept(sfd, NULL, NULL)`.
- Retries on `EINTR` through `TEMP_FAILURE_RETRY`.
- Returns `-1` for `EAGAIN` / `EWOULDBLOCK`.
- Calls `ERR("accept")` for other errors.

## Why It Exists
In `task3`, `task4`, `website_lab`, and header helpers, the code often does not care about peer address details and only needs “new client or no client”.

## Good Use Case
- Listening socket is nonblocking.
- `epoll` reported readability on the listening socket.
- You only need the client fd.

## Bad Use Case
- You need the peer IP/port.
- Example: `last_year_lab/sop-crone.c` stage 2 and stage 4 need the peer address, so direct `accept()` is better.

## Example
```c
int cfd = add_new_client(listen_fd);
if (cfd < 0)
    return;
```

## Related
- `accept()`
- `TEMP_FAILURE_RETRY`
- `ERR`
