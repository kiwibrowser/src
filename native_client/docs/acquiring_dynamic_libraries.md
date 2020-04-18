# Acquiring dynamic libraries

_Draft_

## Overview

The normal model for shared libraries in Unix is that libraries are installed
into the filesystem by a package manager into the centralised locations `/lib`
and `/usr/lib`. Native Client, however, does not have a built-in filesystem, and
the concept of a centralised package manager is not applicable to web apps.
Instead, we propose to use a virtualised filesystem namespace, implemented via
IPC calls. Each NaCl process may be launched with a custom filesystem namespace
populated with the library versions the web app chooses to use.

## How files are fetched

Each library and executable can be fetched from a URL.

There are at least two interfaces through which libraries can be fetched for use
in NaCl processes: * XMLHttpRequest * NaCl's `__urlAsNaClDesc()`, a method
provided to Javascript on the NaCl plugin object. This returns a NaCl file
descriptor via an asynchronous callback.

In principle, any mechanism that Javascript code can currently use for fetching
data can be used for fetching libraries.

However, using the latter, NaCl-specific descriptor-based interface has two
advantages: * It reduces the need to copy data between processes via sockets. *
It can be used with an mmap() interface which has the potential to allow library
and executable code to be mapped rather than copied into memory. Whether this
potential can be realised depends on the underlying mechanism for dynamic
loading; see [mmapping code]
(DynamicLoadingOptions#mmapping_code_to_share_memory.md) in
DynamicLoadingOptions. If mapping can be used, it means that two NaCl processes
using the same library will share physical memory for the library, provided that
the library is retained in the browser's cache.

The basic interface for fetching files is therefore a Javascript API. We need a
way to hook that up to Native Client.

## How files are requested by NaCl processes

NaCl processes will open files by making requests over IPC, using NaCl IMC
sockets. Javascript code running in the browser can handle these requests and
call `__urlAsNaclDesc()` on behalf of the NaCl process. Javascript objects can
provide a virtual file namespace that may contain a Unix-like file layout.

The `open()` library function will be implemented as a remote procedure call
which sends a message across an IMC socket and expects to receive a reply
message containing a file descriptor. This `open()` implementation will be used
by the dynamic linker and can be made available in libc/libnacl.

*   This solves a bootstrap problem. The NaCl file namespace does not need to be
    implemented by another NaCl process that would need to load its own files
    somehow. The file namespace does not have to be implemented by trusted code.
*   This allows namespaces to be defined in a flexible way. Rules for mapping
    filenames to URLs can be written in a scripting language.
*   The task of mapping filenames to URLs is not computationally intensive so
    using Javascript should not be a performance problem. Javascript code passes
    NaCl file descriptors around. File data does not need to be copied to or
    from Javascript strings.
*   We can provide a sample implementation or standard library that implements
    the Javascript side and provides the kind of file namespaces that developers
    are likely to need.
*   This design does not involve adding too many NaCl-specific interfaces to
    trusted code.

### Receiving messages from NaCl asynchronously in Javascript

The current NaCl Javascript API does not allow Javascript code to receive
messages asynchronously from NaCl processes. We propose to extend the Javascript
API to allow this. Javascript will need to be able to receive `open()` requests
from the NaCl process. Currently the only way to do this is to busy-wait.

Implementing this in the NaCl NPAPI plugin will require using
[NPN\_PluginThreadAsyncCall()]
(https://developer.mozilla.org/en/NPN_PluginThreadAsyncCall).

### Initial socket connections

The current interface assumes that the Javascript code will be sending requests
to the NaCl process. The NaCl plugin creates the NaCl process with a BoundSocket
descriptor. The NaCl process is expected to start by going into an
`imc_accept()` loop on this descriptor to receive connections from Javascript.

We would like to remove this assumption and allow the reverse arrangement. It
should be possible to start the NaCl process with a SocketAddress descriptor --
or ideally, an array of NaCl descriptors of any descriptor type. The NaCl
process should be able to send `open()` requests early on and should not need to
call `imc_accept()` on startup.

### Prototype implementation

I wrote a prototype of this earlier in 2009. As an example web app I implemented
a [Python read-eval-print loop (REPL)]
(http://lackingrhoticity.blogspot.com/2009/06/python-standard-library-in-native.html),
using CPython running under Native Client using dynamic linking. It is able to
use Python extension modules such as Sqlite. The prototype works in Firefox on
Linux.

The code is in Git: * [hello.html]
(http://repo.or.cz/w/nativeclient.git/blob/7b77b13ebfae704ac4492827d4431b5e70789c37:/imcplugin/hello.html):
contains the Javascript side of the Python REPL * [imcplugin.c]
(http://repo.or.cz/w/nativeclient.git/blob/7b77b13ebfae704ac4492827d4431b5e70789c37:/imcplugin/imcplugin.c):
a minimal trusted NPAPI plugin for Native Client allowing Javascript to send and
receive asynchrous messages * [demo.py]
(http://repo.or.cz/w/nativeclient.git/blob/7b77b13ebfae704ac4492827d4431b5e70789c37:/imcplugin/demo.py):
the Python code

`imcplugin` provides the following interfaces to Javascript:

*   `plugin.get_file(url_string, function(nacl_file) { ... })`

> Fetches a file from the given URL. When the file becomes available, the plugin
> calls the callback function passing a Javascript wrapper object for a NaCl
> file descriptor. This simple interface lacks error handling for when the URL
> cannot be fetched.

*   `plugin.launch(nacl_file, [arg1, arg2, ...], function(msg) { ... }) -> proc`

> Spawns a NaCl process. Under the hood, this runs `sel_ldr`. * Takes a NaCl
> file object specifying the executable to run. * Takes an array of strings to
> pass as command line arguments which the NaCl process receives via main(). *
> Takes a callback function which receives messages from the NaCl process. Each
> message is a string. * Returns an object which can be used to send messages to
> the process.

*   `proc.send(string_arg, [nacl_file, ...])`

> Sends a message to the process. Messages consist of an array of bytes
> (represented as a Javascript string) and an array of file descriptor wrapper
> objects. (The latter array may of course be empty.)

### Call-return over IMC

There are two ways we might implement call-return on top of IMC sockets.

Option 1: Use the same channel, C, for sending and receiving: * `imc_sendmsg(C,
request)` * `imc_recvmsg(C) -> reply` * This does not allow the channel to be
shared between processes.

Option 2: Create a new channel for each request: * `imc_connect(C) -> D` *
`imc_sendmsg(D, request)` * `imc_recvmsg(D) -> reply` * `close(D)`

See [IMCSockets](imc_sockets.md) for a further discussion.

### Questions

How will this interact with Web Workers?

## Sharing libraries across sites

It will be desirable to share library files across sites, so that the browser
does not have to download identical files multiple times. This issue already
occurs for Javascript libraries. NaCl executables and libraries are expected to
be larger than Javascript libraries which makes this issue more important for
NaCl.

### Background: Same Origin Policy

XMLHttpRequest is constrained by a Same Origin Policy (SOP). `__urlAsNaClDesc`
will also be constrained by a SOP. (Note that the NaCl NPAPI plugin has to
implement the SOP itself because NPAPI does not provide a way to reuse the
browser's SOP.)

The main reason for the SOP is that XMLHttpRequest requests convey cookies -- a
type of [ambient authority](http://en.wikipedia.org/wiki/Ambient_authority). The
Same Origin Policy is not intended to prevent web apps from sending messages
across origins; it is only intended to prevent the web app from seeing the
server's response to the request. (Sending cross-origin messages can already be
done using mechanisms other than XMLHttpRequest, including redirects and `<img>`
elements.)

### Comparison: `script` element

Loading libraries in NaCl is analogous to loading Javascript files via the
`<script src=...>` element. Interestingly, `<script>` is not constrained by the
SOP. By setting the response's content-type to `text/javascript`, the server
effectively opts in to revealing the response to the web app. Supposedly, the
response is not revealed directly to the web app. The DOM, a trusted part of the
browser, evaluates the Javascript code, and the web app gets access only to the
values the script assigns to variables. In practice, one cannot rely on
`text/javascript` data from being revealed across origins.

In NaCl's case, however, interpreting .so files is unambiguously the
responsibility of untrusted code. We have to reveal the fetched data to the web
app, so NaCl cannot be as unconstrained as the `<script>` tag.

The `<script>` element permits a centralised model for sharing library code.
Suppose multiple web apps use the library `libjfoo.js`. If this is hosted at
`http://libjfoo.org/libjfoo-1.0.js`, the web apps can opt to link to this URL.
The down side of using the `<script>` element in this way is that the web apps
will be vulnerable to the centralised site, `libjfoo.org`. This site can change
the file contents it serves up (there was [an example of this happening with
json.org's copy of json.js]
(http://www.stevesouders.com/blog/2009/12/10/crockford-alert/)) and thereby run
arbitrary Javascript in the context of the web apps. Since the script text is
not available across origins, the web app cannot check the text against a hash
before `eval`'ing it.

### Fetching libraries across origins

For NaCl, web apps could fetch libraries using [Uniform Messaging]
(http://lists.w3.org/Archives/Public/public-webapps/2009OctDec/att-0931/draft.html)
(formerly known as GuestXHR) or [CORS](http://www.w3.org/Security/wiki/CORS),
which are not NaCl-specific.

We might also wish to allow decentralised sharing of files. For example, sites A
and B both host `libfoo.so`. If the browser has already downloaded `libfoo.so`
from site A, it won't need to download it again from site B, and vice-versa.
Schemes for doing this by embedding secure hashes into URLs have been proposed;
for example, see [Douglas Crockford's post]
(http://profiles.yahoo.com/blog/GSBHPXZFNRM2QRAP3PXNGFMFVU?eid=vbNraNs6kXn9E4kaLDYAml5ESuTWLnf9pNVJDWj5zGMu8Ltwiw).

This problem is not unique to NaCl, so we should not adopt a solution which is
NaCl-specific.

## Trust relationship between Javascript and NaCl process

In the above scheme, there are two principals: * the untrusted NaCl process
running under the NaCl trusted runtime; * the Javascript code running on the web
page under the browser

The NaCl process depends on the Javascript code to provide its execution
environment. The Javascript code provides all the code running in the NaCl
process. The Javascript code therefore has at least as much authority as the
NaCl process.

This is at odds with the current same origin policy, described in [issue 238]
(http://code.google.com/p/nativeclient/issues/detail?id=238). In the current
scheme, executable `http://a.com/foo.nexe` can be embedded in the page
`http://b.com`. Javascript on `b.com`'s page can use `__urlAsNaClDesc()` to
fetch an `a.com` URL, getting a file descriptor in return. However, only the
NaCl process can use the file descriptor to read the file's contents. The NaCl
process therefore has strictly greater authority than the Javascript. However,
it has no trusted path for fetching files from `a.com`. This is a dangerous
situation which is likely to lead to XRSF-like Confused Deputy vulnerabilities.
`foo.nexe` is expected to distrust the messages and file descriptors it receives
from the page; this is difficult or impossible to achieve. It is incompatible
with the dynamic library scenario above in which the NaCl process must trust the
library data supplied by the page.

We propose that if `__urlAsNaClDesc()` (or a similar API) is to follow a same
origin policy at all, it should use the origin of the page, not the origin of
the executable's URL.

It may be that directly embedding a NaCl plugin object across origins should not
be permitted at all. In this case, it would still be possible to embed a NaCl
plugin object across origins indirectly, through a cross-origin iframe. In such
a scenario, one is embedding a combination of Javascript and NaCl code in which
the latter can legitimately trust the former.

## Prefetching files

The simplest approach to fetching library files is to fetch them one by one, as
ld.so does synchronous open() calls. However, this means the inbound network
connection will be idle after the end of a file is received by the client and
before ld.so's request for the next file is received by the server. This costs
one network round trip per file.

We could reduce the time taken to fetch the whole set of files by pipelining the
requests. A simple way to do this, which does not involve changing the dynamic
linker, is to list up-front all the libraries we expect to load. The Javascript
code could request the files on startup in order to pre-populate the browser's
cache.

## Versioning

As with static linking, each web app gets to choose its own version of libc and
other libraries. Furthermore, different NaCl processes in the same web app can
use different libc versions. Libc is not supplied by the browser.

We don't expect there to be a huge number of libc versions, but older and newer
versions of the same libc are likely to be around at the same time, as are
different libc implementations (such as newlib and glibc).

Web apps get to pick a set of libraries that are known to work well together.
This is analogous to selecting a set of Javascript libraries, or selecting a set
of packages for a software distribution such as Debian or Fedora. This way we
can avoid "[DLL hell](http://en.wikipedia.org/wiki/Dll_hell)"; libraries are not
the responsibility of the end user.

This provides extra flexibility that is not available to typical applications on
Linux when packaged with commonly-used packaging systems like dpkg or RPM.
Packaging systems such as Zero-Install and Nix allow multiple library versions
to coexist in the same way that I am proposing for NaCl.

Though we have this extra flexibility we will still have all the versioning
mechanisms that are available in ELF shared libraries normally: libraries can
opt to provide stable ABIs and declare interfaces via [sonames]
(http://en.wikipedia.org/wiki/Soname) and ELF symbol versioning; we get the
benefit of separate compilation.

Upgrading libraries is the responsibility of the web app. A web app may choose
to delegate this responsibility to another site by fetching libraries from that
site.
