# Deterministic execution

Some users of NaCl (web apps and non-web apps) may want to run [guest code]
(guest_code.md) and be assured that its execution is deterministic.

Use cases: * Build systems: Run a compiler and be sure that it always produces
the same output. * In this case, determinism across versions of NaCl is
desirable. * This could be useful for a NaCl code generation service. * Enables
dependable memoization. * High-integrity app containers: Although web apps are
not confined, there could be web apps that host confined guest code. e.g. Host a
text editor that cannot leak the text it edits to its creator over overt
channels, and cannot receive new instructions from its creator at all. * In this
case, determinism in the presence of other processes on the machine is
important, and determinism across versions of NaCl matters less because the
latter conveys little information. * Distributed protocols in which hosts need
to be able to independently derive the same information from some shared inputs.

Confinement: Although it is not in general possible to prevent programs
_writing_ to covert channels ("wall-banging"), it is possible to prevent
programs _reading_ from covert channels ("wall-listening") by denying access to
sources of non-determinism. Hence though it is not possible to fully achieve
_information confinement_ (preventing a process from leaking data to a
conspirator), it is possible to achieve _authority confinement_ (preventing a
process from receiving instructions from a conspirator).

## Sources of non-determinism

*   Timers
    *   gettimeofday()
    *   x86 RDTSC instruction
*   Concurrency
    *   Multiple threads
    *   Multiple processes
    *   It may be possible to construct safe forms of multi-process concurrency
        that don't expose non-determinism.
*   x86 CPUID instruction. This is not only used to get the CPU type. It is also
    before RDTSC for its side effect of synchronising the CPU. We could permit
    the side-effect-only usage by requiring CPUID to be followed an instruction
    that overwrites the target registers (potentially`%eax`, `%ebx`, `%ecx` and
    `%edx`).
*   sleep() and nanosleep(), _if_ they return the remaining time. If they don't
    return a time, they are not very useful in the absence of concurrency (safe
    but useless).
*   Memory allocation success/failure: When allocation fails, don't return an
    error directly to the guest process. Return the error to the host process,
    which can decide how to handle it, e.g. abort the guest.
*   Allocation policies for address space (addresses chosen by mmap()) and FDs
    (FD numbers chosen by open()).
*   read()/write(): short reads/writes.
*   Non-blocking imc\_recvmsg()
*   getpid()
*   Non-sandboxing of reads: On ARM and [x86-64](x8664_sandboxing.md), memory
    reads will not normally be sandboxed for performance reasons.
*   In the x86-64 sandbox, the sandbox's base address is exposed to untrusted
    code. See [issue 1235]
    (http://code.google.com/p/nativeclient/issues/detail?id=1235).
*   Default setting of the "extended precision" flag for x87 floating point
    arithmetic ([issue 253]
    (http://code.google.com/p/nativeclient/issues/detail?id=253)).
*   Trampoline code sequences:
    *   x86-32: The code sequence will include the address of `NaClSyscallSeg`
        and the %cs value to `lcall`. We could hide this code by using separate
        code/data segments (the "Harvard architecture" in
        DynamicLoadingOptions), but that uses up a lot of address space.
    *   x86-64: The code sequence will include the address of `NaClSwitch` to
        `call`. We could hide this address by placing NaClSwitch outside the
        untrusted address space (plus guard regions) at a fixed offset, and by
        computing its address based on %r15 (on Google Code). See issue 1996 (on
        Google Code).

**Source**                 | **Need to constrain for confinement?**                                | **Need to constrain for build system determinism?**
:------------------------- | :-------------------------------------------------------------------- | :--------------------------------------------------
Timers                     | yes                                                                   | yes
Concurrency                | yes                                                                   | yes
CPUID                      | no                                                                    | yes
Allocation success/failure | maybe: depends whether it makes system-wide allocation limits visible | yes
Allocation layout          | no                                                                    | yes
Short read/writes          | depends what causes them                                              | yes
getpid()                   | yes, if launching new processes is allowed                            | yes

## Related work

*   [Joe-E](http://code.google.com/p/joe-e/), a subset of Java: Joe-E allows
    methods to be statically verified as functionally pure and deterministic.
    See [Verifiable Functional Purity in Java]
    (http://www.cs.berkeley.edu/~daw/papers/pure-ccs08.pdf), Matthew Finifter,
    Adrian Mettler, Naveen Sastry, David Wagner; October 2008.
*   [Caja](http://code.google.com/p/google-caja/), a subset of Javascript. Caja
    is being extended to provide a [deterministic subset]
    (http://code.google.com/p/google-caja/issues/detail?id=1175).
    *   [Sources of non-determinism]
        (http://code.google.com/p/google-caja/wiki/SourcesOfNonDeterminism) in
        Javascript

## See also

*   [V8 issue 436](http://code.google.com/p/v8/issues/detail?id=436) about x87
    extended precision
*   [NaCl issue 84](http://code.google.com/p/nativeclient/issues/detail?id=84)
    about RDTSC and side channels
*   SystemCalls

## Caveats

Determinism only applies within a given target architecture. Assuring that, for
example, ARM and x86-32 give the same results is out of scope.
