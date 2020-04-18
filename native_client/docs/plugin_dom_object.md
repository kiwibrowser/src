Using the `<embed>` element creates a plugin DOM object. The NaCl plugin object
has the following methods:

*   `__urlAsNaClDesc(url, callback)`: Fetches a URL; passes a file descriptor to
    the callback.
    *   This applies a same origin policy. The URL must be for the same origin
        as the embedding page.
*   `__shmFactory(size)`: Returns a descriptor for a newly-created shared memory
    segment.
*   `__socketAddressFactory(string)`: Converts a socket address string to a
    SocketAddress descriptor. See [IMCSockets](imc_sockets.md).
*   `__defaultSocketAddress()`: Returns descriptor for a socket.
*   `__nullPluginMethod()`: A no-op. For testing purposes.

It has the following properties:

*   `height`
*   `width`
*   `src`
*   `__moduleReady`
*   `videoUpdateMode`

These methods and properties are hooked up in [plugin/srpc/plugin.cc]
(http://code.google.com/p/nativeclient/source/browse/trunk/src/native_client/src/trusted/plugin/srpc/plugin.cc).
