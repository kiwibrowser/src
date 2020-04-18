#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Executable specification of valid instructions and superinstructions (in terms
# of their disassembler listing).
# Should serve as formal and up-to-date ABI reference and as baseline for
# validator exhaustive tests.

# It is generally organized as a set of functions responsible for recognizing
# and validating specific patterns (jump instructions, regular instructions,
# superinstructions, etc.)
# There are three outcomes for running such function:
#   - function raises DoNotMatchError (which means instruction is of completely
#     different structure, for example when we call ValidateSuperinstruction on
#     nop)
#   - function raises SandboxingError (which means instruction generally matches
#     respective pattern, but some rules are violated)
#   - function returns (which means instruction(s) is(are) safe)
#
# Why exceptions instead of returning False or something? Because they carry
# stack traces, which makes it easier to investigate why particular instruction
# was rejected.
# Why distinguish DoNotMatchError and SandboxingError? Because on the topmost
# level we attempt to call all matchers and we need to see which error message
# was most relevant.

import re


class DoNotMatchError(Exception):
  pass


class SandboxingError(Exception):
  pass


BUNDLE_SIZE = 32


def _ValidateLongNop(instruction):
  # Short nops do not require special exceptions (such as allowing repeated
  # prefixes and segment access), so they are handled as regular instructions.
  if re.match(r'nopw 0x0\(%[er]ax,%[er]ax,1\)$',
      instruction.disasm):
    return
  if re.match(
      r'(data32 )*nopw %cs:0x0\(%[er]ax,%[er]ax,1\)$',
      instruction.disasm):
    return
  raise DoNotMatchError(instruction)


def _ValidateStringInstruction(instruction):
  prefix_re = r'(rep |repz |repnz )?'
  lods_re = r'lods %ds:\(%esi\),(%al|%ax|%eax)'
  stos_re = r'stos (%al|%ax|%eax),%es:\(%edi\)'
  scas_re = r'scas %es:\(%edi\),(%al|%ax|%eax)'
  movs_re = r'movs[bwl] %ds:\(%esi\),%es:\(%edi\)'
  cmps_re = r'cmps[bwl] %es:\(%edi\),%ds:\(%esi\)'

  string_insn_re = '%s(%s)$' % (
      prefix_re,
      '|'.join([lods_re, stos_re, scas_re, movs_re, cmps_re]))

  if re.match(string_insn_re, instruction.disasm):
    return

  raise DoNotMatchError(instruction)


def _ValidateTlsInstruction(instruction):
  if re.match(r'mov %gs:(0x0|0x4),%e[a-z][a-z]$', instruction.disasm):
    return

  raise DoNotMatchError(instruction)


# What can follow 'j' in conditional jumps 'je', 'jno', etc.
_CONDITION_SUFFIX_RE = r'(a(e?)|b(e?)|g(e?)|l(e?)|(n?)e|(n?)o|(n?)p|(n?)s)'


def _AnyRegisterRE(group_name='register'):
  # TODO(shcherbina): explicitly list all kinds of registers we care to
  # distinguish for validation purposes.
  return r'(?P<%s>%%(st\(\d+\)|\w+))' % group_name


def _HexRE(group_name='value'):
  return r'(?P<%s>-?0x[\da-f]+)' % group_name


CONDITION_JUMPS_RE = re.compile(
    r'(data16 )?'
    r'(?P<name>j%s|loop(n?e)?|j[er]?cxz)(?P<branch_hint>,p[nt])? %s$'
    % (_CONDITION_SUFFIX_RE, _HexRE('destination')))


def _ImmediateRE(group_name='immediate'):
  return r'(?P<%s>\$%s)' % (
      group_name,
      _HexRE(group_name=group_name + '_value'))


def MemoryRE(group_name='memory'):
  # Possible forms:
  #   (%eax)
  #   (%eax,%ebx,1)
  #   (,%ebx,1)
  #   0x42(...)
  # and even
  #   0x42
  return r'(?P<%s>(?P<%s_segment>%%[cdefgs]s:)?%s?(\(%s?(,%s,\d)?\))?)' % (
      group_name,
      group_name,
      _HexRE(group_name=group_name + '_offset'),
      _AnyRegisterRE(group_name=group_name + '_base'),
      _AnyRegisterRE(group_name=group_name + '_index'))


def _IndirectJumpTargetRE(group_name='target'):
  return r'(?P<%s>\*(%s|%s))' % (
      group_name,
      _AnyRegisterRE(group_name=group_name + '_register'),
      MemoryRE(group_name=group_name + '_memory'))


def _OperandRE(group_name='operand'):
  return r'(?P<%s>%s|%s|%s|%s)' % (
      group_name,
      _AnyRegisterRE(group_name=group_name + '_register'),
      _ImmediateRE(group_name=group_name + '_immediate'),
      MemoryRE(group_name=group_name + '_memory'),
      _IndirectJumpTargetRE(group_name=group_name + '_target'))


def _SplitOps(insn, args):
  # We can't use just args.split(',') because operands can contain commas
  # themselves, for example '(%r15,%rax,1)'.
  ops = []
  i = 0
  while True:
    # We do not use mere re.match(_OperandRE(), args, i) here because
    # python backtracking regexes do not guarantee to find longest match.
    m = re.compile(r'(%s)($|,)' % _OperandRE()).match(args, i)
    assert m is not None, (args, i)
    ops.append(m.group(1))
    i = m.end(1)
    if i == len(args):
      break
    assert args[i] == ',', (insn, args, i)
    i += 1
  return ops


def ParseInstruction(instruction):
  """Parse an instruction into operands.

  Args:
    instruction: objdump_parser.Instruction tuple
  Returns:
    prefixes, mnemonic, operands
  Raises:
    SandboxingError on erroneous bytes.
  """
  # Strip comment.
  disasm, _, _ = instruction.disasm.partition('#')
  elems = disasm.split()

  if elems == []:
    raise SandboxingError(
        'disasm is empty', instruction)

  prefixes = []
  while elems != [] and elems[0] in [
      'lock', 'rep', 'repz', 'repnz',
      'data16', 'data32', 'addr16', 'addr32', 'addr64']:
    prefixes.append(elems.pop(0))

  if elems == []:
    raise SandboxingError(
        'dangling legacy prefixes', instruction)

  name = elems[0]

  if re.match(r'rex([.]W?R?X?B?)?$', name):
    raise SandboxingError('dangling rex prefix', instruction)

  # There could be branching expectation information in instruction names:
  #    jo,pt      <addr>
  #    jge,pn     <addr>
  name_re = r'[a-z]\w*(,p[nt])?$'
  assert re.match(name_re, name) or name == "nop/reserved", name

  if len(elems) == 1:
    ops = []
  elif len(elems) == 2:
    ops = _SplitOps(instruction, elems[1])
  else:
    assert False, instruction

  return prefixes, name, ops


