# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import operator
import os
import re
import struct
import subprocess
import sys
import unittest
import xml.etree.ElementTree

import gdb_rsp


if sys.platform == 'win32':
  RETURNCODE_KILL = -9
else:
  RETURNCODE_KILL = -9 & 0xff


NACL_SIGILL = 4
NACL_SIGTRAP = 5
NACL_SIGSEGV = 11

# GDB specific errno constants.
GDB_EBADF = 9


# These are set up by Main().
ARCH = None
NM_TOOL = None
SEL_LDR_COMMAND = None


def AssertEquals(x, y):
  if x != y:
    raise AssertionError('%r != %r' % (x, y))


def DecodeHex(data):
  assert len(data) % 2 == 0, data
  return ''.join([chr(int(data[index * 2 : (index + 1) * 2], 16))
                  for index in xrange(len(data) / 2)])


def EncodeHex(data):
  return ''.join('%02x' % ord(byte) for byte in data)


def DecodeEscaping(data):
  ret = ''
  last = None
  repeat = False
  escape = False
  for byte in data:
    if escape:
      last = chr(ord(byte) ^ 0x20)
      ret += last
      escape = False
    elif repeat:
      count = ord(byte) - 29
      assert count >= 3 and count <= 97
      assert byte != '$' and byte != '#'
      ret += last * count
      repeat = False
    elif byte == '}':
      escape = True
    elif byte == '*':
      assert last is not None
      repeat = True
    else:
      ret += byte
      last = byte
  return ret


X86_32_REG_DEFS = [
    ('eax', 'I'),
    ('ecx', 'I'),
    ('edx', 'I'),
    ('ebx', 'I'),
    ('esp', 'I'),
    ('ebp', 'I'),
    ('esi', 'I'),
    ('edi', 'I'),
    ('eip', 'I'),
    ('eflags', 'I'),
    ('cs', 'I'),
    ('ss', 'I'),
    ('ds', 'I'),
    ('es', 'I'),
    ('fs', 'I'),
    ('gs', 'I'),
]


X86_64_REG_DEFS = [
    ('rax', 'Q'),
    ('rbx', 'Q'),
    ('rcx', 'Q'),
    ('rdx', 'Q'),
    ('rsi', 'Q'),
    ('rdi', 'Q'),
    ('rbp', 'Q'),
    ('rsp', 'Q'),
    ('r8', 'Q'),
    ('r9', 'Q'),
    ('r10', 'Q'),
    ('r11', 'Q'),
    ('r12', 'Q'),
    ('r13', 'Q'),
    ('r14', 'Q'),
    ('r15', 'Q'),
    ('rip', 'Q'),
    ('eflags', 'I'),
    ('cs', 'I'),
    ('ss', 'I'),
    ('ds', 'I'),
    ('es', 'I'),
    ('fs', 'I'),
    ('gs', 'I'),
]


ARM_REG_DEFS = ([('r%d' % regno, 'I') for regno in xrange(16)]
                + [('cpsr', 'I')])


MIPS_REG_DEFS = [
     ('zero', 'I'),
     ('at', 'I'),
     ('v0', 'I'),
     ('v1', 'I'),
     ('a0', 'I'),
     ('a1', 'I'),
     ('a2', 'I'),
     ('a3', 'I'),
     ('t0', 'I'),
     ('t1', 'I'),
     ('t2', 'I'),
     ('t3', 'I'),
     ('t4', 'I'),
     ('t5', 'I'),
     ('t6', 'I'),
     ('t7', 'I'),
     ('s0', 'I'),
     ('s1', 'I'),
     ('s2', 'I'),
     ('s3', 'I'),
     ('s4', 'I'),
     ('s5', 'I'),
     ('s6', 'I'),
     ('s7', 'I'),
     ('t8', 'I'),
     ('t9', 'I'),
     ('k0', 'I'),
     ('k1', 'I'),
     ('global_ptr', 'I'),
     ('stack_ptr', 'I'),
     ('frame_ptr', 'I'),
     ('return_addr', 'I'),
     ('prog_ctr', 'I'),
]


REG_DEFS = {
    'x86-32': X86_32_REG_DEFS,
    'x86-64': X86_64_REG_DEFS,
    'arm': ARM_REG_DEFS,
    'mips32': MIPS_REG_DEFS,
    }


SP_REG = {
    'x86-32': 'esp',
    'x86-64': 'rsp',
    'arm': 'r13',
    'mips32': 'stack_ptr',
    }


IP_REG = {
    'x86-32': 'eip',
    'x86-64': 'rip',
    'arm': 'r15',
    'mips32': 'prog_ctr',
    }


X86_TRAP_FLAG = 1 << 8

# RESET_X86_FLAGS_VALUE is what ASM_WITH_REGS() resets the x86 flags
# to.  Copied from tests/common/register_set.h.
RESET_X86_FLAGS_VALUE = (1 << 2) | (1 << 6)
KNOWN_X86_FLAGS_MASK = (1<<0) | (1<<2) | (1<<6) | (1<<7) | (1<<11) | (1<<8)

# These are the only ARM CPSR bits that user code and untrusted code
# can read and modify, excluding the IT bits which are for Thumb-2
# (for If-Then-Else instructions).  Copied from
# tests/common/register_set.h.
ARM_USER_CPSR_FLAGS_MASK = (
    (1<<31) | # N
    (1<<30) | # Z
    (1<<29) | # C
    (1<<28) | # V
    (1<<27) | # Q
    (1<<19) | (1<<18) | (1<<17) | (1<<16)) # GE bits


def UsingQemu():
  return os.path.basename(SEL_LDR_COMMAND[0]).startswith('run_under_qemu_')


