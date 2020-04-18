# Replacing use of host OSes' bind() and connect()

These are notes on how to address [issue 344]
(http://code.google.com/p/nativeclient/issues/detail?id=344).

## What I thought the current protocol was

In the server process: * imc\_makeboundsock() does the following: * `token =
ChooseRandomToken()` * `fd = socket(AF_UNIX, SOCK_STREAM)` * `bind(fd, token)`
// An error here means two processes chose the same token, which is not supposed
to happen. * `listen(fd, backlog)` // Currently backlog=5 on Mac OS X. This is
bad because it exposes an arbitrary limit to NaCl processes. * `return
(SocketAddress(token), BoundSocket(fd))` * imc\_accept(BoundSocket(fd)) does: *
`sock_fd1 = accept(fd)` * `return ConnectedSocket(sock_fd1)`

In a client process: * imc\_connect(SocketAddress(token)) does: * `sock_fd2 =
socket(AF_UNIX, SOCK_STREAM)` * `connect(sock_fd2, token)` * `return
ConnectedSocket(sock_fd2)`

## Current protocol on Linux

In the server process: * imc\_makeboundsock() does the following: * `token =
ChooseRandomToken()` * `fd = socket(AF_UNIX, SOCK_DGRAM)` * `bind(fd, token)` //
An error here means two processes chose the same token, which is not supposed to
happen. * `return (SocketAddress(token), BoundSocket(fd))` *
imc\_accept(BoundSocket(fd)) does: * `sock_fd1 = recvmsg(fd)` * `return
ConnectedSocket(sock_fd1)`

In a client process: * imc\_connect(SocketAddress(token)) does: * `sock_fd1,
sock_fd2 = socketpair(AF_UNIX, SOCK_SEQPACKET)` * `sendmsg(global_fd, "",
[sock_fd1], address=token)` // global\_fd is only available if sel\_ldr is run
with "-X -1" * `close(sock_fd2)` * // The problem with this is that it succeeds
immediately. We don't know if our connection request was received. * `return
ConnectedSocket(sock_fd2)`

## Replacement protocol

In the server process: * imc\_makeboundsock() does the following: * `client_fd,
server_fd = socketpair(AF_UNIX, SOCK_STREAM)` * `return
(SocketAddress(client_fd), BoundSocket(server_fd))` *
imc\_accept(BoundSocket(server\_fd)) does: * `msg, sock_fd1 = recvmsg(server_fd,
single_byte_buffer)` * `assert msg == "c"` * `return ConnectedSocket(sock_fd1)`
// We assume that the FD that is sent to us behaves sanely.

In a client process: * imc\_connect(SocketAddress(client\_fd)) does: *
`sock_fd1, sock_fd2 = socketpair()` * `sendmsg(client_fd, "c", [sock_fd1])` *
`close(sock_fd1)` * // The problem with this is that it succeeds immediately. We
don't know if our connection request was received. * `return
ConnectedSocket(sock_fd2)`
