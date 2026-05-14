# `make_address()`

## Purpose
Lab helper that resolves a textual host and port into a ready `sockaddr_in`.

## Definition Shape
```c
struct sockaddr_in make_address(char *address, char *port);
```

## What It Does
- Uses `getaddrinfo()`.
- Restricts to `AF_INET`.
- Extracts the resulting `sockaddr_in`.

## Used In These Labs
Shared helper in `task3`, `task4`, `website_lab`, `polish_lab`, `last_year_lab`.

## Example
```c
struct sockaddr_in addr = make_address("localhost", "8080");
```

## Why It Is Handy
It hides the repetitive `addrinfo` boilerplate and gives you a concrete IPv4 structure directly.

## Related
- `getaddrinfo()`
- `sockaddr_in`
