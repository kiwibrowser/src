# Problems with existing syscall interface

## Problem 1: IPC facilities are limited

NaCl's [IPC facilities (called "IMC")](imc_sockets.md) do not easily support
capability design patterns. They are a thin wrapper over what happened to be
easy to implement on Windows.

There is no shareable invocation-capability type, such as what is provided in
the KeyKOS/EROS family. Instead, there is a confusing array of socket types,
with different properties. * ConnectedSockets are message streams and can send
shareable descriptors, but are not themselves shareable. * SocketAddresses are
shareable but are likely to be slow. They don't get garbage collected, unlike
ConnectedSockets. * DataOnlySockets are message streams but can't be used to
send descriptors; they are shareable, but not reliably so: concurrent use will
corrupt the messages. * There are no reliable byte stream sockets, but we would
need these to support the semantics of Unix pipes that can be written to
concurrently.

The result is that each application is likely to invent its own combination of
these socket types to suit its own IPC purposes, and there will be no uniform
capability invocation convention.

We could invent a convention
(imc\_connect()+imc\_sendmsg()+imc\_recvmsg()+close() given a SocketAddress),
but it is likely to be slow.

## Problem 2: Some ambient authority remains

NaCl contains a number of ad-hoc [system calls](system_calls.md) such as
gettimeofday() and nanosleep(). Currently there is no way to launch a process
that doesn't have access to timers, for example. This is less serious than
problem 1, but it would preclude implementing more sophisticated sandboxes, such
as [enforcing deterministic execution](deterministic_execution.md).

# Requirements for Unix emulation

*   Support Unix FD types including pipes and sockets. (Supporting job control
    for TTYs is not required -- this mechanism is too weird.)
*   FD-passing using sendmsg() should work, and FDs should work correctly when
    shared between processes and used concurrently.
*   Support poll() and select().

There are two basic approaches to providing Unix file descriptor semantics: *
**FDs-atop-caps**: FDs layered on top of a more basic type of capability. * This
is how GNU Hurd works. FDs are implemented in terms of Mach ports. * Note that
this is **not** how Mac OS X works. In this case, the kernel provides both Mach
ports and Unix FDs; one is not implemented in terms of the other. *
**FDs-are-caps**: FDs and capabilities are the same thing. * This is the
approach that FreeBSD-Capsicum takes. It extends the set of FD types that the
kernel implements. However, it does not implement any invocation-capability
types. It does not make existing FD operations virtualisable. * To a limited
extent, this is what NaCl is doing today. It implements mutexes as a new FD
type.

# Proposal

Continue to use the FDs-are-caps model.

1.  Introduce a uniform capability invocation mechanism, with two types of
    invocation:
    *   sync\_call(): Invoke a capability and wait for a reply.
    *   async\_call(): Invoke a capability and return immediately. Can take a
        return continuation argument.

read(), write() and lseek() become sync\_call() invocations instead of built-in
syscalls.

1.  Introduce new built-in capability types to replace ambient system calls:
    *   _Address space_ capability: provides mmap(), munmap(), and
        dyncode\_load()
    *   _Timer_ capability: provides gettimeofday(), nanosleep(), rdtsc()
    *   _Thread spawner_ capability: provides thread\_create()
    *   These calls become methods that are invoked via sync\_call().
    *   Making these into capabilities makes these facilities deniable. By
        denying access to gettimeofday(), thread\_create(), etc., we can enforce
        deterministic execution.
    *   The capability types would be virtualisable. We can provide a
        virtualised view of time by replacing gettimeofday(). Wrapping
        thread\_create() could be useful for debuggers.
    *   These capability types are shareable, which provides more flexibility
        than Unix normally provides.

We would probably retain some built-in system calls: * tls\_init(), which sets
up %gs on x86-32. Making this concurrently invokable by other processes would
probably be awkward. This call sets a register on behalf of the caller, and it
is closely coupled to the SFI-based sandboxing mechanism. * thread\_exit():
Though it would be nice to be able to delegate the ability to kill a thread to
other threads or processes, it can be hard to implement this correctly on all
our host OSes.

Question: How would we implement poll() and select()?
