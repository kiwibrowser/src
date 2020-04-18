The dynamic linker tends to be one of the more architecture-specific parts of a
system. It needs to know about: * How to read ELF files, which come in 32-bit
and 64-bit flavours. * How to perform dynamic relocations. * How to set up
Thread Local Storage, which involves setting an architecture-specific thread
pointer register and filling out architecture-specific TLS data structures
(variant I or variant II in tls.pdf).

If we are going to have a portable, architecture-independent binary format along
the lines of LLVM, we would probably want the dynamic linker to be a portable
binary too.

Architecture-specific dynamic relocation code would normally be chosen at
compile time, for example:

```
switch(r_type) {
#if defined(__i386__)
  case R_386_JMP_SLOT: ...
  case R_386_GLOB_DAT: ...
  ...
#elif defined(__x86_64__)
  case R_X86_64_JUMP_SLOT: ...
  case R_X86_64_GLOB_DAT: ...
  ...
#endif
}
```

(Or the code can be split into arch-specific files.)

Instead, we can compile code that handles all known architectures:

```
switch(get_machine_type()) {
  case EM_386:
    switch(r_type) {
      case R_386_JMP_SLOT: ...
      case R_386_GLOB_DAT: ...
      ...
    }
    break;
  case EM_X86_64:
    switch(r_type) {
      case R_X86_64_JUMP_SLOT: ...
      case R_X86_64_GLOB_DAT: ...
      ...
    }
    break;
}
```

get\_machine\_type() can be a function which returns a constant at runtime, in
which case we will link code that will never be run. Alternatively,
get\_machine\_type() could be a compiler/linker intrinsic. The linker and/or
code generator would perform an optimisation to drop all but the code path for
the targeted architecture.

In terms of programming language design, this is a more principled approach than
using a preprocessor for conditional compilation. It has the advantage that the
compiler type checks all variants of the code. In contrast, using preprocessor
conditionals can produce a combinatorial explosion of compile configurations
which are hard to test; it is easy to make changes that fail to build on some
configurations.

All the targeted architectures will use ILP32 with the same struct layout, so we
can use ELFCLASS32 for all binaries, even on x86-64.

## Caveat

Suppose someone ports NaCl to PowerPC. This is not very useful to PowerPC users
if existing web apps' copies of ld.so are portable binaries but these predate
the addition of PowerPC support. ld.so would need to support relocations for
PowerPC (`R_PPC_*`).

There could be a mechanism for the browser/NaCl to supply overrides. NaCl would
provide a copy of ld.so for the local architecture. The web app would
_voluntarily_ use this instead of its own copy (assuming glibc versions match
up).
