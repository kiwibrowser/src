# Dynamic loading in Native Client

## Introduction

Native Client needs to ensure that only validated code can be executed. This
requires a mechanism to separate code and data. We would like to extend Native
Client to support dynamic loading of code. There are two main use cases for
this: * Dynamic libraries * Language implementations with JIT (just-in-time)
compilers

There are two types of interface that we might provide for loading code
dynamically: * InterleavedCode: Code can be loaded with a validate-and-mmap
operation which allows code and data to be interleaved in address space. This
can be implemented using the [no-execute (NX) bit]
(http://en.wikipedia.org/wiki/NX_bit), which is supported by some but not all
systems. On older x86 systems that do not support NX page protection, we can use
a "Harvard architecture" approach in which the x86 code and data segments are
disjoint. * ContiguousCode: A contiguous range of address space (typically at
the bottom of address space) is reserved for code. Only validated code can be
loaded into this range, and nothing can be executed outside this range.

## Current scheme

The current sandboxing scheme supports statically-linked executables only. It
uses the following address space layout: * `0`: bottom 64k, unmapped
(technically, this is mapped but with no permission bits set) * `0x10000`:
syscall trampolines (trusted code) * `0x20000`: executable's code segment
(untrusted but validated code) * `code_top`: executable's data segment

The Native Client process is limited so that the only instructions it can
execute are below `code_top`. Each of the three architectures has a different
way to do this. All three require indirect jumps to be preceded by a masking
instruction that forces the destination address to be aligned, but whether this
instruction limits the range of the address varies.

*   On x86, the range of indirect jumps is restricted using segmentation, not
    the masking instruction. The x86 code segment is set to be `code_top` bytes
    in size. Jumping outside this segment will cause a segmentation fault.
    `code_top` has to be page-aligned.
*   On ARM, the masking instruction does restrict range. Currently the masking
    instruction is `bic reg, reg, #0xf000000f` (see `NACL_CONTROL_FLOW_MASK` in
    the code). In general, `code_top` will need to be a power of 2. The current
    mask value means that `code_top` is 256MB. The mask value fits neatly into
    an ARM immediate value.
*   On x86-64, the masking instruction implicitly restricts range to 4GB by
    operating on a 32-bit register, but this is not used to enforce a split
    between code and data, because the NaCl process's address space is not
    bigger than 4GB. Instead, the x86-64 scheme uses the NX bit to separate code
    and data. The x86-64 scheme uses the same 3-byte masking instructions that
    x86 uses (see AddRegisterJumpIndirect64() in the code).

## Interface 1: ContiguousCode

This interface scheme adds an extra region into address space into which code
can be dynamically loaded. This region appears after the executable's code
segment, before its data segment, so that the address space layout becomes: *
`0`: bottom 64k, unmapped * `0x10000`: syscall trampolines (trusted code) *
`0x20000`: executable's code segment (untrusted but validated code) *
`dynamic_code_start`: dynamic code region (untrusted but validated code) *
`code_top`: executable's data segment

### Implementation: CC-HLTRewrite

One proposed implementation for this interface works as follows: * The size of
the dynamic code region is specified implicitly by the initially-loaded
executable: it is simply the space between the executable's code and data
segments. * The dynamic code region allocated by sel\_ldr on startup and filled
with HLT instructions (or the architecture's equivalent of HLT). * sel\_ldr maps
the region into its address space twice: once into the NaCl process's address
space (as read-execute; the "read view"), and again in a location that the
untrusted code cannot access (as read-write; the "write view"). * On Unix this
double-mapping is done using POSIX shared memory. * When the NaCl process
requests loading code dynamically (via a syscall), the runtime does the
following: * Copies the code into a temporary area. * Runs the validator on the
code. * Checks that the destination region contains HLTs. (Whether this is
necessary depends on the strategy for allocating space in the dynamic code
region. If we always allocate sequentially, this check is not necessary.) *
Copies the code into the writable view of the dynamic code region, in reverse
order: high addresses are copied first, for safety in the presence of multiple
threads.

Pros: * Allow greater _defense-in-depth_ for x86-32 systems, where segment
protection is available. * Keep the TCB simple and smaller. * Memory mappings
for code do not need to be changed after the NaCl process has been started, so
we do not need to worry about race conditions involving the underlying OS's mmap
calls. * Allows small pieces of code to be loaded (<4k, <64k), so this is
suitable for loading JITted code.

Cons: * Writing and executing code concurrently might trigger CPU bugs. * We
could address this by not allowing dynamic loading when there are multiple
threads, or by requiring other threads to be in a "parked" state. However, this
would not be good for loading JITted code. * The size of the dynamic code region
has to be specified up front, when the executable is loaded; address space has
to be traded off between code and data. * Worse, memory must be allocated for
the whole dynamic code region from the underlying OS: all the pages for the
dynamic code region must be dirtied in order to fill them with HLT instructions.
So, if an executable requests a dynamic code region of 256MB, it will use 256MB
worth of memory bandwidth; if this region is unused and is swapped out, it will
cause 256MB worth of disc IO and consume 256MB of swap space. * This could be a
problem for limited-memory devices such as phones or netbooks. It will waste
energy as well as memory. * We could run out of swap space, cause thrashing or
(on Linux) trigger the OOM killer if many NaCl processes are running. * We lose
the ability to do useful resource accounting for web sites using NaCl. * Does
not allow memory occupied by libraries to be shared between processes. * On ARM,
data segment accesses can potentially use fewer instructions or use immediates
instead of loads when the data segment is close to the code segment. A large
segment gap could make data segment accesses slower. [robertm: this effect is
minor and can probably be ignored]

### Implementation: CC-MProtect

We could fix the memory usage problem of CC-HLTRewrite by using page
protections. Instead of filling the entire dynamic code region with HLTs on
startup, we initially set the page mappings in the read view to be unreadable
(no permission bits set). Whenever we need to allocate a page, we use the write
view to fill it with HLTs, and then make the read view mapping readable using
mprotect(). This assumes that the OS allocates pages on demand, when we write
the HLTs rather than when we map the pages.

The Windows equivalent of mprotect() is VirtualProtect(). The Windows docs
state, for the PAGE\_NOACCESS flag, that "This flag is not supported by the
CreateFileMapping function", which may mean that we cannot make pages in shared
memory segments selectively unreadable. Instead, we could map pages into the
code region dynamically; this is subject to the [WindowsDLLInjectionProblem]
(windows_dll_injection_problem.md).

### Implications for dynamic linking

One of the main points of dynamic linking is that <em>programs don't know in
advance what they will be loading</em>, so they don't know how much space to
reserve. With the scheme above, programs will want to reserve a large amount of
space to be on the safe side. For example, if address space is 1Gb, we might
reserve a large proportion of that for code, maybe 512MB or 256MB. Otherwise, a
process could get into a situation where it can't continue when, say, a required
plugin cannot be loaded because insufficient space was reserved up front for
code, while plenty of address space remains for data and plenty of memory
remains available.

If we knew in advance what libraries we were going to load, <em>we wouldn't need
dynamic loading support</em>. Instead we could concatenate our libraries (.so
files) and executable into one big executable before running `sel_ldr`. We could
fudge the dynamic linker to find the pre-loaded libraries in memory rather than
loading them from the virtual filesystem.

### Dynamic library segment layout

ELF dynamic libraries are normally set up so that a library's data segment
immediately follows its code segment. (On x86-64 systems there is a ~1MB gap
between the code and data segments in order to support hypothetical systems with
a 1MB page size. The resulting address space wastage is not considered
significant when you have a 48-bit address space to play with.) This means that
code and data are interleaved in address space, i.e. * `0x100000`: library 1
code * `0x180000`: library 1 data * `0x200000`: library 2 code * `0x280000`:
library 2 data If NaCl's dynamic loading facility does not support interleaved
code and data, we would have to change the layout to something like this: *
`0x00100000`: library 1 code * `0x00180000`: library 2 code * `0x10100000`:
library 1 data * `0x10180000`: library 2 data This example assumes that at most
256MB (`0x10000000`) is set aside for loading code.

Once an ELF shared library is linked to become a .so file, its code and data
segments are fixed relative to each other (with the caveat that `--emit-relocs`
causes `ld` to retain the relocations that were applied). The ELF shared library
is relocatable, but whatever offset the code segment is moved by, the data
segment must be moved by too. This is what the ELF Program Headers format
assumes, and any dynamic loader will assume. This means the size of the gap
between segments gets linked in to the shared library at link time. Shared
libraries linked with different segment gap sizes will be difficult to load
together (because of the difficulty of allocating address space), so we may want
to choose a standard segment gap size, such as 256MB.

Let's call this scheme Big Segment Gap (BSG).

### Linker script changes

It is straightforward to specify the segment gap size by changing `ld`'s [linker
script](http://sourceware.org/binutils/docs-2.20/ld/Scripts.html). Normally a
linker script contains an instruction like the following (taken from
`/usr/lib/ldscripts/elf_i386.x`) to ensure that the code and data segments are
on different pages: `/* Adjust the address for the data segment. We want to
adjust up to the same address within the page on the next page up. */ . = ALIGN
(CONSTANT (MAXPAGESIZE)) - ((CONSTANT (MAXPAGESIZE) - .) & (CONSTANT
(MAXPAGESIZE) - 1));
` NaCl's linker script for statically linked executables has a slightly
different instruction. This forces the data segment to start on a new page: `. =
ALIGN(CONSTANT (MAXPAGESIZE)); /* NaCl wants page alignment */
` (For a discussion of the relative merits of those two instructions, see [issue
193](http://code.google.com/p/nativeclient/issues/detail?id=193).)

To link a shared library with a segment gap of 256MB, we would instead use: `. =
0x10000000;
`

The same effect can also be achieved without a linker script change by using the
`ld` option `--section-start .rodata=0x10000000`.

### Address space wastage

This segment layout wastes some data address space. Suppose library 1's code
segment is 192k and its data segment is 64k (after rounding up to a page size of
64k). We will have to set aside 192k of address space for the data segment so
that library 2's segments (which are fixed relative to each other) can fit in
after library 1's segments. Hence we waste 128k of address space. However, we
don't waste memory because nothing needs to be mapped into this space.
Furthermore, this is really fragmentation rather than wastage because mmap()
could still allocate from these gaps. Note that space in the code region can be
similarly wasted if the library's data segment is larger than the code segment,
but it is more usual for the code segment to be larger.

### Address space layout diagram

![http://nativeclient.googlecode.com/svn/wiki/images/nacl-address-space.png]
(http://nativeclient.googlecode.com/svn/wiki/images/nacl-address-space.png)

### Deferring the choice of segment gap size

There are two ways in which we might defer the choice of segment gap size until
after linking the .so:

*   Movable Segment Gap (MSG):

> Use `ld`'s `--emit-relocs` option. This tells `ld` to include all ELF
> relocations in the output. It may be possible to use this information to
> rewrite the .so file to move the segments relative to each other. However, it
> is not clear that this is the purpose of `--emit-relocs`, and it may be easier
> to simply re-link the .so file from the original inputs to `ld`.
>
> We could use this to change sets of libraries and executables en masse from
> one segment gap size to another. So if a gap size of 256MB turns out to be too
> small and we want to load more than 256MB of code, we can rewrite our ELF
> objects to use a gap size of 300MB without having to rebuild them.
>
> There are two potential obstacles: * The linker does not generate relocations
> for the code and data it generates at link time, such as PLT entries'
> references to the GOT. This could be fixed; otherwise, it seems to be possible
> to infer the relocations. * If the linker performs rewrites of TLS accesses,
> it does not update the relocations to match the new code (see [this mailing
> list post](http://www.sourceware.org/ml/binutils/2006-02/msg00354.html)). This
> could be fixed; otherwise, it should not be a problem if these TLS rewrites
> are rare. I have written a prototype implementation of a tool to perform this
> rewrite.

*   Sliding Segment Gap with TEXTRELs (SSG-textrel):

> Extend ELF with a new Program Headers format in which code and data segments
> are not fixed relative to each other. In this scenario, each dynamic
> relocation gains an extra flag to say whether it is relative to code or data,
> and the dynamic linker is extended to understand these relocations.
>
> This means libraries and executables do not have an inbuilt preferred segment
> gap size. The main advantage of this scheme is that it can avoid the address
> space wastage mentioned above. The resulting executables would contain a load
> of TEXTRELs (relocations in the code/text segment). TEXTRELs usually come from
> failing to compile libraries with `-fPIC`, but in SSG they would occur
> regardless of whether we compile with `-fPIC`. TEXTRELs are usually considered
> to be bad, because they prevent the memory for the code from being shared
> between processes. Although NaCl does not currently implement sharing code via
> mmap, pervasive use of TEXTRELs prevents this sharing from being introduced in
> the future, so this seems like a step in the wrong direction. Extending ELF
> would involve a lot of toolchain work to implement a feature that no-one else
> would use, just to save some address space fragmentation, so implementing this
> does not seem worthwhile.

If we wanted to reduce the number of TEXTRELs in SSG, there are a couple of
things we could do.

When code is compiled with `-fPIC`, many of the TEXTRELs will come from
references to the GOT (Global Offset Table). The GOT is the data structure
through which code acquires the addresses of symbols in other ELF objects
(including symbols in the same ELF object that can potentially be overridden by
other ELF objects). On x86, which lacks a PC-relative addressing mode, the GOT's
address is also used as a starting point for finding the relocated addresses of
code and data in the same ELF object (this is implemented via the `R_386_GOTOFF`
relocation). On x86, there is a set of tiny functions called
`__i686.get_pc_thunk.reg` (where `reg` is a register name) which are used for
copying `%eip` into a general purpose register (usually `%ebx`) whose value is
then adjusted to get the GOT's address. (See also [issue 229]
(http://code.google.com/p/nativeclient/issues/detail?id=229) for a discussion of
how this works.)

*   SSG-getgot:

> We could introduce a `__get_got_address` function. The adjustment to find the
> GOT address would be done in only one place in the code segment so there would
> be only one TEXTREL. We would use this function on all architectures in place
> of PC-relative addressing or x86's `__i686.get_pc_thunk`. All data segment
> references would have to go via the GOT, as in x86. This would add overhead to
> every function that uses a global variable or calls a function in another ELF
> object.
>
> Finding code segment addresses by adding a constant to `%ebx` via
> `R_386_GOTOFF` (e.g. taking the address of a static function) would no longer
> work. These addresses could be added as GOT entries.

*   SSG-codegot:

> Instead of placing the GOT in the data segment, we could place it in the code
> segment. Though we can't place large amounts of data in the code segment, we
> can place chunks upto 28 or 31 bytes (depending on architecture) in the code
> segment provided that each chunk is preceded by a HLT instruction to prevent
> the data from being executed as code. The GOT can fit these constraints
> because the pointers it contains do not need to be laid out contiguously. In
> this scheme, 1 out of every 8 GOT slots will be set aside to contain a HLT
> instruction.
>
> The dynamic linker would have to make a NaCl syscall to relocate a GOT entry.
> This would add overhead for lazy symbol binding (PLTGOT entries), but GOT
> relocations for eager symbol binding can be done in bulk. The syscall's
> implementation can be simple: the trusted runtime just has to check that the
> 32 byte block being written to starts with a HLT instruction. Note that this
> assumes we are not using HLTs at the starts of blocks for some other scheme,
> such as the proposed SFI-invariant-preserved-across-blocks scheme. Direct
> jumps into blocks starting with a HLT would have to be disallowed.
>
> Finding data segment addresses (e.g. addresses of static variables or string
> literals) by adding a constant to `%ebx` (via `R_386_GOTOFF`) on x86 or by
> PC-relative addressing on other architectures would no longer work. These
> addresses could be added as GOT entries.

### Allocating address space at runtime

The dynamic linker normally allocates address space for a library by first
mmapp()ing a region large enough for the whole library, both code and data
segments, which assumes that the segments are contiguous. It passes a `NULL`
`start` argument to mmap(), so the kernel chooses the start address. The kernel
keeps track of what parts of address space have been allocated so the process
doesn't have to.

Allocation will be less straightforward when each library's code and data
segments are discontiguous. The dynamic linker will have to allocate space from
the code and data regions of address space simultaneously. The data region may
contain mappings created by the main program or by libraries; the dynamic linker
will need to avoid overwriting these. We may need to extend mmap() to support
this; some possibilities are: * Add a flag to mmap() for use with MAP\_FIXED to
return an error if the requested range overlaps some mappings. * Add a system
call for enumerating existing mappings or enumerating unmapped ranges (perhaps
within a specified range). * Remove the `start=NULL` variant of the mmap()
syscall and require allocation of address space to be done in user space (that
is, in the untrusted NaCl process) by an mmap() library call that records
allocations.

### Implications for debuggers

Debuggers that assume that each ELF object is contiguous in address space may
get confused because ELF objects will appear to them to overlap. The same
applies to debugging tools such as backtrace generators. These tools may need
fixing to correctly map addresses to symbol names.

## Interface 2: InterleavedCode

In this interface scheme, NaCl's mmap() call is extended to provide a
verify-and-map-code operation. Code and data may be interleaved in the NaCl
process's address space. Code may be mapped with page granularity, which is 64k
for compatibility with Windows.

### Implementation: IC-NXBit

On systems that support it, this can be implemented using NX page protection.

Architectures: * x86: recent processors support the NX bit * x86-64: all systems
have NX bit? * ARM: has an NX bit as of ARMv7, which I believe covers all ARM
systems we are likely to want to target

There are other architectures that we may not target ourselves. However, the
ability of NaCl to be ported to them may affect community adoption: * PowerPC:
appears to have an NX bit * MIPS: appears not to have an NX bit (sources:
[Gentoo](http://www.gentoo.org/proj/en/hardened/gnu-stack.xml), [PaX]
(http://pax.grsecurity.net/docs/pax.txt))

### Implementation: IC-HarvardX86

On x86 systems without the NX bit, we implement this using x86 segmentation. The
x86 code segment is set up to be disjoint from the x86 data segment. Pages that
contain validated code are mapped into the region covered by the x86 code
segment. Other pages are mapped into the x86 data segment. As an example,
`sel_ldr`'s address space would contain the following after loading the initial
executable:

**x86 code segment covers this 512MB region:**                     | **x86 data segment covers this 512MB region:**
:----------------------------------------------------------------- | :--------------------------------------------------------------------------------------
`code_start + 0`: unmapped 64k                                     | `data_start + 0`: unmapped 64k
`code_start + 0x10000`: syscall trampolines (trusted code)         | `data_start + 0x10000`:unmapped 64k
`code_start + 0x20000`: executable's code segment (validated code) | `data_start + 0x20000`: unmapped (alternatively, the code could be mapped as read-only)
`code_start + executable_code_size`: unmapped                      | `data_start + executable_code_size`: executable data segment
unmapped                                                           | heap
unmapped                                                           | unmapped
unmapped                                                           | stack

(Note that as before, "unmapped" really means "mapped with no permission bits
set". The x86 segments are shown side by side to illustrate the correspondence
though really one is after the other.)

This is a [Harvard architecture]
(http://en.wikipedia.org/wiki/Harvard_architecture)-style approach, because any
given address in the NaCl process's address space could potentially have two
meanings depending on whether it is used for code or data access. In practice we
would not allow differing code and data pages to be mapped at the same address,
because of the potential for confusion that this could cause.

Note that PaX, a set of patches to the Linux kernel, uses a similar approach to
emulate the NX bit on x86 in its [SEGMEXEC]
(http://pax.grsecurity.net/docs/segmexec.txt) scheme.

#### x86 code segment size

The layout above halves the amount of address space available to the NaCl
process from 1024MB to 512MB. However, we could reduce the address space loss if
it were acceptable for address space to be non-uniform.

Suppose Windows lets us allocate 1024MB of `sel_ldr`'s address space. We could
allocate 300MB to the x86 code segment and 724MB to the x86 data segment. Hence
the NaCl process sees 724MB of address space, but it can map code only into the
bottom 300MB. However code is still allowed to be interleaved with data. The
remaining 424MB can only contain data. Though the resulting address space is
non-uniform, it is still more flexible than the ContiguousCode scheme.

#### Whether code is readable as data

Should pages that we map into the x86 code segment also be mapped into the x86
data segment?

If we do this, code will be readable as data, as on other platforms. However,
there may be difficulties in mapping pages twice on Windows.

If we do not do this, we will have a discrepancy between platforms. This could
lead to portability problems if programmers rely on being able to read code as
data, and test their software on only one platform. We cannot change the other
platforms to match because processors do not usually provide an
executable-but-not-readable page permission. PROT\_EXEC usually implies
PROT\_READ. This may not be a problem because reading code is inherently
unportable -- any program that does so will depend on the instruction set. NaCl
does not allow arbitrary data in an ELF code segment (with the possible
exception of 31 byte chunks guarded by a HLT byte), so no data in the code
segment can be in a portable format.

### Evaluation

Pros: * Less need to pre-allocate address space (or actual memory) to trade off
expected code and data sizes. * Closer to existing ABIs.

Cons: * May require the underlying OS to provide a mechanism to map pages
atomically. Linux has mremap(), which overwrites the destination mappings.
Windows does not have an equivalent; the original pages must be unmapped first
(see [WindowsDLLInjectionProblem](windows_dll_injection_problem.md)). However,
it may be sufficient to have an mprotect() call which can switch executable
permission on.

## Hybrid interfaces

We could combine the benefits of interleaved code and data with the benefits of
loading chunks of code that are smaller than page size. NaCl could provide
operations to map some HLT-filled pages with code, and incrementally fill those
pages with validated code using the HLT-overwriting technique above.

## Deallocation of code

In some situations it will be desirable to be able to unload code (e.g.
dlclose()) so that the space can be reused.

### Dealing with jumps

Code is loaded in chunks. If chunks are allowed to contain internal, unaligned
jumps (i.e. jumps to validated instructions in the middle of instruction
bundles), we must ensure that code is unloaded in chunks too, which means we
must record which chunks have been loaded. This applies whether we load code by
mapping pages or by overwriting HLTs. If all direct jumps are required to be
aligned, this saves us the trouble of having to remember chunks.

### Dealing with multiple threads

What happens in the presence of multi-threading? We must deal with the case
where other threads are executing the code that we are attempting to unload. *
For IC-HLTRewrite: Although it appears to be possible to overwrite HLTs safely,
going in the opposite direction -- overwriting multi-byte instructions with
HLTs -- probably cannot be made safe. This means we need a way to "park" the
other threads. It would be good if the OS provides a way to pause threads;
otherwise, the threads must voluntarily perform NaCl syscalls to enter a parked
state. If we solve this problem it may help with loading code as well as
unloading code. On Windows, SuspendThread() can be used (see
[WindowsDLLInjectionProblem](windows_dll_injection_problem.md)). * For
InterleavedCode: If we unmap a page (or remove its execute permission), any
thread executing code from that page should eventually fault. The problem is
that we must find some way to ensure that the thread has had a chance to execute
(or else is not currently executing code from that page) before mapping new code
into the same location.

## mmapping code to share memory

Most operating systems that support dynamic linking allow multiple processes
using the same library to share the memory pages containing the library code.
This usually works by mmapping the code from a common file. This would be
desirable to support in Native Client if we expect to have many NaCl processes
running simultaneously while using the same libraries.

If we were to support this, we would have to ensure that the mapped code will
not change after it has been validated, otherwise this would violate the safety
of the system. On Linux, mmap()'s `MAP_PRIVATE` flag is not sufficient to
achieve this because it is not fully copy-on-write: writes to the mapping in
memory cause a copy, but writes to the file will be visible in the mapped
region. We would therefore have to ensure that the file's contents will not be
changed. There are two ways we might achieve this:

*   Only allow file descriptors for files in the browser's cache to be mapped as
    code. It appears that NaCl currently has this constraint --
    `__urlAsNaClDesc()` is currently the only way of obtaining file FDs for NaCl
    programs running in the browser. This would rely on the browser never to
    modify a fully-fetched cached file after it has been returned through
    `__urlAsNaClDesc()`. If NaCl is running as an NPAPI plugin in an arbitrary
    browser, it may not be safe to rely on this, because NPAPI likely does not
    guarantee that cached files will not be changed.

*   NaCl could operate its own code cache. When a NaCl untrusted process does a
    NaCl-`mmap()` to map code from a downloaded file, the NaCl runtime makes a
    copy of the file in its cache, or reuses an existing identical copy in the
    cache, and does an OS-level `mmap()` from that file. This has the advantage
    that two processes would not have to fetch the same library through the same
    URL in order to achieve memory sharing. Furthermore, it does not rely on
    NaCl programs using an FD-based mmap() interface to achieve memory sharing;
    a "load code from memory" interface could also introduce sharing.

## Proposed NaCl-syscall interface

The basic syscall interface would be: `/* Load code from memory */ int
nacl_copy_code(void *src_addr, void *dest_addr, size_t size);
`

`dest_addr` must be non-NULL: it is the caller's responsibility to allocate
address ranges in the code region. In a higher level interface provided by a
library, the library would allocate addresses and would record which parts of
the code region have been allocated so far.

`dest_addr` need not be page-aligned.

We can also provide a convenience interface: `/* Load code from file descriptor.
*/ int nacl_map_code(int filedesc, int file_offset, void *dest_addr, size_t
size);
`

This can be implemented as a library function or as a second syscall.

If `nacl_map_code()` is a library function, a CC-HLTRewrite implementation will
involve copying the code three times: * Untrusted library read()s code into a
temporary buffer. * Trusted runtime copies code into its own temporary buffer to
validate. * Trusted runtime copies code to destination.

If `nacl_map_code()` is a syscall, it can avoid the first copy.

## Status

Support for dynamic loading in the trusted codebase: * CC-HLTRewrite is the
current scheme and it was implemented by May 2010 (issue 318 (on Google Code)).
A significant bug was later found (issue 714 (on Google Code)). * There have
been extensions to support modifying and unloading dynamic code (issue 731 (on
Google Code)). * CC-MProtect has not been implemented yet (issue 503 (on Google
Code)).

Support for dynamic loading in the untrusted codebase: * nacl-glibc currently
uses the Big Segment Gap (BSG) scheme. * Movable Segment Gap (MSG) is not
implemented (issue 1125 (on Google Code)). * Sliding Segment Gap (SSG) is not
implemented (issue 1126 (on Google Code)).
