# `struct epoll_event`

## Purpose
Describe what epoll should watch, and report which fd became ready.

## Typical Layout
```c
struct epoll_event ev;
ev.events = EPOLLIN;
ev.data.fd = fd;
```

## Important Fields
- `events`: bitmask like `EPOLLIN`, `EPOLLRDHUP`.
- `data.fd`: the file descriptor value you want back later.

## Used In These Labs
All epoll-based labs.

## Example
```c
struct epoll_event event, events[MAX_EVENTS];
event.events = EPOLLIN;
event.data.fd = sock;
```

## Common Mistakes
- Forgetting `memset(&event, 0, sizeof(event))` when reusing it.
- Confusing `event` you register with `events[]` returned from wait.

## Related
- `epoll_ctl()`
- `epoll_pwait()`
