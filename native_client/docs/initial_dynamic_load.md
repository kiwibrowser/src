# Loading the dynamic linker and executable

_Draft_

## Overview

This document discusses how address space should be laid out when a dynamic
linker and an executable are loaded, and how this can be orchestrated with
sel\_ldr and its associated in-browser interface.

## Background

On a typical ELF system such as Linux, the kernel is normally responsible for
loading both the executable and the dynamic linker. The executable is invoked by
filename with execve(). The kernel loads the executable into the process, and
looks for a PT\_INTERP entry in its ELF Program Headers; this specifies the
filename of the dynamic linker (/lib/ld-linux.so.2 for glibc on Linux). If there
is a PT\_INTERP entry, the kernel loads this file too.

Either of these two ELF objects can be relocatable (ET\_DYN) or require loading
at a fixed position in address space (ET\_EXEC). Most often, the dynamic linker
is relocatable and the executable is fixed-position with a standard base address
(0x08048000 on i386). Sometimes the executable is relocatable too (these are
known as PIEs - position-independent executables). For relocatable objects, the
kernel chooses the load address.

There is another way to load the two ELF objects: the dynamic linker can be
invoked directly with execve(). If passed the filename of an executable, the
dynamic linker will load the executable itself.

There are two ways in which we might wish to do this differently in Native
Client's `sel_ldr`.

### Finding the dynamic linker

The Linux kernel looks up PT\_INTERP in a file namespace. In NaCl, however,
there is no built-in filesystem so it is not appropriate for `sel_ldr` to
interpret the PT\_INTERP filename (see AcquiringDynamicLibraries). The second
method -- invoking the dynamic linker directly -- is more appropriate to NaCl.

### Address space allocation

The Linux kernel makes address space allocation decisions: * Allocation
decisions have varied between versions of Linux. Sometimes libc.so and ld.so are
placed below the executable, sometimes above. Additionally, recent Linux
versions perform address space randomisation. * The heap (`brk()`-allocated
memory) goes after whichever object was invoked with execve(). Normally it does
not matter whether this is ld.so or libc.so. (The one exception I have
encountered is where invoking an old version of Emacs through ld.so failed,
because it caused the heap to be placed at a top-bit-set address, and Emacs
wanted to use the top address bit for GC purposes.)

In Native Client, for portability and testability reasons, ideally we do not
want address space allocation decisions to change between versions of NaCl. We
want behaviour to be as deterministic as possible.

Furthermore, address space is likely to be more constrained under NaCl, both
quantitatively (a 1GB limit) and qualitatively (a code/data split -- see
DynamicLoadingOptions).

For these reasons, it may be better to leave load address choices to untrusted
code.

## Possible layouts

*   Load ld.so at 0x20000 (the bottom of available address space).
    *   This is simple and involves only minimal changes to `sel_ldr`. However,
        it does not take advantage of the relocatability of ld.so.
    *   We could pick a larger standard address at which to load the executable,
        e.g. 0x01000000 (16MB). This places limits on how big ld.so and the
        executable can grow, though not severe limits.
    *   Alternatively, we could attempt to place the executable at the top of
        the code region, so that executables grow downwards from 256MB. Hence we
        have a standard executable end address rather than a standard start
        address. This would require linker changes. It involves fixing knowledge
        of the code region size in executables.
*   Load the executable at 0x20000 and either:
    *   load ld.so immediately above, or
    *   load ld.so at the top of the code region.

### Heap placement

If we adopt the Big Segment Gap scheme, address space looks like this: *
0-256MB: ELF code segments can mapped here * 256-512MB: ELF data segments are
mapped here It may be desirable to place the heap at 512MB so that its expansion
does not limit, and is not limited by, mapping of libraries.

If ld.so is loaded at a high address, we can arrange for the heap to start just
before 512MB, at the end of ld.so's data segment's BSS, reusing an otherwise
wasted page and saving upto 4k or 64k.

## Interface 1: sel\_ldr loads both

We could change sel\_ldr to load two ELF objects instead of one, in order to
load both the executable and dynamic linker. The interface for starting a NaCl
process could take two file descriptors.

This involves adding extra complexity to the trusted codebase. Later options
show that this is not necessary.

## Interface 2: sel\_ldr loads ld.so

sel\_ldr can load ld.so, which in turns loads the executable. This requires
little or no change to sel\_ldr.

Suppose we load ld.so at the bottom of address space, at 0x20000. This is where
statically linked, ET\_EXEC executables are loaded at the moment; sel\_ldr
currently only supports loading such executables. There are two ways to
implement loading ld.so at this address: * Change sel\_ldr to support loading
ET\_DYN executables (such as ld.so), but load them with the fixed address of
0x20000. This is a small change. * Link ld.so as ET\_DYN but rewrite its ELF
headers in a post-link step to be ET\_EXEC with a load address of 0x20000.

Alternatively, we could load ld.so at a higher address. sel\_ldr could take an
extra parameter to specify the ELF object's load address, or it could default to
loading the object at the highest possible address in the code region.

### ld.so's PHDRs