def MainNexe():
  # Assume that the last parameter ending in .nexe is the main nexe.
  for arg in reversed(SEL_LDR_COMMAND):
    if arg.endswith('.nexe'):
      return arg
  assert False


def IrtNexe():
  # Assume that the irt starts with 'irt' and ends with '.nexe'.
  for arg in reversed(SEL_LDR_COMMAND):
    basename = os.path.basename(arg)
    if basename.startswith('irt_') and basename.endswith('.nexe'):
      return arg
  return None


def DecodeRegs(reply):
  defs = REG_DEFS[ARCH]
  names = [reg_name for reg_name, reg_fmt in defs]
  fmt = ''.join([reg_fmt for reg_name, reg_fmt in defs])

  values = struct.unpack_from(fmt, DecodeHex(reply))
  return dict(zip(names, values))


def EncodeRegs(regs):
  defs = REG_DEFS[ARCH]
  names = [reg_name for reg_name, reg_fmt in defs]
  fmt = ''.join([reg_fmt for reg_name, reg_fmt in defs])

  values = [regs[r] for r in names]
  return EncodeHex(struct.pack(fmt, *values))


def PopenDebugStub(test):
  gdb_rsp.EnsurePortIsAvailable()
  return subprocess.Popen(SEL_LDR_COMMAND + ['-g', test])


def KillProcess(process):
  if process.returncode is not None:
    # kill() won't work if we've already wait()'ed on the process.
    return
  try:
    process.kill()
  except OSError:
    if sys.platform == 'win32':
      # If process is already terminated, kill() throws
      # "WindowsError: [Error 5] Access is denied" on Windows.
      pass
    else:
      raise
  process.wait()


def ChangeReg(conn, name, func):
  regs = DecodeRegs(conn.RspRequest('g'))
  regs[name] = func(regs[name])
  return conn.RspRequest('G' + EncodeRegs(regs))


def SkipBreakpoint(connection, stop_reply):
  # Skip past the faulting instruction in debugger_test.c's
  # breakpoint() function.
  regs = DecodeRegs(connection.RspRequest('g'))
  if ARCH in ('x86-32', 'x86-64'):
    AssertReplySignal(stop_reply, NACL_SIGSEGV)
    # Skip past the single-byte HLT instruction.
    regs[IP_REG[ARCH]] += 1
  elif ARCH == 'arm':
    AssertReplySignal(stop_reply, NACL_SIGILL)
    bundle_size = 16
    assert regs['r15'] % bundle_size == 0, regs['r15']
    regs['r15'] += bundle_size
  elif ARCH == 'mips32':
    AssertReplySignal(stop_reply, NACL_SIGTRAP)
    bundle_size = 16
    assert regs['prog_ctr'] % bundle_size == 0, regs['prog_ctr']
    regs['prog_ctr'] += bundle_size
  else:
    raise AssertionError('Unknown architecture')
  AssertEquals(connection.RspRequest('G' + EncodeRegs(regs)), 'OK')


class LaunchDebugStub(object):

  def __init__(self, test):
    self._proc = PopenDebugStub(test)

  def __enter__(self):
    try:
      return gdb_rsp.GdbRspConnection()
    except:
      KillProcess(self._proc)
      raise

  def __exit__(self, exc_type, exc_value, traceback):
    KillProcess(self._proc)


def GetSymbols():
  assert '-f' in SEL_LDR_COMMAND
  nexe_filename = SEL_LDR_COMMAND[SEL_LDR_COMMAND.index('-f') + 1]
  symbols = {}
  proc = subprocess.Popen([NM_TOOL, '--format=posix', nexe_filename],
                          stdout=subprocess.PIPE)
  for line in proc.stdout:
    match = re.match('(\S+) [TtWwBbDd] ([0-9a-fA-F]+)', line)
    if match is not None:
      name = match.group(1)
      addr = int(match.group(2), 16)
      symbols[name] = addr
  result = proc.wait()
  assert result == 0, result
  return symbols


def ParseThreadStopReply(reply):
  match = re.match('T([0-9a-f]{2})thread:([0-9a-f]+);$', reply)
  if match is None:
    raise AssertionError('Bad thread stop reply: %r' % reply)
  return {'signal': int(match.group(1), 16),
          'thread_id': int(match.group(2), 16)}


def AssertReplySignal(reply, signal):
  AssertEquals(ParseThreadStopReply(reply)['signal'], signal)


def ReadMemory(connection, address, size):
  reply = connection.RspRequest('m%x,%x' % (address, size))
  assert not reply.startswith('E'), reply
  return DecodeHex(reply)


def ReadUint32(connection, address):
  return struct.unpack('I', ReadMemory(connection, address, 4))[0]


def SingleSteppingWorks():
  # Single-stepping is not yet supported on ARM and MIPS.
  # TODO(eaeltsin):
  #   http://code.google.com/p/nativeclient/issues/detail?id=2911
  return ARCH in ('x86-32', 'x86-64')


