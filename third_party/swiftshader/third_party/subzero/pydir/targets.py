#!/usr/bin/env python2

from collections import namedtuple
import glob


# Why have 'cross_headers':
# For some reason, clang doesn't know how to find some of the libstdc++
# headers (c++config.h). Manually add in one of the paths:
# https://llvm.org/bugs/show_bug.cgi?id=22937
# Otherwise, we could assume the system has arm-linux-gnueabihf-g++ and
# use that instead of clang, but so far we've just been using clang for
# the unsandboxed build.
def FindARMCrossInclude():
  return glob.glob(
      '/usr/arm-linux-gnueabihf/include/c++/*/arm-linux-gnueabihf')[-1]

def FindMIPSCrossInclude():
  globs = glob.glob('/usr/mipsel-linux-gnu/include/c++/*/mipsel-linux-gnu')
  return globs[-1] if globs else '/invalid/mips/include/path'

TargetInfo = namedtuple('TargetInfo',
                        ['target', 'compiler_arch', 'triple', 'llc_flags',
                         'ld_emu', 'sb_emu', 'cross_headers'])

X8632Target = TargetInfo(target='x8632',
                         compiler_arch='x8632',
                         triple='i686-none-linux',
                         llc_flags=['-mcpu=pentium4m'],
                         ld_emu='elf_i386_nacl',
                         sb_emu='elf_i386_nacl',
                         cross_headers=[])

X8664Target = TargetInfo(target='x8664',
                         compiler_arch='x8664',
                         triple='x86_64-none-linux-gnux32',
                         llc_flags=['-mcpu=x86-64'],
                         ld_emu='elf32_x86_64_nacl',
                         sb_emu='elf_x86_64_nacl',
                         cross_headers=[])

ARM32Target = TargetInfo(target='arm32',
                         compiler_arch='armv7',
                         triple='armv7a-none-linux-gnueabihf',
                         llc_flags=['-mcpu=cortex-a9',
                                    '-float-abi=hard',
                                    '-mattr=+neon',
                                    '-arm-enable-dwarf-eh=1'],
                         ld_emu='armelf_nacl',
                         sb_emu='armelf_nacl',
                         cross_headers=['-isystem', FindARMCrossInclude()])

# Investigate:
# ld_emu script mips_nacl is not present in binutils. How to get it?
MIPS32Target = TargetInfo(target='mips32',
                         compiler_arch='mips32',
                         triple='mipsel-linux-gnu',
                         llc_flags=[],
                         ld_emu='mips_nacl',
                         sb_emu='mips_nacl',
                         cross_headers=['-isystem', FindMIPSCrossInclude()])

def ConvertTripleToNaCl(nonsfi_triple):
  return nonsfi_triple[:nonsfi_triple.find('-linux')] + '-nacl'
