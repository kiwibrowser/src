The following is dead code, i.e. not reachable from public interfaces or covered
by tests:

*   trusted/sandbox - ptrace()-based sandbox for Linux

    *   This is not used and it seems unlikely that we'd use it, given that
        Chromium is switching to use the seccomp sandbox from the SUID sandbox.
        ptrace() is slow, complicated and not designed for security.

*   trusted/service\_runtime/fs - Some sort of filesystem. Not used.

*   `nacl_cur_thread_key`, `GetCurThread`, `GetCurProc` in
    trusted/service\_runtime/nacl\_globals.c - _Removed_

    *   These might be used in the future for supporting x86-64 on Mac OS X.

*   `NaClTsd*()` in shared/platform/linux/nacl\_threads.c - _Removed_

    *   Ditto. Might be used for x86-64 on Mac.

*   `NaClAppDtor()` in sel\_ldr.c, and the various functions it calls. -
    _Removed_

    *   This is not completely dead, but it is rarely called. sel\_ldr normally
        just does exit() to shut down the process. `NaClAppDtor()` is only
        called if sel\_ldr gets an error during startup.
    *   There is a TODO to remove this function.
    *   See also http://code.google.com/p/nativeclient/issues/detail?id=560

*   The field `holding_sr_locks` in `struct NaClAppThread`, and anything that
    sets it. (Nothing reads it.)

    *   The comments in nacl\_app\_thread.h say this is intended as a way to
        kill another thread. It mentions using `pthread_kill()`, but this does
        not work for killing threads on Linux.

*   Memory objects recorded in the mapping list? Since we don't support mremap()
    or relocating the address space (a hack that was proposed for Windows but is
    not implemented), I don't think the memory objects are used.

*   service\_runtime/web\_worker\_stub.c

*   service\_runtime/main.c: Some sort of wrapper that uses dlopen()

*   `origin` field in `struct NaClApp` - _Removed_

*   "chroot me" functionality in sel\_main.c (NACL\_SANDBOX\_CHROOT\_FD). This
    is not strictly dead, but there are no users for it. There are no tests for
    it.

*   ioctl syscall and method. These are both no-ops. I shouldn't think we'd ever
    support an ioctl() syscall, so we should remove the stubs.

*   srpc\_get\_fd(). This is called, but the result is never used as a file
    descriptor number. The result is only ever compared with -1, to determine
    whether the process is running in standalone mode or from the browser
    plugin. The same result could be achieved with an environment variable. -
    _Removed_

*   LogAtServiceRuntime() in the plugin. - _Removed_

This has all become dead as a result of removing SDL support and video/audio
syscalls from sel\_ldr: * The "result" field in NaClAppThread: object created
but not used. * service\_runtime/nacl\_bottom\_half.c * The "work\_queue" field
in NaClApp: read from but never written to. *
service\_runtime/nacl\_sync\_queue.c * service\_runtime/nacl\_closure.c * The
"restrict\_to\_main\_thread" field in NaClApp - _Removed_ * The "is\_privileged"
field in NaClAppThread - _Removed_
