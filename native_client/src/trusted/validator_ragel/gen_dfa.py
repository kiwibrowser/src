#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import copy
import itertools
import optparse
import re
import StringIO

import def_format


def Attribute(name):
  assert name in def_format.SUPPORTED_ATTRIBUTES
  return name


class Operand(object):

  __slots__ = [
      'read_write_attr',
      'arg_type',
      'size',
      'implicit',
      'index']

  read_write_attr_regex = r'[%s%s%s%s]?' % (
      def_format.OperandReadWriteMode.UNUSED,
      def_format.OperandReadWriteMode.READ,
      def_format.OperandReadWriteMode.WRITE,
      def_format.OperandReadWriteMode.READ_WRITE)
  arg_type_regex = '|'.join(def_format.ALL_OPERAND_TYPES)
  size_regex = (
      r'|2|7|b|d|do|dq|fq|o|'
      r'p|pb|pbx|pd|pdw|pdwx|pdx|ph|phx|pi|pix|'
      r'pj|pjx|pk|pkx|pq|pqw|pqwx|pqx|ps|psx|pw|pwx|'
      r'q|r|s|sb|sd|se|si|sq|sr|ss|st|sw|sx|'
      r'v|w|x|y|z')
  implicit_mark_regex = r'\*?'

  operand_regex = re.compile(r'(%s)(%s)(%s)(%s)$' % (
      read_write_attr_regex,
      arg_type_regex,
      size_regex,
      implicit_mark_regex))

  @staticmethod
  def Parse(s, default_rw):
    m = Operand.operand_regex.match(s)
    assert m is not None, s

    return Operand(
        read_write_attr=m.group(1) or default_rw,
        arg_type=m.group(2),
        size=m.group(3),
        implicit=(m.group(4) == '*'))

  def __init__(self, read_write_attr, arg_type, size, implicit=False):
    self.read_write_attr = read_write_attr
    self.arg_type = arg_type
    self.size = size
    self.implicit = implicit

  def Readable(self):
    return self.read_write_attr in [def_format.OperandReadWriteMode.READ,
                                    def_format.OperandReadWriteMode.READ_WRITE]

  def Writable(self):
    return self.read_write_attr in [def_format.OperandReadWriteMode.WRITE,
                                    def_format.OperandReadWriteMode.READ_WRITE]

  def ResidesInModRM(self):
    return self.arg_type in [
        def_format.OperandType.CONTROL_REGISTER,
        def_format.OperandType.DEBUG_REGISTER,
        def_format.OperandType.REGISTER_IN_REG,
        def_format.OperandType.REGISTER_IN_RM,
        def_format.OperandType.REGISTER_OR_MEMORY,
        def_format.OperandType.MEMORY,
        def_format.OperandType.SEGMENT_REGISTER_IN_REG,
        def_format.OperandType.MMX_REGISTER_IN_RM,
        def_format.OperandType.MMX_REGISTER_IN_REG,
        def_format.OperandType.MMX_REGISTER_OR_MEMORY,
        def_format.OperandType.XMM_REGISTER_IN_RM,
        def_format.OperandType.XMM_REGISTER_IN_REG,
        def_format.OperandType.XMM_REGISTER_OR_MEMORY]

  def GetFormat(self, bitness):
    """Get human-readable string for operand type and size.

    This string is used as a suffix in action names like 'operand0_8bit'. Values
    set by these actions are in turn used by disassembler to determine how to
    print operand.

    Actually, there is one format ('memory') that is never returned because it
    is handled at a higher level.

    This format will also be needed by validator64 in order to identify
    zero-extending instruction.

    Args:
      bitness: 32 or 64. It is only needed when operand size is 'r' (register
               size (32 bit in 32-bit mode, 64 bit in 64-bit mode)).

    Returns:
      String like '8bit', '32bit', 'xmm', 'mmx', etc.
    """
    if self.arg_type == def_format.OperandType.MEMORY:
      return 'memory'

    if self.arg_type == def_format.OperandType.SEGMENT_REGISTER_IN_REG:
      return 'segreg'

    if self.arg_type == def_format.OperandType.CONTROL_REGISTER:
      return 'creg'
    if self.arg_type == def_format.OperandType.DEBUG_REGISTER:
      return 'dreg'

    if self.arg_type in [def_format.OperandType.MMX_REGISTER_IN_REG,
                         def_format.OperandType.MMX_REGISTER_IN_RM]:
      return 'mmx'

    if (self.arg_type == def_format.OperandType.AX and
        self.size in ['pd', 'ps', 'pb']):
      return 'xmm'

    if self.arg_type in [def_format.OperandType.XMM_REGISTER_IN_REG,
                         def_format.OperandType.XMM_REGISTER_IN_RM,
                         def_format.OperandType.XMM_REGISTER_IN_VVVV,
                         def_format.OperandType.XMM_REGISTER_IN_LAST_BYTE]:
      if self.size.endswith('-ymm') or self.size in ['fq', 'do']:
        return 'ymm'
      else:
        return 'xmm'

    if self.size == 'b':
      return '8bit'
    if self.size == 'w':
      return '16bit'
    if self.size == 'd':
      return '32bit'
    if self.size == 'q':
      return '64bit'

    if self.size == 'r':
      return {32: '32bit', 64: '64bit'}[bitness]

    if self.size == '7':
      return 'x87'

    if self.size == '2':
      assert self.arg_type == def_format.OperandType.IMMEDIATE
      return '2bit'

    raise NotImplementedError(self)

  def __str__(self):
    return '%s%s%s%s' % (
        self.read_write_attr,
        self.arg_type,
        self.size,
        '*' if self.implicit else '')


