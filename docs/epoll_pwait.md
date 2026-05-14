# `epoll_pwait()`

## Purpose
Wait until one or more watched fds become ready, with optional temporary signal mask.

## Prototype
```c
int epoll_pwait(int epfd, struct epoll_event *events, int maxevents,
                int timeout, const sigset_t *sigmask);
```

## Parameters
- `epfd`: epoll instance.
- `events`: output array.
- `maxevents`: size of that array.
- `timeout`: milliseconds, `-1` means wait forever.
- `sigmask`: temporary mask while waiting, often used with `SIGINT`.

## Returns
- Number of ready events.
- `0` on timeout.
- `-1` on error.

## Used In These Labs
Core event loop primitive in `task3`, `task4`, `website_lab`, `polish_lab`, `last_year_lab`.

## Why Not Plain `epoll_wait()`?
`epoll_pwait()` lets you control signal delivery during the blocking wait.

## Common Mistakes
- Not handling `EINTR`.
- Passing `maxevents <= 0`.
- Forgetting that a timeout of `0` means “poll immediately”.

## Related
- `sigprocmask()`
- `epoll_create1()`
- `epoll_ctl()`
