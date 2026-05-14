# `ERR`

## Purpose
Lab macro for fatal errors.

## Definition Shape
```c
#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
```

## What It Does
- Prints `source` with `perror()`.
- Prints file and line number.
- Exits the program.

## Why It Is Useful
It keeps the lab code short and consistent.

## Example
```c
if (listen(fd, 10) < 0)
    ERR("listen");
```

## When Not To Use It
- Recoverable conditions like `EAGAIN`.
- Expected disconnects that should only close one client.

## Related
- `perror()`
- `TEMP_FAILURE_RETRY`
