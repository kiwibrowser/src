# Introduction

It is possible to run [Valgrind/Memcheck](http://www.valgrind.org) on NaCl.
Valgrind can now debug both trusted and untrusted code.

Currently, Valgrind for NaCl is supported only on **Linux x86\_64**. This
support is **experimental** -- please try it gives us your feedback.

# Terminology

*   **Valgrind** is a binary translation system.
*   **Memcheck** is a Valgrind-based memory error detector which finds these
    bugs:
    *   Memory leaks
    *   Uninitialized memory reads
    *   Out of bound memory accesses
    *   Accesses to free-ed heap
    *   Some more

# Running Valgrind/Memcheck

Currently, Valgrind for NaCl is supported only on Linux x86\_64. From
`native_client` directory you can run it like this: `./scons MODE=nacl,dbg-linux
platform=x86-64 sdl=none
run_under=src/third_party/valgrind/bin/memcheck.sh,--suppressions=src/third_party/valgrind/nacl.supp,--error-exitcode=1
scale_timeout=20 running_on_valgrind=True with_valgrind=True memcheck_bot_tests
`

*   `run_under` allows you to pass the name of the tool under which you want to
    run tests. If the tool has options, pass them after comma:
    'tool,--opt1,--opt2'. (the tool name and the parameters can not contain
    commas)
*   `scale_timeout=20` multiplies all timeouts by 20 (Remember, valgrind is
    slow!).
*   `running_on_valgrind=True` modifies test behaviour in a way suitable for
    Valgrind: reduce iteration count for long loops, disable some tests.
*   `with_valgrind=True` links an untrusted Valgrind module to the test binary.
    This will hopefully not be needed when NaCl supports dynamic linking.
*   `src/third_party/valgrind/bin/memcheck.sh` is a modified valgrind binary
    which can run on NaCl.
*   The most useful valgrind parameters:
    *   `--log-file=<file_name>`: put warnings to a file instead of `stderr`.
    *   `--error-exitcode=<N>`: if at least one warning is reported, exit with
        this error code (by default, valgrind uses the program's exit code)
    *   `--leak-check=no|yes|full`: Perform leak checking (default: yes)

For more options, see the [Memcheck manual]
(http://valgrind.org/docs/manual/mc-manual.html) or type
`src/third_party/valgrind/bin/memcheck.sh --help
`

There is a shorter alias for all these options: `./scons --mode=dbg-linux,nacl
sdl=none platform=x86-64 sdl=none buildbot=memcheck memcheck_bot_tests
`

## Running Valgrind manually

If you want to run NaCl binary under valgrind w/o using scons: * build the
binary in valgrind mode: * add `-g` to compilation flags; * add
`-Wl,-u,have_nacl_valgrind_interceptors -lvalgrind` to link flags; * run
`sel_ldr` under `memcheck.sh`. ```

src/third_party/valgrind/memcheck.sh scons-out/dbg-linux-x86-64/staging/sel_ldr
scons-out/nacl-x86-64/obj/tests/hello_world/hello_world.nexe ```

# Build bot

We have a simplistic [build bot]
(http://build.chromium.org/buildbot/nacl/waterfall?builder=karmic64-valgrind)
which we will enhance over time.

# Implementation details

Valgrind treats the NaCl process (`sel_ldr`) as any other regular Linux process.
The only difference is when NaCl mmaps 84G for untrusted region, Valgrind
ignores this allocation. So, initially, all memory within untrusted region is
treated by Memcheck as unaccessible.

Later, we call [VALGRIND\_MAKE\_MEM\_UNDEFINED]
(http://valgrind.org/docs/manual/mc-manual.html#mc-manual.clientreqs) in few
places in the trusted code to tell valgrind that a specific portion of those 84G
is accessible. This annotation is applied to memory locations where we put the
untrusted code.

We intercept untrusted memory allocations (malloc, realloc, calloc, free) in
untrusted/valgrind/valgrind\_interceptors.c and notify Memcheck about them with
client requests. These interceptors must be compiled with the untrusted compiler
and, because dynamic linking is not yet supported, statically linked into the
target binary.

## Changes in valgrind

The patch to make valgrind work for NaCl (both trusted and untrusted code) is in
progress. Here is a short summary.

*   Allow mmap of 88G:
    *   Change N\_PRIMARY\_BITS from 19 to 22 in memcheck/mc\_main.c, set magic
        constants accordingly.
    *   Ignore all mmaps greater than 84G (notify\_tool\_of\_mmap in
        coregrind/m\_syswrap/syswrap-generic.c)
    *   Set `aspacem_maxAddr = (Addr)0x4000000000 - 1; // 256G`
        (coregrind/m\_aspacemgr/aspacemgr-linux.c)
*   Increase VG\_N\_SEGMENTS and VG\_N\_SEGNAMES.
*   Increase VG\_N\_THREADS in include/pub\_tool\_threadstate.h (from 500 to
    10000).
*   Notify Valgrind about the NaCl's untrusted memory region:
    *   add one more client request VG\_USERREQNACL\_MEM\_START
        (coregrind/pub\_core\_clreq.h and coregrind/m\_scheduler/scheduler.c)
    *   Intercept NaCl's StopForDebuggerInit
        (coregrind/m\_replacemalloc/vg\_replace\_malloc.c).
*   Load debug info from .nexe file:
    *   Modify di\_notify\_mmap (coregrind/m\_debuginfo/debuginfo.c)
*   valgrind.h, see here:
    http://code.google.com/p/nativeclient/source/browse/trunk/src/native_client/src/third_party/valgrind/nacl_valgrind.h

TODOs: * get rid of `Warning: client switching stacks? SP change: 0x404e0b8 -->
0xbffffff80` in some clean way.
