# `sigprocmask()`

## Purpose
Block or unblock signals for the current thread/process context.

## Prototype
```c
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
```

## Why It Appears With `epoll_pwait()`
Common pattern in these labs:
- block `SIGINT`,
- call `epoll_pwait()` with the old mask,
- let `SIGINT` interrupt the wait in a controlled way.

## Example
```c
sigset_t mask, oldmask;
sigemptyset(&mask);
sigaddset(&mask, SIGINT);
sigprocmask(SIG_BLOCK, &mask, &oldmask);
```

## Common Mistakes
- Forgetting to restore the old mask.
- Mixing up blocking during setup versus temporary unblocking during wait.

## Related
- `epoll_pwait()`
- `sigaction()`
