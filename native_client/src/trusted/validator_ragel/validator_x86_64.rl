/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This is the core of amd64-mode validator.  Please note that this file
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

%%{
  machine x86_64_validator;
  alphtype unsigned char;
  variable p current_position;
  variable pe end_position;
  variable eof end_position;
  variable cs current_state;

  include byte_machine "byte_machines.rl";

  include prefixes_parsing_validator
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include rex_actions
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include rex_parsing
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include vex_actions_amd64
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include vex_parsing_amd64
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include displacement_fields_parsing
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include modrm_actions_amd64
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include modrm_parsing
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include operand_format_actions
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include operand_source_actions_amd64_common
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include immediate_fields_parsing
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include relative_fields_validator_actions
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include relative_fields_parsing
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";
  include cpuid_actions
    "native_client/src/trusted/validator_ragel/parse_instruction.rl";

  action check_memory_access {
    CheckMemoryAccess(instruction_begin - codeblock,
                      base,
                      index,
                      restricted_register,
                      valid_targets,
                      &instruction_info_collected);
  }

  action unsupported_instruction {
    instruction_info_collected |= UNSUPPORTED_INSTRUCTION;
  }

  action modifiable_instruction {
    instruction_info_collected |= MODIFIABLE_INSTRUCTION;
  }

  action process_0_operands {
    Process0Operands(&restricted_register, &instruction_info_collected);
  }
  action process_1_operand {
    Process1Operand(&restricted_register, &instruction_info_collected,
                    rex_prefix, operand_states);
  }
  action process_1_operand_zero_extends {
    Process1OperandZeroExtends(&restricted_register,
                               &instruction_info_collected, rex_prefix,
                               operand_states);
  }
  action process_2_operands {
    Process2Operands(&restricted_register, &instruction_info_collected,
                     rex_prefix, operand_states);
  }
  action process_2_operands_zero_extends {
    Process2OperandsZeroExtends(&restricted_register,
                                &instruction_info_collected, rex_prefix,
                                operand_states);
  }

  include decode_x86_64 "validator_x86_64_instruction.rl";

  # Special %rbp modifications - the ones which don't need a sandboxing.
  #
  # Note that there are two different opcodes for "mov": in x86-64 there are two
  # fields in ModR/M byte (REG field and RM field) and "mov" may be used to move
  # from REG field to RM or in the other direction thus there are two encodings
  # for the register-to-register move.
  rbp_modifications =
    (b_0100_10x0 0x89 0xe5                         | # mov %rsp,%rbp
     b_0100_10x0 0x8b 0xec)                          # mov %rsp,%rbp
    @process_0_operands;

  # Special instructions used for %rbp sandboxing.
  #
  # This is the "second half" of the %rbp sandboxing.  Any zero-extending
  # instruction which stores the data in %ebp can be first half, but unlike
  # the situation with other "normal" registers you can not just write to
  # %ebp and continue: such activity MUST restore the status quo immediately
  # via one of these instructions.
  rbp_sandboxing =
    (b_0100_11x0 0x01 0xfd                  | # add %r15,%rbp
     b_0100_10x1 0x03 0xef                  | # add %r15,%rbp
     # Note that unlike %rsp case, there is no 'lea (%rbp,%r15,1),%rbp'
     # instruction (it gets assembled as 'lea 0x00(%rbp,%r15,1),%rbp').
     0x4a 0x8d 0x6c 0x3d 0x00               | # lea 0x00(%rbp,%r15,1),%rbp
     0x4a 0x8d 0xac 0x3d 0x00 0x00 0x00 0x00) # lea 0x00000000(%rbp,%r15,1),%rbp
    # Note: restricted_register keeps the restricted register as explained in
    # http://www.chromium.org/nativeclient/design-documents/nacl-sfi-model-on-x86-64-systems
    #
    # "Normal" instructions can not be used in a place where %rbp is restricted.
    # But since these instructions are "second half" of the %rbp sandboxing they
    # can be used *only* when %rbp is restricted.
    #
    # Compare:
    #   mov %eax,%ebp
    #   mov %esi,%edi   <- Error: %ebp is restricted
    # vs
    #   mov %esi,%edi
    #   add %r15,%rbp   <- Error: %ebp is *not* restricted
    # vs
    #   mov %eax,%ebp
    #   add %r15,%rbp   <- Ok: %rbp is restricted as it should be
    #
    # Check this precondition and mark the beginning of the instruction as
    # invalid jump for target.
    @{ if (restricted_register == NC_REG_RBP)
         /* RESTRICTED_REGISTER_USED is informational flag used in tests.  */
         instruction_info_collected |= RESTRICTED_REGISTER_USED;
       else
         /* UNRESTRICTED_RSP_PROCESSED is error flag used in production.  */
         instruction_info_collected |= UNRESTRICTED_RBP_PROCESSED;
       restricted_register = NC_NO_REG;
       UnmarkValidJumpTarget((instruction_begin - codeblock), valid_targets);
    };

  # Special %rsp modifications - the ones which don't need a sandboxing.
  #
  # Note that there are two different opcodes for "mov": in x86-64 there are two
  # fields in ModR/M byte (REG field and RM field) and "mov" may be used to move
  # from REG field to RM or in the other direction thus there are two encodings
  # for the register-to-register move.
  rsp_modifications =
    (b_0100_10x0 0x89 0xec                         | # mov %rbp,%rsp
     b_0100_10x0 0x8b 0xe5                         | # mov %rbp,%rsp
     # Superfluous bits are not supported:
     # http://code.google.com/p/nativeclient/issues/detail?id=3012
     b_0100_1000 0x83 0xe4 (0x80 .. 0xff))           # and $XXX,%rsp
    @process_0_operands;

  # Special instructions used for %rsp sandboxing.
  #
  # This is the "second half" of the %rsp sandboxing.  Any zero-extending
  # instruction which stores the data in %esp can be first half, but unlike
  # the situation with other "normal" registers you can not just write to
  # %esp and continue: such activity MUST restore the status quo immediately
  # via one of these instructions.
  rsp_sandboxing =
    (b_0100_11x0 0x01 0xfc                  | # add %r15,%rsp
     b_0100_10x1 0x03 0xe7                  | # add %r15,%rsp
     # OR can be used as well, see
     # http://code.google.com/p/nativeclient/issues/detail?id=3070
     b_0100_11x0 0x09 0xfc                  | # or %r15,%rsp
     b_0100_10x1 0x0b 0xe7                  | # or %r15,%rsp
     0x4a 0x8d 0x24 0x3c                    | # lea (%rsp,%r15,1),%rsp
     0x4a 0x8d 0x64 0x3c 0x00               | # lea 0x00(%rsp,%r15,1),%rsp
     0x4a 0x8d 0xa4 0x3c 0x00 0x00 0x00 0x00) # lea 0x00000000(%rsp,%r15,1),%rsp
    # Note: restricted_register keeps the restricted register as explained in
    # http://www.chromium.org/nativeclient/design-documents/nacl-sfi-model-on-x86-64-systems
    #
    # "Normal" instructions can not be used in a place where %rsp is restricted.
    # But since these instructions are "second half" of the %rsp sandboxing they
    # can be used *only* when %rsp is restricted.
    #
    # That is (normal instruction):
    #   mov %eax,%esp
    #   mov %esi,%edi   <- Error: %esp is restricted
    # vs
    #   mov %esi,%edi
    #   add %r15,%rsp   <- Error: %esp is *not* restricted
    # vs
    #   mov %eax,%esp
    #   add %r15,%rsp   <- Ok: %rsp is restricted as it should be
    #
    # Check this precondition and mark the beginning of the instruction as
    # invalid jump for target.
    @{ if (restricted_register == NC_REG_RSP)
         instruction_info_collected |= RESTRICTED_REGISTER_USED;
       else
         instruction_info_collected |= UNRESTRICTED_RSP_PROCESSED;
       restricted_register = NC_NO_REG;
       UnmarkValidJumpTarget((instruction_begin - codeblock), valid_targets);
    };

  # naclcall or nacljmp. These are three-instruction indirection-jump sequences.
  #    and $~0x1f, %eXX
  #    and RBASE, %rXX
  #    jmpq *%rXX   (or: callq *%rXX)
  # Note: first "and $~0x1f, %eXX" is a normal instruction (it can occur not
  # just as part of the naclcall/nacljmp, but also as a standalone instruction).
  #
  # This means that when naclcall_or_nacljmp ragel machine will be combined with
  # "normal_instruction*" regular action process_1_operand_zero_extends will be
  # triggered when main ragel machine will accept "and $~0x1f, %eXX" x86-64
  # instruction.  This action will check if %rbp/%rsp is legally modified thus
  # we don't need to duplicate this logic in naclcall_or_nacljmp ragel machine.
  #
  # There are number of variants present which differ by the REX prefix usage:
  # we need to make sure "%eXX" in "and", "%rXX" in "add", and "%eXX" in "jmpq"
  # or "callq" is the same register and it's much simpler to do if one single
  # action handles only fixed number of bytes.
  #
  # Additional complication arises because x86-64 contains two different "add"
  # instruction: with "0x01" and "0x03" opcode.  They differ in the direction
  # used: both can encode "add %src_register, %dst_register", but the first one
  # uses field REG of the ModR/M byte for the src and field RM of the ModR/M
  # byte for the dst while last one uses field RM of the ModR/M byte for the src
  # and field REG of the ModR/M byte for dst.  Both should be allowed.
  #
  # See AMD/Intel manual for clarification about "add" instruction encoding.
  #
  # REGISTER USAGE ABBREVIATIONS:
  #     E86:   legacy ia32 registers (all eight: %eax to %edi)
  #     R86:   64-bit counterparts for legacy 386 registers (%rax to %rdi)
  #     E64:   32-bit counterparts for new amd64 registers (%r8d to %r14d)
  #     R64:   new amd64 registers (only seven: %r8 to %r14)
  #     RBASE: %r15 (used as "base of untrusted world" in NaCl for amd64)
  #
  # Note that in the actions below instruction_begin points to the start of the
  # "call" or "jmp" instruction and current_position points to its end.
  naclcall_or_nacljmp =
    # This block encodes call and jump "superinstruction" of the following form:
    #     0: 83 e_ e0    and    $~0x1f,E86
    #     3: 4_ 01 f_    add    RBASE,R86
    #     6: ff e_       jmpq   *R86
    #### INSTRUCTION ONE (three bytes)
    # and $~0x1f, E86
    (0x83 b_11_100_xxx 0xe0
    #### INSTRUCTION TWO (three bytes)
    # add RBASE, R86 (0x01 opcode)
     b_0100_11x0 0x01 b_11_111_xxx
    #### INSTRUCTION THREE: call (two bytes plus optional REX prefix)
    # callq R86
     ((REX_WRX? 0xff b_11_010_xxx) |
    #### INSTRUCTION THREE: jmp (two bytes plus optional REX prefix)
    # jmpq R86
      (REX_WRX? 0xff b_11_100_xxx)))
    @{
      ProcessNaclCallOrJmpAddToRMNoRex(&instruction_info_collected,
                                       &instruction_begin,
                                       current_position,
                                       codeblock,
                                       valid_targets);
    } |

    # This block encodes call and jump "superinstruction" of the following form:
    #     0: 83 e_ e0    and    $~0x1f,E86
    #     3: 4_ 03 _f    add    RBASE,R86
    #     6: ff e_       jmpq   *R86
    #### INSTRUCTION ONE (three bytes)
    # and $~0x1f, E86
    (0x83 b_11_100_xxx 0xe0
    #### INSTRUCTION TWO (three bytes)
    # add RBASE, R86 (0x03 opcode)
     b_0100_10x1 0x03 b_11_xxx_111
    #### INSTRUCTION THREE: call (two bytes plus optional REX prefix)
    # callq R86
     ((REX_WRX? 0xff b_11_010_xxx) |
    #### INSTRUCTION THREE: jmp (two bytes plus optional REX prefix)
    # jmpq R86
      (REX_WRX? 0xff b_11_100_xxx)))
    @{
      ProcessNaclCallOrJmpAddToRegNoRex(&instruction_info_collected,
                                        &instruction_begin,
                                        current_position,
                                        codeblock,
                                        valid_targets);
    } |

    # This block encodes call and jump "superinstruction" of the following form:
    #     0: 4_ 83 e_ e0 and    $~0x1f,E86
    #     4: 4_ 01 f_    add    RBASE,R86
    #     7: ff e_       jmpq   *R86
    #### INSTRUCTION ONE (four bytes)
    # and $~0x1f, E86
    ((REX_RX 0x83 b_11_100_xxx 0xe0
    #### INSTRUCTION TWO (three bytes)
    # add RBASE, R86 (0x01 opcode)
     b_0100_11x0 0x01 b_11_111_xxx
    #### INSTRUCTION THREE: call (two bytes plus optional REX prefix)
    # callq R86
     ((REX_WRX? 0xff b_11_010_xxx) |
    #### INSTRUCTION THREE: jmp (two bytes plus optional REX prefix)
    # jmpq R86
      (REX_WRX? 0xff b_11_100_xxx))) |

    # This block encodes call and jump "superinstruction" of the following form:
    #     0: 4_ 83 e_ e0 and    $~0x1f,E64
    #     4: 4_ 01 f_    add    RBASE,R64
    #     7: 4_ ff e_    jmpq   *R64
    #### INSTRUCTION ONE (four bytes)
    # and $~0x1f, E64
    (b_0100_0xx1 0x83 (b_11_100_xxx - b_11_100_111) 0xe0
    #### INSTRUCTION TWO (three bytes)
    # add RBASE, R64 (0x01 opcode)
     b_0100_11x1 0x01 (b_11_111_xxx - b_11_111_111)
    #### INSTRUCTION THREE: call (three bytes)
    # callq R64
     ((b_0100_xxx1 0xff (b_11_010_xxx - b_11_010_111)) |
    #### INSTRUCTION THREE: jmp (three bytes)
    # jmpq R64
      (b_0100_xxx1 0xff (b_11_100_xxx - b_11_100_111)))))
    @{
      ProcessNaclCallOrJmpAddToRMWithRex(&instruction_info_collected,
                                         &instruction_begin,
                                         current_position,
                                         codeblock,
                                         valid_targets);
    } |

    # This block encodes call and jump "superinstruction" of the following form:
    #     0: 4_ 83 e_ e0 and    $~0x1f,E86
    #     4: 4_ 03 _f    add    RBASE,R86
    #     7: ff e_       jmpq   *R86
    #### INSTRUCTION ONE (four bytes)
    # and $~0x1f, E86
    ((REX_RX 0x83 b_11_100_xxx 0xe0
    #### INSTRUCTION TWO (three bytes)
    # add RBASE, R86 (0x03 opcode)
     b_0100_10x1 0x03 b_11_xxx_111
    #### INSTRUCTION THREE: call (two bytes plus optional REX prefix)
    # callq R86
     ((REX_WRX? 0xff b_11_010_xxx) |
    #### INSTRUCTION THREE: jmp (two bytes plus optional REX prefix)
    # jmpq R86
      (REX_WRX? 0xff b_11_100_xxx))) |

    # This block encodes call and jump "superinstruction" of the following form:
    #     0: 4_ 83 e_ e0 and    $~0x1f,E64
    #     4: 4_ 03 _f    add    RBASE,R64
    #     7: 4_ ff e_    jmpq   *R64
    #### INSTRUCTION ONE (four bytes)
    # and $~0x1f, E64
     (b_0100_0xx1 0x83 (b_11_100_xxx - b_11_100_111) 0xe0
    #### INSTRUCTION TWO (three bytes)
    # add RBASE, R64 (0x03 opcode)
      b_0100_11x1 0x03 (b_11_xxx_111 - b_11_111_111)
    #### INSTRUCTION THREE: call (three bytes)
    # callq R64
      ((b_0100_xxx1 0xff (b_11_010_xxx - b_11_010_111)) |
    #### INSTRUCTION THREE: jmp (three bytes)
    # jmpq R64
       (b_0100_xxx1 0xff (b_11_100_xxx - b_11_100_111)))))
    @{
      ProcessNaclCallOrJmpAddToRegWithRex(&instruction_info_collected,
                                          &instruction_begin,
                                          current_position,
                                          codeblock,
                                          valid_targets);
    };

  # EMMX/SSE/SSE2/AVX instructions which have implicit %ds:(%rsi) operand

  # maskmovq %mmX,%mmY (EMMX or SSE)
  maskmovq = REX_WRXB? 0x0f 0xf7 @CPUFeature_EMMXSSE modrm_registers;

  # maskmovdqu %xmmX, %xmmY (SSE2)
  maskmovdqu = 0x66 REX_WRXB? 0x0f 0xf7 @CPUFeature_SSE2 modrm_registers;

  # vmaskmovdqu %xmmX, %xmmY (AVX)
  vmaskmovdqu = ((0xc4 (VEX_RB & VEX_map00001) b_0_1111_0_01) |
                 (0xc5 b_X_1111_0_01)) 0xf7 @CPUFeature_AVX modrm_registers;

  mmx_sse_rdi_instruction = maskmovq | maskmovdqu | vmaskmovdqu;

  # Temporary fix: for string instructions combination of data16 and rep(ne)
  # prefixes is disallowed to mimic old validator behavior.
  # See http://code.google.com/p/nativeclient/issues/detail?id=1950

  # data16rep = (data16 | rep data16 | data16 rep);
  # data16condrep = (data16 | condrep data16 | data16 condrep);
  data16rep = data16;
  data16condrep = data16;

  # String instructions which use only %ds:(%rsi)
  string_instruction_rsi_no_rdi =
    (rep? 0xac               | # lods   %ds:(%rsi),%al
     data16rep 0xad          | # lods   %ds:(%rsi),%ax
     rep? REXW_NONE? 0xad);    # lods   %ds:(%rsi),%eax/%rax

  # String instructions which use only %ds:(%rdi)
  string_instruction_rdi_no_rsi =
    condrep? 0xae            | # scas   %es:(%rdi),%al
    data16condrep 0xaf       | # scas   %es:(%rdi),%ax
    condrep? REXW_NONE? 0xaf | # scas   %es:(%rdi),%eax/%rax

    rep? 0xaa                | # stos   %al,%es:(%rdi)
    data16rep 0xab           | # stos   %ax,%es:(%rdi)
    rep? REXW_NONE? 0xab;      # stos   %eax/%rax,%es:(%rdi)

  # String instructions which use both %ds:(%rsi) and %es:(%rdi)
  string_instruction_rsi_rdi =
    condrep? 0xa6            | # cmpsb    %es:(%rdi),%ds:(%rsi)
    data16condrep 0xa7       | # cmpsw    %es:(%rdi),%ds:(%rsi)
    condrep? REXW_NONE? 0xa7 | # cmps[lq] %es:(%rdi),%ds:(%rsi)

    rep? 0xa4                | # movsb    %ds:(%rsi),%es:(%rdi)
    data16rep 0xa5           | # movsw    %ds:(%rsi),%es:(%rdi)
    rep? REXW_NONE? 0xa5;      # movs[lq] %ds:(%rsi),%es:(%rdi)

  # Sandboxing operations for %rsi.  There are two versions: 6 bytes long and
  # 7 bytes long (depending on presence of spurious REX prefix).
  #
  # Note that both "0x89 0xf6" and "0x8b 0xf6" encode "mov %edi,%edi": in x86-64
  # there are two fields in ModR/M byte (REG field and RM field) and "mov" may
  # be used to move from REG field to RM or in the other direction thus there
  # are two encodings for the register-to-register move (and since REG and RM
  # are identical here only opcode differs).
  sandbox_rsi_6_bytes =
    (0x89 | 0x8b) 0xf6         # mov %esi,%esi
    0x49 0x8d 0x34 0x37;       # lea (%r15,%rsi,1),%rsi
  sandbox_rsi_7_bytes =
    REX_X (0x89 | 0x8b) 0xf6   # mov %esi,%esi
    0x49 0x8d 0x34 0x37;       # lea (%r15,%rsi,1),%rsi

  # Sandboxing operations for %rdi.  There are two versions: 6 bytes long and
  # 7 bytes long (depending on presence of spurious REX prefix).
  #
  # Note that both "0x89 0xff" and "0x8b 0xff" encode "mov %edi,%edi": in x86-64
  # there are two fields in ModR/M byte (REG field and RM field) and "mov" may
  # be used to move from REG field to RM or in the other direction thus there
  # are two encodings for the register-to-register move (and since REG and RM
  # are identical here only opcode differs).
  sandbox_rdi_6_bytes =
    (0x89 | 0x8b) 0xff         # mov %edi,%edi
    0x49 0x8d 0x3c 0x3f;       # lea (%r15,%rdi,1),%rdi
  sandbox_rdi_7_bytes =
    REX_X (0x89 | 0x8b) 0xff   # mov %edi,%edi
    0x49 0x8d 0x3c 0x3f;       # lea (%r15,%rdi,1),%rdi

  # "Superinstruction" which includes %rsi sandboxing.
  #
  # There are two variants which handle spurious REX prefixes.
  sandbox_instruction_rsi_no_rdi =
    sandbox_rsi_6_bytes
    string_instruction_rsi_no_rdi
    @{
       ExpandSuperinstructionBySandboxingBytes(
         6 /* sandbox_rsi_6_bytes */,
         &instruction_begin,
         codeblock,
         valid_targets);
    } |

    sandbox_rsi_7_bytes
    string_instruction_rsi_no_rdi
    @{
       ExpandSuperinstructionBySandboxingBytes(
         7 /* sandbox_rsi_7_bytes */,
         &instruction_begin,
         codeblock,
         valid_targets);
    };

  # "Superinstruction" which includes %rdi sandboxing.
  #
  # There are two variants which handle spurious REX prefixes.
  sandbox_instruction_rdi_no_rsi =
    sandbox_rdi_6_bytes
    (string_instruction_rdi_no_rsi | mmx_sse_rdi_instruction)
    @{
       ExpandSuperinstructionBySandboxingBytes(
         6 /* sandbox_rdi_6_bytes */,
         &instruction_begin,
         codeblock,
         valid_targets);
    } |

    sandbox_rdi_7_bytes
    (string_instruction_rdi_no_rsi | mmx_sse_rdi_instruction)
    @{
       ExpandSuperinstructionBySandboxingBytes(
         7 /* sandbox_rdi_7_bytes */,
         &instruction_begin,
         codeblock,
         valid_targets);
    };


  # "Superinstruction" which includes both %rsi and %rdi sandboxing.
  #
  # There are four variants which handle spurious REX prefixes.
  sandbox_instruction_rsi_rdi =
    sandbox_rsi_6_bytes
    sandbox_rdi_6_bytes
    string_instruction_rsi_rdi
    @{
       ExpandSuperinstructionBySandboxingBytes(
         6 /* sandbox_rsi_6_bytes */ + 6 /* sandbox_rdi_6_bytes */,
         &instruction_begin,
         codeblock,
         valid_targets);
    } |

    ((sandbox_rsi_6_bytes
      sandbox_rdi_7_bytes) |

     (sandbox_rsi_7_bytes
      sandbox_rdi_6_bytes))
     string_instruction_rsi_rdi
    @{
       ExpandSuperinstructionBySandboxingBytes(
         6 /* sandbox_rsi_6_bytes */ + 7 /* sandbox_rdi_7_bytes */
         /* == 7 (* sandbox_rsi_6_bytes *) + 6 (* sandbox_rdi_6_bytes *) */,
         &instruction_begin,
         codeblock,
         valid_targets);
    } |

    sandbox_rsi_7_bytes
    sandbox_rdi_7_bytes
    string_instruction_rsi_rdi
    @{
       ExpandSuperinstructionBySandboxingBytes(
         7 /* sandbox_rsi_7_bytes */ + 7 /* sandbox_rdi_7_bytes */,
         &instruction_begin,
         codeblock,
         valid_targets);
    };

  # All the "special" instructions (== instructions which obey non-standard
  # rules).  Three groups:
  #  * %rsp/%rsp related instructions (these registers and operations which
  #    operate on them are special because registers must be in the range
  #    %r15...%r15+4294967295 except momentarily they can be in the range
  #    0...4294967295, but then the very next instruction MUST restore the
  #    status quo).
  #  * string instructions (which can not use %r15 as base and thus need special
  #    handling both in compiler and validator)
  #  * naclcall/nacljmp (indirect jumps need special care)
  special_instruction =
    (rbp_modifications |
     rsp_modifications |
     rbp_sandboxing |
     rsp_sandboxing |
     sandbox_instruction_rsi_no_rdi |
     sandbox_instruction_rdi_no_rsi |
     sandbox_instruction_rsi_rdi |
     naclcall_or_nacljmp)
    # Mark the instruction as special - currently this information is used only
    # in tests, but in the future we may use it for dynamic code modification
    # support.
    @{
       instruction_info_collected |= SPECIAL_INSTRUCTION;
    };

  # Remove special instructions which are only allowed in special cases.
  normal_instruction = one_instruction - special_instruction;

  # For direct call we explicitly encode all variations.
  direct_call = (data16 REX_RXB? 0xe8 rel16) |
                (REX_WRXB? 0xe8 rel32) |
                (data16 REXW_RXB 0xe8 rel32);

  # For indirect call we accept only near register-addressed indirect call.
  indirect_call_register = data16? REX_WRXB? 0xff (opcode_2 & modrm_registers);

  # Ragel machine that accepts one call instruction or call superinstruction and
  # checks if call is properly aligned.
  call_alignment =
    ((normal_instruction & direct_call) |
     # For indirect calls we accept all the special instructions which ends with
     # register-addressed indirect call.
     (special_instruction & (any* indirect_call_register)))
    # Call instruction must aligned to the end of bundle.  Previously this was
    # strict requirement, today it's just warning to aid with debugging.
    @{
      if (((current_position - codeblock) & kBundleMask) != kBundleMask)
        instruction_info_collected |= BAD_CALL_ALIGNMENT;
    };

  # This action calls users callback (if needed) and cleans up validator
  # internal state.
  #
  # We call the user callback either on validation errors or on every
  # instruction, depending on CALL_USER_CALLBACK_ON_EACH_INSTRUTION option.
  #
  # After that we move instruction_begin and clean all the variables which
  # are only used in the processing of a single instruction (prefixes, operand
  # states and instruction_info_collected).
  action end_of_instruction_cleanup {
    /* Call user-supplied callback.  */
    instruction_end = current_position + 1;
    if ((instruction_info_collected & VALIDATION_ERRORS_MASK) ||
        (options & CALL_USER_CALLBACK_ON_EACH_INSTRUCTION)) {
      result &= user_callback(
          instruction_begin, instruction_end,
          instruction_info_collected |
          ((restricted_register << RESTRICTED_REGISTER_SHIFT) &
           RESTRICTED_REGISTER_MASK), callback_data);
    }

    /* On successful match the instruction_begin must point to the next byte
     * to be able to report the new offset as the start of instruction
     * causing error.  */
    instruction_begin = instruction_end;

    /*
     * We may set instruction_begin at the first byte of the instruction instead
     * of here but in the case of incorrect one byte instructions user callback
     * may be called before instruction_begin is set.
     */
    MarkValidJumpTarget(instruction_begin - codeblock, valid_targets);

    /* Clear variables.  */
    instruction_info_collected = 0;
    SET_REX_PREFIX(FALSE);
    /* Top three bits of VEX2 are inverted: see AMD/Intel manual.  */
    SET_VEX_PREFIX2(VEX_R | VEX_X | VEX_B);
    SET_VEX_PREFIX3(0x00);
    operand_states = 0;
    base = 0;
    index = 0;
  }

  # This action reports fatal error detected by DFA.
  action report_fatal_error {
    result &= user_callback(instruction_begin, current_position,
                            UNRECOGNIZED_INSTRUCTION, callback_data);
    /*
     * Process the next bundle: "continue" here is for the "for" cycle in
     * the ValidateChunkAMD64 function.
     *
     * It does not affect the case which we really care about (when code
     * is validatable), but makes it possible to detect more errors in one
     * run in tools like ncval.
     */
    continue;
  }

  # This is main ragel machine: it does 99% of validation work. There are only
  # one thing to do with bundle if this ragel machine accepts the bundle:
  #  * check for the state of the restricted_register at the end of the bundle.
  #     It's an error is %rbp or %rsp is restricted at the end of the bundle.
  # Additionally if all the bundles are fine you need to check that direct jumps
  # are corect.  Thiis is done in the following way:
  #  * DFA fills two arrays: valid_targets and jump_dests.
  #  * ProcessInvalidJumpTargets checks that "jump_dests & !valid_targets == 0".
  # All other checks are done here.

  main := ((call_alignment | normal_instruction | special_instruction)
     @end_of_instruction_cleanup)*
    $!report_fatal_error;

}%%

