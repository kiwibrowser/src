This problem is covered by [issue 140]
(http://code.google.com/p/nativeclient/issues/detail?id=140).

## Background

This problem is caused by a combination of three factors: * On Unix, mmap() can
replace an existing memory mapping atomically. Windows does not support this as
an atomic operation, however, so instead we must do munmap() followed by mmap().
* sel\_ldr mmaps the whole of the NaCl process's address space range in order to
reserve this range. However, doing munmap()+mmap() temporarily creates an
unallocated hole in address space. * There is a practice on Windows of injecting
DLLs (or other code) into other processes asynchronously. This technique is used
by semi-legitimate (or least non-malicious) programs. The technique is to start
a new thread in the target process using CreateRemoteThread(). This can be used
to invoke LoadLibrary() to load a DLL (after copying the DLL filename into the
target process), or to run code directly.

## Problem

There is a race condition: while there is a temporary hole in the NaCl process's
address range, the injector might map something into it.

This is a problem if it occurs in either the code or data regions. It is
vulnerable to exploit by untrusted NaCl code running in another thread: * In the
code region: NaCl code might jump to instructions in the DLL that was injected
and escape the sandbox. A single `ret` instruction can be used to run unaligned
code. * In the data region: NaCl code might modify the DLL's data and cause it
to behave in an exploitable way.

## Solution

The NaCl runtime could ensure that no other NaCl threads are running while it
does the munmap()+mmap() operation. It should be able to use SuspendThread() and
ResumeThread() for this. (Incidentally, I don't think there is an equivalent of
SuspendThread() on Linux.)

If the mmap() operation fails, a DLL has been injected, and `sel_ldr` has to
abort.

## See also

*   [DLL injection](http://en.wikipedia.org/wiki/DLL_injection) on Wikipedia
*   [X8664Sandboxing](x8664_sandboxing.md): the potential presence of DLLs
    affects whether we want to do sandboxing of read instructions