ld.so normally contains ELF Program Headers that are currently rejected by
sel\_ldr, in particular `PT_DYNAMIC`, `PT_GNU_EH_FRAME` and `PT_GNU_RELRO`.
(`objdump -x /lib/ld-linux.so.2` also lists `PT_GNU_STACK`, but this is not
relevant to NaCl because the stack is never executable.) Again there are two
ways to deal with these, depending on how much we want to change sel\_ldr: *
Change sel\_ldr to ignore unrecognised Program Headers. This has no security
implications. It is what most ELF loaders do. Most ELF executable loaders look
only at `PT_LOAD` headers. * Change ld.so so that it does not contain the
headers that sel\_ldr does not like. As before, this can be done as a post-link
step, so that we do not have to modify the linker to omit `PT_DYNAMIC` etc. when
linking with `-shared`.

These headers have two uses: * ld.so can read them during initialisation.
Currently, only the non-essential `PT_GNU_RELRO` is used this way. ld.so locates
the `.dynamic` section statically via the symbol `_DYNAMIC` rather than by
searching for `PT_DYNAMIC`. * They can be returned by libc's public interface
`dl_iterate_phdr()`, which is useful for garbage collectors, debuggers, etc.

The ELF Program Headers are normally included at the start of the ELF object's
text segment. We cannot do this under NaCl, because this data will not validate
as code. But we also cannot move the Program Headers to the data segment
because, while in principle they can live at any offset in the ELF file, it is
only at the start of the file that file offsets are the same as in-memory
offsets. glibc's ld.so is taking a shortcut by assuming that it can find its own
Program Headers in memory at runtime using `e_phoff` from its own ELF header.

If we want to support a `dl_iterate_phdr()` that lists ld.so (which could be
desirable for debugging tools), we will probably need to copy ld.so's Program
Headers into ld.so's data segment as a post-link step. This is a small point and
may not be important.

## Interface 3: move ELF parsing out of sel\_ldr

Instead of passing an ELF executable to sel\_ldr, the invoker can pass a list of
mapping instructions. Each instruction specifies a code or data segment to
map/copy into memory. The primary difference from ELF loading is that each
segment can come from a different file.

This arrangement means that ELF parsing can be moved out of the trusted
codebase. ELF parsing could be done by untrusted NaCl code or Javascript code.
Doing this in Javascript would avoid bootstrapping problems. Using Javascript
for this should not be a performance problem because, even though Javascript's
string manipulation facilities are limited, the task is simple and ELF Program
Headers comprise only a small amount of data.

This scheme means we do not need to worry about whether sel\_ldr supports
ET\_DYN executables or whether it rejects Program Header entries that it does
not recognise.

The main advantage of this scheme is not so much the reduction in trusted code
(instead of parsing ELF, we parse a different format), but the increase in
flexibility. It reduces the need to bootstrap a process from inside the process.

"sel\_ldr" would no longer be an accurate name because it would no longer be an
ELF loader!

## Passing arguments from the web browser

If the dynamic linker is responsible for loading the executable, it requires a
mechanism for receiving either the filename of the executable (which it can pass
to open(), which works as described in AcquiringDynamicLibraries) or a file
descriptor for the executable.

The traditional way to pass a filename is via a Unix-style argv list of strings
(as used in `main()`/`execve()`). This is such a widely used interface that it
would make sense to support it in NaCl. Currently `sel_ldr` supports passing
argv to the NaCl process, but this is not exposed in the browser interface. We
have a number of options for supporting argv from the browser:

*   `launch_with_argv()`: Provide a Javascript method for launching a NaCl
    process that takes an argv list-of-strings parameter (as in [the prototype
    implementation](AcquiringDynamicLibraries#Prototype_implementation.md)).
*   Generic IMC messages: On startup, the NaCl process does `imc_accept()` +
    `imc_recvmsg()` to receive a message containing argv, which Javascript code
    must send.
*   Generic startup message: Allow Javascript code to provide a blob of data to
    copy into the NaCl process on startup. This dovetails with interface 3 above
    in which Javascript code specifies segment layout in detail.
    *   This could be a blob with which to initialise the stack. argv and envp
        are stored at the top of the stack on ELF systems. This data structure
        contains pointers, so whoever provides the blob needs to know what
        address it will be loaded at.
    *   The blob could contain argv and envp in some other format.

`launch_with_argv()` has the disadvantage that it introduces a new type of
message that is only used in the special case of process startup.

If we use generic messages to supply argv, we might want to remove the existing
argv mechanism from `sel_ldr` so that NaCl does not have two competing
mechanisms for argv.

## Initial load using existing interfaces

Here is how the initial load can be done using existing browser interfaces where
possible:

HTML: `<embed src="path/to/ld.so.2" type="application/x-nacl-srpc"/>
`

Javascript: `nacl_elt.startup_argv(["arg0", "path/to/executable", "arg1",
"arg2"])
`

ld.so code in NaCl process: `fd = imc_accept(initial_fd); message =
imc_recvmsg(fd); // decode argv from message // continue rest of startup using
argv
`

*   ld.so is ET\_EXEC with a start address of 0x20000.
*   executable is ET\_EXEC with a start address of 0x1000000.
