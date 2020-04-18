/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This is the core of ia32-mode validator.  Please note that this file
 * combines ragel machine description and C language actions.  Please read
 * validator_internals.html first to understand how the whole thing is built:
 * it explains how the byte sequences are constructed, what constructs like
 * "@{}" or "REX_WRX?" mean, etc.
 */

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/trusted/validator_ragel/bitmap.h"
#include "native_client/src/trusted/validator_ragel/validator_internal.h"

/* Ignore this information: it's not used by security model in IA32 mode.  */
/* TODO(khim): change gen_dfa to remove needs for these lines.  */
#undef GET_VEX_PREFIX3
#define GET_VEX_PREFIX3 0
#undef SET_VEX_PREFIX3
#define SET_VEX_PREFIX3(PREFIX_BYTE)

%%{
  machine x86_32_validator;
  alphtype unsigned char;
  variable p current_position;
  variable pe end_position;
  variable eof end_position;
  variable cs current_state;

  include byte_machine "byte_machines.rl";

  include prefixes_parsing_validator
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include vex_actions_ia32
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include vex_parsing_ia32
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include displacement_fields_parsing
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include modrm_parsing_ia32_validator
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include immediate_fields_parsing
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include relative_fields_validator_actions
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include relative_fields_parsing
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include cpuid_actions
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";

  action unsupported_instruction {
    instruction_info_collected |= UNSUPPORTED_INSTRUCTION;
  }

  include decode_x86_32 "validator_x86_32_instruction.rl";

  special_instruction =
   # and $~0x1f, %eXX  call  %eXX
   #                   vvvvvvvvvv
    (0x83 0xe0 0xe0    0xff (0xd0|0xe0)  | # naclcall/jmp %eax
     0x83 0xe1 0xe0    0xff (0xd1|0xe1)  | # naclcall/jmp %ecx
     0x83 0xe2 0xe0    0xff (0xd2|0xe2)  | # naclcall/jmp %edx
     0x83 0xe3 0xe0    0xff (0xd3|0xe3)  | # naclcall/jmp %ebx
     0x83 0xe4 0xe0    0xff (0xd4|0xe4)  | # naclcall/jmp %esp
     0x83 0xe5 0xe0    0xff (0xd5|0xe5)  | # naclcall/jmp %ebp
     0x83 0xe6 0xe0    0xff (0xd6|0xe6)  | # naclcall/jmp %esi
     0x83 0xe7 0xe0    0xff (0xd7|0xe7))   # naclcall/jmp %edi
   #                   ^^^^       ^^^^
   # and $~0x1f, %eXX     jmp %eXX
    @{
      instruction_begin -= 3;
      instruction_info_collected |= SPECIAL_INSTRUCTION;
    };

  # For direct call we explicitly encode all variations.
  direct_call = (data16 0xe8 rel16) | (0xe8 rel32);

  # For indirect call we accept only near register-addressed indirect call.
  indirect_call_register = data16? 0xff (opcode_2 & modrm_registers);

  # Ragel machine that accepts one call instruction or call superinstruction and
  # checks if call is properly aligned.
  call_alignment =
    ((one_instruction & direct_call) |
     # For indirect calls we accept all the special instructions which ends with
     # register-addressed indirect call.
     (special_instruction & (any* indirect_call_register)))
    # Call instruction must aligned to the end of bundle.  Previously this was
    # strict requirement, today it's just warning to aid with debugging.
    @{
      if (((current_position - codeblock) & kBundleMask) != kBundleMask)
        instruction_info_collected |= BAD_CALL_ALIGNMENT;
    };

  # This action calls user callback (if needed) and cleans up validator
  # internal state.
  #
  # We call the user callback either on validation errors or on every
  # instruction, depending on CALL_USER_CALLBACK_ON_EACH_INSTRUTION option.
  #
  # After that we move instruction_begin and clean all the variables which
  # are only used in the processing of a single instruction (here it's just
  # instruction_info_collected, there are more state in x86-64 case).
  action end_of_instruction_cleanup {
    /* Mark start of this instruction as a valid target for jump.  */
    MarkValidJumpTarget(instruction_begin - codeblock, valid_targets);

    /* Call user-supplied callback.  */
    instruction_end = current_position + 1;
    if ((instruction_info_collected & VALIDATION_ERRORS_MASK) ||
        (options & CALL_USER_CALLBACK_ON_EACH_INSTRUCTION)) {
      result &= user_callback(instruction_begin, instruction_end,
                              instruction_info_collected, callback_data);
    }

    /*
     * We may set instruction_begin at the first byte of the instruction instead
     * of here but in the case of incorrect one byte instructions user callback
     * may be called before instruction_begin is set.
     */
    instruction_begin = instruction_end;

    /* Clear variables (well, one variable currently).  */
    instruction_info_collected = 0;
  }

  # This action reports fatal error detected by DFA.
  action report_fatal_error {
    result &= user_callback(instruction_begin, current_position,
                            UNRECOGNIZED_INSTRUCTION, callback_data);
    /*
     * Process the next bundle: "continue" here is for the "for" cycle in
     * the ValidateChunkIA32 function.
     *
     * It does not affect the case which we really care about (when code
     * is validatable), but makes it possible to detect more errors in one
     * run in tools like ncval.
     */
    continue;
  }

  # This is main ragel machine: it does 99% of validation work. There are only
  # one thing to do if this ragel machine accepts the bundles - check that
  # direct jumps are correct. This is done in the following way:
  #  * DFA fills two arrays: valid_targets and jump_dests.
  #  * ProcessInvalidJumpTargets checks that "jump_dests & !valid_targets == 0".
  # All other checks are done here.
  main := ((call_alignment | one_instruction | special_instruction)
     @end_of_instruction_cleanup)*
    $!report_fatal_error;

}%%