class Instruction(object):

  __slots__ = [
      'name',
      'operands',
      'opcodes',
      'attributes',
      'rex',
      'required_prefixes',
      'optional_prefixes']

  class RexStatus(object):
    __slots__ = [
        'b_matters',
        'x_matters',
        'r_matters',
        'w_matters',
        'w_set']

  def __init__(self):
    self.opcodes = []
    self.attributes = []

    self.rex = self.RexStatus()
    self.rex.b_matters = False
    self.rex.x_matters = False
    self.rex.r_matters = False
    self.rex.w_matters = True
    self.rex.w_set = False

    self.required_prefixes = []
    self.optional_prefixes = []

  def CollectPrefixes(self):
    if Attribute('branch_hint') in self.attributes:
      self.optional_prefixes.append('branch_hint')
    if Attribute('condrep') in self.attributes:
      self.optional_prefixes.append('condrep')
    if Attribute('rep') in self.attributes:
      self.optional_prefixes.append('rep')
    if Attribute('lock') in self.attributes:
      self.optional_prefixes.append('lock')

    while True:
      opcode = self.opcodes[0]
      if opcode == 'rexw':
        self.rex.w_set = True
      elif opcode in [
          'data16',
          '0x66',  # data16 as well (the difference is that it is treated as
                   # part of opcode, and not as operand size indicator)
          '0xf0',  # lock
          '0xf2',  # rep(nz)
          '0xf3']:  # repz/condrep/branch_hint
        if opcode in self.required_prefixes:
          # Long nops (see nops.def) are the only instruction that allow
          # multiple data16 prefixes. Since we don't wont to deal with duplicate
          # prefixes in general, we leave all except one as part of opcode.
          assert opcode == '0x66'
          assert 'nopw' in self.name
          break
        self.required_prefixes.append(opcode)
      else:
        # Prefixes ended, we get to the regular part of the opcode.
        break
      del self.opcodes[0]

  @staticmethod
  def Parse(line):
    """Parse one line of def file and return initialized Instruction object.

    Args:
      line: One line of def file (two or three columns separated by
          def_format.COLUMN_SEPARATOR).
          First column defines instruction name and operands,
          second one - opcodes and encoding details,
          third (optional) one - instruction attributes.

    Returns:
      Fully initialized Instruction object.
    """

    instruction = Instruction()

    columns = line.split(def_format.COLUMN_SEPARATOR)

    # Third column is optional.
    assert 2 <= len(columns) <= 3, line
    name_and_operands_column = columns[0]
    opcodes_column = columns[1]
    if len(columns) == 3:
      attributes = columns[2].split()
    else:
      attributes = []

    instruction.ParseNameAndOperands(name_and_operands_column)
    instruction.ParseOpcodes(opcodes_column)

    instruction.attributes = map(Attribute, attributes)

    return instruction

  def ParseNameAndOperands(self, s):
    # If instruction name is quoted, it is allowed to contain spaces.
    if s.startswith('"'):
      i = s.index('"', 1)
      self.name = s[1:i]
      operands = s[i+1:].split()
    else:
      operands = s.split()
      self.name = operands.pop(0)

    self.operands = []
    for i, op in enumerate(operands):
      # By default one- and two-operand instructions are assumed to read all
      # operands and store result to the last one, while instructions with
      # three or more operands are assumed to read all operands except last one
      # which is used to store the result of the execution.
      last = (i == len(operands) - 1)
      if last:
        if len(operands) <= 2:
          default_rw = def_format.OperandReadWriteMode.READ_WRITE
        else:
          default_rw = def_format.OperandReadWriteMode.WRITE
      else:
        default_rw = def_format.OperandReadWriteMode.READ
      operand = Operand.Parse(op, default_rw=default_rw)
      self.operands.append(operand)

  def ParseOpcodes(self, opcodes_column):
    opcodes = opcodes_column.split()

    opcode_regex = re.compile(
        r'(0x[0-9a-f]{2}|'  # raw bytes
        r'data16|'
        r'rexw|'  # REX prefix with W-bit set
        r'/[0-7]|'  # opcode stored in ModRM byte
        r'/|'  # precedes 3DNow! opcode map
        r'RXB\.([01][0-9A-F]|[01]{5})|'  # VEX-specific, 5 bits (hex or binary)
        r'[xW01]\.(src|src1|dest|cntl|1111)\.[Lx01]\.[01]{2})$'  # VEX-specific
    )

    assert len(opcodes) > 0
    for opcode in opcodes:
      assert opcode_regex.match(opcode), opcode

    self.opcodes = opcodes

  def HasModRM(self):
    return any(operand.ResidesInModRM() for operand in self.operands)

  def FindOperand(self, arg_type):
    result = None
    for operand in self.operands:
      if operand.arg_type == arg_type:
        assert result is None, 'multiple operands of type %s' % arg_type
        result = operand
    return result

  def HasRegisterInOpcode(self):
    return (self.FindOperand(
                def_format.OperandType.REGISTER_IN_OPCODE) is not None or
            self.FindOperand(
                def_format.OperandType.X87_REGISTER_IN_OPCODE) is not None)

  def HasOpcodeInsteadOfImmediate(self):
    return '/' in self.opcodes

  def GetOpcodeInModRM(self):
    """Return either opcode (0-7) or None."""
    for opcode in self.opcodes:
      m = re.match(r'/(\d)$', opcode)
      if m is not None:
        return int(m.group(1))
    return None

  def GetMainOpcodePart(self):
    result = []
    for opcode in self.opcodes:
      if opcode.startswith('/'):
        # Either '/' (opcode in immediate) or '/0'-'/7' (opcode in ModRM).
        # Anyway, main part of the opcode is over.
        break
      result.append(opcode)
    if self.IsVexOrXop():
      del result[:3]
    return result

  def GetNameAsIdentifier(self):
    """Return name in a form suitable to use as part of C identifier.

    In principle, collisions are possible, but will result in compilation
    failure, so we are not checking for them here for simplicity.

    Returns:
      Instruction name with all non-alphanumeric characters replaced with '_'.
    """
    return re.sub(r'\W', '_', self.name)

  def IsVexOrXop(self):
    return (self.opcodes[0] == '0xc4' and self.name != 'les' or
            self.opcodes[0] == '0x8f' and self.name != 'pop')

  def __str__(self):
    result = ' '.join([self.name] + map(str, self.operands))
    result += ', ' + ' '.join(self.opcodes)
    if len(self.attributes) > 0:
      result += ', ' + ' '.join(self.attributes)
    return result.strip()

  def RMatters(self):
    """Return True iff rex.r bit influences instruction."""
    # TODO(shcherbina): perhaps do something with rex.r_matters field?

    if self.FindOperand(def_format.OperandType.REGISTER_IN_REG) is not None:
      return True
    if self.FindOperand(def_format.OperandType.XMM_REGISTER_IN_REG) is not None:
      return True

    # Control registers are switched from %cr0..%cr7 to %cr8..%cr15 iff rex.r
    # bit is set or instruction has 0xf0 (lock) prefix.
    # Hence, rex.r bit influences control register if and only if lock prefix
    # is not used.
    if self.FindOperand(def_format.OperandType.CONTROL_REGISTER) is not None:
      return '0xf0' not in self.required_prefixes

    return False


