# Docs Index

These notes are based on the APIs and helper wrappers actually used in `task1`, `task3`, `task4`, `website_lab`, `polish_lab`, and `last_year_lab`.

## Epoll
- [epoll_create1](./epoll_create1.md)
- [epoll_ctl](./epoll_ctl.md)
- [epoll_event](./epoll_event.md)
- [epoll_pwait](./epoll_pwait.md)

## Socket Syscalls
- [socket](./socket.md)
- [bind](./bind.md)
- [listen](./listen.md)
- [accept](./accept.md)
- [connect](./connect.md)
- [setsockopt](./setsockopt.md)
- [recv](./recv.md)
- [read](./read.md)
- [write](./write.md)
- [close](./close.md)
- [unlink](./unlink.md)

## Address Resolution And Types
- [getaddrinfo](./getaddrinfo.md)
- [addrinfo](./addrinfo.md)
- [sockaddr_in](./sockaddr_in.md)
- [sockaddr_un](./sockaddr_un.md)

## Byte Order
- [htons](./htons.md)
- [htonl](./htonl.md)
- [ntohs](./ntohs.md)
- [ntohl](./ntohl.md)

## Memory / String Helpers
- [memchr](./memchr.md)
- [memmove](./memmove.md)
- [memset](./memset.md)
- [strncpy](./strncpy.md)

## Signals / Descriptor Flags
- [sigaction](./sigaction.md)
- [sigprocmask](./sigprocmask.md)
- [fcntl](./fcntl.md)
- [getline](./getline.md)

## Lab Helper Wrappers
- [ERR](./err.md)
- [TEMP_FAILURE_RETRY](./temp_failure_retry.md)
- [sethandler](./sethandler.md)
- [make_address](./make_address.md)
- [add_new_client](./add_new_client.md)
- [bulk_read](./bulk_read.md)
- [bulk_write](./bulk_write.md)
- [bind_tcp_socket](./bind_tcp_socket.md)
- [connect_tcp_socket](./connect_tcp_socket.md)
- [bind_local_socket](./bind_local_socket.md)
- [connect_local_socket](./connect_local_socket.md)

## Suggested Reading Order
1. `socket`, `sockaddr_in`, `bind`, `listen`, `accept`, `connect`
2. `getaddrinfo`, `htons`, `htonl`, `ntohs`, `ntohl`
3. `epoll_create1`, `epoll_ctl`, `epoll_pwait`, `epoll_event`
4. `read`, `write`, `bulk_read`, `bulk_write`
5. `fcntl`, `sigaction`, `sigprocmask`