/*
 * The "write data" statement causes Ragel to emit the constant static data
 * needed by the ragel machine.
 */
%% write data;

Bool ValidateChunkIA32(const uint8_t codeblock[],
                       size_t size,
                       uint32_t options,
                       const NaClCPUFeaturesX86 *cpu_features,
                       ValidationCallbackFunc user_callback,
                       void *callback_data) {
  bitmap_word valid_targets_small;
  bitmap_word jump_dests_small;
  bitmap_word *valid_targets;
  bitmap_word *jump_dests;
  const uint8_t *current_position;
  const uint8_t *end_position;
  int result = TRUE;

  CHECK(sizeof valid_targets_small == sizeof jump_dests_small);
  CHECK(size % kBundleSize == 0);

  /* For a very small sequences (one bundle) malloc is too expensive.  */
  if (size <= (sizeof valid_targets_small * 8)) {
    valid_targets_small = 0;
    valid_targets = &valid_targets_small;
    jump_dests_small = 0;
    jump_dests = &jump_dests_small;
  } else {
    valid_targets = BitmapAllocate(size);
    jump_dests = BitmapAllocate(size);
    if (!valid_targets || !jump_dests) {
      free(jump_dests);
      free(valid_targets);
      errno = ENOMEM;
      return FALSE;
    }
  }

  /*
   * This option is usually used in tests: we will process the whole chunk
   * in one pass. Usually each bundle is processed separately which means
   * instructions (and "superinstructions") can not cross borders of the bundle.
   */
  if (options & PROCESS_CHUNK_AS_A_CONTIGUOUS_STREAM)
    end_position = codeblock + size;
  else
    end_position = codeblock + kBundleSize;

  /*
   * Main loop.  Here we process the data array bundle-after-bundle.
   * Ragel-produced DFA does all the checks with one exception: direct jumps.
   * It collects the two arrays: valid_targets and jump_dests which are used
   * to test direct jumps later.
   */
  for (current_position = codeblock;
       current_position < codeblock + size;
       current_position = end_position,
       end_position = current_position + kBundleSize) {
    /* Start of the instruction being processed.  */
    const uint8_t *instruction_begin = current_position;
    /* Only used locally in the end_of_instruction_cleanup action.  */
    const uint8_t *instruction_end;
    uint32_t instruction_info_collected = 0;
    int current_state;

    /*
     * The "write init" statement causes Ragel to emit initialization code.
     * This should be executed once before the ragel machine is started.
     */
    %% write init;
    /*
     * The "write exec" statement causes Ragel to emit the ragel machine's
     * execution code.
     */
    %% write exec;
  }

  /*
   * Check the direct jumps.  All the targets from jump_dests must be in
   * valid_targets.
   */
  result &= ProcessInvalidJumpTargets(codeblock,
                                      size,
                                      valid_targets,
                                      jump_dests,
                                      user_callback,
                                      callback_data);

  /* We only use malloc for a large code sequences  */
  if (jump_dests != &jump_dests_small) free(jump_dests);
  if (valid_targets != &valid_targets_small) free(valid_targets);
  if (!result) errno = EINVAL;
  return result;
}