AddressMode = collections.namedtuple(
    'AddressMode',
    ['mode', 'x_matters', 'b_matters'])

ALL_ADDRESS_MODES = [
    AddressMode('operand_disp', False, True),
    AddressMode('operand_pure_disp', False, False),
    AddressMode('single_register_memory', False, True),
    AddressMode('operand_sib_pure_index', True, False),
    AddressMode('operand_sib_base_index', True, True)]


DECODER = object()
VALIDATOR = object()


class InstructionPrinter(object):

  def __init__(self, mode, bitness, reverse_operands=False):
    assert mode in [DECODER, VALIDATOR]
    assert bitness in [32, 64]
    self._mode = mode
    self._bitness = bitness
    self._reverse_operands = reverse_operands
    self._out = StringIO.StringIO()
    self._printed_operand_sources = set()

  def GetContent(self):
    return self._out.getvalue()

  def _SetOperandIndices(self, instruction):
    instruction = copy.deepcopy(instruction)
    index = 0

    # TODO(shcherbina): Get rid of it when
    # https://code.google.com/p/nativeclient/issues/detail?id=3299 is fixed.
    operands = instruction.operands
    if self._reverse_operands:
      operands = reversed(operands)

    for operand in operands:
      if self._NeedOperandInfo(operand):
        operand.index = index
        index += 1
    return instruction

  def _PrintRexPrefix(self, instruction):
    """Print a machine for REX prefix."""
    if self._bitness != 64:
      return
    if instruction.IsVexOrXop():
      return
    if Attribute('norex') in instruction.attributes:
      return

    if instruction.rex.w_matters:
      if instruction.rex.w_set:
        self._out.write('REXW_RXB\n')
      else:
        self._out.write('REX_RXB?\n')
    else:
      self._out.write('REX_WRXB?\n')


  def _PrintVexOrXopPrefix(self, instruction):
    """Print machine that parses VEX or XOP prefix.

    First three opcodes of the instruction are expected to be of the form
      0xc4 (or 0x8f)
      RXB.<map_select>
      <W>.<vvvv>.<L>.<pp>

    So they serve as a description of long form of VEX/XOP prefix.
    Short two-byte form is printed automatically when appropriate.

    Args:
      instruction: instruction (if it's not VEX/XOP-instruction, nothing
                   is printed).

    Returns:
      None.
    """
    # Note the difference between 'X' and 'x' below.
    # 'x' means bit insignificant to the processor but which objdump
    #     expects to be 0
    # 'X' means bit insignificant both to processor and to objdump.

    if not instruction.IsVexOrXop():
      return

    assert instruction.opcodes[0] in ['0xc4', '0x8f']

    m = re.match(r'RXB\.([01][0-9A-F]|[01]{5})$', instruction.opcodes[1])
    map_select = m.group(1)

    m = re.match(r'([xW01])\.(src|src1|dest|cntl|1111)\.([Lx01])\.([01]{2})',
                 instruction.opcodes[2])
    w, vvvv, L, pp = m.groups()
    assert L != 'L', 'L-bit should be splitted already'

    if w == 'W':
      assert instruction.rex.w_matters
      w = '1' if instruction.rex.w_set else '0'

    has_short_form = (
        instruction.opcodes[0] == '0xc4' and
        map_select in ['01', '00001'] and
        w in ['0', 'x'])

    if vvvv in ['src', 'src1', 'dest', 'cntl']:
      vvvv = {32: '1XXX', 64: 'XXXX'}[self._bitness]

    # Adjustment for objdump.
    w = w.replace('x', '0')
    L = L.replace('x', '0')

    vex_rxb = 'VEX_'
    if self._bitness == 64:
      if instruction.rex.r_matters:
        vex_rxb += 'R'
      if instruction.rex.x_matters:
        vex_rxb += 'X'
      if instruction.rex.b_matters:
        vex_rxb += 'B'
    if vex_rxb.endswith('_'):
      vex_rxb += 'NONE'

    long_form = '%s (%s & VEX_map%s) b_%s_%s_%s_%s @vex_prefix3' % (
        instruction.opcodes[0],
        vex_rxb, map_select,
        w, vvvv, L, pp)

    if has_short_form:
      if self._bitness == 64 and instruction.rex.r_matters:
        inverted_r = 'X'
      else:
        inverted_r = '1'
      short_form = '0xc5 b_%s_%s_%s_%s @vex_prefix_short' % (
          inverted_r, vvvv, L, pp)

    if has_short_form:
      self._out.write('(%s | %s)\n' % (long_form, short_form))
    else:
      self._out.write('%s\n' % long_form)

  def _PrintOpcode(self, instruction):
    """Print a machine for opcode."""
    main_opcode_part = instruction.GetMainOpcodePart()
    if instruction.HasRegisterInOpcode():
      assert not instruction.HasModRM()
      assert not instruction.HasOpcodeInsteadOfImmediate()

      self._out.write(' '.join(main_opcode_part[:-1]))

      # Register is encoded in three least significant bits of the last byte
      # of the opcode (in 64-bit case REX.B bit also will be involved, but it
      # will be handled elsewhere).
      last_byte = int(main_opcode_part[-1], 16)
      assert last_byte & 0b00000111 == 0
      self._out.write(' (%s)' % '|'.join(
          hex(b) for b in range(last_byte, last_byte + 2**3)))

      self._out.write(' ')

      operand = instruction.FindOperand(
          def_format.OperandType.REGISTER_IN_OPCODE)
      if operand is not None:
        assert operand.size != '7'
        self._PrintOperandSource(operand, 'from_opcode')
      else:
        operand = instruction.FindOperand(
            def_format.OperandType.X87_REGISTER_IN_OPCODE)
        assert operand.size == '7'
        self._PrintOperandSource(operand, 'from_opcode_x87')

    else:
      self._out.write(' '.join(main_opcode_part))

  def _PrintSpuriousRexInfo(self, instruction):
    if (self._mode == DECODER and
        self._bitness == 64 and
        not instruction.IsVexOrXop()):
      # Note that even if 'norex' attribute is present, we print
      # @spurious_rex_... actions because NOP needs them (and it has REX
      # prefix specified as part of the opcode).
      # TODO(shcherbina): fix that?
      if not instruction.rex.b_matters:
        self._out.write('@set_spurious_rex_b\n')
      if not instruction.rex.x_matters:
        self._out.write('@set_spurious_rex_x\n')
      if not instruction.rex.r_matters:
        self._out.write('@set_spurious_rex_r\n')
      if not instruction.rex.w_matters:
        self._out.write('@set_spurious_rex_w\n')

  def _PrintSignature(self, instruction):
    """Print actions specifying instruction name and info about its operands.

    Example signature:
      @instruction_add
      @operands_count_is_2
      @operand0_8bit
      @operand1_8bit

    Depending on the mode, parts of the signature may be omitted.

    Args:
      instruction: instruction (with full information about operand types, etc.)

    Returns:
      None.
    """
    if self._mode == DECODER:
      self._out.write('@instruction_%s\n' % instruction.GetNameAsIdentifier())
      for attribute in instruction.attributes:
        if attribute.startswith('att-show-name-suffix-'):
          self._out.write('@%s\n' % attribute.replace('-', '_'))

      self._out.write('@operands_count_is_%d\n' % len(instruction.operands))

      # For some instructions legacy prefixes are actually part of opcode,
      # in such cases we conventionally refer to them as literal bytes ('0x66'
      # instead of 'data16'). Disassembler should not print prefixes themselves.
      if '0x66' in instruction.required_prefixes:
        self._out.write('@not_data16_prefix\n')
      if '0xf2' in instruction.required_prefixes:
        self._out.write('@not_repnz_prefix\n')
      if '0xf3' in instruction.required_prefixes:
        self._out.write('@not_repz_prefix\n')

      if '0xf0' in instruction.required_prefixes:
        operand = instruction.FindOperand(
            def_format.OperandType.CONTROL_REGISTER)
        if operand is not None:
          assert self._NeedOperandInfo(operand)
          self._out.write('@lock_extends_cr_operand%d\n' % operand.index)

    for operand in instruction.operands:
      if self._NeedOperandInfo(operand):
        self._out.write('@operand%d_%s\n' %
                        (operand.index, operand.GetFormat(self._bitness)))

    for attr in instruction.attributes:
      if attr.startswith('CPUFeature_'):
        self._out.write('@%s\n' % attr)

    if self._mode == VALIDATOR and self._bitness == 64:
      if Attribute('nacl-amd64-modifiable') in instruction.attributes:
        self._out.write('@modifiable_instruction\n')

    if self._mode == VALIDATOR:
      if Attribute('nacl-unsupported') in instruction.attributes:
        self._out.write('@unsupported_instruction\n')

  def _NeedOperandInfo(self, operand):
    """Whether we need to print actions describing operand format and source."""
    if self._mode == DECODER:
      return True

    if self._bitness == 32:
      return False

    # In 64-bit validator we only care about general purpose registers we
    # are writing to.
    return (
        operand.Writable() and
        operand.arg_type in [
            def_format.OperandType.REGISTER_IN_OPCODE,
            def_format.OperandType.REGISTER_IN_VVVV,
            def_format.OperandType.REGISTER_IN_REG,
            def_format.OperandType.REGISTER_IN_RM,
            def_format.OperandType.AX,
            def_format.OperandType.CX,
            def_format.OperandType.DX])

  def _PrintOperandSource(self, operand, source):
    """Print action specifying operand source."""
    # TODO(shcherbina): add mechanism to check that all operand sources are
    # printed.
    if self._NeedOperandInfo(operand):
      assert operand.index not in self._printed_operand_sources
      self._printed_operand_sources.add(operand.index)
      self._out.write('@operand%d_%s\n' % (operand.index, source))

  def _PrintDetachedOperandSources(self, instruction):
    """Print actions specifying sources of some operands.

    Print operand source actions which do not need to read bytes relative to
    current position.

    Args:
      instruction: instruction.

    Returns:
      None.
    """
    # TODO(shcherbina): maybe inline it into PrintSignature?
    for operand in instruction.operands:
      if operand.arg_type == def_format.OperandType.AX:
        self._PrintOperandSource(operand, 'rax')
      if operand.arg_type == def_format.OperandType.CX:
        self._PrintOperandSource(operand, 'rcx')
      if operand.arg_type == def_format.OperandType.DX:
        self._PrintOperandSource(operand, 'rdx')
      elif operand.arg_type == def_format.OperandType.DS_SI:
        self._PrintOperandSource(operand, 'ds_rsi')
      elif operand.arg_type == def_format.OperandType.ES_DI:
        self._PrintOperandSource(operand, 'es_rdi')
      elif operand.arg_type == def_format.OperandType.DS_BX:
        self._PrintOperandSource(operand, 'ds_rbx')
      elif operand.arg_type == def_format.OperandType.PORT_IN_DX:
        self._PrintOperandSource(operand, 'port_dx')
      elif operand.arg_type == def_format.OperandType.XMM_REGISTER_IN_VVVV:
        self._PrintOperandSource(operand, 'from_vex')
      elif operand.arg_type == def_format.OperandType.REGISTER_IN_VVVV:
        self._PrintOperandSource(operand, 'from_vex')
      elif operand.arg_type == def_format.OperandType.X87_ST:
        self._PrintOperandSource(operand, 'st')

  def _PrintLegacyPrefixes(self, instruction):
    """Print a machine for all combinations of legacy prefixes."""
    legacy_prefix_combinations = GenerateLegacyPrefixes(
        self._bitness,
        instruction.required_prefixes,
        instruction.optional_prefixes)
    assert len(legacy_prefix_combinations) > 0
    if legacy_prefix_combinations == [()]:
      return
    self._out.write('(%s)' % ' | '.join(
        ' '.join(combination)
        for combination in legacy_prefix_combinations
        if combination != ()))
    # Use '(...)?' since Ragel does not allow '( | ...)'.
    if () in legacy_prefix_combinations:
      self._out.write('?')
    self._out.write('\n')

  def _PrintImmediates(self, instruction):
    """Print a machine that parses immediate operands (if present)."""

    imm2 = None  # two-bit immediate if found

    operand = instruction.FindOperand(def_format.OperandType.IMMEDIATE)
    if operand is not None:
      format = operand.GetFormat(self._bitness)
      if format == '2bit':
        imm2 = operand  # handled below
      elif format == '8bit':
        self._out.write('imm8\n')
      elif format == '16bit':
        self._out.write('imm16\n')
      elif format == '32bit':
        self._out.write('imm32\n')
      elif format == '64bit':
        self._out.write('imm64\n')
      else:
        assert False, format
      self._PrintOperandSource(operand, 'immediate')

    operand = instruction.FindOperand(def_format.OperandType.SECOND_IMMEDIATE)
    if operand is not None:
      format = operand.GetFormat(self._bitness)
      if format == '8bit':
        self._out.write('imm8n2\n')
      elif format == '16bit':
        self._out.write('imm16n2\n')
      else:
        assert False, format
      self._PrintOperandSource(operand, 'second_immediate')

    operand = instruction.FindOperand(
        def_format.OperandType.XMM_REGISTER_IN_LAST_BYTE)
    if operand is not None:
      highest_bit = {32: '0', 64: 'x'}[self._bitness]
      if imm2 is None:
        self._out.write('b_%sxxx_0000\n' % highest_bit)
        self._PrintOperandSource(operand, 'from_is4')
      else:
        self._out.write('b_%sxxx_00xx @imm2_operand\n' % highest_bit)
        self._PrintOperandSource(operand, 'from_is4')

      if self._mode == VALIDATOR:
        self._out.write('@last_byte_is_not_immediate\n')
    else:
      assert imm2 is None

  def _PrintProcessOperandsActions(self, instruction):
    if self._mode == DECODER or self._bitness == 32:
      return

    num_operands = 0
    zero_extends = False
    for operand in instruction.operands:
      if not self._NeedOperandInfo(operand):
        continue
      num_operands += 1
      if (operand.GetFormat(self._bitness) == '32bit' and
          Attribute('nacl-amd64-zero-extends') in instruction.attributes):
        zero_extends = True

    if num_operands == 1:
      self._out.write('@process_1_operand')
    else:
      self._out.write('@process_%d_operands' % num_operands)

    if zero_extends:
      self._out.write('_zero_extends')

    self._out.write('\n')

  def PrintInstructionWithoutModRM(self, instruction):
    assert not instruction.HasModRM()

    instruction = self._SetOperandIndices(instruction)
    self._PrintLegacyPrefixes(instruction)
    self._PrintRexPrefix(instruction)
    self._PrintVexOrXopPrefix(instruction)

    assert instruction.GetOpcodeInModRM() is None

    assert not instruction.HasOpcodeInsteadOfImmediate(), 'should not happen'

    operand = instruction.FindOperand(def_format.OperandType.REGISTER_IN_OPCODE)
    if operand is not None:
      # In 64-bit mode there are 16 general-purpose registers, so fourth bit
      # goes to rex.B.
      if self._bitness == 64:
        assert operand.size in ['b', 'w', 'd', 'q', 'r']
        instruction.rex.b_matters = True

    self._PrintOpcode(instruction)
    self._out.write('\n')

    self._PrintSignature(instruction)
    self._PrintSpuriousRexInfo(instruction)
    self._PrintDetachedOperandSources(instruction)

    self._PrintImmediates(instruction)

    # Displacement encoded in the instruction.
    operand = instruction.FindOperand(def_format.OperandType.ABSOLUTE_DISP)
    if operand is not None:
      self._PrintOperandSource(operand, 'absolute_disp')
      self._out.write('disp%d\n' % self._bitness)

    # Relative jump/call target encoded in the instruction.
    operand = instruction.FindOperand(def_format.OperandType.RELATIVE_TARGET)
    if operand is not None:
      format = operand.GetFormat(self._bitness)
      if format == '8bit':
        self._out.write('rel8\n')
      elif format == '16bit':
        self._out.write('rel16\n')
      elif format == '32bit':
        self._out.write('rel32\n')
      else:
        assert False, format
      self._PrintOperandSource(operand, 'jmp_to')

    self._PrintProcessOperandsActions(instruction)
    self._FinalCheck(instruction)

  def _PrintModRMOperandSources(self, instruction):
    """Print sources for operands encoded in modrm."""
    for operand in instruction.operands:
      if operand.arg_type in [
          def_format.OperandType.CONTROL_REGISTER,
          def_format.OperandType.DEBUG_REGISTER,
          def_format.OperandType.REGISTER_IN_REG,
          def_format.OperandType.XMM_REGISTER_IN_REG]:
        source = 'from_modrm_reg'
      elif operand.arg_type in [
          def_format.OperandType.MMX_REGISTER_IN_REG,
          def_format.OperandType.SEGMENT_REGISTER_IN_REG]:
        source = 'from_modrm_reg_norex'
      elif operand.arg_type in [
          def_format.OperandType.REGISTER_IN_RM,
          def_format.OperandType.XMM_REGISTER_IN_RM]:
        source = 'from_modrm_rm'
      elif operand.arg_type == def_format.OperandType.MMX_REGISTER_IN_RM:
        source = 'from_modrm_rm_norex'
      elif operand.arg_type == def_format.OperandType.MEMORY:
        source = 'rm'
      else:
        continue
      self._PrintOperandSource(operand, source)

  def PrintInstructionWithModRMReg(self, instruction):
    """Print instruction that encodes register in its ModRM.r/m field."""

    instruction = self._SetOperandIndices(instruction)

    assert instruction.HasModRM()
    assert instruction.FindOperand(def_format.OperandType.MEMORY) is None

    assert not instruction.HasRegisterInOpcode()

    if instruction.RMatters():
      instruction.rex.r_matters = True

    # TODO(shcherbina): generalize and move somewhere?
    if (instruction.FindOperand(
            def_format.OperandType.REGISTER_IN_RM) is not None or
        instruction.FindOperand(
            def_format.OperandType.XMM_REGISTER_IN_RM) is not None):
      instruction.rex.b_matters = True

    self._PrintLegacyPrefixes(instruction)
    self._PrintRexPrefix(instruction)
    self._PrintVexOrXopPrefix(instruction)

    self._PrintOpcode(instruction)
    self._out.write('\n')

    opcode_in_modrm = instruction.GetOpcodeInModRM()

    if opcode_in_modrm is not None:
      assert not instruction.HasOpcodeInsteadOfImmediate()
      assert instruction.FindOperand(
          def_format.OperandType.SEGMENT_REGISTER_IN_REG) is None
      self._out.write('(modrm_registers & opcode_%d)\n' % opcode_in_modrm)
    elif instruction.FindOperand(
          def_format.OperandType.SEGMENT_REGISTER_IN_REG) is not None:
      self._out.write('(modrm_registers & opcode_s)\n')
    else:
      self._out.write('modrm_registers\n')

    self._PrintModRMOperandSources(instruction)

    if instruction.HasOpcodeInsteadOfImmediate():
      assert instruction.opcodes[-2] == '/'
      self._out.write('%s\n' % instruction.opcodes[-1])
      if self._mode == VALIDATOR:
        self._out.write('@last_byte_is_not_immediate\n')

    self._PrintSignature(instruction)
    self._PrintDetachedOperandSources(instruction)

    self._PrintSpuriousRexInfo(instruction)

    self._PrintImmediates(instruction)
    self._PrintProcessOperandsActions(instruction)
    self._FinalCheck(instruction)

  def PrintInstructionWithModRMMemory(self, instruction, address_mode):
    """Print instruction that has memory access.

    There are several addressing modes corresponding to various combinations of
    ModRM and SIB (and additional displacement fields).

    Args:
      instruction: instruction.
      address_mode: instance of AddressMode.

    Returns:
      None.
    """

    instruction = self._SetOperandIndices(instruction)

    assert instruction.HasModRM()
    assert instruction.FindOperand(def_format.OperandType.MEMORY) is not None

    assert address_mode in ALL_ADDRESS_MODES

    assert not instruction.HasRegisterInOpcode()

    if instruction.RMatters():
      instruction.rex.r_matters = True
    if address_mode.x_matters:
      instruction.rex.x_matters = True
    if address_mode.b_matters:
      instruction.rex.b_matters = True

    self._PrintLegacyPrefixes(instruction)
    self._PrintRexPrefix(instruction)
    self._PrintVexOrXopPrefix(instruction)

    self._PrintOpcode(instruction)
    self._out.write('\n')

    # Here we print something like
    # (any @operand0_from_modrm_reg @operand1_rm any* &
    #  operand_sib_base_index)
    # The first term specifies operand sources (they are known after the first
    # byte we read, ModRM), in this case operand0 come from ModRM.reg, and
    # operand1 is memory operand.
    # The second term parses information about specific addressing mode
    # (together with disp), independently of which operand will refer to it.
    # Signature is printed either after ModRM byte or after 3dnow opcode
    # extension.
    self._out.write('(')
    opcode_in_modrm = instruction.GetOpcodeInModRM()
    if opcode_in_modrm is not None:
      self._out.write('opcode_%d\n' % opcode_in_modrm)
    elif instruction.FindOperand(
        def_format.OperandType.SEGMENT_REGISTER_IN_REG) is not None:
      self._out.write('opcode_s\n')
    else:
      self._out.write('any\n')

    if not instruction.HasOpcodeInsteadOfImmediate():
      self._PrintSignature(instruction)
      self._PrintDetachedOperandSources(instruction)

    self._PrintModRMOperandSources(instruction)

    self._out.write('  any* &\n')
    self._out.write(address_mode.mode)
    if self._mode == VALIDATOR and self._bitness == 64:
      if Attribute('no_memory_access') not in instruction.attributes:
        self._out.write(' @check_memory_access')
    self._out.write(')\n')

    if instruction.HasOpcodeInsteadOfImmediate():
      assert instruction.opcodes[-2] == '/'
      self._out.write('%s\n' % instruction.opcodes[-1])
      if self._mode == VALIDATOR:
        self._out.write('@last_byte_is_not_immediate\n')
      self._PrintSignature(instruction)
      self._PrintDetachedOperandSources(instruction)

    # TODO(shcherbina): we don't have to parse parts like disp (only ModRM
    # (and possibly SIB)) to determine spurious rex bits. Perhaps move it
    # a little bit earlier? But it would require third intersectee, ew...
    self._PrintSpuriousRexInfo(instruction)

    self._PrintImmediates(instruction)
    self._PrintProcessOperandsActions(instruction)
    self._FinalCheck(instruction)

  def _FinalCheck(self, instruction):
    expected_operands = filter(self._NeedOperandInfo, instruction.operands)
    expected_indices = set(operand.index for operand in expected_operands)
    assert self._printed_operand_sources == expected_indices, (
        str(instruction),
        self._printed_operand_sources,
        expected_indices,
        expected_operands)