/*
 * The "write data" statement causes Ragel to emit the constant static data
 * needed by the ragel machine.
 */
%% write data;

/*
 * Operand's kind WRT sandboxing effect: no effect, can be used for sandboxing
 * and will make register invalid if used.
 *
 * No effect is the "initial state", 32bit stores can be used for sandboxing (in
 * that case he high 32-bit bits of the corresponding 64-bit register are set to
 * zero) and we do not distinguish modifications of 16bit and 64bit registers to
 * match the behavior of the old validator.
 *
 * 8bit operands must be distinguished from other types because the REX prefix
 * regulates the choice between %ah and %spl, as well as %ch and %bpl.
 */
enum OperandKind {
  OPERAND_SANDBOX_IRRELEVANT   = 0x00,
  /* 8bit register that is modified by instruction.  */
  OPERAND_SANDBOX_8BIT         = 0x10,
  /*
   * 32-bit register that is modified by instruction.  The high 32-bit bits of
   * the corresponding 64-bit register are set to zero.
   */
  OPERAND_SANDBOX_RESTRICTED   = 0x20,
  /* 64-bit or 16-bit register that is modified by instruction.  */
  OPERAND_SANDBOX_UNRESTRICTED = 0x30,
  /* This is mask for OperandKind.  */
  OPERAND_KIND_MASK            = 0x030
};

