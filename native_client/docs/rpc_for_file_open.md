This is a design discussion for [issue 607]
(http://code.google.com/p/nativeclient/issues/detail?id=607).

## Context: implementation in Plash

In [Plash](http://plash.beasts.org), the RPC for doing an open() is fairly
straightforward and just involves two syscalls on the same socket:

*   sendmsg(socket\_fd, request\_message); // Send open() request
*   recvmsg(socket\_fd, reply\_buffer); // Get result, including opened FD

It is not ideal that the same bidirectional socket is used for the request and
the reply, because doing two of these RPCs concurrently would not be safe, so
locking is required.

The code for this RPC is linked into both ld.so (since this needs to open
libraries) and libc.so (since this exports open()). The two share the same
socket FD. Since the RPC interface is simple, sharing is not a problem.

The process is endowed with socket\_fd when it is started, so the dynamic linker
does not have to do anything to acquire it.

## Implementation in NaCl browser plugin

Doing this RPC when running under the NaCl browser plugin is more complicated,
however:

1) It is not directly possible for the NaCl process to send an asynchronous
message to a Javascript object. The NaCl process can normally only invoke
Javascript objects in a Javascript event loop turn while the browser/renderer is
blocked waiting for a reply from NaCl.

The workaround is that we can artificially induce the browser/renderer to block
to wait for NaCl using the NPN\_PluginThreadAsyncCall message. With this,
sending a message involves 5 IMC messages:

*   NaCl -> plugin: NPN\_PluginThreadAsyncCall(closure\_id)
*   plugin -> NaCl: call closure\_id
    *   NaCl -> plugin: synchronously invoke Javascript object: obj(args)
    *   plugin -> NaCl: return result of obj(args)
*   NaCl -> plugin: return from closure

This does not include any result that obj(args) may wish to return
asynchronously, which would involve further messages:

*   plugin -> NaCl: invoke NaCl-provided JS object: callback(result)
*   NaCl -> plugin: return result of callback

2) The NaCl process does not start out with a suitable socket\_fd. Instead, the
NaCl process is started with a BoundSocket, which the plugin expects to
imc\_connect() to twice. Hence the NaCl process has to imc\_accept() on the
BoundSocket at least twice.

3) The NaCl process must perform some initialisation to indicate that it
supports NPAPI-over-SRPC.

4) There are no NPobjects exported across the connection either way initially.
Since the browser side has no way to acquire an initial NaCl-side object,
NaCl-side would have to acquire an object from Javascript's global scope.

5) There are two layers of message encoding: * SRPC marshalling * NPAPI argument
marshalling, on top of SRPC

6) The current NaCl-side SRPC/NPAPI library code expects to have control of
main().

(2) and (3) make it difficult for multiple users of the connection to coexist in
the same process.

## Proposed plugin extension

Add a Javascript method to the plugin object:

`plugin.make_socket(callback) -> fd`

This creates a socket pair. It returns an FD that can be passed to the NaCl
process. It listens for messages sent to this FD and invokes `callback` for
every message it receives. This is implemented by spawning a thread and doing
NPN\_PluginThreadAsyncCall() for each message.

The plugin's SRPC interface already supports sending messages to the NaCl
process (even if not receiving messages), so we can use this to send the FD at
startup.