REG32_TO_REG64 = {
    '%eax' : '%rax',
    '%ebx' : '%rbx',
    '%ecx' : '%rcx',
    '%edx' : '%rdx',
    '%esi' : '%rsi',
    '%edi' : '%rdi',
    '%esp' : '%rsp',
    '%ebp' : '%rbp',
    '%r8d' : '%r8',
    '%r9d' : '%r9',
    '%r10d' : '%r10',
    '%r11d' : '%r11',
    '%r12d' : '%r12',
    '%r13d' : '%r13',
    '%r14d' : '%r14',
    '%r15d' : '%r15'}

REGS32 = REG32_TO_REG64.keys()
REGS64 = REG32_TO_REG64.values()


class Condition(object):
  """Represents assertion about the state of 64-bit registers.

  (used as precondition and postcondition)

  Supported assertions:
    0. %rpb and %rsp are sandboxed (and nothing is known about other registers)
    1. {%rax} is restricted, %rbp and %rsp are sandboxed
    2-13. same for %rbx-%r14 not including %rbp and %rsp
    14. %rbp is restricted, %rsp is sandboxed
    15. %rsp is restricted, %rpb is sandboxed

  It can be observed that all assertions 1..15 differ from default 0 in a single
  register, which prompts internal representation of a single field,
  _restricted_register, which stores name of this standing out register
  (or None).

  * 'restricted' means higher 32 bits are zeroes
  * 'sandboxed' means within [%r15, %r15 + 2**32) range
  It goes without saying that %r15 is never changed and by definition sandboxed.
  """

  def __init__(self, restricted=None, restricted_instead_of_sandboxed=None):
    self._restricted_register = None
    if restricted is not None:
      assert restricted_instead_of_sandboxed is None
      assert restricted in REGS64
      assert restricted not in ['%r15', '%rbp', '%rsp']
      self._restricted_register = restricted
    if restricted_instead_of_sandboxed is not None:
      assert restricted is None
      assert restricted_instead_of_sandboxed in ['%rbp', '%rsp']
      self._restricted_register = restricted_instead_of_sandboxed

  def GetAlteredRegisters(self):
    """ Return pair (restricted, restricted_instead_of_sandboxed).

    Each item is either register name or None.
    """
    if self._restricted_register is None:
      return None, None
    elif self._restricted_register in ['%rsp', '%rbp']:
      return None, self._restricted_register
    else:
      return self._restricted_register, None

  def __eq__(self, other):
    return self._restricted_register == other._restricted_register

  def __ne__(self, other):
    return not self == other

  def Implies(self, other):
    return self.WhyNotImplies(other) is None

  def WhyNotImplies(self, other):
    if other._restricted_register is None:
      if self._restricted_register in ['%rbp', '%rsp']:
        return '%s should not be restricted' % self._restricted_register
      else:
        return None
    else:
      if self._restricted_register != other._restricted_register:
        return (
            'register %s should be restricted, '
            'while in fact %r is restricted' % (
                other._restricted_register, self._restricted_register))
      else:
        return None

  def __repr__(self):
    if self._restricted_register is None:
      return 'Condition(default)'
    elif self._restricted_register in ['%rbp', '%rsp']:
      return ('Condition(%s restricted instead of sandboxed)'
              % self._restricted_register)
    else:
      return 'Condition(%s restricted)' % self._restricted_register

  @staticmethod
  def All():
    yield Condition()
    for reg in REGS64:
      if reg not in ['%r15', '%rbp', '%rsp']:
        yield Condition(restricted=reg)
    yield Condition(restricted_instead_of_sandboxed='%rbp')
    yield Condition(restricted_instead_of_sandboxed='%rsp')


def _ValidateSpecialStackInstruction(instruction):
  # Validate 64-bit instruction that is in special relationship with rsp/rbp.

  if instruction.disasm in ['mov %rsp,%rbp', 'mov %rbp,%rsp']:
    return Condition(), Condition()

  m = re.match(
      'and %s,%%rsp$' % _ImmediateRE(),
      instruction.disasm)
  if m is not None:
    # We only allow 1-byte immediate, so we have to look at machine code.
    if (len(instruction.bytes) == 4 and
        0x48 <= instruction.bytes[0] <= 0x4f and
        instruction.bytes[1:3] == [0x83, 0xe4]):
      # We extract mask from bytes, not from textual representation, because
      # objdump and RDFA decoder print it differently
      # (-1 is displayed as '0xffffffffffffffff' by objdump and as '0xff' by
      # RDFA decoder).
      # See https://code.google.com/p/nativeclient/issues/detail?id=3164
      mask = instruction.bytes[3]
      assert mask == int(m.group('immediate_value'), 16) & 0xff
      if mask < 0x80:
        raise SandboxingError(
            'mask should be negative to ensure that higher '
            'bits of %rsp do not change',
            instruction)
    else:
      raise SandboxingError(
          'unsupported form of "and <mask>,%rsp" instruction', instruction)
    return Condition(), Condition()

  if (instruction.disasm in ['add %r15,%rbp', 'add %r15,%rbp'] or
      re.match(r'lea (0x0+)?\(%rbp,%r15,1\),%rbp$', instruction.disasm)):
    return Condition(restricted_instead_of_sandboxed='%rbp'), Condition()

  if (instruction.disasm in ['add %r15,%rsp', 'add %r15,%rsp'] or
      re.match(r'lea (0x0+)?\(%rsp,%r15,1\),%rsp$', instruction.disasm)):
    return Condition(restricted_instead_of_sandboxed='%rsp'), Condition()

  # TODO(shcherbina): disallow this instruction once
  # http://code.google.com/p/nativeclient/issues/detail?id=3070
  # is fixed.
  if instruction.disasm == 'or %r15,%rsp':
    return Condition(restricted_instead_of_sandboxed='%rsp'), Condition()

  raise DoNotMatchError(instruction)


def _GetLegacyPrefixes(instruction):
  result = []
  for b in instruction.bytes:
    if b not in [
        0x66, 0x67, 0x2e, 0x3e, 0x26, 0x64, 0x65, 0x36, 0xf0, 0xf3, 0xf2]:
      break
    if b == 0x67:
      raise SandboxingError('addr prefix is not allowed', instruction)
    if b in result:
      raise SandboxingError('duplicate legacy prefix', instruction)
    result.append(b)
  return result


