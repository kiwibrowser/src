#!/usr/bin/env python
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This takes two command-line arguments:
      INFILE                  raw linked ELF file name
      OUTFILE                 output file name

It makes a copy of INFILE, and changes the ELF PHDR in place in the copy.
Then it moves the copy to OUTFILE.

nacl_helper_bootstrap's large (~1G) bss segment could cause the kernel
to refuse to load the program because it didn't think there was enough
free memory in the system for so large an allocation of anonymous memory

To avoid the second problem, the bootstrap program no longer has a large
bss.  Instead, it has a special ELF segment (i.e. PT_LOAD header) that
specifies no memory access, and a large (~1G) mapping size from the file.
This mapping is way off the end of the file, but the kernel doesn't mind
that, and since it's all a file mapping, the kernel does not do its normal
memory accounting for consuming a large amount of anonymous memory.

Unfortunately, it's impossible to get the linker to produce exactly the
right PT_LOAD header by itself.  Using a custom linker script, we get the
layout exactly how we want it and a PT_LOAD header that is almost right.
We then use a build-time helper program to munge one field of the PT_LOAD
to make it exactly what we need.
"""

import argparse
import ctypes
import mmap
import shutil
import sys


class Error(Exception):
  pass


class ElfFormatError(Error):

  def __init__(self, message, offset=None):
    if offset is not None:
      message += ' (offset=%d)' % (offset,)
    super(ElfFormatError, self).__init__(message)


class ElfStructMixIn:

  def GetBytes(self):
    return buffer(self)[:]

  @classmethod
  def FromString(cls, bytes):
    inst = cls()
    assert len(bytes) >= ctypes.sizeof(inst)
    ctypes.memmove(ctypes.addressof(inst), bytes, ctypes.sizeof(inst))
    return inst


class EHDRIdent(ctypes.Structure, ElfStructMixIn):
  _fields_ = [
      ('ei_magic', (ctypes.c_char * 4)),
      ('ei_class', ctypes.c_byte),
      ('ei_data', ctypes.c_byte),
  ]


EHDR_FIELDS = {
  'name': 'EHDR',

  32: [
    ('e_ident', (ctypes.c_byte * 16)),
    ('c_type', ctypes.c_uint16),
    ('e_machine', ctypes.c_uint16),
    ('e_version', ctypes.c_uint32),
    ('e_entry', ctypes.c_uint32),
    ('e_phoff', ctypes.c_uint32),
    ('e_shoff', ctypes.c_uint32),
    ('e_flags', ctypes.c_uint32),
    ('e_ehsize', ctypes.c_uint16),
    ('e_phentsize', ctypes.c_uint16),
    ('e_phnum', ctypes.c_uint16),
    ('e_shentsize', ctypes.c_uint16),
    ('e_shnum', ctypes.c_uint16),
    ('e_shstrndx', ctypes.c_uint16),
  ],

  64: [
    ('e_ident', (ctypes.c_byte * 16)),
    ('c_type', ctypes.c_uint16),
    ('e_machine', ctypes.c_uint16),
    ('e_version', ctypes.c_uint32),
    ('e_entry', ctypes.c_uint64),
    ('e_phoff', ctypes.c_uint64),
    ('e_shoff', ctypes.c_uint64),
    ('e_flags', ctypes.c_uint32),
    ('e_ehsize', ctypes.c_uint16),
    ('e_phentsize', ctypes.c_uint16),
    ('e_phnum', ctypes.c_uint16),
    ('e_shentsize', ctypes.c_uint16),
    ('e_shnum', ctypes.c_uint16),
    ('e_shstrndx', ctypes.c_uint16),
  ],
}

PHDR_FIELDS = {
  'name': 'PHDR',

  32: [
    ('p_type', ctypes.c_uint32),
    ('p_offset', ctypes.c_uint32),
    ('p_vaddr', ctypes.c_uint32),
    ('p_paddr', ctypes.c_uint32),
    ('p_filesz', ctypes.c_uint32),
    ('p_memsz', ctypes.c_uint32),
    ('p_flags', ctypes.c_uint32),
    ('p_align', ctypes.c_uint32),
  ],

  64: [
    ('p_type', ctypes.c_uint32),
    ('p_flags', ctypes.c_uint32),
    ('p_offset', ctypes.c_uint64),
    ('p_vaddr', ctypes.c_uint64),
    ('p_paddr', ctypes.c_uint64),
    ('p_filesz', ctypes.c_uint64),
    ('p_memsz', ctypes.c_uint64),
    ('p_align', ctypes.c_uint64),
  ],
}


def HexDump(value):
  return ''.join('%02X' % (ord(x),) for x in value)


class Elf(object):

  ELF_MAGIC = '\x7FELF'

  PT_LOAD = 1

  def __init__(self, elf_map, elf_class):
    self._elf_map = elf_map
    self._elf_class = elf_class

  @classmethod
  def LoadMap(cls, elf_map):
    elf_ehdr = EHDRIdent.FromString(elf_map[0:])
    if elf_ehdr.ei_magic != cls.ELF_MAGIC:
      raise ElfFormatError('Missing ELF magic number (%s)' % (
                           HexDump(elf_ehdr.ei_magic),), 0)

    if elf_ehdr.ei_class == 1:
      elf_class = 32
    elif elf_ehdr.ei_class == 2:
      elf_class = 64
    else:
      raise ElfFormatError('Unhandled ELF "ei_class" (%d)' %
                           (elf_ehdr.ei_class,), 4)

    if elf_ehdr.ei_data != 1:
      raise ElfFormatError('Wrong endian "ei_data" (%d): expected 1 (little)' %
                           (elf_ehdr.ei_data,), 5)

    return cls(elf_map, elf_class)

  def Structure(self, fields):
    name = fields.get('name', 'AnonymousStructure')
    name = '%s%sLITTLE' % (name, self._elf_class)

    class Result(ctypes.LittleEndianStructure, ElfStructMixIn):
      _pack_ = 1
      _fields_ = fields[self._elf_class]
    Result.__name__ = name
    return Result

  def GetEhdr(self):
    return self.Structure(EHDR_FIELDS).FromString(self._elf_map[0:])

  def _GetPhdrOffset(self, index):
    ehdr = self.GetEhdr()
    if index >= ehdr.e_phnum:
      raise IndexError('Index out of bounds (e_phnum=%d)' % (ehdr.e_phnum,))
    return ehdr.e_phoff + (ehdr.e_phentsize * index)

  def GetPhdr(self, index):
    return self.Structure(PHDR_FIELDS).FromString(
        self._elf_map[self._GetPhdrOffset(index):])

  def SetPhdr(self, index, phdr):
    phdr_off = self._GetPhdrOffset(index)
    self._elf_map[phdr_off:(phdr_off + ctypes.sizeof(phdr))] = phdr.GetBytes()


def RunMain(args):
  parser = argparse.ArgumentParser()
  parser.add_argument('infile', metavar='PATH',
      help='The ELF binary to be read')
  parser.add_argument('outfile', metavar='PATH',
      help='The munged ELF binary to be written')
  parser.add_argument('-s', '--phdr_index', metavar='OFFSET', type=int,
                      default=2,
                      help='The zero-index program header to modify'
                           '(default=%(default)s)')
  args = parser.parse_args(args)

  # Copy the input file to a temporary file, so that we can write it in place.
  tmpfile = args.outfile + '.tmp'
  shutil.copy(args.infile, tmpfile)
  # Create the ELF map of the temporary file and edit the PHDR in place.
  with open(tmpfile, 'rw+b') as fd:
    # Map the file.
    elf_map = mmap.mmap(fd.fileno(), 0)
    elf = Elf.LoadMap(elf_map)
    phdr = elf.GetPhdr(args.phdr_index)
    if phdr.p_type != Elf.PT_LOAD:
      raise Error('Invalid segment number; not PT_LOAD (%d)' % phdr.p_type)
    if phdr.p_filesz != 0:
      raise Error("Program header %d has nonzero p_filesz" % args.phdr_index)
    phdr.p_filesz = phdr.p_memsz
    elf.SetPhdr(args.phdr_index, phdr)
    elf_map.flush()
  # Move the munged temporary file to the output location.
  shutil.move(tmpfile, args.outfile)
  return 0


def main(args):
  try:
    return RunMain(args)
  except Error as e:
    sys.stderr.write('nacl_bootstrap_munge_phdr: ' + str(e) + '\n')
    return 1


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