/*
 * operand_states variable keeps one byte of information per operand in the
 * current instruction:
 *  * the first 4 bits (least significant ones) are for register numbers,
 *  * the next 2 bits for register kinds.
 *
 * Macroses below are used to access this data.
 */
#define SET_OPERAND_NAME(INDEX, REGISTER_NAME)                                 \
  operand_states |= ((REGISTER_NAME) << ((INDEX) << 3))
#define SET_OPERAND_FORMAT(INDEX, FORMAT)                                      \
  SET_OPERAND_FORMAT_ ## FORMAT(INDEX)
#define SET_OPERAND_FORMAT_OPERAND_FORMAT_8_BIT(INDEX)                         \
  operand_states |= OPERAND_SANDBOX_8BIT << ((INDEX) << 3)
#define SET_OPERAND_FORMAT_OPERAND_FORMAT_16_BIT(INDEX)                        \
  operand_states |= OPERAND_SANDBOX_UNRESTRICTED << ((INDEX) << 3)
#define SET_OPERAND_FORMAT_OPERAND_FORMAT_32_BIT(INDEX)                        \
  operand_states |= OPERAND_SANDBOX_RESTRICTED << ((INDEX) << 3)
#define SET_OPERAND_FORMAT_OPERAND_FORMAT_64_BIT(INDEX)                        \
  operand_states |= OPERAND_SANDBOX_UNRESTRICTED << ((INDEX) << 3)