def InstructionToString(mode, bitness, instruction, reverse_operands=False):
  header = '# %s\n' % instruction

  if not instruction.HasModRM():
    printer = InstructionPrinter(
        mode,
        bitness,
        reverse_operands=reverse_operands)
    printer.PrintInstructionWithoutModRM(instruction)
    if instruction.name == 'xchg':
      # Exclude nop
      return header + '(%s - (0x90 | 0x48 0x90))' % printer.GetContent()
    return header + printer.GetContent()

  if instruction.FindOperand(def_format.OperandType.MEMORY) is None:
    printer = InstructionPrinter(
        mode,
        bitness,
        reverse_operands=reverse_operands)
    printer.PrintInstructionWithModRMReg(instruction)
    return header + printer.GetContent()
  else:
    instrs = []
    for address_mode in ALL_ADDRESS_MODES:
      printer = InstructionPrinter(
          mode,
          bitness,
          reverse_operands=reverse_operands)
      printer.PrintInstructionWithModRMMemory(instruction, address_mode)
      instrs.append(printer.GetContent())
    return header + '(%s)\n' % '|\n'.join(instrs)


def SplitRM(instruction):
  """Split instruction into two versions (using register or memory).

  Args:
    instruction: instruction.

  Returns:
    List of one or two instructions. If original instruction contains operand
    that can be either register or memory (such as 'E'), two specific versions
    are produced. Otherwise, instruction is returned unchanged.
  """
  splits = {
      def_format.OperandType.REGISTER_OR_MEMORY: (
          def_format.OperandType.REGISTER_IN_RM,
          def_format.OperandType.MEMORY),
      def_format.OperandType.MMX_REGISTER_OR_MEMORY: (
          def_format.OperandType.MMX_REGISTER_IN_RM,
          def_format.OperandType.MEMORY),
      def_format.OperandType.XMM_REGISTER_OR_MEMORY: (
          def_format.OperandType.XMM_REGISTER_IN_RM,
          def_format.OperandType.MEMORY)}

  instr_register = copy.deepcopy(instruction)
  instr_memory = copy.deepcopy(instruction)
  splitted = False
  for i, operand in enumerate(instruction.operands):
    if operand.arg_type in splits:
      assert not splitted, 'more than one r/m-splittable operand'
      splitted = True
      (instr_register.operands[i].arg_type,
       instr_memory.operands[i].arg_type) = splits[operand.arg_type]

  if splitted:
    # Lock prefix is only allowed for instructions that access memory.
    assert 'lock' not in instruction.required_prefixes
    if 'lock' in instruction.optional_prefixes:
      instr_register.optional_prefixes.remove('lock')

    return [instr_register, instr_memory]
  else:
    return [instruction]


