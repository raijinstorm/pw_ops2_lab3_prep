# `sigaction()`

## Purpose
Install a signal handler or signal disposition.

## Prototype
```c
int sigaction(int signum, const struct sigaction *act,
              struct sigaction *oldact);
```

## Main Ideas
- `SIGINT` is often used for graceful stop.
- `SIGPIPE` is often ignored in socket code so a failed write becomes an error return instead of killing the process.

## Used In These Labs
Wrapped by `sethandler()` in the shared headers.

## Example
```c
struct sigaction act = {};
act.sa_handler = sigint_handler;
sigaction(SIGINT, &act, NULL);
```

## Common Mistakes
- Using unsafe work inside the signal handler.
- Forgetting to initialize the structure.

## Related
- `sethandler()`
- `sigprocmask()`
