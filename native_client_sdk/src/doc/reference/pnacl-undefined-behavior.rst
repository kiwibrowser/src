.. include:: /migration/deprecation.inc

========================
PNaCl Undefined Behavior
========================

.. contents::
   :local:
   :backlinks: none
   :depth: 3

.. _undefined_behavior:

Overview
========

C and C++ undefined behavior allows efficient mapping of the source
language onto hardware, but leads to different behavior on different
platforms.

PNaCl exposes undefined behavior in the following ways:

* The Clang frontend and optimizations that occur on the developer's
  machine determine what behavior will occur, and it will be specified
  deterministically in the *pexe*. All targets will observe the same
  behavior. In some cases, recompiling with a newer PNaCl SDK version
  will either:

  * Reliably emit the same behavior in the resulting *pexe*.
  * Change the behavior that gets specified in the *pexe*.

* The behavior specified in the *pexe* relies on PNaCl's bitcode,
  runtime or CPU architecture vagaries.

  * In some cases, the behavior using the same PNaCl translator version
    on different architectures will produce different behavior.
  * Sometimes runtime parameters determine the behavior, e.g. memory
    allocation determines which out-of-bounds accesses crash versus
    returning garbage.
  * In some cases, different versions of the PNaCl translator
    (i.e. after a Chrome update) will compile the code differently and
    cause different behavior.
  * In some cases, the same versions of the PNaCl translator, on the
    same architecture, will generate a different *nexe* for
    defense-in-depth purposes, but may cause code that reads invalid
    stack values or code sections on the heap to observe these
    randomizations.

Specification
=============

PNaCl's goal is that a single *pexe* should work reliably in the same
manner on all architectures, irrespective of runtime parameters and
through Chrome updates. This goal is unfortunately not attainable; PNaCl
therefore specifies as much as it can and outlines areas for
improvement.

One interesting solution is to offer good support for LLVM's sanitizer
tools (including `UBSan
<http://clang.llvm.org/docs/UsersManual.html#controlling-code-generation>`_)
at development time, so that developers can test their code against
undefined behavior. Shipping code would then still get good performance,
and diverging behavior would be rare.

Note that none of these issues are vulnerabilities in PNaCl and Chrome:
the NaCl sandboxing still constrains the code through Software Fault
Isolation.

Behavior in PNaCl Bitcode
=========================

Well-Defined
------------

The following are traditionally undefined behavior in C/C++ but are well
defined at the *pexe* level:

* Dynamic initialization order dependencies: the order is deterministic
  in the *pexe*.
* Bool which isn't ``0``/``1``: the bitcode instruction sequence is
  deterministic in the *pexe*.
* Out-of-range ``enum`` value: the backing integer type and bitcode
  instruction sequence is deterministic in the *pexe*.
* Aggressive optimizations based on type-based alias analysis: TBAA
  optimizations are done before stable bitcode is generated and their
  metadata is stripped from the *pexe*; behavior is therefore
  deterministic in the *pexe*.
* Operator and subexpression evaluation order in the same expression
  (e.g. function parameter passing, or pre-increment): the order is
  defined in the *pexe*.
* Signed integer overflow: two's complement integer arithmetic is
  assumed.
* Atomic access to a non-atomic memory location (not declared as
  ``std::atomic``): atomics and ``volatile`` variables all lower to the
  same compatible intrinsics or external functions; the behavior is
  therefore deterministic in the *pexe* (see :ref:`Memory Model and
  Atomics <memory_model_and_atomics>`).
* Integer divide by zero: always raises a fault (through hardware on
  x86, and through integer divide emulation routine or explicit checks
  on ARM).

Not Well-Defined
----------------

The following are traditionally undefined behavior in C/C++ which also
exhibit undefined behavior at the *pexe* level. Some are easier to fix
than others.

Potentially Fixable
^^^^^^^^^^^^^^^^^^^

* Shift by greater-than-or-equal to left-hand-side's bit-width or
  negative (see `bug 3604
  <https://code.google.com/p/nativeclient/issues/detail?id=3604>`_).

  * Some of the behavior will be specified in the *pexe* depending on
    constant propagation and integer type of variables.
  * There is still some architecture-specific behavior.
  * PNaCl could force-mask the right-hand-side to `bitwidth-1`, which
    could become a no-op on some architectures while ensuring all
    architectures behave similarly. Regular optimizations could also be
    applied, removing redundant masks.

* Using a virtual pointer of the wrong type, or of an unallocated
  object.

  * Will produce wrong results which will depend on what data is treated
    as a `vftable`.
  * PNaCl could add runtime checks for this, and elide them when types
    are provably correct (see this CFI `bug 3786
    <https://code.google.com/p/nativeclient/issues/detail?id=3786>`_).