def _ProcessMemoryAccess(instruction, operands):
  """Make sure that memory access is valid and return precondition required.

  (only makes sense for 64-bit instructions)

  Args:
    instruction: Instruction tuple
    operands: list of instruction operands as strings, for example
              ['%eax', '(%r15,%rbx,1)']
  Returns:
    Condition object representing precondition required for memory access (if
    it's present among operands) to be valid.
  Raises:
    SandboxingError if memory access is invalid.
  """
  precondition = Condition()
  for op in operands:
    m = re.match(MemoryRE() + r'$', op)
    if m is not None:
      assert m.group('memory_segment') is None
      base = m.group('memory_base')
      index = m.group('memory_index')
      allowed_bases = ['%r15', '%rbp', '%rsp', '%rip']
      if base not in allowed_bases:
        raise SandboxingError(
            'memory access only is allowed with base from %s'
            % allowed_bases,
            instruction)
      if index is not None:
        if index == '%riz':
          pass
        elif index in REGS64:
          if index in ['%r15', '%rsp', '%rbp']:
            raise SandboxingError(
                '%s can\'t be used as index in memory access' % index,
                instruction)
          else:
            assert precondition == Condition()
            precondition = Condition(restricted=index)
        else:
          raise SandboxingError(
              'unrecognized register is used for memory access as index',
              instruction)
  return precondition


def _ProcessOperandWrites(instruction, write_operands, zero_extending=False):
  """Check that writes to operands are valid, return postcondition established.

  (only makes sense for 64-bit instructions)

  Args:
    instruction: Instruction tuple
    write_operands: list of operands instruction writes to as strings,
                    for example ['%eax', '(%r15,%rbx,1)']
    zero_extending: whether instruction is considered zero extending
  Returns:
    Condition object representing postcondition established by operand writes.
  Raises:
    SandboxingError if write is invalid.
  """
  postcondition = Condition()
  for i, op in enumerate(write_operands):
    if op in ['%r15', '%r15d', '%r15w', '%r15b']:
      raise SandboxingError('changes to r15 are not allowed', instruction)
    if op in ['%bpl', '%bp', '%rbp']:
      raise SandboxingError('changes to rbp are not allowed', instruction)
    if op in ['%spl', '%sp', '%rsp']:
      raise SandboxingError('changes to rsp are not allowed', instruction)

    if op in REGS32:
      # Only last of the operand writes is considered zero-extending.
      # For example,
      #   xchg %eax, (%rbp)
      # does not zero-extend %rax.
      if zero_extending and i == len(write_operands) - 1:
        r = REG32_TO_REG64[op]
        if r in ['%rbp', '%rsp']:
          postcondition = Condition(restricted_instead_of_sandboxed=r)
        else:
          postcondition = Condition(restricted=r)
      else:
        if op in ['%ebp', '%esp']:
          raise SandboxingError(
              'non-zero-extending changes to ebp and esp are not allowed',
              instruction)

  return postcondition


def _InstructionNameIn(name, candidates):
  return re.match('(%s)[bwlq]?$' % '|'.join(candidates), name) is not None


_X87_INSTRUCTIONS = set([
  'f2xm1',
  'fabs',
  'fadd', 'fadds', 'faddl', 'faddp',
  'fiadd', 'fiaddl',
  'fbld',
  'fbstp',
  'fchs',
  'fnclex',
  'fcmovb', 'fcmovbe', 'fcmove', 'fcmovnb',
  'fcmovnbe', 'fcmovne', 'fcmovnu', 'fcmovu',
  'fcom', 'fcoms', 'fcoml',
  'fcomp', 'fcomps', 'fcompl',
  'fcompp',
  'fcomi',
  'fcomip',
  'fcos',
  'fdecstp',
  'fdiv', 'fdivs', 'fdivl',
  'fdivp',
  'fdivr', 'fdivrs', 'fdivrl',
  'fdivrp',
  'fidiv', 'fidivl',
  'fidivp',
  'fidivr', 'fidivrl',
  'ffree',
  'ficom', 'ficoml',
  'ficomp', 'ficompl',
  'fild', 'fildl', 'fildll',
  'fincstp',
  'fninit',
  'fist', 'fistl',
  'fistp', 'fistpl', 'fistpll',
  'fisttp', 'fisttpl', 'fisttpll',
  'fld', 'flds', 'fldl', 'fldt',
  'fld1',
  'fldcw',
  'fldenv',
  'fldl2e',
  'fldl2t',
  'fldlg2',
  'fldln2',
  'fldpi',
  'fldz',
  'fmul', 'fmuls', 'fmull',
  'fmulp',
  'fimul', 'fimull',
  'fnop',
  'fpatan',
  'fprem',
  'fprem1',
  'fptan',
  'frndint',
  'frstor',
  'fnsave',
  'fscale',
  'fsin',
  'fsincos',
  'fsqrt',
  'fst', 'fsts', 'fstl',
  'fstp', 'fstps', 'fstpl', 'fstpt',
  'fnstcw',
  'fnstenv',
  'fnstsw',
  'fsub', 'fsubs', 'fsubl',
  'fsubp',
  'fsubr', 'fsubrs', 'fsubrl',
  'fsubrp',
  'fisub', 'fisubl',
  'fisubr', 'fisubrl',
  'ftst',
  'fucom', 'fucomp', 'fucompp',
  'fucomi', 'fucomip',
  'fwait',
  'fxam',
  'fxch',
  'fxtract',
  'fyl2x',
  'fyl2xp1',
])


