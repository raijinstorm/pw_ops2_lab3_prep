# `ntohl()`

## Purpose
Convert a 32-bit integer from network byte order to host byte order.

## Prototype
```c
uint32_t ntohl(uint32_t netlong);
```

## Used In These Labs
- `website_lab` arithmetic protocol.
- `last_year_lab/sop-crone.c` when printing 4-byte integers received from the mother chain.

## Example
```c
int32_t host_value = ntohl(net_value);
```

## Related
- `htonl()`
