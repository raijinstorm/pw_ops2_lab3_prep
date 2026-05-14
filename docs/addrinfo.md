# `struct addrinfo`

## Purpose
Configuration and result structure used by `getaddrinfo()`.

## Typical Fields You Use
```c
struct addrinfo hints = {};
hints.ai_family = AF_INET;
```

Important fields:
- `ai_family`: address family, e.g. `AF_INET`.
- `ai_socktype`: optional socket type, e.g. `SOCK_STREAM`.
- `ai_protocol`: optional protocol filter.
- `ai_addr`: resulting address buffer.
- `ai_addrlen`: size of `ai_addr`.
- `ai_next`: next result in the linked list.

## Used In These Labs
In the helper `make_address()` implementation inside `l7_common.h` / `l7-common.h`.

## Key Idea
You fill `hints`, call `getaddrinfo()`, then read the resolved `ai_addr` and finally free the list with `freeaddrinfo()`.

## Example
```c
struct addrinfo hints = {};
struct addrinfo *result;
hints.ai_family = AF_INET;
getaddrinfo("localhost", "8080", &hints, &result);
```

## Common Mistakes
- Forgetting `freeaddrinfo(result)`.
- Assuming only one result exists.
- Forgetting to initialize `hints` to zero.

## Related
- `getaddrinfo()`
- `sockaddr_in`