# Instructions from mmx_instructions.def (besides MMX, they include SSE2/3
# and other stuff that works with MMX registers).
_MMX_INSTRUCTIONS = set([
  'cvtpd2pi',
  'cvtpi2pd',
  'cvtpi2ps',
  'cvtps2pi',
  'cvttpd2pi',
  'cvttps2pi',
  'emms',
  'femms',
  'frstor',
  'fnsave',
  'movntq',
  'movdq2q',
  'movq2dq',
  'pabsb',
  'pabsd',
  'pabsw',
  'packssdw',
  'packsswb',
  'packuswb',
  'paddb',
  'paddd',
  'paddq',
  'paddsb',
  'paddsw',
  'paddusb',
  'paddusw',
  'paddw',
  'palignr',
  'pand',
  'pandn',
  'pavgb',
  'pavgusb',
  'pavgw',
  'pcmpeqb',
  'pcmpeqd',
  'pcmpeqw',
  'pcmpgtb',
  'pcmpgtd',
  'pcmpgtw',
  'pextrw',
  'pf2id',
  'pf2iw',
  'pfacc',
  'pfadd',
  'pfcmpeq',
  'pfcmpge',
  'pfcmpgt',
  'pfmax',
  'pfmin',
  'pfmul',
  'pfnacc',
  'pfpnacc',
  'pfrcp',
  'pfrcpit1',
  'pfrcpit2',
  'pfrsqit1',
  'pfrsqrt',
  'pfsub',
  'pfsubr',
  'phaddd',
  'phaddsw',
  'phaddw',
  'phsubd',
  'phsubsw',
  'phsubw',
  'pi2fd',
  'pi2fw',
  'pinsrw',
  'pmaddubsw',
  'pmaddwd',
  'pmaxsw',
  'pmaxub',
  'pminsw',
  'pminub',
  'pmovmskb',
  'pmulhrw',
  'pmulhuw',
  'pmulhw',
  'pmulhrsw',
  'pmullw',
  'pmuludq',
  'por',
  'psadbw',
  'pshufb',
  'pshufw',
  'psignb',
  'psignd',
  'psignw',
  'pslld',
  'psllq',
  'psllw',
  'psrad',
  'psraw',
  'psrld',
  'psrlq',
  'psrlw',
  'psubb',
  'psubd',
  'psubq',
  'psubsb',
  'psubsw',
  'psubusb',
  'psubusw',
  'psubw',
  'pswapd',
  'punpckhbw',
  'punpckhdq',
  'punpckhwd',
  'punpcklbw',
  'punpckldq',
  'punpcklwd',
  'pxor',
])


# Instructions from xmm_instructions.def (that is, instructions that work
# with XMM registers). These instruction names can be prepended with 'v', which
# results in their AVX counterpart.
_XMM_AVX_INSTRUCTIONS = set([
  'addpd',
  'addps',
  'addsd',
  'addss',
  'addsubpd',
  'addsubps',
  'aesdec',
  'aesdeclast',
  'aesenc',
  'aesenclast',
  'aesimc',
  'aeskeygenassist',
  'andnpd',
  'andnps',
  'andpd',
  'andps',
  'blendpd',
  'blendps',
  'blendvpd',
  'blendvps',
  'comisd',
  'comiss',
  'cvtdq2pd',
  'cvtdq2ps',
  'cvtpd2dq',
  'cvtpd2ps',
  'cvtps2dq',
  'cvtps2pd',
  'cvtsd2si',
  'cvtsd2ss',
  'cvtsi2sd', 'cvtsi2sdl', 'cvtsi2sdq',
  'cvtsi2ss', 'cvtsi2ssl', 'cvtsi2ssq',
  'cvtss2sd',
  'cvtss2si',
  'cvttpd2dq',
  'cvttps2dq',
  'cvttsd2si',
  'cvttss2si',
  'divpd',
  'divps',
  'divsd',
  'divss',
  'dppd',
  'dpps',
  'extractps',
  'extrq',
  'haddpd',
  'haddps',
  'hsubpd',
  'hsubps',
  'insertps',
  'insertq',
  'lddqu',
  'ldmxcsr',
  'maxpd',
  'maxps',
  'maxsd',
  'maxss',
  'minpd',
  'minps',
  'minsd',
  'minss',
  'movapd',
  'movaps',
  'movddup',
  'movdqa',
  'movdqu',
  'movhlps',
  'movhpd',
  'movhps',
  'movlhps',
  'movlpd',
  'movlps',
  'movmskpd',
  'movmskps',
  'movntdq',
  'movntdqa',
  'movntpd',
  'movntps',
  'movsd',
  'movshdup',
  'movsldup',
  'movss',
  'movupd',
  'movups',
  'mpsadbw',
  'mulpd',
  'mulps',
  'mulsd',
  'mulss',
  'orpd',
  'orps',
  'pabsb',
  'pabsd',
  'pabsw',
  'packssdw',
  'packsswb',
  'packusdw',
  'packuswb',
  'paddb',
  'paddd',
  'paddq',
  'paddsb',
  'paddsw',
  'paddusb',
  'paddusw',
  'paddw',
  'palignr',
  'pand',
  'pandn',
  'pavgb',
  'pavgw',
  'pblendvb',
  'pblendw',
  'pclmulqdq',
  'pclmullqlqdq',
  'pclmulhqlqdq',
  'pclmullqhqdq',
  'pclmulhqhqdq',
  'pcmpeqb',
  'pcmpeqd',
  'pcmpeqq',
  'pcmpeqw',
  'pcmpestri',
  'pcmpestrm',
  'pcmpgtb',
  'pcmpgtd',
  'pcmpgtq',
  'pcmpgtw',
  'pcmpistri',
  'pcmpistrm',
  'pextrb',
  'pextrd',
  'pextrq',
  'pextrw',
  'phaddd',
  'phaddsw',
  'phaddw',
  'phminposuw',
  'phsubd',
  'phsubsw',
  'phsubw',
  'pinsrb',
  'pinsrd',
  'pinsrq',
  'pinsrw',
  'pmaddubsw',
  'pmaddwd',
  'pmaxsb',
  'pmaxsd',
  'pmaxsw',
  'pmaxub',
  'pmaxud',
  'pmaxuw',
  'pminsb',
  'pminsd',
  'pminsw',
  'pminub',
  'pminud',
  'pminuw',
  'pmovmskb',
  'pmovsxbd',
  'pmovsxbq',
  'pmovsxbw',
  'pmovsxdq',
  'pmovsxwd',
  'pmovsxwq',
  'pmovzxbd',
  'pmovzxbq',
  'pmovzxbw',
  'pmovzxdq',
  'pmovzxwd',
  'pmovzxwq',
  'pmuldq',
  'pmulhrsw',
  'pmulhuw',
  'pmulhw',
  'pmulld',
  'pmullw',
  'pmuludq',
  'por',
  'psadbw',
  'pshufb',
  'pshufd',
  'pshufhw',
  'pshuflw',
  'psignb',
  'psignd',
  'psignw',
  'pslld',
  'pslldq',
  'psllq',
  'psllw',
  'psrad',
  'psraw',
  'psrld',
  'psrldq',
  'psrlq',
  'psrlw',
  'psubb',
  'psubd',
  'psubq',
  'psubsb',
  'psubsw',
  'psubusb',
  'psubusw',
  'psubw',
  'ptest',
  'punpckhbw',
  'punpckhdq',
  'punpckhqdq',
  'punpckhwd',
  'punpcklbw',
  'punpckldq',
  'punpcklqdq',
  'punpcklwd',
  'pxor',
  'rcpps',
  'rcpss',
  'roundpd',
  'roundps',
  'roundsd',
  'roundss',
  'rsqrtps',
  'rsqrtss',
  'shufpd',
  'shufps',
  'sqrtpd',
  'sqrtps',
  'sqrtsd',
  'sqrtss',
  'stmxcsr',
  'subpd',
  'subps',
  'subsd',
  'subss',
  'ucomisd',
  'ucomiss',
  'unpckhpd',
  'unpckhps',
  'unpcklpd',
  'unpcklps',
  'xorpd',
  'xorps',
])

