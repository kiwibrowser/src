# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

################################################################################
# File format:
#     three columns separated by commas.  Each line describes one instruction.
#     Notation for argument types and sizes and for opcodes is based on
#     AMD64 Architecture Programmer's Manual.
################################################################################
# First column: instruction description.
#   Includes name of the instruction and arguments.
#
#   Arguments consist of four parts:
#      1. Read/write attribute (optional).
#      2. Argument type.
#      3. Argument size.
#      4. Implicit argument mark (optional).
#
#      Read/write attribute:
#       ': Instruction does not use this argument (lea or nop).
#       =: Instruction reads from this argument.
#       !: Instruction writes to this argument.
#       &: Instruction reads this argument and writes the result to it.
#        By default one- and two-operand instructions are assumed to read all
#        operands and store result to the last one, while instructions with
#        three or more operands are assumed to read all operands except last one
#        which is used to store the result of the execution.
#      Possible argument types:
#       a: Accumulator: %al/%ax/%eax/%rax/%xmm0 (depending on size).
#       c: Counter register: %cl/%cx/%ecx/%rcx (depending on size).
#       d: Data register: %dl/%dx/%edx/%rdx (depending on size).
#       f: x87 register in opcode (3 least significant bits).
#       i: Second immediate value encoded in the instruction.
#       o: I/O port in %dx (used in "in"/"out" instructions).
#       r: Register in opcode (3 least significant bits plus rex.B).
#       t: Top of the x87 stack (%st).
#       x: A memory operand addressed by the %ds:(%[er]bx). See "xlat".
#       B: General purpose register specified by the VEX/XOP.vvvv field.
#       C: Control register specified by the ModRM.reg field.
#       D: Debug register specified by the ModRM.reg field.
#       E: General purpose register or memory operand specified by the r/m
#          field of the ModRM byte.  For memory operands, the ModRM byte may
#          be followed by a SIB byte to specify one of the indexed
#          register-indirect addressing forms.
#       G: General purpose register specified by the reg field of ModRM.
#       H: YMM or XMM register specified by the VEX/XOP.vvvv field.
#       I: Immediate value encoded in the instruction.
#       J: The instruction encoding includes a relative offset that is added to
#          the rIP.
#       L: YMM or XMM register specified using the most-significant 4 bits of
#          the last byte of the instruction.  In legacy or compatibility mode
#          the most significant bit is ignored.
#       M: A memory operand specified by the {mod, r/m} field of the ModRM byte.
#          ModRM.mod != 11b.
#       N: 64-bit MMX register specified by the ModRM.r/m field. The ModRM.mod
#          field must be 11b.
#       O: The offset of an operand is encoded in the instruction. There is no
#          ModRM byte in the instruction encoding. Indexed register-indirect
#          addressing using the SIB byte is not supported.
#       P: 64-bit MMX register specified by the ModRM.reg field.
#       Q: 64-bit MMX-register or memory operand specified by the {mod, r/m}
#          field of the ModRM byte.  For memory operands, the ModRM byte may
#          be followed by a SIB byte to specify one of the indexed
#          register-indirect addressing forms.
#       R: General purpose register specified by the ModRM.r/m field.
#          The ModRM.mod field must be 11b.
#       S: Segment register specified by the ModRM.reg field.
#       U: YMM/XMM register specified by the ModRM.r/m field.
#          The ModRM.mod field must be 11b.
#       V: YMM/XMM register specified by the ModRM.reg field.
#       W: YMM/XMM register or memory operand specified by the {mod, r/m} field
#          of the ModRM byte.   For memory operands, the ModRM byte may be
#          followed by a SIB byte to specify one of the indexed
#          register-indirect addressing forms.
#       X: A memory operand addressed by the %ds:%[er]si registers. Used in
#          string instructions.
#       Y: A memory operand addressed by the %es:%[er]di registers. Used in
#          string instructions.
#      Possible sizes:
#       (no size provided):
#             A byte, word, doubleword, or quadword (in 64-bit mode),
#             depending on the effective operand size.
#       2:    Two bits (see VPERMIL2Px instruction).
#       7:    x87 register %st(N).
#       b:    A byte, irrespective of the effective operand size.
#       d:    A doubleword (32-bit), irrespective of the effective operand size.
#       do:   A double octword (256 bits), irrespective of the effective operand
#             size.
#       dq:   A double quadword (128 bits), irrespective of the effective
#             operand size.
#       fq:   A quadra quadword (256 bits), irrespective of the effective
#             operand size.
#       o:    An octword (128 bits), irrespective of the effective operand size.
#       p:    A 32-bit or 48-bit far pointer, depending on the effective operand
#             size.
#       pb:   A Vector with byte-wide (8-bit) elements (packed byte).
#       pbx:  A Vector with byte-wide (8-bit) elements (packed byte). L bit
#             selects 256bit YMM registers.
#       pd:   A double-precision (64-bit) floating-point vector operand (packed
#             double-precision).
#       pdw:  Vector composed of 32-bit doublewords.
#       pdwx: Vector composed of 32-bit doublewords. L bit selects 256bit YMM
#             registers.
#       pdx:  A double-precision (64-bit) floating-point vector operand (packed
#             double-precision).  L bit selects 256bit YMM registers.
#       ph:   A half-precision (16-bit) floating-point vector operand (packed
#             half-precision).
#       phx:  A half-precision (16-bit) floating-point vector operand (packed
#             half-precision).  L bit selects 256bit YMM registers.
#       pi:   Vector composed of 16-bit integers (packed integer).
#       pix:  Vector composed of 16-bit integers (packed integer).
#             L bit selects 256 bit YMM registers.
#       pj:   Vector composed of 32-bit integers (packed double integer).
#       pjx:  Vector composed of 32-bit integers (packed double integer).
#             L bit selects 256bit YMM registers.
#       pk:   Vector composed of 8-bit integers (packed half-word integer).
#       pkx:  Vector composed of 8-bit integers (packed half-word integer).
#             L bit selects 256bit YMM registers.
#       pq:   Vector composed of 64-bit integers (packed quadword integer).
#       pqw:  Vector composed of 64-bit quadwords (packed quadword).
#       pqwx: Vector composed of 64-bit quadwords (packed quadword).  L bit
#             selects 256bit YMM registers.
#       pqx:  Vector composed of 64-bit integers (packed quadword integer).
#             L bit selects 256bit YMM registers.
#       ps:   A single-precision floating-point vector operand (packed
#             single-precision).
#       psx:  A single-precision floating-point vector operand (packed
#             single-precision).  L bit selects 256bit YMM registers.
#       pw:   Vector composed of 16-bit words (packed word).
#       pwx:  Vector composed of 16-bit words (packed word). L bit selects
#             256 bit YMM registers.
#       q:    A quadword (64-bit), irrespective of the effective operand size.
#       r:    Register size (32bit in 32bit mode, 64bit in 64bit mode).
#       s:    Segment register (if register operand).
#       s:    A 6-byte or 10-byte pseudo-descriptor (if memory operand).
#       sb:   A scalar 10-byte packed BCD value (scalar BCD).
#       sd:   A scalar double-precision floating-point operand (scalar double).
#       se:   A 14-byte or 28-byte x87 environment.
#       si:   A scalar doubleword (32-bit) integer operand (scalar integer).
#       sq:   A scalar quadword (64-bit) integer operand (scalar integer).
#       sr:   A 94-byte or 108-byte x87 state.
#       ss:   A scalar single-precision floating-point operand (scalar single).
#       st:   A scalar 80bit-precision floating-point operand (scalar tenbytes).
#       sw:   A scalar word (16-bit) integer operand (scalar integer).
#       sx:   A 512-byte extended x87/MMX/XMM state.
#       v:    A word, doubleword, or quadword (in 64-bit mode), depending on
#             the effective operand size.
#       w:    A word, irrespective of the effective operand size.
#       x:    Instruction supports both vector sizes (128 bits or 256 bits).
#             Size is encoded using the VEX/XOP.L field. (L=0: 128 bits;
#             L=1: 256 bits). Usually this symbol is appended to ps or pd, but
#             sometimes it is used alone. For gen_dfa psx, pdx and x
#             are the same.
#       y:    A doubleword or quadword depending on effective operand size.
#       z:    A word if the effective operand size is 16 bits, or a doubleword
#             if the effective operand size is 32 or 64 bits.
#      Implicit argument mark:
#       *: This argument is implicit. It's not shown in the diassembly listing.
################################################################################
# Second column: instruction opcodes.
#   Includes all opcode bytes.  If first opcode bytes is 0x66/data16,
#   0xf2/repnz, or 0xf3/rep/repz then they can be moved before other prefixes
#   (and will be moved before REX prefix if it's allowed).  Note: data16, repnz,
#   and rep/repz opcodes will set appropriate flags while 0x66, 0xf2, and 0xf3
#   will not.
#   If part of the opcode is stored in ModRM byte then opcode should include the
#   usual "/0", "/1", ..., "/7" "bytes".
#   For VEX/XOP instructions it is expected that first three opcode bytes are
#   specified in the following form:
#     0xc4 (or 0x8f)
#     RXB.<map_select>
#     <W>.<vvvv>.<L>.<pp>
#   (so they describe long form of VEX prefix; short form is deduced
#   automatically when appropriate)
################################################################################
# Third column: additional instruction notes.
#   Different kind of notes for the instruction: non-typical prefixes (for
#   example "lock" prefix or "rep" prefix), CPUID checks, etc.
#
#     Possible prefixes:
#       branch_hint: branch hint prefixes are allowed (0x2E, 0x3E)
#       condrep: prefixes "repnz" and "repz" are allowed for the instruction
#       lock: prefix "lock" is allowed for the instruction
#       rep: prefix "rep" is allowed for the instruction (it's alias of "repz")
#       no_memory_access: command does not access memory in detectable way: lea,
#         nop, prefetch* instructions...
#       norex: "rex" prefix can not be used with this instruction (various "nop"
#         instructions use this flag)
#       norexw: "rex.W" can not be used with this instruction (usually used when
#         instruction with "rex.W" have a different name: e.g. "movd"/"movq")
#
#     Instruction enabling/disabling:
#       ia32: ia32-only instruction
#       amd64: amd64-only instruction
#       nacl-forbidden: instruction is not supported in NaCl sandbox
#       nacl-ia32-forbidden: instruction is not supported in ia32 NaCl sandbox
#       nacl-amd64-forbidden: instruction is not supported in amd64 NaCl sandbox
#       disabled_untested: instruction is disabled because it is not tested yet.
#
#     Special marks:
#       nacl-amd64-zero-extends: instruction can be used to zero-extend register
#         in amd64 mode
#       nacl-amd64-modifiable: instruction can be modified in amd64 mode
#       att-show-name-suffix-{b,l,ll,t,s,q,x,y,w}: instruction is shown with the
#         given suffix by objdump in AT&T mode
#
#     CPU features are defined in validator_internal.h.
################################################################################


