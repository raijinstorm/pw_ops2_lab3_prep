# `ntohs()`

## Purpose
Convert a 16-bit integer from network byte order to host byte order.

## Prototype
```c
uint16_t ntohs(uint16_t netshort);
```

## Used In These Labs
- Conceptually when interpreting network ports.
- Important in `last_year_lab` protocol understanding.

## Example
```c
uint16_t port = ntohs(net_port);
```

## Related
- `htons()`
