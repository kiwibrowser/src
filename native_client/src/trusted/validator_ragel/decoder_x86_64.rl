/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Full-blown decoder for amd64 case.  Can be used to decode instruction
 * sequence and process it, but right now is only used in tests.
 *
 * The code is in [hand-written] "parse_instruction.rl" and in [auto-generated]
 * "decoder_x86_64_instruction.rl" file.  This file only includes tiny amount
 * of the glue code.
 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/include/elf32.h"
#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/validator_ragel/decoder_internal.h"

#define GET_REX_PREFIX() instruction.prefix.rex
#define SET_REX_PREFIX(PREFIX_BYTE) instruction.prefix.rex = (PREFIX_BYTE)
#define GET_VEX_PREFIX2() vex_prefix2
#define SET_VEX_PREFIX2(PREFIX_BYTE) vex_prefix2 = (PREFIX_BYTE)
#define CLEAR_SPURIOUS_REX_B() \
  instruction.prefix.rex_b_spurious = FALSE
#define SET_SPURIOUS_REX_B() \
  if (GET_REX_PREFIX() & REX_B) instruction.prefix.rex_b_spurious = TRUE
#define CLEAR_SPURIOUS_REX_X() \
  instruction.prefix.rex_x_spurious = FALSE
#define SET_SPURIOUS_REX_X() \
  if (GET_REX_PREFIX() & REX_X) instruction.prefix.rex_x_spurious = TRUE
#define CLEAR_SPURIOUS_REX_R() \
  instruction.prefix.rex_r_spurious = FALSE
#define SET_SPURIOUS_REX_R() \
  if (GET_REX_PREFIX() & REX_R) instruction.prefix.rex_r_spurious = TRUE
#define CLEAR_SPURIOUS_REX_W() \
  instruction.prefix.rex_w_spurious = FALSE
#define SET_SPURIOUS_REX_W() \
  if (GET_REX_PREFIX() & REX_W) instruction.prefix.rex_w_spurious = TRUE

%%{
  machine x86_64_decoder;
  alphtype unsigned char;
  variable p current_position;
  variable pe end_position;
  variable eof end_position;
  variable cs current_state;

  include byte_machine "byte_machines.rl";

  include prefixes_parsing_decoder
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include rex_actions
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include rex_parsing
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include vex_actions_amd64
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include vex_parsing_amd64
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include att_suffix_actions
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include set_spurious_prefixes
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include displacement_fields_parsing
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include modrm_actions_amd64
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include modrm_parsing
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include operand_format_actions
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include operand_source_actions_amd64_decoder
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include immediate_fields_parsing
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include relative_fields_decoder_actions
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include relative_fields_parsing
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include cpuid_actions
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";

  include decode_x86_64 "decoder_x86_64_instruction.rl";

  action end_of_instruction_cleanup {
    process_instruction(instruction_begin, current_position + 1, &instruction,
                        userdata);
    instruction_begin = current_position + 1;
    SET_DISPLACEMENT_FORMAT(DISPNONE);
    SET_IMMEDIATE_FORMAT(IMMNONE);
    SET_SECOND_IMMEDIATE_FORMAT(IMMNONE);
    SET_REX_PREFIX(FALSE);
    SET_DATA16_PREFIX(FALSE);
    SET_LOCK_PREFIX(FALSE);
    SET_REPNZ_PREFIX(FALSE);
    SET_REPZ_PREFIX(FALSE);
    SET_BRANCH_NOT_TAKEN(FALSE);
    SET_BRANCH_TAKEN(FALSE);
    /*
     * Top three bits of VEX2 are inverted: see AMD/Intel manual.
     * Pass VEX2 prefix value that corresponds to zero bits.
     */
    SET_VEX_PREFIX2(VEX_R | VEX_X | VEX_B);
    SET_VEX_PREFIX3(0x00);
    SET_ATT_INSTRUCTION_SUFFIX(NULL);
    CLEAR_SPURIOUS_REX_B();
    CLEAR_SPURIOUS_REX_X();
    CLEAR_SPURIOUS_REX_R();
    CLEAR_SPURIOUS_REX_W();
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

int DecodeChunkAMD64(const uint8_t *data, size_t size,
                     ProcessInstructionFunc process_instruction,
                     ProcessDecodingErrorFunc process_error,
                     void *userdata) {
  const uint8_t *current_position = data;
  const uint8_t *end_position = data + size;
  const uint8_t *instruction_begin = current_position;
  /*
   * Top three bits of VEX2 are inverted: see AMD/Intel manual.
   * Start with VEX2 prefix value that corresponds to zero bits.
   */
  uint8_t vex_prefix2 = VEX_R | VEX_X | VEX_B;
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
