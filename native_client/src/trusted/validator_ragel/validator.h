/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_VALIDATOR_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_VALIDATOR_H_

#include "native_client/src/trusted/validator_ragel/decoder.h"


/*
 * Needed to prevent unnecessary export in gyp builds.
 * Scons passes optiond -D VALIDATOR_EXPORT=DLL_EXPORT.
 * See https://code.google.com/p/nativeclient/issues/detail?id=3367 for details.
 */
#ifndef VALIDATOR_EXPORT
#define VALIDATOR_EXPORT
#endif


EXTERN_C_BEGIN

enum validation_callback_info {
  /*
   * Anyfield info mask: you can use to parse information about anyfields.
   * Anyfields are immediates, displacement (offset in memory operand) and
   * relative offset (address in jump-like instructions).
   *
   * Constants for anyfields are combined with "+" which means that you need
   * to use complex logic to parse this byte (see helper defines under this
   * type).
   */
  ANYFIELD_INFO_MASK            = 0x000000ff,
  /* Immediate sizes (immediates always come at the end of instruction).  */
  IMMEDIATES_SIZE_MASK          = 0x0000000f,
  IMMEDIATE_8BIT                = 0x00000001,
  IMMEDIATE_16BIT               = 0x00000002,
  IMMEDIATE_32BIT               = 0x00000004,
  IMMEDIATE_64BIT               = 0x00000008,
  /* Second immediate present bit — included in SECOND_IMMEDIATE_XXBIT, too.  */
  SECOND_IMMEDIATE_PRESENT      = 0x00000010,
  /* Second 8bit immediate is only used in "extrq/insertq" instructions.  */
  SECOND_IMMEDIATE_8BIT         = 0x00000011,
  /* Second 16bit immediate is only used in "enter" instruction.  */
  SECOND_IMMEDIATE_16BIT        = 0x00000012,
  /* Displacement sizes (displacements come at the end - before immediates).  */
  DISPLACEMENT_SIZE_MASK        = 0x00000060,
  DISPLACEMENT_SIZE_SHIFT       =          5,
  DISPLACEMENT_8BIT             = 0x00000021,
  DISPLACEMENT_16BIT            = 0x00000042,
  DISPLACEMENT_32BIT            = 0x00000064,
  /* Relative size (relative fields always come at the end of instriction).  */
  RELATIVE_PRESENT              = 0x00000080,
  RELATIVE_8BIT                 = 0x00000081,
  RELATIVE_16BIT                = 0x00000082,
  RELATIVE_32BIT                = 0x00000084,
  /* Not a normal immediate: only two bits can be changed.  */
  IMMEDIATE_2BIT                = 0x20000010,
  /* Last restricted register.  */
  RESTRICTED_REGISTER_MASK      = 0x00001f00,
  RESTRICTED_REGISTER_SHIFT     =          8,
  /* Was restricted_register from previous instruction used in this one?  */
  RESTRICTED_REGISTER_USED      = 0x00002000,
  /* Mask to select all validation errors.  */
  VALIDATION_ERRORS_MASK        = 0x05ffc000,
  /* Unrecognized instruction: fatal error, processing stops here.  */
  UNRECOGNIZED_INSTRUCTION      = 0x00004000,
  /* Direct jump to unaligned address outside of given region.  */
  DIRECT_JUMP_OUT_OF_RANGE      = 0x00008000,
  /* Instruction is not allowed on current CPU.  */
  CPUID_UNSUPPORTED_INSTRUCTION = 0x00010000,
  /* Base register can be one of: %r15, %rbp, %rip, %rsp.  */
  FORBIDDEN_BASE_REGISTER       = 0x00020000,
  /* Index must be restricted if present.  */
  UNRESTRICTED_INDEX_REGISTER   = 0x00040000,
  BAD_RSP_RBP_PROCESSING_MASK   = 0x00380000,
  /* Next 4 errors can not happen simultaneously.  */
  /* Operations with %ebp must be followed with sandboxing immediately.  */
  RESTRICTED_RBP_UNPROCESSED    = 0x00080000,
  /* Attemp to "sandbox" %rbp without restricting it first.  */
  UNRESTRICTED_RBP_PROCESSED    = 0x00180000,
  /* Operations with %esp must be followed with sandboxing immediately.  */
  RESTRICTED_RSP_UNPROCESSED    = 0x00280000,
  /* Attemp to "sandbox" %rsp without restricting it first.  */
  UNRESTRICTED_RSP_PROCESSED    = 0x00380000,
  /* Operations with %r15 are forbidden.  */
  R15_MODIFIED                  = 0x00400000,
  /* Operations with %xBP are forbidden. */
  /* This includes %bpl for compatibility with old validator.  */
  BP_MODIFIED                   = 0x00800000,
  /* Operations with %xSP are forbidden. */
  /* This includes %spl for compatibility with old validator.  */
  SP_MODIFIED                   = 0x01000000,
  /* Bad call alignment: "call" must end at the end of the bundle.  */
  BAD_CALL_ALIGNMENT            = 0x02000000,
  /* The instruction should be either forbidden or rewritten. */
  UNSUPPORTED_INSTRUCTION       = 0x04000000,
  /* Instruction is modifiable by nacl_dyncode_modify.  */
  MODIFIABLE_INSTRUCTION        = 0x08000000,
  /* Special instruction.  Uses different, non-standard validation rules.  */
  SPECIAL_INSTRUCTION           = 0x10000000,
  /* Some 3DNow! instructions use immediate byte as opcode extensions.  */
  LAST_BYTE_IS_NOT_IMMEDIATE    = 0x20000000,
  /* Bad jump target.  Note: in this case ptr points to jump target!  */
  BAD_JUMP_TARGET               = 0x40000000
};

