.. include:: /migration/deprecation.inc

==============================
PNaCl Bitcode Reference Manual
==============================

.. contents::
   :local:
   :backlinks: none
   :depth: 3

Introduction
============

This document is a reference manual for the PNaCl bitcode format. It describes
the bitcode on a *semantic* level; the physical encoding level will be described
elsewhere. For the purpose of this document, the textual form of LLVM IR is
used to describe instructions and other bitcode constructs.

Since the PNaCl bitcode is based to a large extent on LLVM IR as of
version 3.3, many sections in this document point to a relevant section
of the LLVM language reference manual. Only the changes, restrictions
and variations specific to PNaCl are described---full semantic
descriptions are not duplicated from the LLVM reference manual.

High Level Structure
====================

A PNaCl portable executable (**pexe** in short) is a single LLVM IR module.

Data Model
----------

The data model for PNaCl bitcode is fixed at little-endian ILP32: pointers are
32 bits in size. 64-bit integer types are also supported natively via the i64
type (for example, a front-end can generate these from the C/C++ type
``long long``).

Floating point support is fixed at IEEE 754 32-bit and 64-bit values (f32 and
f64, respectively).

.. _bitcode_linkagetypes:

Linkage Types
-------------

`LLVM LangRef: Linkage Types
<http://llvm.org/releases/3.3/docs/LangRef.html#linkage>`_

The linkage types supported by PNaCl bitcode are ``internal`` and ``external``.
A single function in the pexe, named ``_start``, has the linkage type
``external``. All the other functions and globals have the linkage type
``internal``.

Calling Conventions
-------------------

`LLVM LangRef: Calling Conventions
<http://llvm.org/releases/3.3/docs/LangRef.html#callingconv>`_

The only calling convention supported by PNaCl bitcode is ``ccc`` - the C
calling convention.

Visibility Styles
-----------------

`LLVM LangRef: Visibility Styles
<http://llvm.org/releases/3.3/docs/LangRef.html#visibility-styles>`_

PNaCl bitcode does not support visibility styles.

.. _bitcode_globalvariables:

Global Variables
----------------

`LLVM LangRef: Global Variables
<http://llvm.org/releases/3.3/docs/LangRef.html#globalvars>`_

Restrictions on global variables:

* PNaCl bitcode does not support LLVM IR TLS models. See
  :ref:`language_support_threading` for more details.
* Restrictions on :ref:`linkage types <bitcode_linkagetypes>`.
* The ``addrspace``, ``section``, ``unnamed_addr`` and
  ``externally_initialized`` attributes are not supported.

Every global variable must have an initializer. Each initializer must be
either a *SimpleElement* or a *CompoundElement*, defined as follows.

A *SimpleElement* is one of the following:

1) An i8 array literal or ``zeroinitializer``:

.. naclcode::
  :prettyprint: 0

     [SIZE x i8] c"DATA"
     [SIZE x i8] zeroinitializer

2) A reference to a *GlobalValue* (a function or global variable) with an
   optional 32-bit byte offset added to it (the addend, which may be
   negative):

.. naclcode::
  :prettyprint: 0

     ptrtoint (TYPE* @GLOBAL to i32)
     add (i32 ptrtoint (TYPE* @GLOBAL to i32), i32 ADDEND)

A *CompoundElement* is a unnamed, packed struct containing more than one
*SimpleElement*.

Functions
---------

`LLVM LangRef: Functions
<http://llvm.org/releases/3.3/docs/LangRef.html#functionstructure>`_

The restrictions on :ref:`linkage types <bitcode_linkagetypes>`, calling
conventions and visibility styles apply to functions. In addition, the following
are not supported for functions:

* Function attributes (either for the the function itself, its parameters or its
  return type).
* Garbage collector name (``gc``).
* Functions with a variable number of arguments (*vararg*).
* Alignment (``align``).

Aliases
-------

`LLVM LangRef: Aliases
<http://llvm.org/releases/3.3/docs/LangRef.html#aliases>`_

PNaCl bitcode does not support aliases.

Named Metadata
--------------

`LLVM LangRef: Named Metadata
<http://llvm.org/releases/3.3/docs/LangRef.html#namedmetadatastructure>`_

While PNaCl bitcode has provisions for debugging metadata, it is not considered
part of the stable ABI. It exists for tool support and should not appear in
distributed pexes.

Other kinds of LLVM metadata are not supported.

Module-Level Inline Assembly
----------------------------

`LLVM LangRef: Module-Level Inline Assembly
<http://llvm.org/releases/3.3/docs/LangRef.html#moduleasm>`_