def HasExplicitSize(operand):
  """Whether size of operand follows from its notation."""
  return operand.arg_type not in [
      def_format.OperandType.IMMEDIATE,
      def_format.OperandType.SECOND_IMMEDIATE,
      def_format.OperandType.MEMORY,
      def_format.OperandType.ES_DI,
      def_format.OperandType.DS_SI]


def SplitByteNonByte(instruction):
  """Split instruction into versions with byte-sized operands and larger ones.

  Args:
    instruction: instruction.

  Returns:
    List of one or two instructions. If original instruction contains operands
    with undetermined size, two more specific versions are produced, otherwise
    original instruction is returned unchanged.
  """

  instr_byte = copy.deepcopy(instruction)
  instr_nonbyte = copy.deepcopy(instruction)

  has_explicit_operand_size = False

  splitted = False
  for i, operand in enumerate(instruction.operands):
    if operand.size == '':
      splitted = True
      if HasExplicitSize(operand):
        has_explicit_operand_size = True

      instr_byte.operands[i].size = 'b'
      if operand.arg_type == def_format.OperandType.IMMEDIATE:
        instr_nonbyte.operands[i].size = 'z'  # word/dword
      else:
        instr_nonbyte.operands[i].size = 'v'  # word/dword/qword

  if not splitted:
    return [instruction]

  # Set the last bit of the main part of the opcode if instruction uses
  # larger-than-byte operands.

  last_byte_index = len(instruction.opcodes) - 1
  for i, opcode in enumerate(instruction.opcodes):
    if opcode.startswith('/'):
      last_byte_index = i - 1
      break
  assert last_byte_index >= 0

  last_byte = int(instruction.opcodes[last_byte_index], 16)
  assert last_byte % 2 == 0
  instr_nonbyte.opcodes[last_byte_index] = hex(last_byte | 1)

  if not has_explicit_operand_size:
    instr_byte.attributes.append(Attribute('att-show-name-suffix-b'))

  return [instr_byte, instr_nonbyte]


