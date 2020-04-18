# Mojo

[TOC]

## Getting Started With Mojo

To get started using Mojo in applications which already support it (such as
Chrome), the fastest path forward will be to look at the bindings documentation
for your language of choice ([**C++**](#C_Bindings),
[**JavaScript**](#JavaScript-Bindings), or [**Java**](#Java-Bindings)) as well
as the documentation for the
[**Mojom IDL and bindings generator**](/mojo/public/tools/bindings/README.md).

If you're looking for information on creating and/or connecting to services, see
the top-level [Services documentation](/services/README.md).

For specific details regarding the conversion of old things to new things, check
out [Converting Legacy Chrome IPC To Mojo](/ipc/README.md).

## System Overview

Mojo is a layered collection of runtime libraries providing a platform-agnostic
abstraction of common IPC primitives, a message IDL format, and a bindings
library with code generation for multiple target languages to facilitate
convenient message passing across arbitrary inter- and intra-process boundaries.

The documentation here is segmented according to the different isolated layers
and libraries comprising the system. The basic hierarchy of features is as
follows:

![Mojo Library Layering: EDK on bottom, different language bindings on top, public system support APIs in the middle](https://docs.google.com/drawings/d/1RwhzKblXUZw-zhy_KDVobAYprYSqxZzopXTUsbwzDPw/pub?w=570&h=324)

## Embedder Development Kit (EDK)
Every process to be interconnected via Mojo IPC is called a **Mojo embedder**
and needs to embed the
[**Embedder Development Kit (EDK)**](/mojo/edk/embedder/README.md) library. The
EDK exposes the means for an embedder to physically connect one process to another
using any supported native IPC primitive (*e.g.,* a UNIX domain socket or
Windows named pipe) on the host platform.

Details regarding where and how an application process actually embeds and
configures the EDK are generaly hidden from the rest of the application code,
and applications instead use the public System and Bindings APIs to get things
done within processes that embed Mojo.

## C System API
Once the EDK is initialized within a process, the public
[**C System API**](/mojo/public/c/system/README.md) is usable on any thread for
the remainder of the process's lifetime. This is a lightweight API with a
relatively small (and eventually stable) ABI. Typically this API is not used
directly, but it is the foundation upon which all remaining upper layers are
built. It exposes the fundamental capabilities to create and interact with
various types of Mojo handles including **message pipes**, **data pipes**, and
**shared buffers**.

## High-Level System APIs

There is a relatively small, higher-level system API for each supported
language, built upon the low-level C API. Like the C API, direct usage of these
system APIs is rare compared to the bindings APIs, but it is sometimes desirable
or necessary.

### C++
The [**C++ System API**](/mojo/public/cpp/system/README.md) provides a layer of
C++ helper classes and functions to make safe System API usage easier:
strongly-typed handle scopers, synchronous waiting operations, system handle
wrapping and unwrapping helpers, common handle operations, and utilities for
more easily watching handle state changes.

### JavaScript
The [**JavaScript System API**](/third_party/blink/renderer/core/mojo/README.md)
exposes the Mojo primitives to JavaScript, covering all basic functionality of the
low-level C API.

### Java
The [**Java System API**](/mojo/public/java/system/README.md) provides helper
classes for working with Mojo primitives, covering all basic functionality of
the low-level C API.

## High-Level Bindings APIs
Typically developers do not use raw message pipe I/O directly, but instead
define some set of interfaces which are used to generate code that resembles
an idiomatic method-calling interface in the target language of choice. This is
the bindings layer.

### Mojom IDL and Bindings Generator
Interfaces are defined using the
[**Mojom IDL**](/mojo/public/tools/bindings/README.md), which can be fed to the
[**bindings generator**](/mojo/public/tools/bindings/README.md) to generate code
in various supported languages. Generated code manages serialization and
deserialization of messages between interface clients and implementations,
simplifying the code -- and ultimately hiding the message pipe -- on either side
of an interface connection.

### C++ Bindings
By far the most commonly used API defined by Mojo, the
[**C++ Bindings API**](/mojo/public/cpp/bindings/README.md) exposes a robust set
of features for interacting with message pipes via generated C++ bindings code,
including support for sets of related bindings endpoints, associated interfaces,
nested sync IPC, versioning, bad-message reporting, arbitrary message filter
injection, and convenient test facilities.

### JavaScript Bindings
The [**JavaScript Bindings API**](/mojo/public/js/README.md) provides helper
classes for working with JavaScript code emitted by the bindings generator.

### Java Bindings
The [**Java Bindings API**](/mojo/public/java/bindings/README.md) provides
helper classes for working with Java code emitted by the bindings generator.

## FAQ

### Why not protobuf? Why a new thing?
There are number of potentially decent answers to this question, but the
deal-breaker is that a useful IPC mechanism must support transfer of native
object handles (*e.g.* file descriptors) across process boundaries. Other
non-new IPC things that do support this capability (*e.g.* D-Bus) have their own
substantial deficiencies.

### Are message pipes expensive?
No. As an implementation detail, creating a message pipe is essentially
generating two random numbers and stuffing them into a hash table, along with a
few tiny heap allocations.

### So really, can I create like, thousands of them?
Yes! Nobody will mind. Create millions if you like. (OK but maybe don't.)

### What are the performance characteristics of Mojo?
Compared to the old IPC in Chrome, making a Mojo call is about 1/3 faster and uses
1/3 fewer context switches. The full data is [available here](https://docs.google.com/document/d/1n7qYjQ5iy8xAkQVMYGqjIy_AXu2_JJtMoAcOOupO_jQ/edit).

### Can I use in-process message pipes?
Yes, and message pipe usage is identical regardless of whether the pipe actually
crosses a process boundary -- in fact this detail is intentionally obscured.

Message pipes which don't cross a process boundary are efficient: sent messages
are never copied, and a write on one end will synchronously modify the message
queue on the other end. When working with generated C++ bindings, for example,
the net result is that an `InterfacePtr` on one thread sending a message to a
`Binding` on another thread (or even the same thread) is effectively a
`PostTask` to the `Binding`'s `TaskRunner` with the added -- but often small --
costs of serialization, deserialization, validation, and some internal routing
logic.

### What about ____?

Please post questions to
[`chromium-mojo@chromium.org`](https://groups.google.com/a/chromium.org/forum/#!forum/chromium-mojo)!
The list is quite responsive.

