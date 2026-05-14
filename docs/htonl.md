# `htonl()`

## Purpose
Convert a 32-bit integer from host byte order to network byte order.

## Prototype
```c
uint32_t htonl(uint32_t hostlong);
```

## Used In These Labs
- `website_lab` message fields.
- `last_year_lab/sop-witch.c` integer response.
- TCP/stream protocols that send 32-bit values.

## When To Use
Before sending a 32-bit integer over the network.

## Example
```c
int32_t value_net = htonl(value_host);
```

## Related
- `ntohl()`
- `htons()`