* Some unaligned load/store (see `bug 3445
  <https://code.google.com/p/nativeclient/issues/detail?id=3445>`_).

  * Could force everything to `align 1`; performance cost should be
    measured.
  * The frontend could also be more pessimistic when it sees dubious
    casts.

* Some values can be marked as ``undef`` (see `bug 3796
  <https://code.google.com/p/nativeclient/issues/detail?id=3796>`_).

* Reaching end-of-value-returning-function without returning a value:
  reduces to ``ret i32 undef`` in bitcode. This is mostly-defined, but
  could be improved (see `bug 3796
  <https://code.google.com/p/nativeclient/issues/detail?id=3796>`_).

* Reaching “unreachable” code.

  * LLVM provides an IR instruction called “unreachable” whose effect
    will be undefined.  PNaCl could change this to always trap, as the
    ``llvm.trap`` intrinsic does.

* Zero or negative-sized variable-length array (and ``alloca``) aren't
  defined behavior. PNaCl's frontend or the translator could insert
  checks with ``-fsanitize=vla-bound``.

.. _undefined_behavior_fp:

Floating-Point
^^^^^^^^^^^^^^

PNaCl offers a IEEE-754 implementation which is as correct as the
underlying hardware allows, with a few limitations. These are a few
sources of undefined behavior which are believed to be fixable:

* Float cast overflow is currently undefined.
* Float divide by zero is currently undefined.
* The default denormal behavior is currently unspecified, which isn't
  IEEE-754 compliant (denormals must be supported in IEEE-754). PNaCl
  could mandate flush-to-zero, and may give an API to enable denormals
  in a future release. The latter is problematic for SIMD and
  vectorization support, where some platforms do not support denormal
  SIMD operations.
* ``NaN`` values are currently not guaranteed to be canonical; see `bug
  3536 <https://code.google.com/p/nativeclient/issues/detail?id=3536>`_.
* Passing ``NaN`` to STL functions (the math is defined, but the
  function implementation isn't, e.g. ``std::min`` and ``std::max``), is
  well-defined in the *pexe*.

SIMD Vectors
^^^^^^^^^^^^

SIMD vector instructions aren't part of the C/C++ standards and as such
their behavior isn't specified at all in C/C++; it is usually left up to
the target architecture to specify behavior. Portable Native Client
instead exposed :ref:`Portable SIMD Vectors <portable_simd_vectors>` and
offers the same guarantees on these vectors as the guarantees offered by
the contained elements. Of notable interest amongst these guarantees are
those of alignment for load/store instructions on vectors: they have the
same alignment restriction as the contained elements.

Hard to Fix
^^^^^^^^^^^

* Null pointer/reference has behavior determined by the NaCl sandbox:

  * Raises a segmentation fault in the bottom ``64KiB`` bytes on all
    platforms, and on some sandboxes there are further non-writable
    pages after the initial ``64KiB``.
  * Negative offsets aren't handled consistently on all platforms:
    x86-64 and ARM will wrap around to the stack (because they mask the
    address), whereas x86-32 will fault (because of segmentation).

* Accessing uninitialized/free'd memory (including out-of-bounds array
  access):

  * Might cause a segmentation fault or not, depending on where memory
    is allocated and how it gets reclaimed.
  * Added complexity because of the NaCl sandboxing: some of the
    load/stores might be forced back into sandbox range, or eliminated
    entirely if they fall out of the sandbox.

* Executing non-program data (jumping to an address obtained from a
  non-function pointer is undefined, can only do ``void(*)()`` to
  ``intptr_t`` to ``void(*)()``).

  * Just-In-Time code generation is supported by NaCl, but is not
    currently supported by PNaCl. It is currently not possible to mark
    code as executable.
  * Offering full JIT capabilities would reduce PNaCl's ability to
    change the sandboxing model. It would also require a "jump to JIT
    code" syscall (to guarantee a calling convention), and means that
    JITs aren't portable.
  * PNaCl could offer "portable" JIT capabilities where the code hands
    PNaCl some form of LLVM IR, which PNaCl then JIT-compiles.

* Out-of-scope variable usage: will produce unknown data, mostly
  dependent on stack and memory allocation.
* Data races: any two operations that conflict (target overlapping
  memory), at least one of which is a store or atomic read-modify-write,
  and at least one of which is not atomic: this will be very dependent
  on processor and execution sequence, see :ref:`Memory Model and
  Atomics <memory_model_and_atomics>`.
