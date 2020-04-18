# Async-only plugin: design questions

## Introduction

Suppose we remove the Javascript bridge from the NaCl plugin -- that is, remove
the implementation of PPAPI's PPVar interface. What are the implications of
this?

One suggestion is that we have a postMessage()-like interface.
window.postMessage() is the DOM interface for sending messages between frames.

## Discussion

postMessage() comes with some baggage: * The receiver receives an "origin"
argument. * postMessage() can send an array of ports. This is useful, but how
would we expose ports to a NaCl process? A ports an appropriate IPC mechanism
for NaCl to use? * Should the argument be a Unicode string (UTF-16?), a byte
string, a JSON data structure, or something else? window.postMessage() takes a
Unicode string, which is not really appropriate for NaCl because we are likely
to want to send binary data.

Can we still do synchronous calls from Javascript to a NaCl process? * How do we
handle DOM events? These must usually be handled synchronously. Event handlers
will indicate whether the event should be propagated to later handlers. * We
could provide a "wait for message" primitive. By putting this in one place, it
would make it easier for the renderer to cancel the blocking. Currently,
blocking prompts the browser to offer to kill the renderer. It is more graceful
when long-running Javascript prompts the renderer to offer to stop the
Javascript. * The DOM already allows a page to block by doing a synchronous
XMLHttpRequest.

What kind of IPC do we provide? * Can we pass file descriptors (FDs) across a
connection? * Can we have multiple connections to the NaCl process? * Can we set
up connections between multiple NaCl processes? * Do we have message size
limits?

Proxying objects: * What conventions do we use for proxying objects? * Who
provides the Javascript-side proxy code? * We can implement a standard
Javascript library for proxying Javascript objects. But how does a web app load
it? * The web app could load the library via `<script>`. It could copy the
library to its own server, or load it from a Google server. * The NaCl plugin
could provide a built-in library and `eval()` it. But this will not work if
PPAPI removes the ability of a plugin to do Javascript calls such as `eval()`. *
In today's Javascript, it is not possible for an object `x` to intercept a
property assignment such as `x.prop = y`, but many DOM interfaces are based on
property assignment (e.g. `window.location = url`). Only native objects (such as
DOM objects) can handle such assignments. * This will be fixed by the "[proxies]
(http://wiki.ecmascript.org/doku.php?do=show&id=harmony:proxies)" extension to
Ecmascript being designed by Tom Van Cutsem. * Until ES proxies or a similar
feature is implemented, it will not be possible to proxy Javascript objects
fully to the extent that PPVar and NPAPI's npruntime can. * In order to
efficiently forward Javascript objects across a connection without creating
multiple proxies when a Javascript object is forwarded multiple times, we need
[weak maps](http://wiki.ecmascript.org/doku.php?id=harmony:weak_maps) in
Javascript, which are proposed but not yet implemented and standardised.

Visibility of garbage collection: * Can a user program discover when a
Javascript proxy object is no longer referenced? If not, we would either leak
memory in the NaCl process, or we would require explicit deletion calls in
Javascript, which would make the proxying non-transparent. * Similarly, if we
provide Javascript wrapper objects for file descriptors, can we ensure that the
file descriptors are automatically garbage collected?

## Buffering semantics

What happens if Javascript repeatedly sends messages and the NaCl process does
not unqueue them? Do we have an unlimited-size buffer? The alternative is to
have a limited-size buffer and do one of the following: * Block when the buffer
is full. However, this creates the risk of deadlocks. * Return an exception when
the buffer is full (as Unix does with O\_NONBLOCK). Neither option is good,
because the behaviour could change when the system is under load. There is also
a risk that the buffer size is different between platforms.

We probably need to have an unlimited-size buffer. The lack of size limit should
not be a problem as we do not constrain memory allocation in general.

## Use case: Flash

Flash supports a synchronous Javascript bridge. If the NaCl plugin does not
provide a way for Javascript to make synchronous calls to NaCl, it would not be
possible to implement all of Flash under NaCl.