#define INFO_ANYFIELDS_SIZE(info) \
  ((info) & IMMEDIATES_SIZE_MASK)
/*
 * 0: no displacement
 * 1: 1-byte displacement
 * 2: 2-byte displacement
 * 3: 4-byte displacement
 */
#define INFO_DISPLACEMENT_CODE(info) \
  (((info) & DISPLACEMENT_SIZE_MASK) >> DISPLACEMENT_SIZE_SHIFT)
#define INFO_DISPLACEMENT_SIZE(info) \
  ((1 << INFO_DISPLACEMENT_CODE(info)) >> 1)
/* Instructions with relative offsets don't include any other anyfields.  */
#define INFO_RELATIVE_SIZE(info) \
  ((info) & RELATIVE_PRESENT ? INFO_ANYFIELDS_SIZE(info) : 0)
/* If we have the second immediate, the first immediate is always one byte.  */
#define INFO_SECOND_IMMEDIATE_SIZE(info) \
  ((info) & IMMEDIATE_2BIT == SECOND_IMMEDIATE_PRESENT ? \
    INFO_ANYFIELDS_SIZE(info) - INFO_DISPLACEMENT_SIZE(info) - 1 : 0)
#define INFO_IMMEDIATE_SIZE(info) \
  INFO_ANYFIELDS_SIZE(info) - \
  INFO_DISPLACEMENT_SIZE(info) - \
  INFO_RELATIVE_SIZE(info) - \
  INFO_SECOND_IMMEDIATE_SIZE(info)

#define kBundleSize 32
#define kBundleMask 31

enum ValidationOptions {
  /* Restricted register initial value.  */
  RESTRICTED_REGISTER_INITIAL_VALUE_MASK = 0x000000ff,
  /* Call process_error function on instruction.  */
  CALL_USER_CALLBACK_ON_EACH_INSTRUCTION = 0x00000100,
  /* Process all instruction as a contiguous stream.  */
  PROCESS_CHUNK_AS_A_CONTIGUOUS_STREAM   = 0x00000200
};

/* NC_NO_REG is default value for restricted register */
#define PACK_RESTRICTED_REGISTER_INITIAL_VALUE(register) \
  ((register) ^ NC_NO_REG)
#define EXTRACT_RESTRICTED_REGISTER_INITIAL_VALUE(option) \
  (((option) & RESTRICTED_REGISTER_INITIAL_VALUE_MASK) ^ NC_NO_REG)

/*
 * Callback is invoked by ValidateChunk* for all erroneous instructions
 * (or for all instructions if CALL_USER_CALLBACK_ON_EACH_INSTRUCTION is set).
 * It's up to the callback to decide whether this particual place is indeed
 * a violation. If callback returns FALSE at least once, validation result
 * becomes FALSE.
 *
 * When callback is called for invalid jump tagret,
 *   instruction_begin = instruction_end = jump target
 *
 * Minimal user_callback for CALL_USER_CALLBACK_ON_EACH_INSTRUCTION looks like
 * this:
 *   ...
 *   if (validation_info & (VALIDATION_ERRORS_MASK | BAD_JUMP_TARGET))
 *     return FALSE;
 *   else
 *     return TRUE;
 *   ...
 *
 * Note that when superinstructions are processed this callback will be first
 * called for the sandboxing instructions (just as if they were regular
 * instructions) and then it'll be called again for the full superinstruction.
 */
typedef Bool (*ValidationCallbackFunc) (const uint8_t *instruction_begin,
                                        const uint8_t *instruction_end,
                                        uint32_t validation_info,
                                        void *callback_data);

/*
 * Returns whether given piece of code is valid.
 * It is assumed that size % kBundleSize = 0.
 * For every error/warning, user_callback is called.
 * Alternatively, if CALL_USER_CALLBACK_ON_EACH_INSTRUCTION flag in options is
 * set, user_callback is called for every instruction.
 * If PROCESS_CHUNK_AS_A_CONTIGUOUS_STREAM flag in options is set,
 * instructions crossing bundle boundaries are not considered erroneous.
 *
 * Actually validation result is computed as conjunction of values returned
 * by all invocations of user_callback, so custom validation logic can be
 * placed there.
 */
VALIDATOR_EXPORT
Bool ValidateChunkAMD64(const uint8_t codeblock[],
                        size_t size,
                        uint32_t options,
                        const NaClCPUFeaturesX86 *cpu_features,
                        ValidationCallbackFunc user_callback,
                        void *callback_data);

/*
 * See ValidateChunkAMD64
 */
VALIDATOR_EXPORT
Bool ValidateChunkIA32(const uint8_t codeblock[],
                       size_t size,
                       uint32_t options,
                       const NaClCPUFeaturesX86 *cpu_features,
                       ValidationCallbackFunc user_callback,
                       void *callback_data);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RAGEL_VALIDATOR_H_ */
