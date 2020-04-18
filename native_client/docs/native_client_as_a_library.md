# Native Client as a synchronously-callable library

Suppose you are writing a web browser and you want to run your image decoder
inside a Native Client sandbox. Currently the main way to interact with a NaCl
domain is from a separate process, or at least a separate thread, via IMC, which
is asynchronous and uses the host OS's IPC system. But using host OS IPC and
switching threads has overhead. It would be more efficient for the host program
to invoke the NaCl domain on the same thread via some synchronous call.

## Terms

*   "NaCl domain": We use the term "domain" instead of "process" to distinguish
    it from the host OS process in which it lives. This is particularly
    important when there could be multiple NaCl domains living inside one host
    OS process.
*   Inter-domain communication (IDC): This would be a better term than "IPC"
    when the communications occurs between domains inside the same host OS
    process.
*   Thread-migrating IPC: A thread-migrating IPC call is a synchronous call in
    which the caller is suspended and the callee runs in the thread borrowed
    from the caller. This term comes from the literature on EROS and L4. e.g.
    See "[Vulnerabilities in Synchronous IPC Designs]
    (http://srl.cs.jhu.edu/courses/600.439/shap03vulnerabilities.pdf)" (Jonathan
    Shapiro).

## Usage

The following usage would happen all in one thread: * Host constructs a NaCl
domain: maps address space, maps/copies and validates code, maps initial stack *
Host starts the NaCl domain running (jumping to the ELF entry point), in order
for the domain to perform its initialisation * NaCl domain returns, indicating
that it is ready to start accepting invocations * Repeat as necessary: * Host
invokes NaCl domain: CallNaCl() * NaCl domain might make system calls (including
to services provided by host) * NaCl domain returns to host

The NaCl domain would need these extra interfaces: * A syscall for returning
results. * A syscall by which the domain indicates how it should receive
invocations. This would include what address to jump to.

Luckily, these can be the same syscall! The "return" syscall would really be
"return-and-wait-for-next-invocation". This syscall would appear to return when
the next invocation occurs. It would be like a combined sendmsg() + recvmsg()
call. This is similar to EROS's "[Return]
(http://www.eros-os.org/devel/ObRef/Intro.html)" capability invocation, which
switches the calling process from the "running" state to the "available" (open
wait) state.

CallNaCl() would save its state using setjmp(). When the NaCl domain calls the
"return" syscall, this would return from CallNaCl() using longjmp().

## Syscall-handling stack

We'd need one change for how the trusted, syscall-handling stack is set up:

Currently, when a NaCl thread is created, sel\_ldr steals **all** of its host OS
thread's stack for use as the syscall-handling stack. sel\_ldr just samples the
stack pointer using NaClGetStackPtr() at an arbitrary point (a short cut). So
after it enters the NaCl domain, it can never return, because the stack is
overwritten.

To address this we could do one of two things: 1. On every call to CallNaCl(),
use the remainder of the host thread's stack as the syscall-handling stack. This
involves calling NaClSetThreadCtxSp() every time. If domains can recursively
call each other via syscalls, there is a risk that this would exhaust the stack.
1. Just allocate a dedicated syscall-handling stack instead of scrounging the
one allocated by the host system. This seems like the simplest option.

## Optional extras

*   A mechanism to catch faults
