# `struct sockaddr_in`

## Purpose
IPv4 socket address structure.

## Typical Fields
```c
struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_port = htons(port);
addr.sin_addr.s_addr = htonl(INADDR_ANY);
```

## Important Fields
- `sin_family`: must be `AF_INET`.
- `sin_port`: port in network byte order.
- `sin_addr.s_addr`: IPv4 address in network byte order.

## Used In These Labs
All TCP-based labs.

## Common Uses
- Server bind address.
- Client connect target.
- Peer address captured from `accept()`.

## Common Mistakes
- Forgetting `htons()` for `sin_port`.
- Forgetting to zero the struct first.

## Related
- `bind()`
- `connect()`
- `htons()`