class DebugStubTest(unittest.TestCase):

  def test_initial_breakpoint(self):
    # Any arguments to the nexe would work here because we are only
    # testing that we get a breakpoint at the _start entry point.
    with LaunchDebugStub('test_getting_registers') as connection:
      reply = connection.RspRequest('?')
      AssertReplySignal(reply, NACL_SIGTRAP)

  def CheckTargetXml(self, connection):
    reply = connection.RspRequest('qXfer:features:read:target.xml:0,fff')
    self.assertEquals(reply[0], 'l')
    # Just check that we are given parsable XML.
    xml.etree.ElementTree.fromstring(reply[1:])

  # Test that we can fetch register values.
  # This check corresponds to the last instruction of debugger_test.c
  def CheckReadRegisters(self, connection):
    registers = DecodeRegs(connection.RspRequest('g'))
    if ARCH == 'x86-32':
      self.assertEquals(registers['eax'], 0x11000022)
      self.assertEquals(registers['ebx'], 0x22000033)
      self.assertEquals(registers['ecx'], 0x33000044)
      self.assertEquals(registers['edx'], 0x44000055)
      self.assertEquals(registers['esi'], 0x55000066)
      self.assertEquals(registers['edi'], 0x66000077)
      self.assertEquals(registers['ebp'], 0x77000088)
      self.assertEquals(registers['esp'], 0x88000099)
      self.assertEquals(registers['eflags'] & KNOWN_X86_FLAGS_MASK,
                        RESET_X86_FLAGS_VALUE)
      self.assertEquals(registers['cs'], 0x00000000)
      self.assertEquals(registers['ds'], 0x00000000)
      self.assertEquals(registers['es'], 0x00000000)
      self.assertEquals(registers['fs'], 0x00000000)
      self.assertEquals(registers['gs'], 0x00000000)
      self.assertEquals(registers['ss'], 0x00000000)
    elif ARCH == 'x86-64':
      self.assertEquals(registers['rax'], 0x1100000000000022)
      self.assertEquals(registers['rbx'], 0x2200000000000033)
      self.assertEquals(registers['rcx'], 0x3300000000000044)
      self.assertEquals(registers['rdx'], 0x4400000000000055)
      self.assertEquals(registers['rsi'], 0x5500000000000066)
      self.assertEquals(registers['rdi'], 0x6600000000000077)
      self.assertEquals(registers['r8'],  0x7700000000000088)
      self.assertEquals(registers['r9'],  0x8800000000000099)
      self.assertEquals(registers['r10'], 0x99000000000000aa)
      self.assertEquals(registers['r11'], 0xaa000000000000bb)
      self.assertEquals(registers['r12'], 0xbb000000000000cc)
      self.assertEquals(registers['r13'], 0xcc000000000000dd)
      self.assertEquals(registers['r14'], 0xdd000000000000ee)
      self.assertEquals(registers['rsp'], registers['r15'] + 0x12300321)
      self.assertEquals(registers['rbp'], registers['r15'] + 0x23400432)
      self.assertEquals(registers['eflags'] & KNOWN_X86_FLAGS_MASK,
                        RESET_X86_FLAGS_VALUE)
      self.assertEquals(registers['cs'], 0x00000000)
      self.assertEquals(registers['ds'], 0x00000000)
      self.assertEquals(registers['es'], 0x00000000)
      self.assertEquals(registers['fs'], 0x00000000)
      self.assertEquals(registers['gs'], 0x00000000)
      self.assertEquals(registers['ss'], 0x00000000)
    elif ARCH == 'arm':
      self.assertEquals(registers['r0'], 0x00000001)
      self.assertEquals(registers['r1'], 0x10000002)
      self.assertEquals(registers['r2'], 0x20000003)
      self.assertEquals(registers['r3'], 0x30000004)
      self.assertEquals(registers['r4'], 0x40000005)
      self.assertEquals(registers['r5'], 0x50000006)
      self.assertEquals(registers['r6'], 0x60000007)
      self.assertEquals(registers['r7'], 0x70000008)
      self.assertEquals(registers['r8'], 0x80000009)
      self.assertEquals(registers['r9'], 0x00000000)
      self.assertEquals(registers['r10'], 0xa000000b)
      self.assertEquals(registers['r11'], 0xb000000c)
      self.assertEquals(registers['r12'], 0xc000000d)
      # Ensure sp is masked.
      self.assertEquals(registers['r13'], 0x12345678)
      self.assertEquals(registers['r14'], 0xe000000f)
      # Only the upper 4 bits of cspr are visible.
      self.assertEquals(registers['cpsr'] & ARM_USER_CPSR_FLAGS_MASK,
                        (1 << 29))
    elif ARCH == 'mips32':
      # We skip zero register because it cannot be set.
      self.assertEquals(registers['at'], 0x11000220)
      self.assertEquals(registers['v0'], 0x22000330)
      self.assertEquals(registers['v1'], 0x33000440)
      self.assertEquals(registers['a0'], 0x44000550)
      self.assertEquals(registers['a1'], 0x55000660)
      self.assertEquals(registers['a2'], 0x66000770)
      self.assertEquals(registers['a3'], 0x77000880)
      self.assertEquals(registers['t0'], 0x88000990)
      self.assertEquals(registers['t1'], 0x99000aa0)
      self.assertEquals(registers['t2'], 0xaa000bb0)
      self.assertEquals(registers['t3'], 0xbb000cc0)
      self.assertEquals(registers['t4'], 0xcc000dd0)
      self.assertEquals(registers['t5'], 0xdd000ee0)
      self.assertEquals(registers['t6'], 0x0ffffff0)
      self.assertEquals(registers['t7'], 0x3fffffff)
      # Skip t8 because it cannot be set by untrusted code.
      self.assertEquals(registers['s0'], 0x11100222)
      self.assertEquals(registers['s1'], 0x22200333)
      self.assertEquals(registers['s2'], 0x33300444)
      self.assertEquals(registers['s3'], 0x44400555)
      self.assertEquals(registers['s4'], 0x55500666)
      self.assertEquals(registers['s5'], 0x66600777)
      self.assertEquals(registers['s6'], 0x77700888)
      self.assertEquals(registers['s7'], 0x88800999)
      self.assertEquals(registers['t9'], 0xaaa00bbb)
      # Skip k0 and k1 registers, since they can be changed by kernel.
      self.assertEquals(registers['global_ptr'],  0xddd00eee)
      self.assertEquals(registers['stack_ptr'],   0x2ee00fff)
      self.assertEquals(registers['frame_ptr'],   0xfff00000)
      self.assertEquals(registers['return_addr'], 0x0a0a0a0a)
    else:
      raise AssertionError('Unknown architecture')

    expected_fault_addr = GetSymbols()['fault_addr']
    if ARCH == 'x86-64':
      expected_fault_addr += registers['r15']
    self.assertEquals(registers[IP_REG[ARCH]], expected_fault_addr)

  # Test that we can write registers.
  def CheckWriteRegisters(self, connection):
    if ARCH == 'x86-32':
      reg_name = 'edx'
    elif ARCH == 'x86-64':
      reg_name = 'rdx'
    elif ARCH == 'arm':
      reg_name = 'r0'
    elif ARCH == 'mips32':
      reg_name = 'a0'
    else:
      raise AssertionError('Unknown architecture')

    # Read registers.
    regs = DecodeRegs(connection.RspRequest('g'))

    # Change a register.
    regs[reg_name] += 1
    new_value = regs[reg_name]

    # Write registers.
    self.assertEquals(connection.RspRequest('G' + EncodeRegs(regs)), 'OK')

    # Read registers. Check for a new value.
    regs = DecodeRegs(connection.RspRequest('g'))
    self.assertEquals(regs[reg_name], new_value)

    # TODO: Resume execution and check that changing the registers really
    # influenced the program's execution. This would require changing
    # debugger_test.c.

  def CheckReadOnlyRegisters(self, connection):
    if ARCH == 'x86-32':
      read_only_regs = []
      read_only_zero_regs = ['cs', 'ds', 'es', 'fs', 'gs', 'ss']
    elif ARCH == 'x86-64':
      read_only_regs = ['r15']
      read_only_zero_regs = ['cs', 'ds', 'es', 'fs', 'gs', 'ss']
    elif ARCH == 'arm':
      read_only_regs = []
      read_only_zero_regs = ['r9']
    elif ARCH == 'mips32':
      read_only_regs = ['zero', 'k0', 'k1']
      read_only_zero_regs = []
    else:
      raise AssertionError('Unknown architecture')

    for reg_name in read_only_regs + read_only_zero_regs:
      # Read registers.
      regs = DecodeRegs(connection.RspRequest('g'))

      # Change a register.
      old_value = regs[reg_name]
      if reg_name in read_only_zero_regs:
        self.assertEquals(old_value, 0)
      regs[reg_name] += 1

      # Check cannot write registers.
      self.assertEquals(connection.RspRequest('G' + EncodeRegs(regs)), 'E03')

      # Read registers. Check for an old value.
      regs = DecodeRegs(connection.RspRequest('g'))
      self.assertEquals(regs[reg_name], old_value)

  # Test that reading from an unreadable address gives a sensible error.
  def CheckReadMemoryAtInvalidAddr(self, connection):
    mem_addr = 0
    result = connection.RspRequest('m%x,%x' % (mem_addr, 8))
    self.assertEquals(result, 'E03')

    # Check non-zero address in the first page.
    mem_addr = 0xffff
    resut = connection.RspRequest('m%x,%x' % (mem_addr, 1))
    self.assertEquals(result, 'E03')

  # Run tests on debugger_test.c binary.
  def test_debugger_test(self):
    with LaunchDebugStub('test_getting_registers') as connection:
      # Tell the process to continue, because it starts at the
      # breakpoint set at its start address.
      reply = connection.RspRequest('c')
      if ARCH == 'arm':
        AssertReplySignal(reply, NACL_SIGILL)
      elif ARCH == 'mips32':
        # The process should have stopped on a BKPT instruction.
        AssertReplySignal(reply, NACL_SIGTRAP)
      else:
        # The process should have stopped on a HLT instruction.
        AssertReplySignal(reply, NACL_SIGSEGV)

      self.CheckTargetXml(connection)
      self.CheckReadRegisters(connection)
      self.CheckWriteRegisters(connection)
      self.CheckReadOnlyRegisters(connection)

  def test_jump_to_address_zero(self):
    if UsingQemu():
      # This test hangs under qemu-arm or qemu-mips.
      return
    with LaunchDebugStub('test_jump_to_address_zero') as connection:
      # Continue from initial breakpoint.
      reply = connection.RspRequest('c')
      AssertReplySignal(reply, NACL_SIGSEGV)
      registers = DecodeRegs(connection.RspRequest('g'))
      if ARCH == 'x86-64':
        self.assertEquals(registers[IP_REG[ARCH]], registers['r15'])
      else:
        self.assertEquals(registers[IP_REG[ARCH]], 0)

  def test_reading_and_writing_memory(self):
    # Any arguments to the nexe would work here because we do not run
    # the executable beyond the initial breakpoint.
    with LaunchDebugStub('test_getting_registers') as connection:
      mem_addr = GetSymbols()['g_example_var']
      # Check reading memory.
      expected_data = 'some_debug_stub_test_data\0'
      reply = connection.RspRequest('m%x,%x' % (mem_addr, len(expected_data)))
      self.assertEquals(DecodeHex(reply), expected_data)

      # On x86-64, for reading/writing memory, the debug stub accepts
      # untrusted addresses with or without the %r15 sandbox base
      # address added, because GDB uses both.
      # TODO(eaeltsin): Fix GDB to not use addresses with %r15 added,
      # and change the expected result in the check below.
      if ARCH == 'x86-64':
        registers = DecodeRegs(connection.RspRequest('g'))
        sandbox_base_addr = registers['r15']
        reply = connection.RspRequest('m%x,%x' % (sandbox_base_addr + mem_addr,
                                                  len(expected_data)))
        self.assertEquals(DecodeHex(reply), expected_data)

      # Check writing memory.
      new_data = 'replacement_data\0'
      assert len(new_data) < len(expected_data)
      reply = connection.RspRequest('M%x,%x:%s' % (mem_addr, len(new_data),
                                                   EncodeHex(new_data)))
      self.assertEquals(reply, 'OK')
      # Check that we can read back what we wrote.
      reply = connection.RspRequest('m%x,%x' % (mem_addr, len(new_data)))
      self.assertEquals(DecodeHex(reply), new_data)

      self.CheckReadMemoryAtInvalidAddr(connection)

  def test_exit_code(self):
    with LaunchDebugStub('test_exit_code') as connection:
      reply = connection.RspRequest('c')
      self.assertEquals(reply, 'W02')

  # Single-step and check IP corresponds to debugger_test:test_single_step
  def CheckSingleStep(self, connection, step_command, thread_id):
    if ARCH == 'x86-32':
      instruction_sizes = [1, 2, 3, 6]
    elif ARCH == 'x86-64':
      instruction_sizes = [1, 3, 4, 6]
    else:
      raise AssertionError('Unknown architecture')

    ip = DecodeRegs(connection.RspRequest('g'))[IP_REG[ARCH]]

    for size in instruction_sizes:
      reply = connection.RspRequest(step_command)
      AssertReplySignal(reply, NACL_SIGTRAP)
      self.assertEquals(ParseThreadStopReply(reply)['thread_id'], thread_id)
      ip += size
      regs = DecodeRegs(connection.RspRequest('g'))
      self.assertEqual(regs[IP_REG[ARCH]], ip)
      # The trap flag should be reported as unset.
      self.assertEqual(regs['eflags'] & X86_TRAP_FLAG, 0)

  def test_single_step(self):
    if not SingleSteppingWorks():
      return
    with LaunchDebugStub('test_single_step') as connection:
      # We expect test_single_step() to stop at a HLT instruction.
      reply = connection.RspRequest('c')
      AssertReplySignal(reply, NACL_SIGSEGV)
      tid = ParseThreadStopReply(reply)['thread_id']
      # Skip past the single-byte HLT instruction.
      regs = DecodeRegs(connection.RspRequest('g'))
      regs[IP_REG[ARCH]] += 1
      AssertEquals(connection.RspRequest('G' + EncodeRegs(regs)), 'OK')

      self.CheckSingleStep(connection, 's', tid)
      # Check that we can continue after single-stepping.
      reply = connection.RspRequest('c')
      self.assertEquals(reply, 'W00')

  def test_vCont(self):
    # Basically repeat test_single_step, but using vCont commands.
    if not SingleSteppingWorks():
      return
    with LaunchDebugStub('test_single_step') as connection:
      # Test if vCont is supported.
      reply = connection.RspRequest('vCont?')
      self.assertEqual(reply, 'vCont;s;S;c;C')

      # Continue using vCont.
      # We expect test_single_step() to stop at a HLT instruction.
      reply = connection.RspRequest('vCont;c')
      AssertReplySignal(reply, NACL_SIGSEGV)
      # Get signalled thread id.
      tid = ParseThreadStopReply(reply)['thread_id']
      # Skip past the single-byte HLT instruction.
      regs = DecodeRegs(connection.RspRequest('g'))
      regs[IP_REG[ARCH]] += 1
      AssertEquals(connection.RspRequest('G' + EncodeRegs(regs)), 'OK')

      self.CheckSingleStep(connection, 'vCont;s:%x' % tid, tid)

      # Single step one thread and continue all others.
      reply = connection.RspRequest('vCont;s:%x;c' % tid)
      # WARNING! This check is valid in single-threaded case only!
      # In multi-threaded case another thread might stop first.
      self.assertEqual(reply, 'T05thread:%x;' % tid)

      # Try to continue the thread and to single-step all others.
      reply = connection.RspRequest('vCont;c:%x;s' % tid)
      self.assertTrue(reply.startswith('E'))

      # Try to single-step wrong thread.
      reply = connection.RspRequest('vCont;s:%x' % (tid + 2))
      self.assertTrue(reply.startswith('E'))

      # Try to single-step all threads.
      reply = connection.RspRequest('vCont;s')
      self.assertTrue(reply.startswith('E'))

  def test_interrupt(self):
    if not SingleSteppingWorks():
      return
    func_addr = GetSymbols()['test_interrupt']
    with LaunchDebugStub('test_interrupt') as connection:
      # Single stepping inside syscalls doesn't work. So we need to reach
      # a point where interrupt will not catch the program inside syscall.
      reply = connection.RspRequest('Z0,%x,0' % func_addr)
      self.assertEquals(reply, 'OK')
      reply = connection.RspRequest('c')
      AssertReplySignal(reply, NACL_SIGTRAP)
      reply = connection.RspRequest('z0,%x,0' % func_addr)
      self.assertEquals(reply, 'OK')

      # Continue (program will spin forever), then interrupt.
      connection.RspSendOnly('c')
      reply = connection.RspInterrupt()
      self.assertEqual(reply, 'T00')

      # Single-step.
      reply = connection.RspRequest('s')
      AssertReplySignal(reply, NACL_SIGTRAP)

  def test_modifying_code_is_disallowed(self):
    with LaunchDebugStub('test_setting_breakpoint') as connection:
      # Pick an arbitrary address in the code segment.
      func_addr = GetSymbols()['breakpoint_target_func']
      # Writing to the code area should be disallowed.
      data = '\x00'
      write_command = 'M%x,%x:%s' % (func_addr, len(data), EncodeHex(data))
      reply = connection.RspRequest(write_command)
      self.assertEquals(reply, 'E03')

  def test_kill(self):
    sel_ldr = PopenDebugStub('test_exit_code')
    try:
      connection = gdb_rsp.GdbRspConnection()
      # Request killing the target.
      reply = connection.RspRequest('k')
      self.assertEquals(reply, 'OK')
      self.assertEquals(sel_ldr.wait(), RETURNCODE_KILL)
    finally:
      KillProcess(sel_ldr)

  def test_detach(self):
    sel_ldr = PopenDebugStub('test_exit_code')
    try:
      connection = gdb_rsp.GdbRspConnection()
      # Request detaching from the target.
      # This resumes execution, so we get the nexe's normal exit() status.
      reply = connection.RspRequest('D')
      self.assertEquals(reply, 'OK')
      self.assertEquals(sel_ldr.wait(), 2)
    finally:
      KillProcess(sel_ldr)

  def test_disconnect(self):
    sel_ldr = PopenDebugStub('test_exit_code')
    try:
      # Connect and record the instruction pointer.
      connection = gdb_rsp.GdbRspConnection()
      # Check something basic responds with sane results.
      reply = connection.RspRequest('vCont?')
      self.assertEqual(reply, 'vCont;s;S;c;C')
      # Store the instruction pointer.
      registers = DecodeRegs(connection.RspRequest('g'))
      initial_ip = registers[IP_REG[ARCH]]
      connection.Close()
      # Reconnect 5 times.
      for _ in range(5):
        connection = gdb_rsp.GdbRspConnection()
        # Confirm the instruction pointer stays where it was, indicating that
        # the thread stayed suspended.
        registers = DecodeRegs(connection.RspRequest('g'))
        self.assertEquals(registers[IP_REG[ARCH]], initial_ip)
        connection.Close()
    finally:
      KillProcess(sel_ldr)

  def RemoteGet(self, connection, download_filename, expected_filename):
    """Use the vFile interface to remote get a file, checking the result.

    Args:
      connection: Rsp connection to debug stub to use.
      download_filename: Remote filename to download.
      expected_filename: Local filename to check downloaded result against.
    """
    # Open the nexe.
    reply = connection.RspRequest(
        'vFile:open:%s,0,0' % EncodeHex(download_filename))
    self.assertEqual(reply[0], 'F')
    fd = int(reply[1:], 16)
    self.assertGreaterEqual(fd, 0)
    # Read in the full contents of the file.
    data = ''
    offset = 0
    while True:
      # Read up to 4096 bytes at a time.
      reply = connection.RspRequest(
        'vFile:pread:%x,%x,%x' % (fd, 4096, offset))
      self.assertEqual(reply[0], 'F')
      retcode, chunk = reply[1:].split(';', 1)
      retcode = int(retcode, 16)
      self.assertGreaterEqual(retcode, 0)
      chunk = DecodeEscaping(chunk)
      self.assertEqual(len(chunk), retcode)
      if retcode == 0:
        break
      data += chunk
      offset += retcode
    expected_data = open(expected_filename, 'rb').read()
    # Check that the length matches first, so that large data mismatch spew
    # is only emitted in the case of more subtle mismatches.
    self.assertEqual(len(data), len(expected_data))
    self.assertEqual(data, expected_data)
    # Close the file handle.
    reply = connection.RspRequest('vFile:close:%x' % fd)
    self.assertEqual(reply, 'F0')

  def test_remote_get_main_nexe(self):
    with LaunchDebugStub('test_interrupt') as connection:
      self.RemoteGet(connection, 'nexe', MainNexe())

  def test_remote_get_irt(self):
    if IrtNexe() is None:
      self.skipTest('Does not work in non-irt mode.')
    with LaunchDebugStub('test_interrupt') as connection:
      self.RemoteGet(connection, 'irt', IrtNexe())

  def test_remote_get_bad_fd(self):
    with LaunchDebugStub('test_interrupt') as connection:
      # Test closing a not-yet-opened nexe.
      reply = connection.RspRequest('vFile:close:%x' % 13)
      self.assertEqual(reply, 'F-1,%x' % GDB_EBADF)

  def test_register_constraints(self):
    if ARCH == 'mips32':
      # This has not been implemented for mips.
      return
    with LaunchDebugStub('test_super_instruction') as connection:
      reply = connection.RspRequest('c')
      SkipBreakpoint(connection, reply)

      regs = DecodeRegs(connection.RspRequest('g'))

      if ARCH == 'x86-32':
        valid_regs = ['eax', 'ebx', 'ecx', 'edx', 'esp',
                      'ebp', 'esi', 'edi', 'eflags']
        read_only_regs = ['cs', 'ss', 'ds', 'es', 'fs', 'gs']
        super_inst_len = 5
      elif ARCH == 'x86-64':
        valid_regs = ['rax', 'rbx', 'rcx', 'rdx', 'rsi', 'rdi', 'eflags',
                      'r8', 'r9', 'r10', 'r11', 'r12', 'r13', 'r14']
        read_only_regs = ['cs', 'ss', 'ds', 'es', 'fs', 'gs', 'r15']
        super_inst_len = 8
      elif ARCH == 'arm':
        valid_regs = ['r0', 'r1', 'r2', 'r3', 'r4', 'r5', 'r6',
                      'r7', 'r8', 'r10', 'r11', 'r12', 'r14']
        read_only_regs = ['r9']
        super_inst_len = 8
      else:
        raise AssertionError('Unknown architecture')

      # Allowed to change all valid registers.
      for reg in valid_regs:
        self.assertEquals(ChangeReg(connection, reg,
                                    lambda x: (x + 1) % 0xffffffff), 'OK')

      # Cannot change read only registers.
      for reg in read_only_regs:
        self.assertEquals(ChangeReg(connection, reg,
                                    lambda x: (x + 1) % 0xffffffff), 'E03')

      if ARCH == 'x86-64':
        # Cannot change the upper 32 bits of x86-64 restricted registers
        # to non-zero value.
        for reg in ['rsp', 'rbp']:
          self.assertEquals(ChangeReg(connection, reg, lambda x: 0), 'OK')
          self.assertEquals(ChangeReg(connection, reg, lambda x: x), 'OK')
          self.assertEquals(ChangeReg(connection, reg,
            lambda x: x + 0xf00000000), 'E03')
      elif ARCH == 'arm':
        # Upper 2 bits of r13 (sp) must be 0.
        self.assertEquals(ChangeReg(connection, 'r13', lambda x: 0), 'OK')
        self.assertEquals(ChangeReg(connection, 'r13',
                                    lambda x: 0x80000000), 'E03')
        # Only upper 4 bits (NZCV) of cpsr can be set.
        self.assertEquals(ChangeReg(connection, 'cpsr',
                                    lambda x: 0xF0000000), 'OK')
        self.assertEquals(ChangeReg(connection, 'cpsr', lambda x: 1), 'E03')
        # PC must fall in sandboxed range
        self.assertEquals(ChangeReg(connection, 'r15',
                                    lambda x: 0xF0000000), 'E03')
        # PC must be on inst boundary (insts are 4 bytes)
        self.assertEquals(ChangeReg(connection, 'r15', lambda x: x + 3), 'E03')

      # Next instruction is a super instruction.
      # Therefore cannot jump anywhere in the middle.
      for i in xrange(1, super_inst_len):
        self.assertEquals(ChangeReg(connection, IP_REG[ARCH],
          lambda x: x + i), 'E03')

      # Allowed to jump over the entire super instruction.
      self.assertEquals(ChangeReg(connection, IP_REG[ARCH],
        lambda x: x + super_inst_len), 'OK')

  def test_step_inside_super_instruction(self):
    if not SingleSteppingWorks():
      return
    with LaunchDebugStub('test_super_instruction') as connection:
      if ARCH == 'x86-32':
        reg = 'eax'
      elif ARCH == 'x86-64':
        reg = 'rax'
      else:
        raise AssertionError('Unknown architecture')

      reply = connection.RspRequest('c')
      SkipBreakpoint(connection, reply)

      reply = connection.RspRequest('vCont;s:1')
      self.assertEquals(reply, 'T05thread:1;')

      regs = DecodeRegs(connection.RspRequest('g'))

      # Cannot change registers within super-instruction.
      self.assertEquals(ChangeReg(connection, reg, lambda x: x + 1), 'E03')

