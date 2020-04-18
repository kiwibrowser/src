# Native Client's IMC sockets interface

## Overview of interface

NaCl has four types of socket endpoint:

*   SocketAddress: created by imc\_makeboundsock(); imc\_connect()able
*   BoundSocket: created by imc\_makeboundsock(); imc\_accept()able
*   ConnectedSocket: created by imc\_connect() or imc\_accept()
    *   Message-oriented: can send/receive messages with
        imc\_sendmsg()/recvmsg().
    *   Can send any descriptor except ConnectedSocket or BoundSocket.
    *   Therefore cannot be shared between processes.
*   DataOnlySocket: created by imc\_socketpair()
    *   Message-oriented: can send/receive messages with
        imc\_sendmsg()/recvmsg().
    *   Can be shared between processes, but messages are not reliably separated
        and may be corrupted if sent/received concurrently.

There are no byte stream sockets.

In addition, NaCl has these non-socket descriptor types:

*   FileDesc: for read-only files fetched by the browser.
*   SharedMemory: created with imc\_mem\_obj\_create().

(Note: The names used here are not necessarily how they appear in the code.)

## SocketAddress/BoundSocket pairs

SocketAddress and BoundSocket are NaCl's answer to Unix domain sockets that are
bound into the filesystem namespace.

With Unix domain sockets, a server process, A, will do the following to create
and listen on a socket bound into the file namespace: `fd0 = socket() bind(fd0,
"/tmp/xxx/sock") listen(fd0, queue_size) /* boring */
announce_to_other_process("/tmp/xxx/sock") while True: fd1 = accept(fd0)
handle_connection(fd1)
` Process A must pass the string "/tmp/xxx/sock" to another process, B, which
does: `fd2 = socket() connect(fd2, "/tmp/xxx/sock")
` Result: fd1 and fd2 are connected together.

In NaCl, the filename string is replaced with a pair of descriptors, created by
the system call imc\_makeboundsock(), of types SocketAddress and BoundSocket.
Server process A does: `sockaddr_fd, boundsock_fd = imc_makeboundsock()
send_to_other_process(sockaddr_fd) while True: fd1 = imc_accept(boundsock_fd)
handle_connection(fd1)
` Process B receives sockaddr\_fd and does: `fd2 = imc_connect(sockaddr_fd)
` Result: fd1 and fd2 are both ConnectedSockets and are connected together.

There is no built-in filesystem namespace in NaCl. Rather than passing the
filename of a Unix domain socket across processes, a process must pass a
SocketAddress descriptor.

SocketAddress is misleadingly named because it is a descriptor, not an address
(a string). It just happens that the way it is implemented, the descriptor wraps
a randomly-chosen string.

In NaCl, the primary means of sharing is to share SocketAddress descriptors,
because ConnectedSockets are not transferrable.

The reason ConnectedSockets are not transferable is that the current
implementation of descriptor-passing on Windows requires that the recipient
process does not change during an imc\_sendmsg() operation.

The call `imc_sendmsg(socket_fd, "data", [desc_handle])` (i.e. sending some data
and a single descriptor) is implemented as follows on Windows by NaCl's trusted
runtime:

1.  Grab in-process lock for socket descriptor
2.  Send message: "What is your process ID?"
3.  Receive message: "My process ID is proc\_pid"
4.  `proc_handle = OpenProcess(proc_pid)`
5.  `new_handle_id = DuplicateHandle(proc_handle, desc_handle)`
    *   This copies our handle into the receiving process's handle table.
6.  `SendMessage("header indicating 1 handle is being sent" + new_handle_id +
    "data")`
    *   Sends the message data along with the ID of the handle in the receiving
        process's handle table.
7.  Release lock

Apparently this is the only way to transfer handles in Windows.

NaCl has to ensure that there can only be one recipient process, otherwise there
could be a race condition: the Windows handle could get transferred to one
process, while the handle ID is received by a different process.

(Note that steps 2/3 could be performed when the ConnectedSocket is first
created, but the current implementation does it on every imc\_sendmsg().)

I suspect there are ways to fix this race condition while allowing
ConnectedSockets to be transferable, but first I'd like to explore what
conventions might be layered on top of this interface to provide
object-capabilities that are sharable by default.

## How to use this as a capability system

The convention could be that all objects are SocketAddresses. ConnectedSockets
are created only temporarily, one per method call.

To make a method call: 1. imc\_connect() to the object, getting a stream S (a
ConnectedSocket, non-first-class). 1. Call imc\_sendmsg() to send the message
(data and capabilities), possibly multiple times, because IMC messages are
limited to (I believe) 128k. 1. Wait for a reply by calling imc\_recvmsg().
Again, multiple calls may be necessary to receive large reply messages. 1.
close() the stream.

If this pattern is going to be common, it could be combined into a single NaCl
system call.

One down side to using SocketAddresses this way is that the holder of a
BoundSocket cannot tell when all the references to the corresponding
SocketAddress have been dropped. (Unix domain sockets that are bound into the
filesystem have the same problem.) This means the system does not provide a way
to garbage collect objects.

Also, doing an imc\_connect() for every method call might be slow.

## DataOnlySocket

DataOnlySockets are created as connected pairs with imc\_socketpair(). Messages
sent across these sockets can contain only data, not descriptors. This means
they are not subject to the race condition above, and so transferring these
sockets between processes using imc\_sendmsg() is allowed.

Although DataOnlySockets are message-oriented, on Windows they are implemented
on top of byte stream sockets using headers to separate the messages. This is
not done in a multiprocess-safe way, so concurrent imc\_sendmsg()/imc\_recvmsg()
calls can corrupt the message headers so that header data is returned by
imc\_recvmsg(). This does not threaten the integrity of the system, however,
because it does not provide a way to acquire other processes' descriptors.

I expect that it will be desirable to add concurrently-usable byte stream
sockets to NaCl in order to support Unix programs that use pipes.

## Extracting address data from SocketAddress descriptors

The NaCl browser plugin allows Javascript code to perform some extra operations
on SocketAddress descriptors that are not directly available to untrusted NaCl
processes.

Javascript can extract the address string (pure data) from a SocketAddress
descriptor wrapper object (using its toString() method), and it can convert
address strings back to SocketAddress descriptor objects (using the
socketAddressFactory() method on the plugin object). Calling
socketAddressFactory() on an invalid socket address string will return a
SocketAddress descriptor on which imc\_connect() will fail.

One of the reasons for exposing socket address strings this way is that it
allows SocketAddresses to be shared with non-NaCl plugins that are statically
linked with NaCl's IMC library, specifically the O3D plugin.

Socket address strings are not scoped at all, so it is possible to create
cross-tab connections -- for example, by passing the socket address string to
the server and back to another web app. This probably allows connections to be
shared across browser instances as well.

## Better terms

The names SocketAddress and BoundSocket come from their implementation. It might
be better to find terms which describe their abstract interface.

*   Instead of SocketAddress, perhaps ConnectEndpoint?
*   Instead of BoundSocket, perhaps AcceptEndpoint?
