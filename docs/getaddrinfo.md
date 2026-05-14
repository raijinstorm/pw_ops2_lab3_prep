# `getaddrinfo()`

## Purpose
Resolve a hostname/service pair into socket addresses.

## Prototype
```c
int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints, struct addrinfo **res);
```

## Parameters
- `node`: hostname or IP string, e.g. `"localhost"`.
- `service`: port string, e.g. `"8080"`.
- `hints`: filters like family/type.
- `res`: output linked list of results.

## Returns
- `0` on success.
- Nonzero error code on failure.

## Used In These Labs
Inside helper `make_address()` in shared headers.

## Why It Is Better Than Older APIs
- Works with name resolution cleanly.
- Cleaner than `gethostbyname()`.
- Scales to IPv6 even if these labs mostly use IPv4.

## Example
```c
struct addrinfo hints = {};
struct addrinfo *result;
hints.ai_family = AF_INET;
getaddrinfo("localhost", "8080", &hints, &result);
```

## Common Mistakes
- Forgetting `freeaddrinfo(result)`.
- Passing integer port instead of string port.

## Related
- `addrinfo`
- `sockaddr_in`
- `connect()`