PNaCl bitcode does not support inline assembly.

Volatile Memory Accesses
------------------------

`LLVM LangRef: Volatile Memory Accesses
<http://llvm.org/releases/3.3/docs/LangRef.html#volatile>`_

PNaCl bitcode does not support volatile memory accesses. The
``volatile`` attribute on loads and stores is not supported. See the
:doc:`pnacl-c-cpp-language-support` for more details.

Memory Model for Concurrent Operations
--------------------------------------

`LLVM LangRef: Memory Model for Concurrent Operations
<http://llvm.org/releases/3.3/docs/LangRef.html#memmodel>`_

See the :doc:`PNaCl C/C++ Language Support <pnacl-c-cpp-language-support>`
for details.

Fast-Math Flags
---------------

`LLVM LangRef: Fast-Math Flags
<http://llvm.org/releases/3.3/docs/LangRef.html#fastmath>`_

Fast-math mode is not currently supported by the PNaCl bitcode.

Type System
===========

`LLVM LangRef: Type System
<http://llvm.org/releases/3.3/docs/LangRef.html#typesystem>`_

The LLVM types allowed in PNaCl bitcode are restricted, as follows:

Scalar types
------------

* The only scalar types allowed are integer, float (32-bit floating point),
  double (64-bit floating point) and void.

  * The only integer sizes allowed are i1, i8, i16, i32 and i64.
  * The only integer sizes allowed for function arguments and function return
    values are i32 and i64.

Vector types
------------

The only vector types allowed are:

* 128-bit vectors integers of elements size i8, i16, i32.
* 128-bit vectors of float elements.
* Vectors of i1 type with element counts corresponding to the allowed
  element counts listed previously (their width is therefore not
  128-bits).

Array and struct types
----------------------

Array and struct types are only allowed in
:ref:`global variable initializers <bitcode_globalvariables>`.

.. _bitcode_pointertypes:

Pointer types
-------------

Only the following pointer types are allowed:

* Pointers to valid PNaCl bitcode scalar types, as specified above, except for
  ``i1``.
* Pointers to valid PNaCl bitcode vector types, as specified above, except for
  ``<? x i1>``.
* Pointers to functions.

In addition, the address space for all pointers must be 0.

A pointer is *inherent* when it represents the return value of an ``alloca``
instruction, or is an address of a global value.

A pointer is *normalized* if it's either:

* *inherent*
* Is the return value of a ``bitcast`` instruction.
* Is the return value of a ``inttoptr`` instruction.

Undefined Values
----------------

`LLVM LangRef: Undefined Values
<http://llvm.org/releases/3.3/docs/LangRef.html#undefvalues>`_

``undef`` is only allowed within functions, not in global variable initializers.

Constant Expressions
--------------------

`LLVM LangRef: Constant Expressions
<http://llvm.org/releases/3.3/docs/LangRef.html#constant-expressions>`_

Constant expressions are only allowed in
:ref:`global variable initializers <bitcode_globalvariables>`.

Other Values
============

Metadata Nodes and Metadata Strings
-----------------------------------

`LLVM LangRef: Metadata Nodes and Metadata Strings
<http://llvm.org/releases/3.3/docs/LangRef.html#metadata>`_

While PNaCl bitcode has provisions for debugging metadata, it is not considered
part of the stable ABI. It exists for tool support and should not appear in
distributed pexes.

Other kinds of LLVM metadata are not supported.

Intrinsic Global Variables
==========================

`LLVM LangRef: Intrinsic Global Variables
<http://llvm.org/releases/3.3/docs/LangRef.html#intrinsic-global-variables>`_

PNaCl bitcode does not support intrinsic global variables.

.. _ir_and_errno:

Errno and errors in arithmetic instructions
===========================================

Some arithmetic instructions and intrinsics have the similar semantics to
libc math functions, but differ in the treatment of ``errno``. While the
libc functions may set ``errno`` for domain errors, the instructions and
intrinsics do not. This is because the variable ``errno`` is not special
and is not required to be part of the program.

Instruction Reference
=====================

List of allowed instructions
----------------------------

This is a list of LLVM instructions supported by PNaCl bitcode. Where
applicable, PNaCl-specific restrictions are provided.

.. TODO: explain instructions or link in the future

The following attributes are disallowed for all instructions:

* ``nsw`` and ``nuw``
* ``exact``

Only the LLVM instructions listed here are supported by PNaCl bitcode.

* ``ret``
* ``br``
* ``switch``

  i1 values are disallowed for ``switch``.

