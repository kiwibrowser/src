Non-SFI NaCl (also known as Bare Metal Mode NaCl) is a minimal sandbox in which
programs are sandboxed at the OS level only, using a Linux Seccomp-BPF sandbox,
without using SFI sandboxing (Software Fault Isolation). Programs running under
Non-SFI NaCl use the same IRT interfaces as under SFI NaCl.

Under SFI NaCl, IRT interfaces are implemented in terms of NaCl syscalls,
whereas under Non-SFI NaCl, IRT interfaces are implemented in terms of Linux
syscalls. The latter provides weaker security guarantees, but is easier to
implement overall since it removes the need to implement NaCl syscalls.

To load your own nexe using a default IRT, check out
native_client/src/nonsfi/loader/elf_loader_main.c to see how a nexe can be
loaded with the default IRT querying function. This loader implements IRT
interfaces in terms of Linux syscalls.

To load a nexe while providing extra IRT interfaces in addition to NaCl's core
IRT interfaces, you need a few components:
- An IRT interface definition.
  - Example: native_client/tests/nonsfi/example_interface.h
- A nexe which is capable of calling a query function to access the interface.
  - Example: native_client/tests/nonsfi/example_irt_caller_test.cc
- An implementation of your interface, a query function, and a nexe loader.
  - Example: native_client/tests/nonsfi/example_loader.cc

Together, the loader will be able to load a nexe, passing in a function
which can query for access to the implementation. Once running, the nexe will
be able to call this query function and gain access to the custom interface.
