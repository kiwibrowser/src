/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_NACL_TLS_PARAMS_H_
#define NATIVE_CLIENT_SRC_UNTRUSTED_NACL_TLS_PARAMS_H_ 1

/*
 * Native Client support for thread local storage
 *
 * Support for TLS in NaCl depends upon the cooperation of the
 * compiler's code generator, the linker (and linker script), the
 * startup code (_start), and the CPU-specific routines defined here.
 *
 * Each thread is associated with both a TLS region, comprising the .tdata
 * and .tbss (zero-initialized) sections of the ELF file, and a thread
 * descriptor block (TDB), a structure containing information about the
 * thread that is mostly private to the thread library implementation.
 * The details of the TLS and TDB regions vary by platform; this module
 * provides a generic abstraction which may be supported by any platform.
 *
 * The "combined area" is an opaque region of memory associated with a
 * thread, containing its TDB and TLS and sufficient internal padding so
 * that it can be allocated anywhere without regard to alignment.  We
 * provide here the CPU-specific parametrization routines that control how
 * that should be layed out.  The src/untrusted/nacl/tls.c module
 * provides some routines for setting up the combined area, which use
 * these parameters.
 *
 * Each time a thread is created (including the main thread via
 * _start), a combined area is allocated and initialized for it.
 *
 * Once the main thread's TLS area is initialized, a nacl_tls_init() call
 * is made to save its location in the "thread register" (aka $tp).  How
 * this is stored in the machine state varies by platform.  Additional
 * threads set up $tp in the thread creation call and don't need to use
 * nacl_tls_init() explicitly.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * Example 1: x86-32.  This diagram shows the combined area:
 *
 *  +-----+-----------------+------+-----+
 *  | pad |  TLS data, bss  | TDB  | pad |
 *  +-----+-----------------+------+-----+
 *                          ^
 *                          |
 *                          +--- %gs
 *
 * The first word of the TDB is its own address, relative to the default
 * segment register (DS).  Negative offsets from %gs will not work since
 * NaCl enforces segmentation violations, so TLS references explicitly
 * retrieve this TLS base pointer and then indirect relative to it (using
 * DS).  This first word the only part of the TDB that is part of any
 * public ABI; the rest is private to the thread library's implementation.
 *
 * The TLS section is aligned as needed by the program's particular TLS
 * data.  Since $tp (%gs) points right after it, the TDB also gets the
 * same alignment, though the TDB itself needs only word-alignment.
 *
 *      mov     %gs:0, %eax              ; load TDB's (DS-)address from TDB.
 *      mov     -0x20(%eax), %ebx        ; load TLS object into ebx
 *
 * It's also possible to use direct %gs:OFFSET accesses (with positive
 * OFFSETS only) to refer to the TDB, though we do not make use of that.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * Example 2: x86-64.
 *
 * The layout of the combined area is the same as for x86-32; the TDB
 * address is accessed via an intrinsic, __nacl_read_tp().
 *
 *      callq  __nacl_read_tp            ; load TDB address into eax.
 *      mov    -0x20(%r15,%rax,1),%eax   ; sandboxed load from r15 "segment".
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * Example 3: ARM.  The positions of TDB and TLS are reversed.
 *
 *  +-----------+--------+----------------+
 *  |   TDB     | header | TLS data, bss  |
 *  +-----------+--------+----------------+
 *              ^        ^
 *              |        |
 *              |        +--- __aeabi_read_tp()+8
 *              +--- __aeabi_read_tp()
 *
 * The header is fixed at 8 bytes by the ARM ELF TLS ABI.  The linker
 * automatically lays out TLS symbols starting at offset 8.  (We do not
 * actually make use of the header space in this implementation, though
 * other, more thorough TLS implementations do.)  The size of the TDB is
 * not part of the ABI, and is private to the thread library.  Code
 * supported by this runtime uses only the non-PIC style, known as the
 * "local exec" TLS model ("initial exec" is similar).  The generated
 * code calls __aeabi_read_tp() to locate the TLS area, then uses
 * positive offsets from this address.  Our implementation of this
 * function just fetches r9.
 *
 *      mov     r1, #192          @ offset of symbol from $tp, i.e. var(tpoff)
 *      bl      __aeabi_read_tp
 *      ldr     r2, [r0, r1]
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * The trivial inline functions below indicate the TLS layout parameters
 * specific to the machine.  The rest of the runtime code is generic to
 * any machine, by using the values of these functions.
 */

