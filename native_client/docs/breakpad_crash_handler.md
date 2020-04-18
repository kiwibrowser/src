## Introduction

Breakpad is a library for catching crashes and sending crash dumps to an
upstream server for debugging purposes. It is enabled in the official Chrome
builds, but not by default in Chromium builds.

Crash handling is complicated by the fact that crashes can potentially occur in
untrusted and trusted code. A crash in NaCl's trusted code would indicate a bug
in NaCl which we would like to track down. A crash in untrusted code is
expected, however, in the sense that it does not necessarily indicate a bug in
NaCl. We need to ensure that untrusted code can not crash in such a way that it
can take control of the crash handler.

## x86-32 Windows

32-bit Windows lets us catch faults in trusted code only. If a fault occurs when
the segment registers are set to non-default values, Windows kills the process
rather than restoring segments registers and running the fault handler. This was
determined experimentally; it is not documented in the Windows documentation.

## x86-64 Windows

64-bit Windows lets us catch faults in trusted code. The fault handler also runs
in the event that an exception is caught in untrusted code; however, we see this
as a significant security issue because Windows does not support sigaltstack().
A fault in untrusted code results in trusted code, including operating system
code, running on the untrusted stack. To further complicate matters, the
exception context--including the CPU state at the time of the fault--appears to
be stored on the stack. Code in a separate untrusted thread could hijack the
stack and/or reset the context RIP, which would cause the thread to return to an
arbitrary unsandboxed code.

Due to these security concerns, the fault handler must immediately abort if its
stack pointer is in the untrusted address space.

## See also

*   [LinuxCrashDumping]
    (http://code.google.com/p/chromium/wiki/LinuxCrashDumping) in Chrome