#define CHECK_OPERAND(INDEX, REGISTER_NAME, KIND)                              \
  ((operand_states &                                                           \
    ((NC_REG_MASK | OPERAND_KIND_MASK) << ((INDEX) << 3))) ==                  \
    (((KIND) | (REGISTER_NAME)) << ((INDEX) << 3)))
#define CHECK_OPERAND_R15_MODIFIED(INDEX)                                      \
  (CHECK_OPERAND((INDEX), NC_REG_R15, OPERAND_SANDBOX_8BIT) ||                 \
   CHECK_OPERAND((INDEX), NC_REG_R15, OPERAND_SANDBOX_RESTRICTED) ||           \
   CHECK_OPERAND((INDEX), NC_REG_R15, OPERAND_SANDBOX_UNRESTRICTED))
/*
 * Note that macroses below access operand_states variable and also rex_prefix
 * variable.  This is to distinguish %ah from %spl, as well as %ch from %bpl.
 */
#define CHECK_OPERAND_BP_MODIFIED(INDEX)                                       \
  ((CHECK_OPERAND((INDEX), NC_REG_RBP, OPERAND_SANDBOX_8BIT) && rex_prefix) || \
    CHECK_OPERAND((INDEX), NC_REG_RBP, OPERAND_SANDBOX_RESTRICTED) ||          \
    CHECK_OPERAND((INDEX), NC_REG_RBP, OPERAND_SANDBOX_UNRESTRICTED))