class DebugStubBreakpointTest(unittest.TestCase):

  def CheckInstructionPtr(self, connection, expected_ip):
    ip_value = DecodeRegs(connection.RspRequest('g'))[IP_REG[ARCH]]
    if ARCH == 'x86-64':
      # TODO(mseaborn): The debug stub should probably omit the top
      # bits of %rip automatically.
      ip_value &= 0xffffffff
    self.assertEquals(ip_value, expected_ip)

  def test_setting_and_removing_breakpoint(self):
    func_addr = GetSymbols()['breakpoint_target_func']
    with LaunchDebugStub('test_setting_breakpoint') as connection:
      # Set a breakpoint.
      reply = connection.RspRequest('Z0,%x,0' % func_addr)
      self.assertEquals(reply, 'OK')
      # Requesting a breakpoint on an address that already has a
      # breakpoint should return an error.
      reply = connection.RspRequest('Z0,%x,0' % func_addr)
      self.assertEquals(reply, 'E03')

      # When we run the program, we should hit the breakpoint.  When
      # we continue, we should hit the breakpoint again because it has
      # not been removed: the debug stub does not step through
      # breakpoints automatically.
      for i in xrange(2):
        reply = connection.RspRequest('c')
        AssertReplySignal(reply, NACL_SIGTRAP)
        self.CheckInstructionPtr(connection, func_addr)

      # If we continue a single thread, the fault the thread receives
      # should still be recognized as a breakpoint.
      tid = ParseThreadStopReply(reply)['thread_id']
      reply = connection.RspRequest('vCont;c:%x' % tid)
      AssertReplySignal(reply, NACL_SIGTRAP)
      self.CheckInstructionPtr(connection, func_addr)

      # Check that we can remove the breakpoint.
      reply = connection.RspRequest('z0,%x,0' % func_addr)
      self.assertEquals(reply, 'OK')
      # Requesting removing a breakpoint on an address that does not
      # have one should return an error.
      reply = connection.RspRequest('z0,%x,0' % func_addr)
      self.assertEquals(reply, 'E03')
      # After continuing, we should not hit the breakpoint again, and
      # the program should run to completion.
      reply = connection.RspRequest('c')
      self.assertEquals(reply, 'W00')

  def test_setting_breakpoint_on_invalid_address(self):
    with LaunchDebugStub('test_exit_code') as connection:
      # Requesting a breakpoint on an invalid address should give an error.
      reply = connection.RspRequest('Z0,%x,1' % (1 << 32))
      self.assertEquals(reply, 'E03')

  def test_setting_breakpoint_on_data_address(self):
    with LaunchDebugStub('test_exit_code') as connection:
      # Pick an arbitrary address in the data segment.
      data_addr = GetSymbols()['g_main_thread_var']
      # Requesting a breakpoint on a non-code address should give an error.
      reply = connection.RspRequest('Z0,%x,1' % data_addr)
      self.assertEquals(reply, 'E03')

  def test_breakpoint_memory_changes_are_hidden(self):
    func_addr = GetSymbols()['breakpoint_target_func']
    with LaunchDebugStub('test_setting_breakpoint') as connection:
      chunk_size = 32
      old_memory = ReadMemory(connection, func_addr, chunk_size)
      reply = connection.RspRequest('Z0,%x,0' % func_addr)
      self.assertEquals(reply, 'OK')

      # The debug stub should hide the memory modification.
      new_memory = ReadMemory(connection, func_addr, chunk_size)
      self.assertEquals(new_memory, old_memory)
      # Check reading a subset of the range.  (This will only be a
      # proper subset on architectures where the breakpoint size is
      # >1, such as ARM not but x86.)
      new_memory = ReadMemory(connection, func_addr, 1)
      self.assertEquals(new_memory, old_memory[:1])

  def test_setting_breakpoint_in_super_instruction(self):
    if ARCH != 'x86-32' and ARCH != 'x86-64':
      # ARM breakpoints are handled separately, and MIPS has not been
      # implemented.
      return
    with LaunchDebugStub('test_super_instruction') as connection:
      reply = connection.RspRequest('c')
      SkipBreakpoint(connection, reply)

      regs = DecodeRegs(connection.RspRequest('g'))

      if ARCH == 'arm':
        offset = 4
      elif ARCH == 'x86-32' or ARCH == 'x86-64':
        offset = 3
      else:
        raise AssertionError('Unknown architecture')

      # Next instruction is inside super instruction.
      invalid_addr = regs[IP_REG[ARCH]] + offset

      # Setting breakpoint here should not be allowed.
      reply = connection.RspRequest('Z0,%x,0' % invalid_addr)
      self.assertEquals(reply, 'E03')

  def test_setting_breakpoint_in_constant_pools(self):
    if ARCH != 'arm':
      return
    with LaunchDebugStub('test_arm_breakpoint') as connection:
      reply = connection.RspRequest('c')
      SkipBreakpoint(connection, reply)

      regs = DecodeRegs(connection.RspRequest('g'))

      # First constant pool is next bundle.
      pc = regs[IP_REG[ARCH]] + 16

      # Check we cannot set breakpoint anywhere inside both bundles.
      for addr in range(pc, pc + 32):
        reply = connection.RspRequest('Z0,%x,0' % addr)
        self.assertEquals(reply, 'E03')


