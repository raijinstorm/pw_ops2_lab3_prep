# `htons()`

## Purpose
Convert a 16-bit integer from host byte order to network byte order.

## Prototype
```c
uint16_t htons(uint16_t hostshort);
```

## Main Use Here
- Filling `sin_port` in `sockaddr_in`.
- Sending 2-byte port numbers in `last_year_lab`.

## Example
```c
addr.sin_port = htons(port);
```

## Common Mistakes
- Forgetting to convert ports before `bind()` or `connect()`.

## Related
- `ntohs()`
- `htonl()`
- `sockaddr_in`