def SplitVYZ(bitness, instruction):
  """Split instruction into versions with 16/32/64-bit operand sizes.

  Actual operand size is determined by the following table:
                            z     y     v
    rex.W=0, data16        16    32    16
    rex.W=0, no data16     32    32    32
    rex.W=1                32    64    64    (this row is only for 64-bit mode)

  When we only have a subset of 'z', 'y', 'v' operands and some rows are
  indistinguishable, we don't produce ones with redundant prefixes.

  Args:
    bitness: 32 or 64.
    instruction: instruction.

  Returns:
    List of one to three instructions with specific operand sizes.
  """
  assert bitness in [32, 64]

  instr16 = copy.deepcopy(instruction)
  instr32 = copy.deepcopy(instruction)
  instr64 = copy.deepcopy(instruction)

  has_explicit_operand_size = False

  z_present = False
  y_present = False
  v_present = False

  for i, operand in enumerate(instruction.operands):
    if operand.size == 'z':
      if HasExplicitSize(operand):
        has_explicit_operand_size = True
      instr16.operands[i].size = 'w'
      instr32.operands[i].size = 'd'
      instr64.operands[i].size = 'd'
      z_present = True
    elif operand.size == 'y':
      if HasExplicitSize(operand):
        has_explicit_operand_size = True
      instr16.operands[i].size = 'd'
      instr32.operands[i].size = 'd'
      instr64.operands[i].size = 'q'
      y_present = True
    elif operand.size == 'v':
      if HasExplicitSize(operand):
        has_explicit_operand_size = True
      instr16.operands[i].size = 'w'
      instr32.operands[i].size = 'd'
      instr64.operands[i].size = 'q'
      v_present = True

  if not z_present and not y_present and not v_present:
    # Even when there are no operands to split by size, we usually want to
    # allow spurious rexw bit.
    # But we don't do that when instruction is split in def-file manually.
    if (Attribute('norex') in instruction.attributes or
        Attribute('norexw') in instruction.attributes or
        instruction.rex.w_set):
      return [instruction]

    # When data16 prefix is present, rexw definitely matters (because rexw takes
    # precedence over data16), so we don't mark it as irrelevant.
    # TODO(shcherbina): remove this exception if
    # https://code.google.com/p/nativeclient/issues/detail?id=3310 is fixed.
    if 'data16' in instruction.required_prefixes:
      return [instruction]

    instr = copy.deepcopy(instruction)
    instr.rex.w_matters = False
    return [instr]

  assert 'data16' not in instruction.required_prefixes
  instr16.required_prefixes.append('data16')
  instr16.rex.w_set = False
  instr32.rex.w_set = False
  instr64.rex.w_set = True

  if not has_explicit_operand_size:
    instr16.attributes.append(Attribute('att-show-name-suffix-w'))
    instr32.attributes.append(Attribute('att-show-name-suffix-l'))
    instr64.attributes.append(Attribute('att-show-name-suffix-q'))

  result = []
  if z_present or v_present:
    result.append(instr16)
  result.append(instr32)
  if (bitness == 64 and
      (y_present or v_present) and
      Attribute('norexw') not in instruction.attributes):
    result.append(instr64)
  return result