class DebugStubThreadSuspensionTest(unittest.TestCase):

  def WaitForTestThreadsToStart(self, connection, symbols):
    # Wait until:
    #  * The main thread starts to modify g_main_thread_var.
    #  * The child thread executes a breakpoint.
    old_value = ReadUint32(connection, symbols['g_main_thread_var'])
    while True:
      reply = connection.RspRequest('c')
      SkipBreakpoint(connection, reply)
      child_thread_id = ParseThreadStopReply(reply)['thread_id']
      if ReadUint32(connection, symbols['g_main_thread_var']) != old_value:
        break
    return child_thread_id

  def test_continuing_thread_with_others_suspended(self):
    if UsingQemu():
      # Suspending a running thread doesn't work under qemu-arm or qemu-mips,
      # so disable this test there.
      return
    with LaunchDebugStub('test_suspending_threads') as connection:
      symbols = GetSymbols()
      child_thread_id = self.WaitForTestThreadsToStart(connection, symbols)

      # Test continuing a single thread while other threads remain
      # suspended.
      for _ in range(3):
        main_thread_val = ReadUint32(connection, symbols['g_main_thread_var'])
        child_thread_val = ReadUint32(connection, symbols['g_child_thread_var'])
        reply = connection.RspRequest('vCont;c:%x' % child_thread_id)
        SkipBreakpoint(connection, reply)
        self.assertEquals(ParseThreadStopReply(reply)['thread_id'],
                          child_thread_id)
        # The main thread should not be allowed to run, so should not
        # modify g_main_thread_var.
        self.assertEquals(
            ReadUint32(connection, symbols['g_main_thread_var']),
            main_thread_val)
        # The child thread should always modify g_child_thread_var
        # between each breakpoint.
        self.assertNotEquals(
            ReadUint32(connection, symbols['g_child_thread_var']),
            child_thread_val)

  def test_single_stepping_thread_with_others_suspended(self):
    if UsingQemu():
      # Suspending a running thread doesn't work under qemu-arm or qemu-mips,
      # so disable this test there.
      return
    with LaunchDebugStub('test_suspending_threads') as connection:
      symbols = GetSymbols()
      child_thread_id = self.WaitForTestThreadsToStart(connection, symbols)

      # Test single-stepping a single thread while other threads
      # remain suspended.
      for _ in range(3):
        main_thread_val = ReadUint32(connection, symbols['g_main_thread_var'])
        child_thread_val = ReadUint32(connection, symbols['g_child_thread_var'])
        while True:
          reply = connection.RspRequest('vCont;s:%x' % child_thread_id)
          if (ARCH in ('x86-32', 'x86-64') and
              ParseThreadStopReply(reply)['signal'] == NACL_SIGTRAP):
            # We single-stepped through an instruction without
            # otherwise faulting.  We did not hit the breakpoint, so
            # there is nothing to do.
            pass
          else:
            SkipBreakpoint(connection, reply)
          self.assertEquals(ParseThreadStopReply(reply)['thread_id'],
                            child_thread_id)
          # The main thread should not be allowed to run, so should not
          # modify g_main_thread_var.
          self.assertEquals(
              ReadUint32(connection, symbols['g_main_thread_var']),
              main_thread_val)
          # Eventually, the child thread should modify g_child_thread_var.
          if (ReadUint32(connection, symbols['g_child_thread_var'])
              != child_thread_val):
            break


def Main():
  # TODO(mseaborn): Clean up to remove the global variables.  They are
  # currently here because unittest does not help with making
  # parameterised tests.
  index = sys.argv.index('--')
  args = sys.argv[index + 1:]
  # The remaining arguments go to unittest.main().
  sys.argv = sys.argv[:index]
  global ARCH
  global NM_TOOL
  global SEL_LDR_COMMAND
  ARCH = args.pop(0)
  NM_TOOL = args.pop(0)
  SEL_LDR_COMMAND = args
  unittest.main()


if __name__ == '__main__':
  Main()
