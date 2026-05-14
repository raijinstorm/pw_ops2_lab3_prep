# `TEMP_FAILURE_RETRY`

## Purpose
Retry a syscall automatically when it fails with `EINTR`.

## Shape
Macro, not a function.

## Why It Matters
Some syscalls can be interrupted by signals before they finish.

## Used In These Labs
Wrapped around:
- `read()`
- `write()`
- `accept()`
- `close()`
- `connect()` in some places

## Example
```c
ssize_t n = TEMP_FAILURE_RETRY(read(fd, buf, count));
```

## What It Does Not Do
- It does not handle `EAGAIN`.
- It does not make a blocking call nonblocking-safe.

## Related
- `ERR`