#define CHECK_OPERAND_SP_MODIFIED(INDEX)                                       \
  ((CHECK_OPERAND((INDEX), NC_REG_RSP, OPERAND_SANDBOX_8BIT) && rex_prefix) || \
    CHECK_OPERAND((INDEX), NC_REG_RSP, OPERAND_SANDBOX_RESTRICTED) ||          \
    CHECK_OPERAND((INDEX), NC_REG_RSP, OPERAND_SANDBOX_UNRESTRICTED))          \
/*
 * This is for Process?OperandsZeroExtends functions: in this case %esp or %ebp
 * can be written to, but %spl/%sp/%rsp or %bpl/%bp/%rbp can not be modified.
 */
#define CHECK_OPERAND_BP_INVALID_MODIFICATION(INDEX)                           \
  ((CHECK_OPERAND((INDEX), NC_REG_RBP, OPERAND_SANDBOX_8BIT) && rex_prefix) || \
    CHECK_OPERAND((INDEX), NC_REG_RBP, OPERAND_SANDBOX_UNRESTRICTED))
#define CHECK_OPERAND_SP_INVALID_MODIFICATION(INDEX)                           \
  ((CHECK_OPERAND((INDEX), NC_REG_RSP, OPERAND_SANDBOX_8BIT) && rex_prefix) || \
    CHECK_OPERAND((INDEX), NC_REG_RSP, OPERAND_SANDBOX_UNRESTRICTED))
