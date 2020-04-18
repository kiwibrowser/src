# Dynamic linking in Native Client

_Draft_

## Intro to dynamic linking

### Why use dynamic libraries?

*   Dynamic linking reduces resource consumption:

    *   Allows executables to be smaller. Reduces download time and disc space
        usage.
    *   Allows libraries to occupy the same physical memory when loaded into
        multiple processes. robertm: this would also save validation overhead
    *   robertm: it would be nice to have some data on memory savings
    *   robertm: Note: some of the proposed implementations will not allow us to
        realize the benefit of sharing a library with multiple processes.

*   Allows library versions to be changed without having to relink executables.

    *   This makes it easier to upgrade to newer versions of libraries, and to
        switch between different implementations of the same library interface.
    *   This also makes it easier to comply with the GNU LGPL. c.f.
        http://en.wikipedia.org/wiki/LGPL, though static linking is not
        completely ruled out by the LGPL

*   Some programs don't know in advance what they will be linking against.

    *   e.g. CPython's C extension modules. Imports are determined by Python
        code.
    *   Plugins
    *   robertm: could we have a dynamic linking system without offering an
        explicit dlopen call?

### Disadvantages of dynamic linking

*   Position independent code can be a little slower than fixed-position code on
    some architectures. On i386, a register (%ebx) often has to be given over to
    locating the current library (specifically, its Global Offset Table) because
    the instruction set lacks a PC-relative addressing mode.
*   There is a cost to doing linking and symbol lookup at runtime. This tends to
    be worse for C++ where there tend to be more symbols, with longer names.
    This cost can be avoided by prelinking, but we don't plan on supporting
    prelinking. Prelinking is being used less these days because it defeats
    address space randomisation.
*   Dynamic linking makes it easier to choose combinations of libraries and
    executables that have not been tested together and that interact badly ("dll
    hell")
*   the increase in complexity of a system with dynamic linking also makes it
    more vulnerable to attacks

### Terminology

These terms all mean much the same thing: * dynamic library * shared library *
shared object * dynamic shared object (DSO) * `.so` file

The dynamic linker sometimes goes by other names: * `ld.so` * `ld-linux.so.2` in
GNU libc * `rtld`, "run time `ld`" (where `ld` means "link editor")

## Related design documents

The following issues are reasonably lengthy so I have split them out into
separate documents: * DynamicLoadingOptions: Interleaved Code vs. Contiguous
Code, and the implications for libraries of the latter. *
AcquiringDynamicLibraries: How the dynamic linker acquires data from library
files. * InitialDynamicLoad: How to do the initial load of the dynamic linker
and executable. * PortableDynamicLinker: How the dynamic linker could be made
into a portable binary.

## Changes involved

*   Toolchain changes:
    *   gcc: -fPIC code generation. <em>-- This already worked for the nacl-x86
        compiler.</em>
    *   binutils: -fPIC code generation. <em>-- Committed: fixed bug in the
        assembler.</em>
    *   binutils: Ensure the linker generates correct PLT entries that pass the
        validator. <em>-- Committed.</em>
    *   binutils: Linker TLS instruction sequence rewriting. <em>-- Binutils
        patch written but not committed.</em>
    *   Linker scripts.
    *   The above will have to be done for ARM and x86-64 as well.
*   Trusted runtime changes:
    *   Extend sel\_ldr to be able to load `ld.so`.
*   Port glibc. I have already done a basic port of glibc.
    *   Add scripts to build glibc as part of the NaCl tree (as an alternative
        to nacl-newlib), on Buildbot, to prevent regressions.
    *   robertm: explain the benfits of using glibc over newlib

## Possible dynamic linker implementations

There are a number of existing dynamic linkers that we could port to Native
Client:

*   GNU libc's dynamic linker
    *   A basic port of glibc and its dynamic linker to NaCl already exists.
    *   glibc is licensed under the GNU LGPL.
        *   We would have to provide the source. That should not be a problem.
        *   Others who supply glibc's .so files may also have to provide the
            source. LGPLv3 may have relaxed this requirement.
        *   The intention of glibc's requirement is that programs linked to the
            LGPL'd library can be relinked to different versions of the library.
            This is satisfied by using dynamic linking.
    *   glibc is quite large.
*   The dynamic linker from Android's Bionic C library
    *   BSD-style licence.
    *   Bionic is derived from a BSD Unix libc.
    *   Bionic includes a dynamic linker that appears to have been written
        specially for Android.
    *   The linker may contain Android-specific assumptions, e.g. using
        Android's prelinking scheme; supporting limited amounts of thread local
        storage.
*   The dynamic linker from a BSD libc
*   dietlibc has dynamic linking support. Licensed under the GNU GPL
*   uClibc has dynamic linking support. Licensed under the GNU LGPL

Dynamic linkers tend to be closely coupled to an associated libc, such that it
is not possible to upgrade the dynamic linker without upgrading libc.so or vice
versa.

For example, in glibc, ld.so and libc.so have the following interdependencies:

*   Memory allocation: ld.so depends on libc.so. ld.so needs to allocate memory
    dynamically and uses malloc() and free(), which come from libc.so. (During
    startup, ld.so provides its own malloc/free implementations which cannot
    free memory. These get replaced after libc.so is loaded because they are
    defined as weak symbols.)
*   Thread Local Storage: libc.so makes assumptions about how ld.so organises
    thread-local storage in order to access TLS quickly.
    *   Libraries can contain thread-local variables (those declared with
        `__thread`) and ld.so has the responsibility of allocating storage for
        them. ld.so has to set up the `%gs` register on i386 on startup, and it
        implements ELF's `__tls_get_addr` function.
    *   Many of the syscall wrappers that libc.so provides access TLS in order
        to check for thread cancellation points.

In addition, ld.so statically links to the same (or similar) syscall wrappers
that libc.so uses.