def SplitL(instruction):
  """Split instruction by L-bit into two versions (xmm and ymm).

  Args:
    instruction: instruction to split.

  Returns:
    List of one or two instructions.
  """

  instr_xmm = copy.deepcopy(instruction)
  instr_ymm = copy.deepcopy(instruction)

  splits = {
      'x': ('x-xmm', 'x-ymm'),
      'pbx': ('pb', 'pb-ymm'),
      'pdx': ('pd', 'pd-ymm'),
      'phx': ('ph', 'ph-ymm'),
      'pix': ('pi', 'pi-ymm'),
      'pjx': ('pj', 'pj-ymm'),
      'pkx': ('pk', 'pk-ymm'),
      'pqx': ('pq', 'pq-ymm'),
      'psx': ('ps', 'ps-ymm'),
      'pwx': ('pw', 'pw-ymm'),
      'pdwx': ('pdw', 'pdw-ymm'),
      'pqwx': ('pqw', 'pqw-ymm')}

  has_explicit_operand_size = False

  splitted = False
  for i, operand in enumerate(instruction.operands):
    if operand.size in splits:
      splitted = True
      if HasExplicitSize(operand):
        has_explicit_operand_size = True

      (instr_xmm.operands[i].size,
       instr_ymm.operands[i].size) = splits[operand.size]

  if not splitted:
    return [instruction]

  assert instruction.IsVexOrXop()
  third_vex_byte = instruction.opcodes[2]
  assert '.L.' in third_vex_byte

  instr_xmm.opcodes[2] = third_vex_byte.replace('.L.', '.0.')
  instr_ymm.opcodes[2] = third_vex_byte.replace('.L.', '.1.')

  if Attribute('CPUFeature_AVX_Lis2') in instruction.attributes:
    index = instruction.attributes.index(Attribute('CPUFeature_AVX_Lis2'))
    instr_xmm.attributes[index] = Attribute('CPUFeature_AVX')
    instr_ymm.attributes[index] = Attribute('CPUFeature_AVX2')

  if not has_explicit_operand_size:
    instr_xmm.attributes.append(Attribute('att-show-name-suffix-x'))
    instr_ymm.attributes.append(Attribute('att-show-name-suffix-y'))

  return [instr_xmm, instr_ymm]