#define CHECK_OPERAND_RESTRICTED(INDEX)                                        \
  ((operand_states &                                                           \
    (OPERAND_KIND_MASK << ((INDEX) << 3))) ==                                  \
     OPERAND_SANDBOX_RESTRICTED << ((INDEX) << 3))
#define GET_OPERAND_NAME(INDEX)                                                \
  ((operand_states >> ((INDEX) << 3)) & NC_REG_MASK)

static INLINE void CheckMemoryAccess(ptrdiff_t instruction_begin,
                                     enum OperandName base,
                                     enum OperandName index,
                                     uint8_t restricted_register,
                                     bitmap_word *valid_targets,
                                     uint32_t *instruction_info_collected) {
  if ((base == NC_REG_RIP) || (base == NC_REG_R15) ||
      (base == NC_REG_RSP) || (base == NC_REG_RBP)) {
    if ((index == NC_NO_REG) || (index == NC_REG_RIZ))
      { /* do nothing. */ }
    else if (index == restricted_register)
      BitmapClearBit(valid_targets, instruction_begin),
      *instruction_info_collected |= RESTRICTED_REGISTER_USED;
    else
      *instruction_info_collected |= UNRESTRICTED_INDEX_REGISTER;
  } else {
    *instruction_info_collected |= FORBIDDEN_BASE_REGISTER;
  }
}

static FORCEINLINE uint32_t CheckValidityOfRegularInstruction(
    enum OperandName restricted_register) {
  /*
   * Restricted %rsp or %rbp must be %rsp or %rbp must be restored from
   * zero-extension state by appropriate "special" instruction, not with
   * regular instruction.
   */
  if (restricted_register == NC_REG_RBP)
    return RESTRICTED_RBP_UNPROCESSED;
  if (restricted_register == NC_REG_RSP)
    return RESTRICTED_RSP_UNPROCESSED;
  return 0;
}

static INLINE void Process0Operands(enum OperandName *restricted_register,
                                    uint32_t *instruction_info_collected) {
  *instruction_info_collected |=
    CheckValidityOfRegularInstruction(*restricted_register);
  /* Every instruction clears restricted register even if it is not modified. */
  *restricted_register = NC_NO_REG;
}

static INLINE void Process1Operand(enum OperandName *restricted_register,
                                   uint32_t *instruction_info_collected,
                                   uint8_t rex_prefix,
                                   uint32_t operand_states) {
  *instruction_info_collected |=
    CheckValidityOfRegularInstruction(*restricted_register);
  if (CHECK_OPERAND_R15_MODIFIED(0))
    *instruction_info_collected |= R15_MODIFIED;
  if (CHECK_OPERAND_BP_MODIFIED(0))
    *instruction_info_collected |= BP_MODIFIED;
  if (CHECK_OPERAND_SP_MODIFIED(0))
    *instruction_info_collected |= SP_MODIFIED;
  /* Every instruction clears restricted register even if it is not modified. */
  *restricted_register = NC_NO_REG;
}

static INLINE void Process1OperandZeroExtends(
    enum OperandName *restricted_register,
    uint32_t *instruction_info_collected,
    uint8_t rex_prefix,
    uint32_t operand_states) {
  *instruction_info_collected |=
    CheckValidityOfRegularInstruction(*restricted_register);
  /* Every instruction clears restricted register even if it is not modified. */
  *restricted_register = NC_NO_REG;
  if (CHECK_OPERAND_R15_MODIFIED(0))
    *instruction_info_collected |= R15_MODIFIED;
  if (CHECK_OPERAND_BP_INVALID_MODIFICATION(0))
    *instruction_info_collected |= BP_MODIFIED;
  if (CHECK_OPERAND_SP_INVALID_MODIFICATION(0))
    *instruction_info_collected |= SP_MODIFIED;
  if (CHECK_OPERAND_RESTRICTED(0))
    *restricted_register = GET_OPERAND_NAME(0);
}

static INLINE void Process2Operands(enum OperandName *restricted_register,
                                    uint32_t *instruction_info_collected,
                                    uint8_t rex_prefix,
                                    uint32_t operand_states) {
  *instruction_info_collected |=
    CheckValidityOfRegularInstruction(*restricted_register);
  if (CHECK_OPERAND_R15_MODIFIED(0) || CHECK_OPERAND_R15_MODIFIED(1))
    *instruction_info_collected |= R15_MODIFIED;
  if (CHECK_OPERAND_BP_MODIFIED(0) || CHECK_OPERAND_BP_MODIFIED(1))
    *instruction_info_collected |= BP_MODIFIED;
  if (CHECK_OPERAND_SP_MODIFIED(0) || CHECK_OPERAND_SP_MODIFIED(1))
    *instruction_info_collected |= SP_MODIFIED;
  /* Every instruction clears restricted register even if it is not modified. */
  *restricted_register = NC_NO_REG;
}

static INLINE void Process2OperandsZeroExtends(
     enum OperandName *restricted_register,
     uint32_t *instruction_info_collected,
     uint8_t rex_prefix,
     uint32_t operand_states) {
  *instruction_info_collected |=
    CheckValidityOfRegularInstruction(*restricted_register);
  /* Every instruction clears restricted register even if it is not modified. */
  *restricted_register = NC_NO_REG;
  if (CHECK_OPERAND_R15_MODIFIED(0) ||
      CHECK_OPERAND_R15_MODIFIED(1))
    *instruction_info_collected |= R15_MODIFIED;
  if (CHECK_OPERAND_BP_INVALID_MODIFICATION(0) ||
      CHECK_OPERAND_BP_INVALID_MODIFICATION(1))
    *instruction_info_collected |= BP_MODIFIED;
  if (CHECK_OPERAND_SP_INVALID_MODIFICATION(0) ||
      CHECK_OPERAND_SP_INVALID_MODIFICATION(1))
    *instruction_info_collected |= SP_MODIFIED;
  if (CHECK_OPERAND_RESTRICTED(0)) {
    *restricted_register = GET_OPERAND_NAME(0);
    /*
     * If both operands are sandboxed, the second one doesn't count.  We can't
     * ignore it completely though, since it can modify %rsp or %rbp which must
     * follow special rules.  In this case NaCl forbids the instruction.
     */
    if (CHECK_OPERAND(1, NC_REG_RSP, OPERAND_SANDBOX_RESTRICTED))
      *instruction_info_collected |= RESTRICTED_RSP_UNPROCESSED;
    if (CHECK_OPERAND(1, NC_REG_RBP, OPERAND_SANDBOX_RESTRICTED))
      *instruction_info_collected |= RESTRICTED_RBP_UNPROCESSED;
  } else if (CHECK_OPERAND_RESTRICTED(1)) {
    *restricted_register = GET_OPERAND_NAME(1);
  }
}