_XMM_AVX_INSTRUCTIONS.update(['v' + name for name in _XMM_AVX_INSTRUCTIONS])

_XMM_AVX_INSTRUCTIONS.update([
  'movntsd',
  'movntss',
  'vbroadcastf128',
  'vbroadcastsd',
  'vbroadcastss',
  'vcvtpd2psx',
  'vcvtpd2psy',
  'vcvtpd2dqx',
  'vcvtpd2dqy',
  'vcvtph2ps',
  'vcvtps2ph',
  'vcvttpd2dqx',
  'vcvttpd2dqy',
  'vextractf128',
  'vfrczpd',
  'vfrczps',
  'vfrczsd',
  'vfrczss',
  'vinsertf128',
  'vmaskmovpd',
  'vmaskmovps',
  'vpcmov',
  'vpcomb',
  'vpcomd',
  'vpcomq',
  'vpcomub',
  'vpcomud',
  'vpcomuq',
  'vpcomuw',
  'vpcomw',
  'vperm2f128',
  'vpermil2pd',
  'vpermil2ps',
  'vpermilpd',
  'vpermilps',
  'vphaddbd',
  'vphaddbq',
  'vphaddbw',
  'vphadddq',
  'vphaddubd',
  'vphaddubq',
  'vphaddubw',
  'vphaddudq',
  'vphadduwd',
  'vphadduwq',
  'vphaddwd',
  'vphaddwq',
  'vphsubbw',
  'vphsubdq',
  'vphsubwd',
  'vpmacsdd',
  'vpmacsdqh',
  'vpmacsdql',
  'vpmacssdd',
  'vpmacssdqh',
  'vpmacssdql',
  'vpmacsswd',
  'vpmacssww',
  'vpmacswd',
  'vpmacsww',
  'vpmadcsswd',
  'vpmadcswd',
  'vpperm',
  'vprotb',
  'vprotd',
  'vprotq',
  'vprotw',
  'vpshab',
  'vpshad',
  'vpshaq',
  'vpshaw',
  'vpshlb',
  'vpshld',
  'vpshlq',
  'vpshlw',
  'vtestpd',
  'vtestps',
  'vzeroall',
  'vzeroupper',
])

# Add AXV2 instructions.
_XMM_AVX_INSTRUCTIONS.update([
  'vpblendd',
  'vpmaskmovd',
  'vpmaskmovq',
  'vpsllvd',
  'vpsllvq',
  'vpsrlvd',
  'vpsrlvq',
  'vpsravd',
  'vpbroadcastb',
  'vpbroadcastw',
  'vpbroadcastd',
  'vpbroadcastq',
  'vbroadcasti128',
  'vbroadastsd',
  'vbroadastss',
  'vextracti128',
  'vinserti128',
  'vpermd',
  'vperm2i128',
  'vpermps',
  'vpermpd',
  'vpermq',
])

# Add instructions like VFMADDPD/VFMADD132PD/VFMADD213PD/VFMADD231PD.
for fma_name in [
    'vfmadd%spd',
    'vfmadd%sps',
    'vfmadd%ssd',
    'vfmadd%sss',
    'vfnmadd%spd',
    'vfnmadd%sps',
    'vfnmadd%ssd',
    'vfnmadd%sss',
    'vfmsub%spd',
    'vfmsub%sps',
    'vfmsub%ssd',
    'vfmsub%sss',
    'vfnmsub%spd',
    'vfnmsub%sps',
    'vfnmsub%ssd',
    'vfnmsub%sss',
    'vfmaddsub%spd',
    'vfmaddsub%sps',
    'vfmsubadd%spd',
    'vfmsubadd%sps',
    ]:
  for operand_order_suffix in ['', '132', '213', '231']:
    _XMM_AVX_INSTRUCTIONS.add(fma_name % operand_order_suffix)

for cmp_suffix in ['pd', 'ps', 'sd', 'ss']:
  for cmp_op in ['', 'eq', 'lt', 'le', 'unord', 'neq', 'nlt', 'nle', 'ord']:
    _XMM_AVX_INSTRUCTIONS.add('cmp%s%s' % (cmp_op, cmp_suffix))
    _XMM_AVX_INSTRUCTIONS.add('vcmp%s%s' % (cmp_op, cmp_suffix))
  for cmp_op in [
      'eq_uq', 'nge', 'ngt', 'false',
      'neq_oq', 'ge', 'gt', 'true',
      'eq_os', 'lt_oq', 'le_oq', 'unord_s',
      'neq_us', 'nlt_uq', 'nle_uq', 'ord_s',
      'eq_us', 'nge_uq', 'ngt_uq', 'false_os',
      'neq_os', 'ge_oq', 'gt_oq', 'true_us']:
    _XMM_AVX_INSTRUCTIONS.add('vcmp%s%s' % (cmp_op, cmp_suffix))