def ParseDefFile(filename):
  # .def file format is documented in def_format.py.

  with open(filename) as def_file:
    lines = [line for line in def_file if not line.startswith('#')]

  lines = iter(lines)
  while True:
    line = next(lines, None)
    if line is None:
      break

    # Comma-terminated lines are expected to be continued.
    while line.endswith(',\n'):
      next_line = next(lines)
      line = line[:-2] + def_format.COLUMN_SEPARATOR + next_line

    assert '#' not in line

    line = line.strip()
    if line == '':
      continue

    yield Instruction.Parse(line)


def GenerateLegacyPrefixes(bitness, required_prefixes, optional_prefixes):
  """Produce list of all possible combinations of legacy prefixes.

  Legacy prefixes are defined in processor manual:
    operand-size override (data16),
    address-size override,
    segment override,
    LOCK,
    REP/REPE/REPZ,
    REPNE/REPNZ.

  All permutations are enumerated, but repeated prefixes are not allowed
  (they make state count too large), even though processor decoder allows
  repetitions.

  In the future we might want to decide on a single preferred order of prefixes
  that validator allows.

  Args:
    required_prefixes: List of prefixes that have to be included in each
        combination produced
    optional_prefixes: List of prefixes that may or may not be present in
        resulting combinations.
  Returns:
    List of tuples of prefixes.
  """

  # For compatibility with old validator, disallow rep prefixes together with
  # data16.
  # See https://code.google.com/p/nativeclient/issues/detail?id=1950
  # TODO(shcherbina): get rid of it when ABI is cleaned up.
  if bitness == 32:
    if required_prefixes == ['data16']:
      if optional_prefixes == ['rep'] or optional_prefixes == ['condrep']:
        optional_prefixes = []

  all_prefixes = required_prefixes + optional_prefixes
  assert len(set(all_prefixes)) == len(all_prefixes), 'duplicate prefixes'

  required_prefixes = tuple(required_prefixes)

  result = []
  for k in range(len(optional_prefixes) + 1):
    for optional in itertools.combinations(optional_prefixes, k):
      for prefixes in itertools.permutations(required_prefixes + optional):
        # For compatibility with old validator, allow lock prefix only
        # after data16.
        # https://code.google.com/p/nativeclient/issues/detail?id=2518
        # TODO(shcherbina): get rid of it when ABI is cleaned up.
        if (bitness == 32 and
            'lock' in prefixes and
            'data16' in prefixes and
            prefixes.index('lock') < prefixes.index('data16')):
          continue
        result.append(prefixes)

  assert len(set(result)) == len(result), 'duplicate resulting combinations'

  return result


def main():
  parser = optparse.OptionParser(
      usage='%prog [options] def_files...')
  parser.add_option('--bitness',
                    type=int,
                    help='The subarchitecture: 32 or 64')
  parser.add_option('--mode',
                    help='validator or decoder')

  options, args = parser.parse_args()

  if options.bitness not in [32, 64]:
    parser.error('specify --bitness 32 or --bitness 64')

  if len(args) == 0:
    parser.error('specify at least one .def file')

  mode = {'validator': VALIDATOR, 'decoder': DECODER}[options.mode]

  print '/*'
  print ' * THIS FILE IS AUTO-GENERATED BY gen_dfa.py. DO NOT EDIT.'
  print ' * mode: %s' % options.mode
  print ' * bitness: %s' % options.bitness
  print ' */'
  print
  print '%%{'
  print '  machine decode_x86_%d;' % options.bitness
  print '  alphtype unsigned char;'

  printed_instrs = []

  instruction_names = set()

  for def_file in args:
    for instruction in ParseDefFile(def_file):
      instruction.CollectPrefixes()

      if options.bitness == 32 and Attribute('amd64') in instruction.attributes:
        continue
      if options.bitness == 64 and Attribute('ia32') in instruction.attributes:
        continue

      if mode == VALIDATOR:
        if Attribute('nacl-forbidden') in instruction.attributes:
          continue
        if (options.bitness == 32 and
            Attribute('nacl-ia32-forbidden') in instruction.attributes):
          continue
        if (options.bitness == 64 and
            Attribute('nacl-amd64-forbidden') in instruction.attributes):
          continue
        if Attribute('disabled_untested') in instruction.attributes:
          continue

      instruction_names.add((instruction.GetNameAsIdentifier(),
                             instruction.name))

      instructions = [instruction]
      instructions = sum(map(SplitRM, instructions), [])
      instructions = sum(map(SplitByteNonByte, instructions), [])
      instructions = sum([SplitVYZ(options.bitness, instr)
                          for instr in instructions],
                         [])
      instructions = sum(map(SplitL, instructions), [])

      header = '##### %s #####\n' % instruction
      variants = [InstructionToString(
                      mode, options.bitness,
                      instr,
                      reverse_operands=True)
                  for instr in instructions]
      printed_instrs.append(header + '|\n'.join(variants))

  for name_id, name in sorted(instruction_names):
    print '  action instruction_%s { SET_INSTRUCTION_NAME("%s"); }' % (
        name_id, name)

  print '  one_instruction = '
  print '|\n'.join(printed_instrs)

  print '  ;'
  print '}%%'


if __name__ == '__main__':
  main()
