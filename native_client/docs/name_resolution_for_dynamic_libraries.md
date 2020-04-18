# Name resolution for dynamic libraries

_Draft_

**Please send comments to [native-client-discuss]
(https://groups.google.com/group/native-client-discuss)** rather than adding
them inline or using the comment form, because the wiki does not send
notifications about changes.

## Introduction

An important question for implementing dynamic library loading in Native Client
is how libraries are named by executables and other libraries, and how the
dynamic linker acquires libraries given these names.

The current plan for dynamic library support is to use ELF, the standard used in
Linux and various other Unixes. In ELF, a dynamic library is named by a short
"[soname](http://en.wikipedia.org/wiki/Soname)", such as "libncurses.so.5".

> _Background note: The version number in a soname is a major version number.
> This is only changed when the ABI of a library is changed in an incompatible
> way. When a library is evolving in an ABI-compatible way (by adding but not
> removing symbols, keeping struct layouts the same, etc.), the soname stays the
> same. For example, Linux glibc provides "libc.so.6", and the ".6" part will
> stay the same for the foreseeable future._

On a Unix system, the responsibility for mapping a soname to a specific library
file is effectively split between two components:

*   The OS provides a filesystem namespace (typically shared between all
    processes) which is populated with libraries when the application and its
    dependencies are _installed_. This installation is often done by a _package
    manager_.
*   The dynamic linker treats the soname as a filename. To find "libfoo.so.123",
    it tries to open "libfoo.so.123" in each directory in the _library search
    path_.

In the web browser, we do not have the same concept of installation: There is no
local filesystem namespace that is shared between all programs. We would
typically want to acquire library files by fetching them from URLs. In this case
the question becomes how to map sonames to URLs, and from URLs to files.

> _Background note: The ELF dynamic linker is a userland program on Unix, not
> part of the kernel. Similarly, in Native Client, the dynamic linker runs
> inside the NaCl sandbox, and is not part of the NaCl trusted runtime._

## Overview

In this document, we propose that the mapping from sonames to files, via URLs,
should happen outside the dynamic linker. This minimises the changes to the
dynamic linker and is also more flexible.

We discuss three ways of specifying the mapping from sonames to files: a
pure-data manifest file format, and two more general mechanisms for defining a
mapping programmatically.

We propose a kernelised implementation that offers a choice between these
mechanisms. Simple web apps would use manifest files. The manifest files would
be interpreted by an untrusted Javascript library that is built on simpler
primitives. More sophisticated web apps would be able to use these primitives
for setting up filename mappings programmatically.

## Goals

Usability: * **Convenience**: It should be easy to create a web app that uses
dynamic libraries. The Javascript/HTML/metadata for setting this up should be
concise. * Support running **legacy applications** with minimal modifications. *
We should not need to make intrusive changes to the build systems of existing
libraries in order to use them with Native Client. * The dynamic linker itself
is a legacy program. Our solution for C dynamic libraries should readily extend
to the library loaders for other languages. * **Data files**: The mechanism that
we use to supply library files should also be usable for supplying data files
that aren't ELF/PNaCl libraries, such as locale data files or Python's `.py` or
`.pyc` library files. * **Testability**: Ensure that NaCl-based web apps are
easy to test. It should be easy to run an isolated instance of a web app on a
test server without depending on production instances. Fixing URLs in binary
files would make this harder. * **Avoid "DLL hell"**: * Allow a web app author
to specify a fixed set of versions of libraries that are known to work together.
* Allow a web app author to delegate the responsibility of picking library
versions to another site (for example, to receive security updates). * **Error
handling**: If the dynamic linker fails to load the executable and its
dependencies, there should be a mechanism for getting the error message. This
should work both during development, and in production, when a web app is
deployed. * **Standard libraries**: Provide a route by which the NaCl plugin can
supply some standard libraries. This might include the dynamic linker, libc,
PPAPI bindings (libppruntime) and OpenGL libraries.

Security considerations: * **Minimise trusted code**: Implementing logic in
trusted code unnecessarily increases the chance of vulnerabilities and makes the
system less flexible. * **Integrity**: Ensure that the library received is the
one intended, even over an insecure network. Rather than referring to a file by
an HTTP URL on its own, a manifest file could refer to an (HTTP URL, secure
hash) pair, where the hash is checked after download. This would allow, for
example, a page served over HTTPS to link to libraries provided over HTTP
without violating the web app's integrity. It can also aid caching. Such
hash+location pairs are known as _strong names_. * Support **secure
compartmentalisation** of NaCl web apps: It should be possible to create NaCl
processes without the ability to access the DOM, fetch arbitrary URLs, etc. Web
apps may want to run NaCl programs without trusting them with DOM access and
network access. However, such programs may still need dynamic libraries. *
**3-party library distro service**: Suppose site A wishes to use an up-to-date
version of a library as specified by site B. (Site B acts as a "distro service".
Like a Linux distribution, it would pick versions of libraries known to work
well together. For example, NaCl Ports would be such a service.) Site B should
not need to host the library itself; it should be able to link to site C. This
should be possible without making site A vulnerable to site B. This would
require using UMP rather than CORS.

Performance and remote loading: * Support **pipelined fetching** of libraries in
order to minimise latency: When an executable _transitively_ depends on a set of
libraries, it should be possible to queue the fetch requests for all the
libraries and the executable in one go, so that the download pipe is not empty
for a round trip between requests. * Support **cross-origin fetching** of
libraries, including [UMP](http://www.w3.org/TR/UMP/) as well as [CORS]
(http://www.w3.org/TR/cors/): It should be possible for a web app on foo.com to
use libraries hosted on bar.org. (CORS and UMP would require bar.org to set an
HTTP header to opt in to the browser revealing responses across origins.) *
Support **decentralised sharing** of libraries: If foo.com and bar.org host
identical copies of a library, using web apps on these sites should not cause
the library to be downloaded twice. Such caching could be enabled by linking to
the file using a (URL, hash) pair -- see "Integrity" -- using a "[share-by-hash]
(share_by_hash.md)" scheme. This may be implemented by the [PNaCl translation
cache](pnacl_translation_cache.md). * Support **local storage** of libraries:
Allow libraries to be stored in locations other than the browser's download
cache, such as in the local storage provided by the proposed ["File API:
Directories and System"]
(http://dev.w3.org/2009/dap/file-system/file-dir-sys.html) extension to the Web
[File API](http://www.w3.org/TR/FileAPI/). Allow a web app to manage its own
cache. * **Copy avoidance**: We want to avoid copying library data
unnecessarily. The dynamic linker should receive library files as file
descriptors. * Allow **progress bars**: A large web app should be able to report
the progress of library downloading and translation.

Maintenance issues: * **PNaCl** support: We do not want to make assumptions that
make it difficult to support PNaCl later on. * Use a **common libc build**:
Allow the same dynamic linker, libc and other libraries to work in both the
browser and non-browser cases. If we have to build different versions for inside
and outside the browser, it will be a maintenance burden. * **Upstreamability**
of libc: We do not want to keep difficult-to-update glibc changes in our branch
in the long term.

## Options for describing a soname mapping

We explore three related options for how the mapping from sonames to files might
be specified:

*   Option A: Via a manifest file that lists soname/URL mappings explicitly
*   Option B: Via a data structure, constructed programmatically at run time,
    which lists mappings explicitly
*   Option C: Via a mapping service object, supplied by the web app, which
    receives soname requests and returns files in return

These options are not mutually exclusive: Options A and B can be implemented in
terms of C. In principle A could be implemented inside the dynamic linker
itself, but B and C would have to be implemented outside the dynamic linker.
Since option C is the most general, we propose to implement that and provide
options A and B via a standard library implemented in untrusted code.

### Option A: Manifest file

The mapping from sonames to files could be expressed as a data structure that
explicits lists all entries in the mapping. This data structure could be
serialised in a format such as JSON or XML. The data structure could be an
extension of the JSON format that the NaCl plugin currently uses for selecting
an architecture-specific executable URL (implemented in issue 885 (on Google
Code)).

Until PNaCl is available, such a file would list binaries for each architecture.
For example:

```
{
  "executable": {
    "arch_urls": {
      "x86-32": "hello_world.x86-32.nexe",
      "x86-64": "hello_world.x86-64.nexe",
      "arm": "hello_world.arm.nexe",
    },
  },
  "libraries": {
    "/lib/libz.so.1": {
      "arch_urls": {
        "x86-32": "http://example.com/lib/libz.so.1.0.x86-32",
        "x86-64": "http://example.com/lib/libz.so.1.0.x86-64",
        "arm": "http://example.com/lib/libz.so.1.0.arm",
      },
    },
    "/lib/libfoo.so.1": {
      "arch_urls": {
        "x86-32": "http://foo.com/libraries/libfoo.so.1.123.x86-32",
        "x86-64": "http://foo.com/libraries/libfoo.so.1.123.arm",
        "arm": "http://foo.com/libraries/libfoo.so.1.123.arm",
      },
    },
  },
}
```

An architecture-neutral, PNaCl-based manifest file would be more concise:

```
{
  "executable": {
    "pnacl_url": "hello_world.bc",
  },
  "libraries": {
    "/lib/libz.so.1": {"pnacl_url": "http://example.com/lib/libz.so.1.0.bc"},
    "/lib/libfoo.so.1": {"pnacl_url": "http://foo.com/libraries/libfoo.so.1.123.bc"},
  },
}
```

The mapping subsystem that interprets this file would be responsible for
fetching the URL that corresponds to the machine's architecture (from
`arch_urls`) or for fetching the PNaCl URL and piping the bitcode data through a
PNaCl translator for the machine.

Such a manifest format is potentially extensible, and could include other
features:

*   Prefetching: To ensure pipelined fetching of library files, manifest entries
    could contain a "prefetch" flag to specify whether a file should be
    unconditionally fetched (and translated) when the manifest file is loaded.
    The default could be to prefetch. If an application uses libraries that are
    conditionally dlopen()'d, these would not need to be prefetched.
*   Cross-origin fetching: The format could provide flags to choose between
    same-origin and cross-origin HTTP fetching, and to specify whether to send
    HTTP credentials (UMP vs. CORS).
*   Integrity: File entries could include a cryptographic hash to use as an
    integrity check. This could make it safe to load a library over HTTP from a
    page served over HTTPS. Of course, this use of hashes would mean that the
    manifest file would need to be updated any time we want it to point to a new
    version of a library. However, we would expect non-trivial manifest files to
    be generated by a build system or package system, in which case the update
    would not be a burden.
*   Data files: This format can be used to list data files as well as libraries.

However, there are some disadvantages to using a manifest file, if the manifest
file is implemented by Native Client as built-in functionality. The mapping
would be static, in the sense that there would not necessarily be a way to alter
the mapping after a NaCl process has been launched. This would make it harder to
support some use cases:

*   Suppose an application wishes to dlopen() a library from a URL that is only
    known after application startup. dlopen() opens a soname using the same
    mapping process as startup-time library loading, but if this mapping is
    unalterable we would have no way of getting dlopen() to use the new URL.
*   It would be awkward to make this work with "File API" files. These files are
    represented as Javascript objects, but they can also be converted into
    locally-usable "Blob URIs" via createObjectURL(). A Blob URI could be
    inserted into a programmatically-generated manifest file, which could be
    passed to the NaCl plugin as a "data:" URL. However, this seems awkward: it
    would negate the benefit of a having a manifest file, namely that the file
    is static and does not need to be constructed by Javascript.

### Option B: Programmatic construction of a soname mapping

Use of a manifest file requires the soname mapping to be represented as a
serialised data structure containing pure data. An alternative would be to allow
the mapping data structure to be constructed programmatically in Javascript.

An example of this might be the following:

```
var plugin = document.getElementById("nacl_plugin");
// Get the filename/soname mapping object.
var mapping = plugin.getMapping();

// Request a file to be downloaded and get a promise object representing it.
var pnacl_file = plugin.fetchUrl("http://example.com/lib/libz.so.1.0.bc");
// Request a translated version of a file.  Also returns a promise.
var library_file = plugin.translatePnaclFile(pnacl_file);
// Bind the translated file into the file namespace.
mapping.bindFile("/lib/libz.so.1", library_file);
```

This code fragment makes use of _[promises]
(http://en.wikipedia.org/wiki/Promise_(programming))_ (sometimes called
_futures_), which represent files that have been requested to be fetched or
generated, for which the result might not yet be available. This makes it
convenient to set up a pipeline that will complete asynchronously without having
to write asynchronous callback functions. When the dynamic linker requests the
file "/lib/libz.so.1", it would block until the file is available (both
downloaded and translated).

This type of description has some advantages over static manifest files:

*   It can reference local file objects without these files needing to have
    string identifiers. For example, a local "File API" file object could be
    usable with bindFile() or translatePnaclFile(). It is easier to manage file
    lifetimes when files are referenced through object references, because a
    file can be freed when object references to it have been dropped, but we
    cannot tell when string ID references have been dropped.
*   It can be more easily extended to work with other types of file. We could
    provide variants of fetchUrl() that use UMP and CORS.
*   It avoids the need to fix a concrete file format. Instead, we would define a
    method call API.
*   A programmatic definition could be more concise since repetition can be
    avoided.
*   The mapping of libraries can be changed after the NaCl process has started.
*   A function-based interface can handle errors more gracefully. If the method
    translatePnaclFile() is not available, for example, it would raise an
    exception. If the method is a recently-added feature, a web app can catch
    the exception and provide fallback logic. In contrast, an unsupported field
    in JSON would likely get ignored without a warning or an error.

At the same time, it would be easy to implement a manifest file reader on top of
a programmatic interface.

### Option C: Mapping object, supplied by web app

In the previous section, a mapping data structure was populated from Javascript
by bindFile() calls. A logical extension would be to allow the data structure to
be implemented in Javascript. This would mean the mapping would not be confined
to being an enumeration of explicit name/file mappings.

A simple interface would be for the mapping to be specified as a function that
takes a library name or filename as an argument. An example of a simple mapping
would be to add a URL prefix to the filename:

```
var plugin = document.getElementById("nacl_plugin");

function ResolveFilename(name) {
    // Assume libraries are always bitcode files in one directory.
    var url = "http://example.com/my-nacl-app/" + name + ".bc";
    return plugin.translatePnaclFile(plugin.fetchUrl(url));
}
plugin.setFilenameResolver(ResolveFilename);
```

Again, this example uses promises, so that the fetchUrl() and
translatePnaclFile() immediately return promise objects.

Allowing the mapping to be specified as a function has these advantages: * It
removes the need to implement a name mapping facility in trusted code. * It
allows the names that the dynamic linker requests to be logged. This would be
useful for debugging file-not-found errors when loading libraries. It could help
a web app author to determine which libraries should be in a prefetch list. * It
removes the need to enumerate every file in the mapping. Catch-alls are
possible. * It would make it easier to compose filename mappings together. With
getMapping()/bindFile() from the previous section, the only possibility is to
add files to one mapping. With mapping functions, one function could delegate to
one or more other functions. * It can readily be extended to support other
filesystem operations, such as listing directories, without requiring changes to
trusted code.

Both the manifest file and the bindFile() interface could be implemented on top
of the setFilenameResolver() interface above.

#### Concurrency

There is one disadvantage to setFilenameResolver() over bindFile(): The
Javascript ResolveFilename() callback function can only process requests when
the renderer process's event loop is not busy. During a long-running event loop
turn, library open requests will wait in a message queue. In contrast, a mapping
implemented in trusted code, populated by bindFile(), could process requests in
a separate thread.

## Implementation

We plan to implement option C since it is the most general while requiring the
least trusted code.

The TCB functionality needed for this was implemented in the NPAPI version of
the NaCl plugin (working in both Chromium and Firefox), but not all of this
functionality works in the current PPAPI version of the NaCl plugin. We will
need to do the following:

*   Ensure that the asynchronous messaging interface added in issue 642 (on
    Google Code) works in the NaCl PPAPI plugin. Sending messages from
    Javascript should work, but receiving messages via a Javascript callback
    requires PPAPI-specific code.
*   Ensure that the `__UrlAsNaClDesc()` method (or equivalent functionality)
    works in the NaCl PPAPI plugin.

In addition, we plan to provide options A and B (manifest files and the
bindFile() call) as a standard library implemented in Javascript. This will be
built into the NaCl plugin, which can load it with `eval`.

## Development tools

For programs with a non-trivial set of dependencies, creating manifest files
manually would be a burden. Therefore, we will provide a tool to generate a
manifest file from an executable and a set of libraries. The tool would omit
unnecessary libraries and include library hashes.

### Upgrading libraries atomically

When upgrading libraries, it is often desirable to upgrade multiple libraries at
the same time, in lockstep, to a combination of versions that has been tested.
This would be a matter of replacing one manifest file with another.

To achieve this atomicity, library URLs should be treated as identifiers for
immutable resources. Otherwise, if the file that a URL points to is changed, a
page load might load an inconsistent set of libraries.

The tool we provide for generating manifest files can take care of ensuring that
new versions of libraries are deployed under unique filenames.

## Alternative approaches considered

### Alternative #1: Library search path

A normal ELF dynamic linker uses a _library search path_: a list of directories
to search for libraries. This list can be modified by setting the
LD\_LIBRARY\_PATH environment variable, and ELF objects (executables and
libraries) can add to the list using ELF's [RPATH]
(http://en.wikipedia.org/wiki/Rpath_(linking)) feature. The default is typically
`["/lib", "/usr/lib"]`.

We could extend this concept to use a list of URLs rather than directories. We
could provide a mechanism for a web app to set the search path. As an example,
if the path is set to `["http://example.com/lib", "http://foo.com/libraries"]`,
then when the dynamic linker requires "libfoo.so.1", the system would first try
to fetch `http://example.com/lib/libfoo.so.1`. If that URL returned a 404
response, the linker would instead try fetching
`http://foo.com/libraries/libfoo.so.1`.

The problems with this arrangement are obvious: It generates unnecessary traffic
only to receive failures. If we do the search requests serially, it requires
mulitple round trips to locate a library. If we do the search requests in
parallel, it generates even more unnecessary traffic. This is therefore not a
viable option, but we include it only as a strawman for comparison with other
options.

### Alternative #2: Interpreting manifest files inside the dynamic linker

In principle it would be possible to interpret a manifest file inside the
dynamic linker. We consider this arrangement for completeness, but it does not
appear to have any advantages. It would require a lot of functionality to be
placed in the dynamic linker, which would be difficult to implement while making
it harder to evolve the system.

Firstly, this would require a mechanism for the NaCl plugin to send the manifest
file to the dynamic linker.

Secondly, the dynamic linker would need to implement all the features of the
manifest file format: * Parsing: We would need to include a JSON parser in the
dynamic linker, if the manifest file format were based on JSON. * PNaCl: To
support PNaCl, the dynamic linker would need to be given the ability to invoke
the PNaCl translator. For the PNaCl translator to provide any cross-origin
caching, the translator would have to be exposed via IPC. * Cross-origin
fetching: If the manifest file provides a choice between same-origin and
cross-origin fetching mechanisms, the dynamic linker would need knowledge of
these. * Integrity checks: If the manifest file format allows a "hash" field for
each file, the hash checking would need to be done in the dynamic linker. *
Pipelining: Any prefetching logic would need to be implemented in the dynamic
linker.

Use of local storage based on the File API would be possible, but would require
a manifest file containing Blob URIs to be generated dynamically in Javascript,
negating the benefit of having a static manifest file, as discussed earlier.

The dynamic linker would be need to be given the ability to fetch URLs. This
tends to be incompatible with secure compartmentalisation, because it would make
it difficult to run a dynamically-linked NaCl process that has not been granted
network access.

#### Bootstrap dependency difficulties at run time and build time

To request URLs, the dynamic linker would need to talk to the NaCl browser
plugin. To do this with the current NaCl plugin, it would need to speak the
PPAPI-over-SRPC-over-IMC protocol that the NaCl browser plugin implements.
However, this protocol is currently not stable.

Assuming this protocol remains unstable, the dynamic linker would probably need
to be linked to the NaCl PPAPI proxy library (libppruntime). However, we cannot
use the dynamically-linked version of libppruntime, since this would obviously
create a Catch-22 situation: In order to fetch a URL we need libppruntime
loaded, but we need libppruntime loaded before we can fetch URLs. So we would
need to statically link libppruntime and its dependencies, which currently
include libsrpc. This would create some problems:

*   **Bootstrap constraints**: During startup, libc is not yet loaded when the
    dynamic linker runs. In order for the dynamic linker to function, it must
    statically link against a subset of libc and many libc functions are not
    available. For example, simple versions of malloc() and free() are available
    and free() only frees memory for some allocation patterns. libppruntime and
    libsrpc would need to be made to work in this context, which would likely be
    non-trivial. libppruntime is currently written in C++ and uses libstdc++.
    However, C++ and libstdc++ are not available in the NaCl toolchain build
    until after we have built nacl-glibc, because libstdc++ depends on libc.
    This means that, since the dynamic linker is built as part of nacl-glibc, it
    is difficult to statically link it against C++ code. libppruntime also uses
    libpthread in some cases.

*   **Ongoing maintenance**: If libppruntime and libsrpc were modified so that
    they can be statically linked into the dynamic linker, every change to
    libppruntime or libsrpc would require that the dynamic linker be rebuilt and
    tested. For example, it would be necessary to verify that libppruntime did
    not gain extra dependencies that do not work inside the dynamic linker.
    Rebuilding and testing the dynamic linker every time the other libraries
    change is time consuming because nacl-glibc takes a long time to build.
    Currently, libppruntime developers do not need to rebuild nacl-glibc.

*   **Connection sharing**: If libppruntime is statically linked into the
    dynamic linker and a dynamically-linked version of libppruntime is used by a
    NaCl application, there would be two instances of libppruntime in the same
    process and this could cause problems. For example, the NaCl browser plugin
    expects to have one PPAPI-over-SRPC connection to the NaCl subprocess.
    Either the two libppruntime instances would have to share this connection or
    the NaCl browser plugin would have to be changed to handle multiple
    connections.

A solution to these problems would be to stabilise the interface presented by
the PPAPI-over-IMC protocol, or at least stabilise a subset of the protocol for
loading URLs. A minimal version of libppruntime could be written that is
suitable for statically linking into the dynamic linker.

### Alternative #3: Replacing sonames with URLs

We could change our use of ELF so that instead of storing short sonames in the
DT\_NEEDED fields of executables and libraries, we store URLs. However, this
would create a number of problems:

*   **Testability**: It would be difficult to test executables with embedded
    URLs in isolation from production systems.

*   **Version management**: Loading two different versions of a library into one
    process does not always work with ELF, because it can cause symbol
    conflicts. If libraries declared dependencies on specific versions of
    libraries via URLs, it would make conflicts more likely than with sonames. A
    soname usually specifies an interface rather than a specific implementation.
    If two libraries both depend on the soname "libfoo.so.1", at runtime they
    will both receive the same version of "libfoo.so.1".

*   **Existing libraries**: The build systems for existing software will expect
    to link libraries with short sonames. For example, libfoo might be linked
    with the option "-Wl,-soname,libfoo.so.1". Using URLs instead would require
    all these locations to be changed, and a conditional case would need to be
    added to support Native Client alongside other platforms.

*   **Local storage**: This does not a offer a way for a web app to manage its
    own caching by copying libraries to local storage.

*   **Secure compartmentalisation**: Every NaCl process would need the ability
    to request arbitrary URLs, so running processes without network access would
    be difficult.

*   **Pipelining**: An ELF object's DT\_NEEDED dependencies are only known after
    the ELF object has been fetched. Although we could queue the fetches for
    these _direct_ dependencies in parallel, this will not lead to optimal
    pipelining. There would be a pipeline stall between loading the executable
    and its immediate library dependencies, and another stall before indirect
    library dependencies are received, etc.

Similarly, we could change the interpretation of the ELF RPATH field to be a URL
prefix, but this has the same testability and compartmentalisation problems as
putting URLs in DT\_NEEDED, as well as the additional problems associated with
library search paths described above.