def ValidateRegularInstruction(instruction, bitness):
  """Validate regular instruction (not direct jump).

  Args:
    instruction: objdump_parser.Instruction tuple
    bitness: 32 or 64
  Returns:
    Pair (precondition, postcondition) of Condition instances.
    (for 32-bit case they are meaningless and are not used)
  Raises:
    According to usual convention.
  """
  assert bitness in [32, 64]

  if instruction.disasm.startswith('.byte ') or '(bad)' in instruction.disasm:
    raise SandboxingError('objdump failed to decode', instruction)

  try:
    _ValidateLongNop(instruction)
    return Condition(), Condition()
  except DoNotMatchError:
    pass

  # Report error on duplicate prefixes (note that they are allowed in
  # long nops).
  _GetLegacyPrefixes(instruction)

  if bitness == 32:
    try:
      _ValidateStringInstruction(instruction)
      return Condition(), Condition()
    except DoNotMatchError:
      pass

    try:
      _ValidateTlsInstruction(instruction)
      return Condition(), Condition()
    except DoNotMatchError:
      pass

  if bitness == 64:
    try:
      return _ValidateSpecialStackInstruction(instruction)
    except DoNotMatchError:
      pass

  prefixes, name, ops = ParseInstruction(instruction)

  for prefix in prefixes:
    if prefix != 'lock':
      raise SandboxingError('prefix %s is not allowed' % prefix, instruction)

  for op in ops:
    if op in ['%cs', '%ds', '%es', '%ss', '%fs', '%gs']:
      raise SandboxingError(
          'access to segment registers is not allowed', instruction)
    if op.startswith('%cr'):
      raise SandboxingError(
          'access to control registers is not allowed', instruction)
    if op.startswith('%db'):
      raise SandboxingError(
          'access to debug registers is not allowed', instruction)
    if op.startswith('%tr'):
      raise SandboxingError(
          'access to test registers is not allowed', instruction)

    m = re.match(MemoryRE() + r'$', op)
    if m is not None and m.group('memory_segment') is not None:
      raise SandboxingError(
          'segments in memory references are not allowed', instruction)


  if bitness == 32:
    if _InstructionNameIn(
        name,
        ['mov',  # including MOVQ
         'add', 'sub', 'and', 'or', 'xor',
         'xchg', 'xadd',
         'inc', 'dec', 'neg', 'not',
         'shl', 'shr', 'sar', 'rol', 'ror', 'rcl', 'rcr',
         'shld', 'shrd',
         'pop', 'cmpxchg8b',
         'lea',
         'nop',
         'prefetch', 'prefetchnta', 'prefetcht0', 'prefetcht1', 'prefetcht2',
         'prefetchw',
         'adc', 'sbb', 'bsf', 'bsr',
         'lzcnt', 'tzcnt', 'popcnt', 'crc32', 'cmpxchg',
         'movbe',
         'movmskpd', 'movmskps', 'movnti',
         'btc', 'btr', 'bts', 'bt',
         'cmp', 'test',
         'imul', 'mul', 'div', 'idiv', 'push',
        ]) or name in ['movd', 'vmovd', 'vmovq']:
      return Condition(), Condition()

    elif name in [
        'cpuid', 'hlt', 'lahf', 'sahf', 'rdtsc', 'pause',
        'sfence', 'lfence', 'mfence',
        'leave',
        'cmc', 'clc', 'cld', 'stc', 'std',
        'cwtl', 'cbtw', 'cltq',  # CBW/CWDE/CDQE
        'cltd', 'cwtd', 'cqto',  # CWD/CDQ/CQO
        'ud2', 'ud2a',
        ]:
      return Condition(), Condition()

    elif re.match(r'mov[sz][bwl][lqw]$', name):  # MOVSX, MOVSXD, MOVZX
      return Condition(), Condition()

    elif name == 'bswap':
      if ops[0] not in REGS32:
        raise SandboxingError(
            'bswap is only allowed with 32-bit operands',
            instruction)
      return Condition(), Condition()

    elif re.match(r'(cmov|set)%s$' % _CONDITION_SUFFIX_RE, name):
      return Condition(), Condition()

    elif name in _X87_INSTRUCTIONS:
      return Condition(), Condition()

    elif name in _MMX_INSTRUCTIONS:
      return Condition(), Condition()

    elif name in _XMM_AVX_INSTRUCTIONS:
      return Condition(), Condition()

    elif name in ['maskmovq', 'maskmovdqu', 'vmaskmovdqu']:
      # In 64-bit mode these instructions are processed in
      # ValidateSuperinstruction64, together with string instructions.
      return Condition(), Condition()

    else:
      raise DoNotMatchError(instruction)

  elif bitness == 64:
    precondition = Condition()
    postcondition = Condition()
    zero_extending = False
    touches_memory = True

    # Here we determine which operands instruction writes to. Note that for
    # our purposes writes are only relevant when they either have potential to
    # zero-extend regular register, or can modify protected registers (r15,
    # rbp, rsp).
    # This means that we don't have to worry about implicit operands (for
    # example it does not matter to us that mul writes to rdx and rax).

    if (_InstructionNameIn(
          name, [
            'mov',  # including MOVQ
            'movabs',
            'movd',
            'add', 'sub', 'and', 'or', 'xor']) or
        name in ['movd', 'vmovd', 'vmovq']):
      # Technically, movabs is not allowed, but it's ok to accept it here,
      # because it will later be rejected because of improper memory access.
      # On the other hand, because of objdump quirk it prints regular
      # mov with 64-bit immediate as movabs:
      #   48 b8 00 00 00 00 00 00 00 00
      #   movabs $0x0,%rax
      assert len(ops) == 2
      zero_extending = True
      write_ops = [ops[1]]

    elif re.match(r'mov[sz][bwl][lqw]$', name):  # MOVSX, MOVSXD, MOVZX
      assert len(ops) == 2
      zero_extending = True
      write_ops = [ops[1]]

    elif _InstructionNameIn(name, ['xchg', 'xadd']):
      assert len(ops) == 2
      zero_extending = True
      write_ops = ops

    elif _InstructionNameIn(name, ['inc', 'dec', 'neg', 'not']):
      assert len(ops) == 1
      zero_extending = True
      write_ops = ops

    elif _InstructionNameIn(name, [
        'shl', 'shr', 'sar', 'rol', 'ror', 'rcl', 'rcr']):
      assert len(ops) in [1, 2]
      write_ops = [ops[-1]]

    elif _InstructionNameIn(name, ['shld', 'shrd']):
      assert len(ops) == 3
      write_ops = [ops[2]]

    elif _InstructionNameIn(name, [
        'pop', 'cmpxchg8b', 'cmpxchg16b']):
      assert len(ops) == 1
      write_ops = ops

    elif name == 'lea':
      assert len(ops) == 2
      write_ops = [ops[1]]
      touches_memory = False
      zero_extending = True

    elif _InstructionNameIn(name, ['nop']):
      assert len(ops) in [0, 1]
      write_ops = []
      touches_memory = False

    elif name in [
        'prefetch', 'prefetchnta', 'prefetcht0', 'prefetcht1', 'prefetcht2',
        'prefetchw']:
      assert len(ops) == 1
      write_ops = []
      touches_memory = False

    elif _InstructionNameIn(
        name,
        ['adc', 'sbb', 'bsf', 'bsr',
         'lzcnt', 'tzcnt', 'popcnt', 'crc32', 'cmpxchg',
         'movbe',
         'movmskpd', 'movmskps', 'movnti']):
      assert len(ops) == 2
      write_ops = [ops[1]]

    elif _InstructionNameIn(name, ['btc', 'btr', 'bts', 'bt']):
      assert len(ops) == 2
      # bt* accept arbitrarily large bit offset when second
      # operand is memory and offset is in register.
      # Interestingly, when offset is immediate, it's taken modulo operand size,
      # even when second operand is memory.
      # Also, validator currently disallows
      #   bt* <register>, <register>
      # which is techincally safe. We disallow it in spec as well for
      # simplicity.
      if not re.match(_ImmediateRE() + r'$', ops[0]):
        raise SandboxingError(
            'bt* is only allowed with immediate as bit offset',
            instruction)
      if _InstructionNameIn(name, ['bt']):
        write_ops = []
      else:
        write_ops = [ops[1]]

    elif _InstructionNameIn(name, ['cmp', 'test']):
      assert len(ops) == 2
      write_ops = []

    elif name == 'bswap':
      assert len(ops) == 1
      if ops[0] not in REGS32 + REGS64:
        raise SandboxingError(
            'bswap is only allowed with 32-bit and 64-bit operands',
            instruction)
      write_ops = ops

    elif name in [
        'cpuid', 'hlt', 'lahf', 'sahf', 'rdtsc', 'pause',
        'sfence', 'lfence', 'mfence',
        'cmc', 'clc', 'cld', 'stc', 'std',
        'cwtl', 'cbtw', 'cltq',  # CBW/CWDE/CDQE
        'cltd', 'cwtd', 'cqto',  # CWD/CDQ/CQO
        'ud2', 'ud2a',
        ]:
      assert len(ops) == 0
      write_ops = []

    elif _InstructionNameIn(name, ['imul']):
      if len(ops) == 1:
        write_ops = []
      elif len(ops) == 2:
        zero_extending = True
        write_ops = [ops[1]]
      elif len(ops) == 3:
        zero_extending = True
        write_ops = [ops[2]]
      else:
        assert False

    elif _InstructionNameIn(name, ['mul', 'div', 'idiv', 'push']):
      assert len(ops) == 1
      write_ops = []

    elif re.match(r'cmov%s$' % _CONDITION_SUFFIX_RE, name):
      assert len(ops) == 2
      write_ops = [ops[1]]

    elif re.match(r'set%s$' % _CONDITION_SUFFIX_RE, name):
      assert len(ops) == 1
      write_ops = ops

    elif name in _X87_INSTRUCTIONS:
      assert 0 <= len(ops) <= 2
      # Actually, x87 instructions can write to x87 registers and to memory,
      # and there is even one instruction (fstsw/fnstsw) that writes to ax.
      # But these writes do not matter for sandboxing purposes.
      write_ops = []

    elif name in _MMX_INSTRUCTIONS:
      assert 0 <= len(ops) <= 3
      write_ops = ops[-1:]

    elif name in _XMM_AVX_INSTRUCTIONS:
      assert 0 <= len(ops) <= 5
      write_ops = ops[-1:]

    else:
      raise DoNotMatchError(instruction)

    if touches_memory:
      precondition = _ProcessMemoryAccess(instruction, ops)

    postcondition = _ProcessOperandWrites(
        instruction, write_ops, zero_extending)

    return precondition, postcondition

  else:
    assert False, bitness


