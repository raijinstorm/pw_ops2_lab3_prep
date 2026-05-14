# `sethandler()`

## Purpose
Lab helper wrapper around `sigaction()`.

## Definition Shape
```c
int sethandler(void (*f)(int), int sigNo);
```

## What It Does
- Builds a `struct sigaction`.
- Sets `sa_handler = f`.
- Calls `sigaction(sigNo, &act, NULL)`.

## Used In These Labs
Almost every lab uses it to ignore `SIGPIPE` or handle `SIGINT`.

## Example
```c
if (sethandler(SIG_IGN, SIGPIPE))
    ERR("Setting SIGPIPE");
```

## Why It Is Useful
Shorter and less error-prone than repeating `sigaction` setup.

## Related
- `sigaction()`