* ``add``, ``sub``, ``mul``, ``shl``,  ``udiv``, ``sdiv``, ``urem``, ``srem``,
  ``lshr``, ``ashr``

  These arithmetic operations are disallowed on values of type ``i1``.

  Integer division (``udiv``, ``sdiv``, ``urem``, ``srem``) by zero is
  guaranteed to trap in PNaCl bitcode.

* ``and``
* ``or``
* ``xor``
* ``fadd``
* ``fsub``
* ``fmul``
* ``fdiv``
* ``frem``

  The frem instruction has the semantics of the libc fmod function for
  computing the floating point remainder. If the numerator is infinity, or
  denominator is zero, or either are NaN, then the result is NaN.
  Unlike the libc fmod function, this does not set ``errno`` when the
  result is NaN (see the :ref:`instructions and errno <ir_and_errno>`
  section).

* ``alloca``

  See :ref:`alloca instructions <bitcode_allocainst>`.

* ``load``, ``store``

  The pointer argument of these instructions must be a *normalized* pointer (see
  :ref:`pointer types <bitcode_pointertypes>`). The ``volatile`` and ``atomic``
  attributes are not supported. Loads and stores of the type ``i1`` and ``<? x
  i1>`` are not supported.

  These instructions must follow the following alignment restrictions:

  * On integer memory accesses: ``align 1``.
  * On ``float`` memory accesses: ``align 1`` or ``align 4``.
  * On ``double`` memory accesses: ``align 1`` or ``align 8``.
  * On vector memory accesses: alignment at the vector's element width, for
    example ``<4 x i32>`` must be ``align 4``.

* ``trunc``
* ``zext``
* ``sext``
* ``fptrunc``
* ``fpext``
* ``fptoui``
* ``fptosi``
* ``uitofp``
* ``sitofp``

* ``ptrtoint``

  The pointer argument of a ``ptrtoint`` instruction must be a *normalized*
  pointer (see :ref:`pointer types <bitcode_pointertypes>`) and the integer
  argument must be an i32.

* ``inttoptr``

  The integer argument of a ``inttoptr`` instruction must be an i32.

* ``bitcast``

  The pointer argument of a ``bitcast`` instruction must be a *inherent* pointer
  (see :ref:`pointer types <bitcode_pointertypes>`).

* ``icmp``
* ``fcmp``
* ``phi``
* ``select``
* ``call``
* ``unreachable``
* ``insertelement``
* ``extractelement``

.. _bitcode_allocainst:

``alloca``
----------

The only allowed type for ``alloca`` instructions in PNaCl bitcode is i8. The
size argument must be an i32. For example:

.. naclcode::
  :prettyprint: 0

    %buf = alloca i8, i32 8, align 4

Intrinsic Functions
===================

`LLVM LangRef: Intrinsic Functions
<http://llvm.org/releases/3.3/docs/LangRef.html#intrinsics>`_

List of allowed intrinsics
--------------------------

The only intrinsics supported by PNaCl bitcode are the following.

* ``llvm.memcpy``
* ``llvm.memmove``
* ``llvm.memset``

  These intrinsics are only supported with an i32 ``len`` argument.

* ``llvm.bswap``

  The overloaded ``llvm.bswap`` intrinsic is only supported with the following
  argument types: i16, i32, i64 (the types supported by C-style GCC builtins).

* ``llvm.ctlz``
* ``llvm.cttz``
* ``llvm.ctpop``

  The overloaded ``llvm.ctlz``, ``llvm.cttz``, and ``llvm.ctpop`` intrinsics
  are only supported with the i32 and i64 argument types (the types
  supported by C-style GCC builtins).

* ``llvm.fabs``

  The overloaded ``llvm.fabs`` intrinsic is supported for float, double and
  ``<4 x float>`` argument types. It returns the absolute value of
  the argument. Some notable points: it returns ``+0.0`` when given ``-0.0``,
  ``+inf`` when given ``-inf``, and a positive ``NaN`` when given any
  signed ``NaN``.

  NOTE: This intrinsic was introduced in the pepper_42 SDK.

* ``llvm.sqrt``

  The overloaded ``llvm.sqrt`` intrinsic is only supported for float
  and double arguments types. This has the same semantics as the libc
  sqrt function, returning ``NaN`` for values less than ``-0.0``.
  However, this does not set ``errno`` when the result is NaN (see the
  :ref:`instructions and errno <ir_and_errno>` section).

* ``llvm.stacksave``
* ``llvm.stackrestore``

  These intrinsics are used to implement language features like scoped automatic
  variable sized arrays in C99. ``llvm.stacksave`` returns a value that
  represents the current state of the stack. This value may only be used as the
  argument to ``llvm.stackrestore``, which restores the stack to the given
  state.