def ValidateDirectJump(instruction, bitness):
  assert bitness in [32, 64]
  m = CONDITION_JUMPS_RE.match(instruction.disasm)
  if m is not None:
    if (m.group('name') == 'jcxz' or
        (m.group('name') == 'jecxz' and bitness == 64)):
      raise SandboxingError('disallowed form of jcxz instruction', instruction)

    if (m.group('name').startswith('loop') and
        m.group('branch_hint') is not None):
      raise SandboxingError(
          'branch hints are not allowed with loop instruction', instruction)
    # Unfortunately we can't rely on presence of 'data16' prefix in disassembly,
    # because neither nacl-objdump nor objdump we base our decoder print it.
    # So we look at bytes.
    if 0x66 in _GetLegacyPrefixes(instruction):
      raise SandboxingError(
          '16-bit conditional jumps are disallowed', instruction)
    return int(m.group('destination'), 16)

  jumps_re = re.compile(r'(jmp|call)(|w|q) %s$' % _HexRE('destination'))
  m = jumps_re.match(instruction.disasm)
  if m is not None:
    if m.group(2) == 'w':
      raise SandboxingError('16-bit jumps are disallowed', instruction)
    return int(m.group('destination'), 16)

  raise DoNotMatchError(instruction)


def ValidateDirectJumpOrRegularInstruction(instruction, bitness):
  """Validate anything that is not superinstruction.

  Args:
    instruction: objdump_parser.Instruction tuple.
    bitness: 32 or 64.
  Returns:
    Triple (jump_destination, precondition, postcondition).
    jump_destination is either absolute offset or None if instruction is not
    jump. Pre/postconditions are as in ValidateRegularInstructions.
  Raises:
    According to usual convention.
  """
  assert bitness in [32, 64]
  try:
    destination = ValidateDirectJump(instruction, bitness)
    return destination, Condition(), Condition()
  except DoNotMatchError:
    pass

  precondition, postcondition = ValidateRegularInstruction(instruction, bitness)
  return None, precondition, postcondition


def ValidateSuperinstruction32(superinstruction):
  """Validate superinstruction with ia32 set of regexps.

  If set of instructions includes something unknown (unknown functions
  or prefixes, wrong number of instructions, etc), then assert is triggered.

  There corner case exist: naclcall/nacljmp instruction sequences are too
  complex to process by DFA alone (it produces too large DFA and MSVC chokes
  on it) thus it's verified partially by DFA and partially by code in
  actions.  For these we generate either "True" or "False".

  Args:
      superinstruction: list of objdump_parser.Instruction tuples
  """

  call_jmp = re.compile(
      r'(call|jmp) '  # call or jmp
      r'[*](?P<register>%e[a-z]+)$')  # register name

  # TODO(shcherbina): actually we only want to allow 0xffffffe0 as a mask,
  # but it's safe anyway because what really matters is that lower 5 bits
  # of the mask are zeroes.
  # Disallow 0xe0 once
  # https://code.google.com/p/nativeclient/issues/detail?id=3164 is fixed.
  and_for_call_jmp = re.compile(
      r'and [$]0x(ffffff)?e0,(?P<register>%e[a-z]+)$')

  dangerous_instruction = superinstruction[-1].disasm

  if call_jmp.match(dangerous_instruction):
    # If "dangerous instruction" is call or jmp then we need to check if two
    # lines match

    if len(superinstruction) != 2:
      raise DoNotMatchError(superinstruction)

    m = and_for_call_jmp.match(superinstruction[0].disasm)
    if m is None:
      raise DoNotMatchError(superinstruction)
    register_and = m.group('register')

    m = call_jmp.match(dangerous_instruction)
    if m is None:
      raise DoNotMatchError(superinstruction)
    register_call_jmp = m.group('register')

    if register_and == register_call_jmp:
      for instruction in superinstruction:
        _GetLegacyPrefixes(instruction)  # to detect repeated prefixes
      return

    raise SandboxingError(
        'nacljump32/naclcall32: {0} != {1}'.format(
            register_and, register_call_jmp),
        superinstruction)

  raise DoNotMatchError(superinstruction)