#include <stddef.h>

#if defined(NACL_DEFINE_EXTERNAL_NATIVE_SUPPORT_FUNCS)
# define NACL_TLS_LAYOUT_FUNC
#else
# define NACL_TLS_LAYOUT_FUNC static inline __attribute__((__unused__))
#endif

#if defined(__pnacl__) && !defined(NACL_DEFINE_EXTERNAL_NATIVE_SUPPORT_FUNCS)

/*
 * Signed offset from $tp to the beginning of TLS data.
 * This is where the actual TLS for a thread is found.
 * The address ($tp + __nacl_tp_tls_offset(tls_size))
 * is what gets initialized with the .tdata image.
 */
ptrdiff_t __nacl_tp_tls_offset(size_t tls_size);

/*
 * Signed offset from $tp to the thread library's private thread data block.
 * This is where implementation-private data for the thread library (if any)
 * is stored.  On some machines it's required that the first word of this
 * be a pointer with value $tp.
 */
ptrdiff_t __nacl_tp_tdb_offset(size_t tdb_size);

#elif defined(__i386__) || defined(__x86_64__)

/*
 *  +-----------------+------+
 *  |  TLS data, bss  | TDB  |
 *  +-----------------+------+
 *                    ^
 *                    |
 *                    +--- $tp points here
 *                    |
 *                    +--- first word's value is $tp address
 *
 * In x86-32, %gs segment prefix gets the $tp address, as does fetching %gs:0.
 * In x86-64, __nacl_read_tp() must be called; it returns the $tp address.
 */

NACL_TLS_LAYOUT_FUNC
ptrdiff_t __nacl_tp_tls_offset(size_t tls_size) {
  return -(ptrdiff_t) tls_size;
}

NACL_TLS_LAYOUT_FUNC
ptrdiff_t __nacl_tp_tdb_offset(size_t tdb_size) {
  return 0;
}

#elif defined(__arm__)

/*
 *  +-----------+--------+----------------+
 *  |   TDB     | header | TLS data, bss  |
 *  +-----------+--------+----------------+
 *              ^        ^
 *              |        |
 *              |        +--- $tp+8 points here
 *              |
 *              +--- $tp points here
 *
 * In ARM EABI, __aeabi_read_tp() gets $tp address.
 * In NaCl, this is defined as register r9.
 */

NACL_TLS_LAYOUT_FUNC
ptrdiff_t __nacl_tp_tls_offset(size_t tls_size) {
  return 8;
}

NACL_TLS_LAYOUT_FUNC
ptrdiff_t __nacl_tp_tdb_offset(size_t tdb_size) {
  return -(ptrdiff_t) tdb_size;
}

#elif defined(__mips__)

/*
 *  +-----------+---------------+
 *  |    TDB    | TLS data, bss |
 *  +-----------+---------------+
 *              ^
 *              |
 *              +--- $tp points here
 */

NACL_TLS_LAYOUT_FUNC
ptrdiff_t __nacl_tp_tls_offset(size_t tls_size) {
  return 0;
}

NACL_TLS_LAYOUT_FUNC
ptrdiff_t __nacl_tp_tdb_offset(size_t tdb_size) {
  return -(ptrdiff_t) tdb_size;
}

#else

#error "unknown platform"

#endif

#undef NACL_TLS_LAYOUT_FUNC

#endif /* NATIVE_CLIENT_SRC_UNTRUSTED_NACL_TLS_PARAMS_H_ */
