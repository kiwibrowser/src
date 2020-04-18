#!/usr/bin/python
# Copyright (c) 2010 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This module contains utilities and constants needed to parse ELF headers.
"""

import exceptions
import struct

# maps yielding actual sizes of various abstract types

e32_size = {
    'Ident': '16s',
    'Addr' : 'I',
    'Half' : 'H',
    'Off' : 'I',
    'Word' : 'I',
    'Sword' : 'i',
}

e64_size = {
    'Ident': '16s',
    'Addr' : 'Q',
    'Half' : 'H',
    'Off' : 'Q',
    'Word' : 'I',
    'Sword' : 'i',
    'Xword' : 'Q',
    'Sxword' : 'q',
}

# map from word size to maps from abstract types to actual sizes

e_size = {
    32: e32_size,
    64: e64_size
}

# These *member_and_type globals provide both layout and abstract type
# information.  Thus, they cannot be a dictionary, since entries must
# be ordered.  The *member_type maps are also available for lookups.
ehdr_member_and_type = [
    ('ident', 'Ident'),
    ('type', 'Half'),
    ('machine', 'Half'),
    ('version', 'Word'),
    ('entry', 'Addr'),
    ('phoff', 'Off'),
    ('shoff', 'Off'),
    ('flags', 'Word'),
    ('ehsize', 'Half'),
    ('phentsize', 'Half'),
    ('phnum', 'Half'),
    ('shentsize', 'Half'),
    ('shnum', 'Half'),
    ('shstndx', 'Half'),
]

ehdr_member_type = dict(ehdr_member_and_type)

def MemberPositionMap(member_and_type):
  return dict((m,i) for i,(m,t) in enumerate(member_and_type))

elf32_phdr_member_and_type = [
    ('type', 'Word'),
    ('offset', 'Off'),
    ('vaddr', 'Addr'),
    ('paddr', 'Addr'),
    ('filesz', 'Word'),
    ('memsz', 'Word'),
    ('flags', 'Word'),
    ('align', 'Word'),
]

elf32_phdr_member_type = dict(elf32_phdr_member_and_type)

elf64_phdr_member_and_type = [
    ('type', 'Word'),
    ('flags', 'Word'),
    ('offset', 'Off'),
    ('vaddr', 'Addr'),
    ('paddr', 'Addr'),
    ('filesz', 'Xword'),
    ('memsz', 'Xword'),
    ('align', 'Xword'),
]

elf64_phdr_member_type = dict(elf64_phdr_member_and_type)

# The program header layout differs depending on the word size,
# primarily so that packing can be improved (move flags to be
# immediately after type, since offset alignment requirement would
# have otherwise have required a 32-bit pad entry there anyway).

phdr_member_and_type_map = {
    32: elf32_phdr_member_and_type,
    64: elf64_phdr_member_and_type
}

elf32_shdr_member_and_type = [
    ('name', 'Word'),
    ('type', 'Word'),
    ('flags', 'Word'),
    ('addr', 'Addr'),
    ('offset', 'Off'),
    ('size', 'Word'),
    ('link', 'Word'),
    ('info', 'Word'),
    ('addralign', 'Word'),
    ('entsize', 'Word'),
]

elf64_shdr_member_and_type = [
    ('name', 'Word'),
    ('type', 'Word'),
    ('flags', 'Xword'),
    ('addr', 'Addr'),
    ('offset', 'Off'),
    ('size', 'Xword'),
    ('link', 'Word'),
    ('info', 'Word'),
    ('addralign', 'Xword'),
    ('entsize', 'Xword'),
]

shdr_member_and_type_map = {
    32: elf32_shdr_member_and_type,
    64: elf64_shdr_member_and_type,
}


elf32_ehdr_format = '<' + ''.join(map(lambda elt_and_type:
                                        e32_size[elt_and_type[1]],
                                      ehdr_member_and_type))

elf64_ehdr_format = '<' + ''.join(map(lambda elt_and_type:
                                        e64_size[elt_and_type[1]],
                                      ehdr_member_and_type))

ehdr_ident_format='<' + ''.join(e_size[32][ehdr_member_type['ident']])


word_sizes = [32, 64]


ehdr_format_map = dict(map(lambda size:
                             (size,
                              ('<' +
                               ''.join([e_size[size][et[1]]
                                        for et in ehdr_member_and_type]))),
                           word_sizes))

phdr_format_map = dict(map(lambda size:
                             (size,
                              ('<' +
                               ''.join([e_size[size][et[1]]
                                        for et
                                        in phdr_member_and_type_map[size]]))),
                           word_sizes))

shdr_format_map = dict(map(lambda size:
                             (size,
                              ('<' +
                               ''.join([e_size[size][et[1]]
                                        for et
                                        in shdr_member_and_type_map[size]]))),
                           word_sizes))

"""
these versions gave pychecker headaches (tickled a pychecker bug):


