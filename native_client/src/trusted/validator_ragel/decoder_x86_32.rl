/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Full-blown decoder for ia32 case.  Can be used to decode instruction sequence
 * and process it, but right now is only used in tests.
 *
 * The code is in [hand-written] "parse_instruction.rl" and in [auto-generated]
 * "decoder_x86_32_instruction.rl" file.  This file only includes tiny amount
 * of the glue code.
 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/validator_ragel/decoder_internal.h"

%%{
  machine x86_32_decoder;
  alphtype unsigned char;
  variable p current_position;
  variable pe end_position;
  variable eof end_position;
  variable cs current_state;

  include byte_machine "byte_machines.rl";

  include prefixes_parsing_decoder
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include vex_actions_ia32
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include vex_parsing_ia32
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include att_suffix_actions
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include set_spurious_prefixes
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include displacement_fields_parsing
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include modrm_actions_ia32
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include modrm_parsing
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include operand_format_actions
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include operand_source_actions_ia32_decoder
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include immediate_fields_parsing
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include relative_fields_decoder_actions
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include relative_fields_parsing
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include cpuid_actions
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";

  include decode_x86_32 "decoder_x86_32_instruction.rl";

  action end_of_instruction_cleanup {
    process_instruction(instruction_begin, current_position + 1, &instruction,
                        userdata);
    instruction_begin = current_position + 1;
    SET_DISPLACEMENT_FORMAT(DISPNONE);
    SET_IMMEDIATE_FORMAT(IMMNONE);
    SET_SECOND_IMMEDIATE_FORMAT(IMMNONE);
    SET_DATA16_PREFIX(FALSE);
    SET_LOCK_PREFIX(FALSE);
    SET_REPNZ_PREFIX(FALSE);
    SET_REPZ_PREFIX(FALSE);
    SET_BRANCH_NOT_TAKEN(FALSE);
    SET_BRANCH_TAKEN(FALSE);
    SET_VEX_PREFIX3(0x00);
    SET_ATT_INSTRUCTION_SUFFIX(NULL);
  }

  action report_fatal_error {
    process_error(current_position, userdata);
    result = FALSE;
    goto error_detected;
  }

  decoder := (one_instruction @end_of_instruction_cleanup)*
             $!report_fatal_error;
}%%

/*
 * The "write data" statement causes Ragel to emit the constant static data
 * needed by the ragel machine.
 */
%% write data;

int DecodeChunkIA32(const uint8_t *data, size_t size,
                    ProcessInstructionFunc process_instruction,
                    ProcessDecodingErrorFunc process_error, void *userdata) {
  const uint8_t *current_position = data;
  const uint8_t *end_position = data + size;
  const uint8_t *instruction_begin = current_position;
  uint8_t vex_prefix3 = 0x00;
  enum ImmediateMode imm_operand = IMMNONE;
  enum ImmediateMode imm2_operand = IMMNONE;
  struct Instruction instruction;
  int result = TRUE;

  int current_state;

  memset(&instruction, 0, sizeof instruction);

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

error_detected:
  return result;
}