/*
 * This function merges "dangerous" instruction with sandboxing instructions to
 * get a "superinstruction" and unmarks in-between jump targets.
 */
static INLINE void ExpandSuperinstructionBySandboxingBytes(
    size_t sandbox_instructions_size,
    const uint8_t **instruction_begin,
    const uint8_t codeblock[],
    bitmap_word *valid_targets) {
  *instruction_begin -= sandbox_instructions_size;
  /*
   * We need to unmark start of the "dangerous" instruction itself, too, but we
   * don't need to mark the beginning of the whole "superinstruction" - that's
   * why we move start by one byte and don't change the length.
   */
  UnmarkValidJumpTargets((*instruction_begin + 1 - codeblock),
                         sandbox_instructions_size,
                         valid_targets);
}

/*
 * Return TRUE if naclcall or nacljmp uses the same register in all three
 * instructions.
 *
 * This version is for the case where "add %src_register, %dst_register" with
 * dst in RM field and src in REG field of ModR/M byte is used.
 *
 * There are five possible forms:
 *
 *              0: 83 eX e0    and    $~0x1f,E86
 *              3: 4? 01 fX    add    RBASE,R86
 *              6: ff eX       jmpq   *R86
 *                 ^  ^
 * instruction_begin  current_position
 *
 *              0: 4? 83 eX e0 and    $~0x1f,E86
 *              4: 4? 01 fX    add    RBASE,R86
 *              7: ff eX       jmpq   *R86
 *                 ^  ^
 * instruction_begin  current_position
 *
 *              0: 83 eX e0    and    $~0x1f,E86
 *              3: 4? 01 fX    add    RBASE,R86
 *              6: 4? ff eX    jmpq   *R86
 *                 ^     ^
 * instruction_begin     current_position
 *
 *              0: 4? 83 eX e0 and    $~0x1f,E86
 *              4: 4? 01 fX    add    RBASE,R86
 *              7: 4? ff eX       jmpq   *R86
 *                 ^     ^
 * instruction_begin     current_position
 *
 *              0: 4? 83 eX e0 and    $~0x1f,E64
 *              4: 4? 01 fX    add    RBASE,R64
 *              7: 4? ff eX    jmpq   *R64
 *                 ^     ^
 * instruction_begin     current_position
 *
 * We don't care about "?" (they are checked by DFA).
 */
static INLINE Bool VerifyNaclCallOrJmpAddToRM(const uint8_t *instruction_begin,
                                              const uint8_t *current_position) {
  return
    RMFromModRM(instruction_begin[-5]) == RMFromModRM(instruction_begin[-1]) &&
    RMFromModRM(instruction_begin[-5]) == RMFromModRM(current_position[0]);
}

/*
 * Return TRUE if naclcall or nacljmp uses the same register in all three
 * instructions.
 *
 * This version is for the case where "add %src_register, %dst_register" with
 * dst in REG field and src in RM field of ModR/M byte is used.
 *
 * There are five possible forms:
 *
 *              0: 83 eX e0    and    $~0x1f,E86
 *              3: 4? 03 Xf    add    RBASE,R86
 *              6: ff eX       jmpq   *R86
 *                 ^  ^
 * instruction_begin  current_position
 *
 *              0: 4? 83 eX e0 and    $~0x1f,E86
 *              4: 4? 03 Xf    add    RBASE,R86
 *              7: ff eX       jmpq   *R86
 *                 ^  ^
 * instruction_begin  current_position
 *
 *              0: 83 eX e0    and    $~0x1f,E86
 *              3: 4? 03 Xf    add    RBASE,R86
 *              6: 4? ff eX       jmpq   *R86
 *                 ^     ^
 * instruction_begin     current_position
 *
 *              0: 4? 83 eX e0 and    $~0x1f,E86
 *              4: 4? 03 Xf    add    RBASE,R86
 *              7: 4? ff eX       jmpq   *R86
 *                 ^     ^
 * instruction_begin     current_position
 *
 *              0: 4? 83 eX e0 and    $~0x1f,E64
 *              4: 4? 03 Xf    add    RBASE,R64
 *              7: 4? ff eX    jmpq   *R64
 *                 ^     ^
 * instruction_begin     current_position
 *
 * We don't care about "?" (they are checked by DFA).
 */
static INLINE Bool VerifyNaclCallOrJmpAddToReg(
    const uint8_t *instruction_begin,
    const uint8_t *current_position) {
  return
    RMFromModRM(instruction_begin[-5]) == RegFromModRM(instruction_begin[-1]) &&
    RMFromModRM(instruction_begin[-5]) == RMFromModRM(current_position[0]);
}

/*
 * This function checks that naclcall or nacljmp are correct (that is: three
 * component instructions match) and if that is true then it merges call or jmp
 * with a sandboxing to get a "superinstruction" and removes in-between jump
 * targets.  If it's not true then it triggers "unrecognized instruction" error
 * condition.
 *
 * This version is for the case where "add with dst register in RM field"
 * (opcode 0x01) and "add without REX prefix" is used.
 *
 * There are two possibile forms:
 *
 *              0: 83 eX e0    and    $~0x1f,E86
 *              3: 4? 01 fX    add    RBASE,R86
 *              6: ff eX       jmpq   *R86
 *                 ^  ^
 * instruction_begin  current_position
 *
 *              0: 83 eX e0    and    $~0x1f,E86
 *              3: 4? 01 fX    add    RBASE,R86
 *              6: 4? ff eX    jmpq   *R86
 *                 ^     ^
 * instruction_begin     current_position
 */
static INLINE void ProcessNaclCallOrJmpAddToRMNoRex(
    uint32_t *instruction_info_collected,
    const uint8_t **instruction_begin,
    const uint8_t *current_position,
    const uint8_t codeblock[],
    bitmap_word *valid_targets) {
  if (VerifyNaclCallOrJmpAddToRM(*instruction_begin, current_position))
    ExpandSuperinstructionBySandboxingBytes(
      3 /* and */ + 3 /* add */,
      instruction_begin,
      codeblock,
      valid_targets);
  else
    *instruction_info_collected |= UNRECOGNIZED_INSTRUCTION;
}

/*
 * This function checks that naclcall or nacljmp are correct (that is: three
 * component instructions match) and if that is true then it merges call or jmp
 * with a sandboxing to get a "superinstruction" and removes in-between jump
 * targets.  If it's not true then it triggers "unrecognized instruction" error
 * condition.
 *
 * This version is for the case where "add with dst register in REG field"
 * (opcode 0x03) and "add without REX prefix" is used.
 *
 * There are two possibile forms:
 *
 *              0: 83 eX e0    and    $~0x1f,E86
 *              3: 4? 03 Xf    add    RBASE,R86
 *              6: ff eX       jmpq   *R86
 *                 ^  ^
 * instruction_begin  current_position
 *
 *              0: 83 eX e0    and    $~0x1f,E86
 *              3: 4? 03 Xf    add    RBASE,R86
 *              6: 4? ff eX    jmpq   *R86
 *                 ^     ^
 * instruction_begin     current_position
 */