def ValidateSuperinstruction64(superinstruction):
  """Validate superinstruction with x86-64 set of regexps.

  If set of instructions includes something unknown (unknown functions
  or prefixes, wrong number of instructions, etc), then assert is triggered.

  There corner case exist: naclcall/nacljmp instruction sequences are too
  complex to process by DFA alone (it produces too large DFA and MSVC chokes
  on it) thus it's verified partially by DFA and partially by code in
  actions.  For these we generate either "True" or "False", other
  superinstruction always produce "True" or throw an error.

  Args:
      superinstruction: list of objdump_parser.Instruction tuples
  """

  dangerous_instruction = superinstruction[-1].disasm

  # This is dangerous instructions in naclcall/nacljmp
  callq_jmpq = re.compile(
      r'(callq|jmpq) ' # callq or jmpq
      r'[*](?P<register>%r[0-9a-z]+)$') # register name
  # These are sandboxing instructions for naclcall/nacljmp
  # TODO(shcherbina): actually we only want to allow 0xffffffe0 as a mask,
  # but it's safe anyway because what really matters is that lower 5 bits
  # of the mask are zeroes.
  # Disallow 0xe0 once
  # https://code.google.com/p/nativeclient/issues/detail?id=3164 is fixed.
  and_for_callq_jmpq = re.compile(
      r'and [$]0x(f)*e0,(?P<register>%e[a-z][a-z]|%r[89]d|%r1[0-4]d)$')
  add_for_callq_jmpq = re.compile(
      r'add %r15,(?P<register>%r[0-9a-z]+)$')

  if callq_jmpq.match(dangerous_instruction):
    # If "dangerous instruction" is callq or jmpq then we need to check if all
    # three lines match

    if len(superinstruction) != 3:
      raise DoNotMatchError(superinstruction)

    m = and_for_callq_jmpq.match(superinstruction[0].disasm)
    if m is None:
      raise DoNotMatchError(superinstruction)
    register_and = m.group('register')

    m = add_for_callq_jmpq.match(superinstruction[1].disasm)
    if m is None:
      raise DoNotMatchError(superinstruction)
    register_add = m.group('register')

    m = callq_jmpq.match(dangerous_instruction)
    if m is None:
      raise DoNotMatchError(superinstruction)
    register_callq_jmpq = m.group('register')

    # Double-check that registers are 32-bit and convert them to 64-bit so
    # they can be compared
    if register_and[1] == 'e':
      register_and = '%r' + register_and[2:]
    elif re.match(r'%r\d+d', register_and):
      register_and = register_and[:-1]
    else:
      assert False, ('Unknown (or possible non-32-bit) register found. '
                     'This should never happen!')
    if register_and == register_add == register_callq_jmpq:
      for instruction in superinstruction:
        _GetLegacyPrefixes(instruction)  # to detect repeated prefixes
      return

    raise SandboxingError(
        'nacljump64/naclcall64: registers do not match ({0}, {1}, {2})'.format(
            register_and, register_add, register_callq_jmpq),
        superinstruction)

    raise DoNotMatchError(superinstruction)

  # These are dangerous string instructions (there are three cases)
  string_instruction_rdi_no_rsi = re.compile(
      r'(maskmovq %mm[0-7],%mm[0-7]|' # maskmovq
      r'v?maskmovdqu %xmm([0-9]|1[0-5]),%xmm([0-9]|1[0-5])|' # [v]maskmovdqu
      r'((repnz|repz) )?scas %es:[(]%rdi[)],(%al|%ax|%eax|%rax)|' # scas
      r'(rep )?stos (%al|%ax|%eax|%rax),%es:[(]%rdi[)])$') # stos
  string_instruction_rsi_no_rdi = re.compile(
      r'(rep )?lods %ds:[(]%rsi[)],(%al|%ax|%eax|%rax)$') # lods
  string_instruction_rsi_rdi = re.compile(
      r'(((repnz|repz) )?cmps[blqw] %es:[(]%rdi[)],%ds:[(]%rsi[)]|' # cmps
      r'(rep )?movs[blqw] %ds:[(]%rsi[)],%es:[(]%rdi[)])$') # movs
  # These are sandboxing instructions for string instructions
  mov_esi_esi = re.compile(r'mov %esi,%esi$')
  lea_r15_rsi_rsi = re.compile(r'lea [(]%r15,%rsi,1[)],%rsi$')
  mov_edi_edi = re.compile(r'mov %edi,%edi$')
  lea_r15_rdi_rdi = re.compile(r'lea [(]%r15,%rdi,1[)],%rdi$')

  if string_instruction_rsi_no_rdi.match(dangerous_instruction):
    if len(superinstruction) != 3:
      raise DoNotMatchError(superinstruction)
    if mov_esi_esi.match(superinstruction[0].disasm) is None:
      raise DoNotMatchError(superinstruction)
    if lea_r15_rsi_rsi.match(superinstruction[1].disasm) is None:
      raise DoNotMatchError(superinstruction)

  elif string_instruction_rdi_no_rsi.match(dangerous_instruction):
    if len(superinstruction) != 3:
      raise DoNotMatchError(superinstruction)
    if mov_edi_edi.match(superinstruction[0].disasm) is None:
      raise DoNotMatchError(superinstruction)
    if lea_r15_rdi_rdi.match(superinstruction[1].disasm) is None:
      raise DoNotMatchError(superinstruction)

  elif string_instruction_rsi_rdi.match(dangerous_instruction):
    if len(superinstruction) != 5:
      raise DoNotMatchError(superinstruction)
    if mov_esi_esi.match(superinstruction[0].disasm) is None:
      raise DoNotMatchError(superinstruction)
    if lea_r15_rsi_rsi.match(superinstruction[1].disasm) is None:
      raise DoNotMatchError(superinstruction)
    if mov_edi_edi.match(superinstruction[2].disasm) is None:
      raise DoNotMatchError(superinstruction)
    if lea_r15_rdi_rdi.match(superinstruction[3].disasm) is None:
      raise DoNotMatchError(superinstruction)

  else:
    raise DoNotMatchError(superinstruction)

  for instruction in superinstruction:
    _GetLegacyPrefixes(instruction)  # to detect repeated prefixes
