# L7/L8 Sockets + epoll — template cheat sheet

Two axes. **Framing** (how you find message boundaries in a byte stream) is the
one that actually changes the code; **transport** is a one-line helper swap.

## Axis 1 — message framing (pick the template by this)

| Framing | How you find a message boundary | Template | Mirrors |
|---|---|---|---|
| **Fixed-size binary** | every message is `sizeof(struct)` bytes; `bulk_read` exactly that | `server_binary_fixed.c` / `client_binary_fixed.c` | calculator, cities, electors |
| **Length-prefixed binary** | 1-byte header = body length, then N body bytes; reassemble across reads | `server_binary_framed.c` / `client_binary_framed.c` | witches' coven |
| **Newline text** | bytes up to `'\n'`; buffer the partial line per client | `server_text.c` / `client_text.c` (or just `nc`) | Father Laurence, cities client |

The fixed case is easy (size known → `bulk_read(fd, &m, sizeof m)`). The other
two are where students lose points, because **TCP is a byte stream**: one
`read()` can return half a message, several messages, or a split one. Both the
framed-binary and text templates therefore keep **per-client state** and peel
off complete messages as bytes arrive.

## Axis 2 — transport (orthogonal; just swap the helper)

| Transport | Listen helper | Connect helper |
|---|---|---|
| TCP/IPv4 | `bind_tcp_socket(port, backlog)` | `connect_tcp_socket(host, port)` |
| UNIX-domain | `bind_local_socket(path, backlog)` | `connect_local_socket(path)` |

Server logic is identical; only the bind/connect call changes (and a UNIX server
must `unlink(path)` on exit). All helpers live in `common.h`.

## Byte order — the rule that bites

Convert **every multi-byte integer** that crosses the wire:
`htons`/`htonl` before sending, `ntohs`/`ntohl` after receiving. A lone byte (or
a char) needs no swap. Use fixed-width types (`uint32_t`) with no struct padding
so `sizeof` is the same on both ends.

## The shared epoll server skeleton (identical in all three servers)

```c
epfd = epoll_create1(0);
epoll_add(epfd, listen_fd, EPOLLIN);          // + STDIN_FILENO if needed
clients_init();
sigprocmask(SIG_BLOCK, &mask{SIGINT}, &oldmask);   // block SIGINT globally
while (do_work) {
    nfds = epoll_pwait(epfd, events, MAX, -1, &oldmask);  // SIGINT only fires here
    if (nfds < 0) { if (errno==EINTR) continue; ERR(); }
    for each event:
        if listen_fd:  add_new_client -> find_free -> epoll_add -> init slot
        elif stdin:    handle_stdin
        else:          handle_client(find_client(fd))   // <-- only this differs
}
// graceful shutdown: close all clients, close epfd, unblock
```

Why this signal pattern: SIGINT stays blocked except *inside* `epoll_pwait`, so
the handler can only run there, returning `EINTR`; no event is ever lost to a
signal landing between the check and the wait. The handler just sets
`volatile sig_atomic_t do_work = 0`. Always `SIG_IGN` SIGPIPE so writing to a
gone peer gives `EPIPE` instead of killing the process.

## Gotchas (the ones that actually bite)

- **Partial reads/writes**: use `bulk_read`/`bulk_write` for fixed sizes; for
  variable framing do ONE `read()` per epoll wakeup and resume from saved state.
- **`bulk_read` of an exact size can block** if a client dribbles a partial
  message. Fine for well-behaved fixed-size protocols; for adversarial/variable
  input use the framed per-client state machine (never blocks).
- **`epoll_ctl(ADD)` on stdin returns `EPERM`** when stdin is a regular-file
  redirect (epoll can't watch regular files) — tolerate it and skip stdin.
- **Level-triggered EOF spin**: a closed/EOF fd stays "readable" forever; remove
  it from epoll (`epoll_del`) once you see `read()==0`, or you busy-loop.
- **Non-blocking listen socket** + `add_new_client` returning -1 on `EAGAIN` lets
  the accept loop coexist with epoll without blocking.
- **`MAX_EVENTS` smaller than fds is fine** — epoll reports the rest next loop.
- A short `bulk_read` (`< expected`) means the peer closed mid-message → drop it.

## Testing

`make` (sanitizers) or `make CI=1` (strict `-Werror`). Each builds standalone:
`gcc -std=c17 server_text.c -o server_text`.

```
# fixed binary
./server_binary_fixed 9000 &      ./client_binary_fixed localhost 9000 6 7 '*'
# length-prefixed binary
./server_binary_framed 9000 &     ./client_binary_framed localhost 9000 hello hi
# text (netcat is a fine client too)
./server_text 9000 &              nc localhost 9000
```
