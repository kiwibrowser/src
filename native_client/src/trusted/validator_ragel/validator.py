# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cStringIO
import ctypes
import os
import re
import subprocess
import sys
import tempfile

import objdump_parser


# Some constants from validator.h
VALIDATION_ERRORS_MASK = 0x05ffc000
UNSUPPORTED_INSTRUCTION = 0x04000000
BAD_JUMP_TARGET = 0x40000000

RESTRICTED_REGISTER_MASK = 0x00001f00
RESTRICTED_REGISTER_SHIFT = 8

NC_REG_RAX = 0
NC_REG_RCX = 1
NC_REG_RDX = 2
NC_REG_RBX = 3
NC_REG_RSP = 4
NC_REG_RBP = 5
NC_REG_RSI = 6
NC_REG_RDI = 7
NC_REG_R8 = 8
NC_REG_R9 = 9
NC_REG_R10 = 10
NC_REG_R11 = 11
NC_REG_R12 = 12
NC_REG_R13 = 13
NC_REG_R14 = 14
NC_REG_R15 = 15
ALL_REGISTERS = range(NC_REG_RAX, NC_REG_R15 + 1)
NC_NO_REG = 0x19

RESTRICTED_REGISTER_INITIAL_VALUE_MASK = 0x000000ff
CALL_USER_CALLBACK_ON_EACH_INSTRUCTION = 0x00000100

# Macroses from validator.h
def PACK_RESTRICTED_REGISTER_INITIAL_VALUE(register):
  return register ^ NC_NO_REG


BUNDLE_SIZE = 32


REGISTER_NAMES = {
    NC_REG_RAX: '%rax',
    NC_REG_RCX: '%rcx',
    NC_REG_RDX: '%rdx',
    NC_REG_RBX: '%rbx',
    NC_REG_RSP: '%rsp',
    NC_REG_RBP: '%rbp',
    NC_REG_RSI: '%rsi',
    NC_REG_RDI: '%rdi',
    NC_REG_R8: '%r8',
    NC_REG_R9: '%r9',
    NC_REG_R10: '%r10',
    NC_REG_R11: '%r11',
    NC_REG_R12: '%r12',
    NC_REG_R13: '%r13',
    NC_REG_R14: '%r14',
    NC_REG_R15: '%r15'}

REGISTER_BY_NAME = dict(map(reversed, REGISTER_NAMES.items()))


CALLBACK_TYPE = ctypes.CFUNCTYPE(
    ctypes.c_uint32,  # Bool result
    ctypes.POINTER(ctypes.c_uint8),  # begin
    ctypes.POINTER(ctypes.c_uint8),  # end
    ctypes.c_uint32,  # validation info
    ctypes.c_void_p,  # callback data
)


class DisassemblerError(Exception):
  pass