# Technically, columns are separated with mere ',' followed by spaces for
# readability, but there are quoted instruction names that include commas
# not followed by spaces (see nops.def).
# For simplicity I choose to rely on this coincidence and use split-based parser
# instead of proper recursive descent one.
# If by accident somebody put ', ' in quoted instruction name, it will fail
# loudly, because closing quote then will fall into second or third column and
# will cause parse error.
# TODO(shcherbina): use for column separator something that is never encountered
# in columns, like semicolon?
COLUMN_SEPARATOR = ', '


SUPPORTED_ATTRIBUTES = [
    # Parsing attributes.
    'branch_hint',
    'condrep',
    'lock',
    'no_memory_access',
    'norex',
    'norexw',
    'rep',

    # CPUID attributes.
    'CPUFeature_3DNOW',
    'CPUFeature_3DPRFTCH',
    'CPUFeature_AES',
    'CPUFeature_AESAVX',
    'CPUFeature_ALTMOVCR8',
    'CPUFeature_AVX',
    'CPUFeature_AVX2',
    'CPUFeature_BMI1',
    'CPUFeature_CLFLUSH',
    'CPUFeature_CLMUL',
    'CPUFeature_CLMULAVX',
    'CPUFeature_CMOV',
    'CPUFeature_CMOVx87',
    'CPUFeature_CX16',
    'CPUFeature_CX8',
    'CPUFeature_E3DNOW',
    'CPUFeature_EMMX',
    'CPUFeature_EMMXSSE',
    'CPUFeature_F16C',
    'CPUFeature_FMA',
    'CPUFeature_FMA4',
    'CPUFeature_FXSR',
    'CPUFeature_LAHF',
    'CPUFeature_LWP',
    'CPUFeature_LZCNT',
    'CPUFeature_MMX',
    'CPUFeature_MON',
    'CPUFeature_MOVBE',
    'CPUFeature_MSR',
    'CPUFeature_POPCNT',
    'CPUFeature_SEP',
    'CPUFeature_SFENCE',
    'CPUFeature_SKINIT',
    'CPUFeature_SSE',
    'CPUFeature_SSE2',
    'CPUFeature_SSE3',
    'CPUFeature_SSE41',
    'CPUFeature_SSE42',
    'CPUFeature_SSE4A',
    'CPUFeature_SSSE3',
    'CPUFeature_SVM',
    'CPUFeature_SYSCALL',
    'CPUFeature_TBM',
    'CPUFeature_TSC',
    'CPUFeature_TSCP',
    'CPUFeature_TZCNT',
    'CPUFeature_x87',
    'CPUFeature_XOP',

    # If L == 1: requires AVX2, else: requires AVX1
    'CPUFeature_AVX_Lis2',

    # Attributes for enabling/disabling based on architecture and validity.
    'ia32',
    'amd64',
    'nacl-ia32-forbidden',
    'nacl-amd64-forbidden',
    'nacl-forbidden',
    'nacl-unsupported',
    'nacl-amd64-zero-extends',
    'nacl-amd64-modifiable',
    'disabled_untested',

    # AT&T Decoder attributes.
    'att-show-name-suffix-b',
    'att-show-name-suffix-l',
    'att-show-name-suffix-ll',
    'att-show-name-suffix-t',
    'att-show-name-suffix-s',
    'att-show-name-suffix-q',
    'att-show-name-suffix-x',
    'att-show-name-suffix-y',
    'att-show-name-suffix-w',
]