static INLINE void ProcessNaclCallOrJmpAddToRegNoRex(
    uint32_t *instruction_info_collected,
    const uint8_t **instruction_begin,
    const uint8_t *current_position,
    const uint8_t codeblock[],
    bitmap_word *valid_targets) {
  if (VerifyNaclCallOrJmpAddToReg(*instruction_begin, current_position))
    ExpandSuperinstructionBySandboxingBytes(
      3 /* and */ + 3 /* add */,
      instruction_begin,
      codeblock,
      valid_targets);
  else
    *instruction_info_collected |= UNRECOGNIZED_INSTRUCTION;
}

/*
 * This function checks that naclcall or nacljmp are correct (that is: three
 * component instructions match) and if that is true then it merges call or jmp
 * with a sandboxing to get a "superinstruction" and removes in-between jump
 * targets.  If it's not true then it triggers "unrecognized instruction" error
 * condition.
 *
 * This version is for the case where "add with dst register in RM field"
 * (opcode 0x01) and "add without REX prefix" is used.
 *
 * There are three possibile forms:
 *
 *              0: 4? 83 eX e0 and    $~0x1f,E86
 *              4: 4? 01 fX    add    RBASE,R86
 *              7: ff eX    jmpq   *R86
 *                 ^  ^
 * instruction_begin  current_position
 *
 *              0: 4? 83 eX e0 and    $~0x1f,E86
 *              4: 4? 01 fX    add    RBASE,R86
 *              7: 4? ff eX    jmpq   *R86
 *                 ^     ^
 * instruction_begin     current_position
 *
 *              0: 4? 83 eX e0 and    $~0x1f,E64
 *              4: 4? 01 fX    add    RBASE,R64
 *              7: 4? ff eX    jmpq   *R64
 *                 ^     ^
 * instruction_begin     current_position
 */
static INLINE void ProcessNaclCallOrJmpAddToRMWithRex(
    uint32_t *instruction_info_collected,
    const uint8_t **instruction_begin,
    const uint8_t *current_position,
    const uint8_t codeblock[],
    bitmap_word *valid_targets) {
  if (VerifyNaclCallOrJmpAddToRM(*instruction_begin, current_position))
    ExpandSuperinstructionBySandboxingBytes(
      4 /* and */ + 3 /* add */,
      instruction_begin,
      codeblock,
      valid_targets);
  else
    *instruction_info_collected |= UNRECOGNIZED_INSTRUCTION;
}

/*
 * This function checks that naclcall or nacljmp are correct (that is: three
 * component instructions match) and if that is true then it merges call or jmp
 * with a sandboxing to get a "superinstruction" and removes in-between jump
 * targets.  If it's not true then it triggers "unrecognized instruction" error
 * condition.
 *
 * This version is for the case where "add with dst register in REG field"
 * (opcode 0x03) and "add without REX prefix" is used.
 *
 * There are three possibile forms:
 *
 *              0: 4? 83 eX e0 and    $~0x1f,E86
 *              4: 4? 03 Xf    add    RBASE,R86
 *              7: ff eX    jmpq   *R86
 *                 ^  ^
 * instruction_begin  current_position
 *
 *              0: 4? 83 eX e0 and    $~0x1f,E86
 *              4: 4? 03 Xf    add    RBASE,R86
 *              7: 4? ff eX    jmpq   *R86
 *                 ^     ^
 * instruction_begin     current_position
 *
 *              0: 4? 83 eX e0 and    $~0x1f,E64
 *              4: 4? 03 Xf    add    RBASE,R64
 *              7: 4? ff eX    jmpq   *R64
 *                 ^     ^
 * instruction_begin     current_position
 */
static INLINE void ProcessNaclCallOrJmpAddToRegWithRex(
    uint32_t *instruction_info_collected,
    const uint8_t **instruction_begin,
    const uint8_t *current_position,
    const uint8_t codeblock[],
    bitmap_word *valid_targets) {
  if (VerifyNaclCallOrJmpAddToReg(*instruction_begin, current_position))
    ExpandSuperinstructionBySandboxingBytes(
      4 /* and */ + 3 /* add */,
      instruction_begin,
      codeblock,
      valid_targets);
  else
    *instruction_info_collected |= UNRECOGNIZED_INSTRUCTION;
}


Bool ValidateChunkAMD64(const uint8_t codeblock[],
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

  /*
   * For a very small sequences (one bundle) malloc is too expensive.
   *
   * Note1: we allocate one extra bit, because we set valid jump target bits
   * _after_ instructions, so there will be one at the end of the chunk.
   *
   * Note2: we don't ever mark first bit as a valid jump target but this is
   * not a problem because any aligned address is valid jump target.
   */
  if ((size + 1) <= (sizeof valid_targets_small * 8)) {
    valid_targets_small = 0;
    valid_targets = &valid_targets_small;
    jump_dests_small = 0;
    jump_dests = &jump_dests_small;
  } else {
    valid_targets = BitmapAllocate(size + 1);
    jump_dests = BitmapAllocate(size + 1);
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
   * Main loop.  Here we process the codeblock array bundle-after-bundle.
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
    int current_state;
    uint32_t instruction_info_collected = 0;
    /*
     * Contains register number and type of register modification (see
     * OperandKind enum) for each operand that is changed in the instruction.
     * Information about read-only and memory operands is not saved in 64-bit
     * mode.
     */
    uint32_t operand_states = 0;
    enum OperandName base = NC_NO_REG;
    enum OperandName index = NC_NO_REG;
    enum OperandName restricted_register =
      EXTRACT_RESTRICTED_REGISTER_INITIAL_VALUE(options);
    uint8_t rex_prefix = FALSE;
    /* Top three bits of VEX2 are inverted: see AMD/Intel manual.  */
    uint8_t vex_prefix2 = VEX_R | VEX_X | VEX_B;
    uint8_t vex_prefix3 = 0x00;

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

    /*
     * Ragel DFA accepted the bundle, but we still need to make sure the last
     * instruction haven't left %rbp or %rsp in restricted state.
     */
    if (restricted_register == NC_REG_RBP)
      result &= user_callback(end_position, end_position,
                              RESTRICTED_RBP_UNPROCESSED |
                              ((NC_REG_RBP << RESTRICTED_REGISTER_SHIFT) &
                               RESTRICTED_REGISTER_MASK), callback_data);
    else if (restricted_register == NC_REG_RSP)
      result &= user_callback(end_position, end_position,
                              RESTRICTED_RSP_UNPROCESSED |
                              ((NC_REG_RSP << RESTRICTED_REGISTER_SHIFT) &
                               RESTRICTED_REGISTER_MASK), callback_data);
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
