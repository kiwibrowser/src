# Introduction

There is limited support for detecting race conditions in multithreaded NaCl
programs, as well as in NaCl trusted code. You can debug either untrusted or
trusted code in each program run, not both at the same time.

Please note, that debugging untrusted code with ThreadSanitizer is a work in
progress. We don't yet support some synchronization primitives, and will produce
false race reports when they are used.

As with [Memcheck](valgrind_memcheck.md), only x86\_64 is supported at the
moment.

# Running ThreadSanitizer

Currently, ThreadSanitizer for NaCl is supported only on Linux x86\_64. From
`native_client` directory you can run it like this: `./scons
--mode=nacl,dbg-linux platform=x86-64 sdl=none \
run_under=src/third_party/valgrind/bin/tsan.sh,--suppressions=src/third_party/valgrind/nacl.supp,--ignore=src/third_party/valgrind/nacl.ignore,--nacl-untrusted,--error-exitcode=1
\ scale_timeout=20 running_on_valgrind=True with_valgrind=True tsan_bot_tests
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
    *   `src/third_party/valgrind/bin/tsan.sh` is a modified
        Valgrind/ThreadSanitizer binary which can run on NaCl.
    *   The most useful valgrind parameters:
        *   `--log-file=<file_name>`: put warnings to a file instead of
            `stderr`.
        *   `--error-exitcode=<N>`: if at least one warning is reported, exit
            with this error code (by default, valgrind uses the program's exit
            code)
        *   `--nacl-untrusted` enables debugging untrusted code. Omit this
            option if you want to debug the trusted parts.

For more info on ThreadSanitizer, visit
http://code.google.com/p/data-race-test/wiki/ThreadSanitizer.

There is a shorter alias for all these options: `./scons --mode=dbg-linux,nacl
sdl=none platform=x86-64 sdl=none buildbot=tsan tsan_bot_tests
`

# Implementation details

We disable handling of synchronization and memory events coming from either
untrusted or trusted code depending on the `--nacl-untrusted` option.

Also see [Memcheck/NaCl implementation details]
(http://code.google.com/p/nativeclient/wiki/ValgrindMemcheck#Implementation_details).