class OperandReadWriteMode(object):
  UNUSED = '\''
  READ = '='
  WRITE = '!'
  READ_WRITE = '&'


class OperandType(object):
  AX = 'a'
  CX = 'c'
  DX = 'd'

  IMMEDIATE = 'I'
  SECOND_IMMEDIATE = 'i'

  CONTROL_REGISTER = 'C'  # in ModRM.reg
  DEBUG_REGISTER = 'D'  # in ModRM.reg

  REGISTER_IN_OPCODE = 'r'
  X87_REGISTER_IN_OPCODE = 'f'

  X87_ST = 't'  # st0 that objdump displays as 'st'

  ABSOLUTE_DISP = 'O'

  RELATIVE_TARGET = 'J'

  REGISTER_IN_RM = 'R'
  REGISTER_IN_REG = 'G'
  REGISTER_OR_MEMORY = 'E'  # in ModRM.mod and .r/m
  MEMORY = 'M'  # in ModRM.mod and .r/m
  SEGMENT_REGISTER_IN_REG = 'S'

  MMX_REGISTER_IN_RM = 'N'
  MMX_REGISTER_IN_REG = 'P'
  MMX_REGISTER_OR_MEMORY = 'Q'  # in ModRM.mod and .r/m

  XMM_REGISTER_IN_RM = 'U'
  XMM_REGISTER_IN_REG = 'V'
  XMM_REGISTER_OR_MEMORY = 'W'  # in ModRM.mod and .r/m

  XMM_REGISTER_IN_LAST_BYTE = 'L'  # most-significant 4 bits

  DS_SI = 'X'
  ES_DI = 'Y'
  DS_BX = 'x'

  REGISTER_IN_VVVV = 'B'
  XMM_REGISTER_IN_VVVV = 'H'

  PORT_IN_DX = 'o'


ALL_OPERAND_TYPES = set(
    v for k, v in OperandType.__dict__.items() if not k.startswith('__'))