class Validator(object):

  def __init__(self, validator_dll=None, decoder_dll=None):
    """Initialize python interface to the validator.

    Should be called before any calls to ValidateChunk.

    Args:
      validator_dll: path to dll that provides ValidateChunkIA32 and
          ValidateChynkAMD64 functions.

    Returns:
      None.
    """

    if validator_dll is not None:
      validator_dll = ctypes.cdll.LoadLibrary(validator_dll)

      self.GetFullCPUIDFeatures = validator_dll.GetFullCPUIDFeatures
      self.GetFullCPUIDFeatures.restype = ctypes.c_void_p

      self._ValidateChunkIA32 = validator_dll.ValidateChunkIA32
      self._ValidateChunkAMD64 = validator_dll.ValidateChunkAMD64

      self._ValidateChunkIA32.argtypes = self._ValidateChunkAMD64.argtypes = [
          ctypes.POINTER(ctypes.c_uint8),  # codeblock
          ctypes.c_size_t,  # size
          ctypes.c_uint32,  # options
          ctypes.c_void_p,  # CPU features
          CALLBACK_TYPE,  # callback
          ctypes.c_void_p,  # callback data
      ]
      self._ValidateChunkIA32.restype = ctypes.c_bool  # Bool
      self._ValidateChunkAMD64.restype = ctypes.c_bool  # Bool

      self._ValidateAndGetFinalRestrictedRegister = (
          validator_dll.ValidateAndGetFinalRestrictedRegister)
      self._ValidateAndGetFinalRestrictedRegister.argtypes = [
          ctypes.POINTER(ctypes.c_uint8),  # codeblock
          ctypes.c_size_t,  # size
          ctypes.c_size_t,  # actual_size
          ctypes.c_uint32,  # initial_restricted_register
          ctypes.c_void_p,  # CPU features
          ctypes.POINTER(ctypes.c_uint32), # resulting_restricted_register
      ]
      self._ValidateChunkIA32.restype = ctypes.c_bool  # Bool

    if decoder_dll is not None:
      decoder_dll = ctypes.cdll.LoadLibrary(decoder_dll)
      self.DisassembleChunk_ = decoder_dll.DisassembleChunk
      self.DisassembleChunk_.argtypes = [
          ctypes.POINTER(ctypes.c_uint8),  # data
          ctypes.c_size_t,  # size
          ctypes.c_int,  # bitness
      ]
      self.DisassembleChunk_.restype = ctypes.c_char_p

  def ValidateChunk(
      self,
      data,
      bitness,
      callback=None,
      on_each_instruction=False,
      restricted_register=None):
    """Validate chunk, calling the callback if there are errors.

    Validator interface must be initialized by calling Init first.

    Args:
      data: raw data to validate as python string.
      bitness: 32 or 64.
      callback: function that takes three arguments
          begin_index, end_index and info (info is combination of flags; it is
          explained in validator.h). It is invoked for every erroneous
          instruction.
      on_each_instruction: whether to invoke callback on each instruction (not
         only on erroneous ones).
      restricted_register: initial value for the restricted_register variable
                           (see validator_internals.html for the details)

    Returns:
      True if the chunk is valid, False if invalid.
    """

    data_addr = ctypes.cast(data, ctypes.c_void_p).value

    def LowLevelCallback(begin, end, info, callback_data):
      if callback is not None:
        begin_index = ctypes.cast(begin, ctypes.c_void_p).value - data_addr
        end_index = ctypes.cast(end, ctypes.c_void_p).value - data_addr
        callback(begin_index, end_index, info)

      # UNSUPPORTED_INSTRUCTION indicates validator failure only for pnacl-mode.
      # Since by default we are in non-pnacl-mode, the flag is simply cleared.
      info &= ~UNSUPPORTED_INSTRUCTION

      # See validator.h for details
      if info & (VALIDATION_ERRORS_MASK | BAD_JUMP_TARGET) != 0:
        return 0
      else:
        return 1

    options = 0
    if on_each_instruction:
      options |= CALL_USER_CALLBACK_ON_EACH_INSTRUCTION
    if restricted_register is not None:
      assert restricted_register in ALL_REGISTERS
      options |= PACK_RESTRICTED_REGISTER_INITIAL_VALUE(restricted_register)

    data_ptr = ctypes.cast(data, ctypes.POINTER(ctypes.c_uint8))

    validate_chunk_function = {
        32: self._ValidateChunkIA32,
        64: self._ValidateChunkAMD64}[bitness]

    result = validate_chunk_function(
        data_ptr,
        len(data),
        options,
        self.GetFullCPUIDFeatures(),
        CALLBACK_TYPE(LowLevelCallback),
        None)

    return bool(result)

  def ValidateAndGetFinalRestrictedRegister(
      self, data, actual_size, initial_rr):
    assert initial_rr is None or initial_rr in ALL_REGISTERS
    if initial_rr is None:
      initial_rr = NC_NO_REG

    data_ptr = ctypes.cast(data, ctypes.POINTER(ctypes.c_uint8))

    p = ctypes.pointer(ctypes.c_uint32(0))
    result = self._ValidateAndGetFinalRestrictedRegister(
        data_ptr, len(data),
        actual_size,
        initial_rr,
        self.GetFullCPUIDFeatures(),
        p)

    if result:
      if p[0] == NC_NO_REG:
        resulting_rr = None
      else:
        resulting_rr = p[0]
      return True, resulting_rr
    else:
      return False, None

  def DisassembleChunk(self, data, bitness):
    data_ptr = ctypes.cast(data, ctypes.POINTER(ctypes.c_uint8))
    result = self.DisassembleChunk_(data_ptr, len(data), bitness)

    instructions = []
    total_bytes = 0
    for line in cStringIO.StringIO(result):
      m = re.match(r'rejected at ([\da-f]+)', line)
      if m is not None:
        offset = int(m.group(1), 16)
        raise DisassemblerError(offset, ' '.join('%02x' % ord(c) for c in data))
      insn = objdump_parser.ParseLine(line)
      insn = objdump_parser.CanonicalizeInstruction(insn)
      instructions.append(insn)
      total_bytes += len(insn.bytes)
    return instructions

  # TODO(shcherbina): Remove it.
  # Currently I'm keeping it around just in case (might be helpful for
  # troubleshooting RDFA decoder).
  def DisassembleChunkWithObjdump(self, data, bitness):
    """Disassemble chunk assuming it consists of valid instructions.

    Args:
      data: raw data as python string.
      bitness: 32 or 64

    Returns:
      List of objdump_parser.Instruction tuples. If data can't be disassembled
      (either contains invalid instructions or ends in a middle of instruction)
      exception is raised.
    """
    # TODO(shcherbina):
    # Replace this shameless plug with python interface to RDFA decoder once
    # https://code.google.com/p/nativeclient/issues/detail?id=3456 is done.

    arch = {32: '-Mi386', 64: '-Mx86-64'}[bitness]

    tmp = tempfile.NamedTemporaryFile(mode='wb', delete=False)
    try:
      tmp.write(data)
      tmp.close()

      objdump_proc = subprocess.Popen(
          ['objdump',
           '-mi386', arch, '--target=binary',
           '--disassemble-all', '--disassemble-zeroes',
           '--insn-width=15',
           tmp.name],
          stdout=subprocess.PIPE)

      instructions = []
      total_bytes = 0
      for line in objdump_parser.SkipHeader(objdump_proc.stdout):
        insn = objdump_parser.ParseLine(line)
        insn = objdump_parser.CanonicalizeInstruction(insn)
        instructions.append(insn)
        total_bytes += len(insn.bytes)

      assert len(data) == total_bytes

      return_code = objdump_proc.wait()
      assert return_code == 0, 'error running objdump'

      return instructions

    finally:
      tmp.close()
      os.remove(tmp.name)


def main():
  print 'Self check'
  print sys.argv
  validator_dll, = sys.argv[1:]
  print validator_dll

  validator = Validator(validator_dll)

  # 'z' is the first byte of JP instruction (which does not validate in this
  # case because it crosses bundle boundary)
  data = '\x90' * 31 + 'z'

  for bitness in 32, 64:
    errors = []

    def Callback(begin_index, end_index, info):
      errors.append(begin_index)
      print 'callback', begin_index, end_index

    result = validator.ValidateChunk(
        data,
        bitness=bitness,
        callback=Callback)

    assert not result
    assert errors == [31], errors


if __name__ == '__main__':
  main()
