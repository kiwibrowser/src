/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Minimal ARM sandbox constants.
 *
 * These constants are used in C code as well as assembly, hence the use of
 * the preprocessor.
 *
 * nacl_qualify_sandbox_instrs.c validates that these instructions trap
 * as expected.
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_ARM_SANDBOX_H_
#define NATIVE_CLIENT_SRC_INCLUDE_ARM_SANDBOX_H_ 1

/*
 * Specially chosen BKPT and UDF instructions that also correspond to
 * BKPT and UDF when decoded as Thumb instructions.
 * All other BKPT/UDF values are disallowed by the validator out of paranoia.
 */

/*
 * BKPT #0x5BE0: literal pool head.
 *
 * Treated as a roadblock by the validator: all words that follow it in
 * a bundle aren't validated and can't be branched to.
 */
#define NACL_INSTR_ARM_LITERAL_POOL_HEAD 0xE125BE70

/*
 * NACL_INSTR_ARM_BREAKPOINT, NACL_INSTR_ARM_HALT_FILL and
 * NACL_INSTR_ARM_ABORT_NOW are intended to be equivalent from a
 * security point of view. We provide the distinction between them just
 * for debugging purposes. They might also generate different POSIX
 * signals. In principle it should be safe for a debugger to skip past
 * one of these (unlike NACL_INSTR_ARM_POOL_HEAD), because the validator
 * validates the instructions that follow.
 */

/*
 * BKPT #0x5BEF: generic breakpoint.
 *
 * Usable statically by users or dynamically by the runtime.
 */
#define NACL_INSTR_ARM_BREAKPOINT        0xE125BE7F

/*
 * UDF #0xEDEF: halt-fill.
 *
 * Generated at load time.
 */
#define NACL_INSTR_ARM_HALT_FILL         0xE7FEDEFF

/*
 * UDF #0xEDE0: abort-now.
 *
 * Required by some language constructs such as __builtin_trap.
 */
#define NACL_INSTR_ARM_ABORT_NOW         0xE7FEDEF0

/*
 * UDF #0xEDE1: always fail validation.
 *
 * It's guaranteed to always fail, and can be used to initialize buffers
 * that are expected to be filled later.
 */
#define NACL_INSTR_ARM_FAIL_VALIDATION   0xE7FEDEF1

/*
 * NOP.
 *
 * This NOP encoding is the newer NOP instead of being the one that aliases
 * to MOV. Note: It can actually decrease performance compared to MOV.
 */
#define NACL_INSTR_ARM_NOP               0xE320F000

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_ARM_SANDBOX_H_ */
