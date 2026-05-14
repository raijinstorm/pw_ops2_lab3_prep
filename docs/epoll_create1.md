# `epoll_create1()`

## Purpose
Create an epoll instance.

## Prototype
```c
int epoll_create1(int flags);
```

## Parameters
- `flags`: usually `0`. `EPOLL_CLOEXEC` is optional in other programs.

## Returns
- New epoll fd on success.
- `-1` on error.

## Used In These Labs
- `task3/server.c`
- `task4/client.c`
- `website_lab/l7-1_server.c`
- `polish_lab/sop-werona.c`
- `last_year_lab/sop-crone.c`
- `last_year_lab/sop-witch.c`

## Why It Matters
It gives you one fd that can wait for many other fds.

## Example
```c
int epoll_fd = epoll_create1(0);
if (epoll_fd < 0)
    ERR("epoll_create1");
```

## Related
- `epoll_ctl()`
- `epoll_pwait()`
- `epoll_event`