ehdr_format_map = dict(map(lambda size:
                             (size,
                              ('<' +
                               ''.join(map(lambda elt_and_type:
                                             e_size[size][elt_and_type[1]],
                                           ehdr_member_and_type)))),
                           word_sizes))


phdr_format_map = dict(map(lambda size:
                             (size,
                              ('<' +
                               ''.join(map(lambda elt_and_type:
                                             e_size[size][elt_and_type[1]],
                                           phdr_member_and_type_map[size])))),
                           word_sizes))

shdr_format_map = dict(map(lambda size:
                             (size,
                              ('<' +
                               ''.join(map(lambda elt_and_type:
                                             e_size[size][elt_and_type[1]],
                                           shdr_member_and_type_map[size])))),
                           word_sizes))


and fully list-comprehension versions let the iteration variables 'et'
escape into the global scope, and other uses of 'et' as an iteration
variable causes pychecker to complain about shadowing a global.

ehdr_format_map = dict([(size,
                         ('<' +
                          ''.join([e_size[size][et[1]]
                                   for et in ehdr_member_and_type])))
                        for size in word_sizes])
"""



ehdr_type = {
    'none':        0,
    'rel':         1,
    'exec':        2,
    'dyn':         3,
    'core':        4,
    'loos':   0xfe00,
    'hios':   0xfeff,
    'loproc': 0xff00,
    'hiproc': 0xffff,
}

ehdr_machine = {
    'none':         0,
    'm32':          1,
    'sparc':        2,
    '386':          3,
    '64k':          4,
    '88k':          5,
    '860':          7,
    'mips':         9,
    'mips_rs4_be': 10,
    'loreserved':  11,
    'hireserved':  16,
    'arm':         40,
    'x86_64':      62,
}

def __rev_map(d):
  return dict((post, pre) for (pre, post) in d.iteritems())

ehdr_type_name = __rev_map(ehdr_type)
ehdr_machine_name = __rev_map(ehdr_machine)

ehdr_version = {
    'none': 0,
    'current': 1,
}
ehdr_version_name = __rev_map(ehdr_version)

ehdr_ident = {
    'mag0':0, 'mag1':1, 'mag2':2, 'mag3':3,
    'class':4, 'data':5, 'version':6,
    'osabi':7, 'abiversion':8,
}  # everything thereafter is padding

ehdr_ident_mag = '\177ELF'

ehdr_ident_class = [ None, 32, 64, ]
ehdr_ident_data = { 'none': 0, 'lsb': 1, 'msb': 2 },

phdr_type = {
    'null':                  0,
    'load':                  1,
    'dynamic':               2,
    'interp':                3,
    'note':                  4,
    'shlib':                 5,
    'phdr':                  6,
    'loos':         0x60000000,
    'hios':         0x6fffffff,
    'loproc':       0x70000000,
    'arm_exidx':    0x70000001,  # exception unwind table, NACL_arm only
    'hiproc':       0x7fffffff,
    # linux / gnu binutils
    'tls':                   7,
    'gnu_eh_frame': 0x6474e550,
    'gnu_stack':    0x6474e551,
    'gnu_relro':    0x6474e552,
}
phdr_type_name = __rev_map(phdr_type)

phdr_flags = {
    'x': 1,
    'w': 2,
    'r': 4,
    'maskos': 0x0ff00000,  # reserved to be OS-specific flags bigs
}

shdr_flags = {
    'write':           0x1,
    'alloc':           0x2,
    'execinstr':       0x4,
    'maskos':   0x0f000000,
    'maskproc': 0xf0000000,
}


def _struct_pos(struct_definition, member):
  # return [ elt for elt, elt_type in struct_definition ].index(member)
  for i in xrange(len(struct_definition)):
    if struct_definition[i][0] == member:
      return i
  return -1


class ElfException(exceptions.Exception):
  pass


class Ehdr(object):
  """class reprsenting an ELF Ehdr"""
  pass


class Phdr(object):
  """class reprsenting an ELF Phdr"""
  def TypeName(self):
    return phdr_type_name.get(self.type, '@unknown@')

  def __str__(self):
    return '%-15s %8x %8x %8x %8x %8x %4x %8x' % (self.TypeName(),
                                                  self.offset,
                                                  self.vaddr,
                                                  self.paddr,
                                                  self.filesz,
                                                  self.memsz,
                                                  self.flags,
                                                  self.align)


def _MakeXhdr(proto, unpacked, member_and_type, wordsize):
  proto.__dict__['wordsize'] = wordsize
  for elt, elt_type in member_and_type:
    proto.__dict__[elt] = unpacked[_struct_pos(member_and_type, elt)]
  return proto


def _ExtractElfPhdr(data, offset, wordsize):
  """Extract an ELF phdr from data"""
  # convert rawdata to tuple
  phdr = struct.unpack_from(phdr_format_map[wordsize], data, offset)
  # populate class members from tuple
  return _MakeXhdr(Phdr(), phdr, phdr_member_and_type_map[wordsize], wordsize)


def _ExtractElfEhdr(data, offset, wordsize):
  """Extract an ELF ehdr from data"""
  # convert rawdata to tuple
  ehdr = struct.unpack_from(ehdr_format_map[wordsize], data, offset)
  # populate class members from tuple
  return _MakeXhdr(Ehdr(), ehdr, ehdr_member_and_type, wordsize)


def _ExtractWordSize(data):
  """Extract an ELF wordsize from data"""
  if data[:len(ehdr_ident_mag)] != ehdr_ident_mag:
    raise ElfException('bad ELF ident string')
  elf_class = ord(data[ehdr_ident['class']])
  if elf_class > len(ehdr_ident_class) or ehdr_ident_class[elf_class] is None:
    raise ElfException('bad ELF class')
  return ehdr_ident_class[elf_class]


class Elf:
  """Class for parsing ELF headers in an excutable"""
  def __init__(self, elf_str):
    self.elf_str = elf_str
    self.wordsize = _ExtractWordSize(elf_str)
    self.ehdr = _ExtractElfEhdr(self.elf_str, 0, self.wordsize)
    if self.ehdr.phentsize < struct.calcsize(phdr_format_map[self.wordsize]):
      raise ElfException('program header size too small')

  def PhdrList(self):
    pos = self.ehdr.phoff
    size = self.ehdr.phentsize
    return [_ExtractElfPhdr(self.elf_str, pos + i * size, self.wordsize)
            for i in range(self.ehdr.phnum)]



if __name__ == '__main__':
  print elf32_ehdr_format
  print elf64_ehdr_format
  print ehdr_format_map
  print phdr_format_map
  print shdr_format_map
  print ehdr_type
  print ehdr_type_name
  print ehdr_machine
  print ehdr_machine_name
  print ehdr_version
  print ehdr_version_name
  print ehdr_ident
  print ehdr_ident_class
  print ehdr_ident_data
  print phdr_type
  print phdr_type_name
  print phdr_flags
  print shdr_flags
