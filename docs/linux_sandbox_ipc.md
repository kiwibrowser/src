# Linux Sandbox IPC

The Sandbox IPC system is separate from the 'main' IPC system. The sandbox IPC
is a lower level system which deals with cases where we need to route requests
from the bottom of the call stack up into the browser.

The motivating example is Skia, which uses fontconfig to load fonts. In a
chrooted renderer we cannot access the user's fontcache, nor the font files
themselves. However, font loading happens when we have called through WebKit,
through Skia and into the SkFontHost. At this point, we cannot loop back around
to use the main IPC system.

Thus we define a small IPC system which doesn't depend on anything but `base`
and which can make synchronous requests to the browser process.

The [zygote](linux_zygote.md) starts with a `UNIX DGRAM` socket installed in a
well known file descriptor slot (currently 4). Requests can be written to this
socket which are then processed on a special "sandbox IPC" process. Requests
have a magic `int` at the beginning giving the type of the request.

All renderers share the same socket, so replies are delivered via a reply
channel which is passed as part of the request. So the flow looks like:

1.  The renderer creates a `UNIX DGRAM` socketpair.
1.  The renderer writes a request to file descriptor 4 with an `SCM_RIGHTS`
    control message containing one end of the fresh socket pair.
1.  The renderer blocks reading from the other end of the fresh socketpair.
1.  A special "sandbox IPC" process receives the request, processes it and
    writes the reply to the end of the socketpair contained in the request.
1.  The renderer wakes up and continues.

The browser side of the processing occurs in
`chrome/browser/renderer_host/render_sandbox_host_linux.cc`. The renderer ends
could occur anywhere, but the browser side has to know about all the possible
requests so that should be a good starting point.

Here is a (possibly incomplete) list of endpoints in the renderer:

### fontconfig

As mentioned above, the motivating example of this is dealing with fontconfig
from a chrooted renderer. We implement our own Skia FontHost, outside of the
Skia tree, in `skia/ext/SkFontHost_fontconfig**`.

There are two methods used. One for performing a match against the fontconfig
data and one to return a file descriptor to a font file resulting from one of
those matches. The only wrinkle is that fontconfig is a single-threaded library
and it's already used in the browser by GTK itself.

Thus, we have a couple of options:

1.  Handle the requests on the UI thread in the browser.
1.  Handle the requests in a separate address space.

The original implementation did the former (handle on UI thread). This turned
out to be a terrible idea, performance wise, so we now handle the requests on a
dedicated process.
