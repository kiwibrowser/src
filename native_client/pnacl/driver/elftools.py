#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Tools for parsing ELF headers.

import struct
from collections import namedtuple

from driver_log import DriverOpen, DriverClose, Log, FixArch

class ELFHeader(object):
  ELF_MAGIC = '\x7fELF'
  ELF_TYPES = { 1: 'REL',  # .o
                2: 'EXEC', # .exe
                3: 'DYN' } # .so
  ELF_MACHINES = {  3: '386',
                    8: 'MIPS',
                   40: 'ARM',
                   62: 'X86_64' }
  ELF_OSABI = { 0: 'UNIX',
                3: 'LINUX',
                123: 'NACL' }
  ELF_ABI_VER = { 0: 'NONE',
                  7: 'NACL' }

  # A list of tuples of pack format and name. 'P' in pack formats will
  # be replaced by 'I' for 32bit ELF and 'Q' for 64bit ELF.
  ELF_HEADER_FORMAT = [
      ('16s', 'e_ident'),
      ('H', 'e_type'),
      ('H', 'e_machine'),
      ('I', 'e_version'),
      ('P', 'e_entry'),
      ('P', 'e_phoff'),
      ('P', 'e_shoff'),
      ('I', 'e_flags'),
      ('H', 'e_ehsize'),
      ('H', 'e_phentsize'),
      ('H', 'e_phnum'),
      ('H', 'e_shentsize'),
      ('H', 'e_shnum'),
      ('H', 'e_shstrndx')
  ]

  ELFCLASS32 = 1
  ELFCLASS64 = 2

  Ehdr = namedtuple('Ehdr', ' '.join(name for _, name in ELF_HEADER_FORMAT))

  def __init__(self, header, filename):
    pack_format = ''.join(fmt for fmt, _ in self.ELF_HEADER_FORMAT)
    e_class = ord(header[4])
    if e_class == ELFHeader.ELFCLASS32:
      pack_format = pack_format.replace('P', 'I')
    elif e_class == ELFHeader.ELFCLASS64:
      pack_format = pack_format.replace('P', 'Q')
    else:
      Log.Fatal('%s: ELF file has unknown class (%d)', filename, e_class)

    ehdr = self.Ehdr(*struct.unpack_from(pack_format, header))
    e_osabi = ord(header[7])
    e_abiver = ord(header[8])

    if e_osabi not in ELFHeader.ELF_OSABI:
      Log.Fatal('%s: ELF file has unknown OS ABI (%d)', filename, e_osabi)
    if e_abiver not in ELFHeader.ELF_ABI_VER:
      Log.Fatal('%s: ELF file has unknown ABI version (%d)', filename, e_abiver)
    if ehdr.e_type not in ELFHeader.ELF_TYPES:
      Log.Fatal('%s: ELF file has unknown type (%d)', filename, ehdr.e_type)
    if ehdr.e_machine not in ELFHeader.ELF_MACHINES:
      Log.Fatal('%s: ELF file has unknown machine type (%d)',
                filename, ehdr.e_machine)

    self.type = self.ELF_TYPES[ehdr.e_type]
    self.machine = self.ELF_MACHINES[ehdr.e_machine]
    self.osabi = self.ELF_OSABI[e_osabi]
    self.abiver = self.ELF_ABI_VER[e_abiver]
    self.arch = FixArch(self.machine)  # For convenience
    self.phoff = ehdr.e_phoff
    self.phnum = ehdr.e_phnum
    self.phentsize = ehdr.e_phentsize

class ProgramHeader(object):
  # Note we cannot use the P => I/Q trick we used for ELF header
  # because the order of members of Elf32_Phdr is different from
  # Elf64_Phdr's.
  PROGRAM_HEADER_FORMAT_32 = [
      ('I', 'p_type'),
      ('I', 'p_offset'),
      ('I', 'p_vaddr'),
      ('I', 'p_paddr'),
      ('I', 'p_filesz'),
      ('I', 'p_memsz'),
      ('I', 'p_flags'),
      ('I', 'p_align'),
  ]

  PROGRAM_HEADER_FORMAT_64 = [
      ('I', 'p_type'),
      ('I', 'p_flags'),
      ('Q', 'p_offset'),
      ('Q', 'p_vaddr'),
      ('Q', 'p_paddr'),
      ('Q', 'p_filesz'),
      ('Q', 'p_memsz'),
      ('Q', 'p_align'),
  ]

  PT_NULL = 0
  PT_LOAD = 1
  PT_DYNAMIC = 2
  PT_INTERP = 3

  def __init__(self, header, filename):
    pack_format = None
    for program_header_format in [self.PROGRAM_HEADER_FORMAT_32,
                                  self.PROGRAM_HEADER_FORMAT_64]:
      pack_format = ''.join(fmt for fmt, _ in program_header_format)
      if len(header) == struct.calcsize(pack_format):
        Phdr = namedtuple(
            'Phdr', ' '.join(name for _, name in program_header_format))
        phdr = Phdr(*struct.unpack(pack_format, header))
        break
    else:
      Log.Fatal('%s: Invalid program header size (%d)', filename, len(header))

    self.type = phdr.p_type
    self.offset = phdr.p_offset
    self.vaddr = phdr.p_vaddr
    self.paddr = phdr.p_paddr
    self.filesz = phdr.p_filesz
    self.memsz = phdr.p_memsz
    self.flags = phdr.p_flags
    self.align = phdr.p_align


# If the file is not ELF, returns None.
# Otherwise, returns an ELFHeader object.
def GetELFHeader(filename):
  fp = DriverOpen(filename, 'rb')
  # Read max(sizeof(Elf64_Ehdr), sizeof(Elf32_Ehdr)), which is 64 bytes.
  header = fp.read(64)
  DriverClose(fp)
  return DecodeELFHeader(header, filename)

def DecodeELFHeader(header, filename):
  # Pull e_ident, e_type, e_machine
  if header[0:4] != ELFHeader.ELF_MAGIC:
    return None
  return ELFHeader(header, filename)

# If the file is not ELF, returns None.
# Otherwise, returns a tuple of ELFHeader and list of ProgramHeader objects.
def GetELFAndProgramHeaders(filename):
  ehdr = GetELFHeader(filename)
  if not ehdr:
    return None
  phdrs = []
  fp = open(filename, 'rb')
  fp.seek(ehdr.phoff)
  for i in xrange(ehdr.phnum):
    phdrs.append(ProgramHeader(fp.read(ehdr.phentsize), filename))
  return (ehdr, phdrs)

# filetype.IsELF calls this IsElf. Top-level tools should prefer filetype.IsELF,
# both for consistency (i.e., all checks for file type come from that library),
# and because its results are cached.
def IsELF(filename):
  return GetELFHeader(filename) is not None