* ``llvm.trap``

  This intrinsic is lowered to a target dependent trap instruction, which aborts
  execution.

* ``llvm.nacl.read.tp``

  See :ref:`thread pointer related intrinsics
  <bitcode_threadpointerintrinsics>`.

* ``llvm.nacl.longjmp``
* ``llvm.nacl.setjmp``

  See :ref:`Setjmp and Longjmp <bitcode_setjmplongjmp>`.

* ``llvm.nacl.atomic.store``
* ``llvm.nacl.atomic.load``
* ``llvm.nacl.atomic.rmw``
* ``llvm.nacl.atomic.cmpxchg``
* ``llvm.nacl.atomic.fence``
* ``llvm.nacl.atomic.fence.all``
* ``llvm.nacl.atomic.is.lock.free``

  See :ref:`atomic intrinsics <bitcode_atomicintrinsics>`.

.. _bitcode_threadpointerintrinsics:

Thread pointer related intrinsics
---------------------------------

.. naclcode::
  :prettyprint: 0

    declare i8* @llvm.nacl.read.tp()

Returns a read-only thread pointer. The value is controlled by the embedding
sandbox's runtime.

.. _bitcode_setjmplongjmp:

Setjmp and Longjmp
------------------

.. naclcode::
  :prettyprint: 0

    declare void @llvm.nacl.longjmp(i8* %jmpbuf, i32)
    declare i32 @llvm.nacl.setjmp(i8* %jmpbuf)

These intrinsics implement the semantics of C11 ``setjmp`` and ``longjmp``. The
``jmpbuf`` pointer must be 64-bit aligned and point to at least 1024 bytes of
allocated memory.

.. _bitcode_atomicintrinsics:

Atomic intrinsics
-----------------

.. naclcode::
  :prettyprint: 0

    declare iN @llvm.nacl.atomic.load.<size>(
            iN* <source>, i32 <memory_order>)
    declare void @llvm.nacl.atomic.store.<size>(
            iN <operand>, iN* <destination>, i32 <memory_order>)
    declare iN @llvm.nacl.atomic.rmw.<size>(
            i32 <computation>, iN* <object>, iN <operand>, i32 <memory_order>)
    declare iN @llvm.nacl.atomic.cmpxchg.<size>(
            iN* <object>, iN <expected>, iN <desired>,
            i32 <memory_order_success>, i32 <memory_order_failure>)
    declare void @llvm.nacl.atomic.fence(i32 <memory_order>)
    declare void @llvm.nacl.atomic.fence.all()

Each of these intrinsics is overloaded on the ``iN`` argument, which is
reflected through ``<size>`` in the overload's name. Integral types of
8, 16, 32 and 64-bit width are supported for these arguments.

The ``@llvm.nacl.atomic.rmw`` intrinsic implements the following
read-modify-write operations, from the general and arithmetic sections
of the C11/C++11 standards:

 - ``add``
 - ``sub``
 - ``or``
 - ``and``
 - ``xor``
 - ``exchange``

For all of these read-modify-write operations, the returned value is
that at ``object`` before the computation. The ``computation`` argument
must be a compile-time constant.

All atomic intrinsics also support C11/C++11 memory orderings, which
must be compile-time constants.

Integer values for these computations and memory orderings are defined
in ``"llvm/IR/NaClAtomicIntrinsics.h"``.

The ``@llvm.nacl.atomic.fence.all`` intrinsic is equivalent to the
``@llvm.nacl.atomic.fence`` intrinsic with sequentially consistent
ordering and compiler barriers preventing most non-atomic memory
accesses from reordering around it.

.. Note::
  :class: note

    These intrinsics allow PNaCl to support C11/C++11 style atomic
    operations as well as some legacy GCC-style ``__sync_*`` builtins
    while remaining stable as the LLVM codebase changes. The user isn't
    expected to use these intrinsics directly.

.. naclcode::
  :prettyprint: 0

    declare i1 @llvm.nacl.atomic.is.lock.free(i32 <byte_size>, i8* <address>)

The ``llvm.nacl.atomic.is.lock.free`` intrinsic is designed to
determine at translation time whether atomic operations of a certain
``byte_size`` (a compile-time constant), at a particular ``address``,
are lock-free or not. This reflects the C11 ``atomic_is_lock_free``
function from header ``<stdatomic.h>`` and the C++11 ``is_lock_free``
member function in header ``<atomic>``. It can be used through the
``__nacl_atomic_is_lock_free`` builtin.
