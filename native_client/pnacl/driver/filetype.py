#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utilities for determining (and overriding) the types of files
"""

import os

import artools
import driver_log
import elftools

LLVM_BITCODE_MAGIC = 'BC\xc0\xde'
LLVM_WRAPPER_MAGIC = '\xde\xc0\x17\x0b'
PNACL_BITCODE_MAGIC = 'PEXE'

class SimpleCache(object):
  """ Cache results of a function using a dictionary. """

  __all_caches = dict()

  @classmethod
  def ClearAllCaches(cls):
    """ Clear cached results from all functions. """
    for d in cls.__all_caches.itervalues():
      d.clear()

  def __init__(self, f):
    SimpleCache.__all_caches[self] = dict()
    self.cache = SimpleCache.__all_caches[self]
    self.func = f

  def __call__(self, *args):
    if args in self.cache:
      return self.cache[args]
    else:
      result = self.func(*args)
      self.cache[args] = result
      return result

  def __repr__(self):
    return self.func.__doc__

  def OverrideValue(self, value, *args):
    """ Force a function call with |args| to return |value|. """
    self.cache[args] = value

  def ClearCache(self):
    """ Clear cached results for one instance (function). """
    self.cache.clear()


@SimpleCache
def IsNative(filename):
  return (IsNativeObject(filename) or
          IsNativeDSO(filename) or
          IsNativeArchive(filename))

@SimpleCache
def IsNativeObject(filename):
  return FileType(filename) == 'o'

@SimpleCache
def IsNativeDSO(filename):
  return FileType(filename) == 'so'

@SimpleCache
def IsPll(filename):
  return FileType(filename) == 'pll'

@SimpleCache
def GetBitcodeMagic(filename):
  fp = driver_log.DriverOpen(filename, 'rb')
  header = fp.read(4)
  driver_log.DriverClose(fp)
  return header

def IsLLVMBitcodeWrapperHeader(data):
  return data[:4] == LLVM_WRAPPER_MAGIC

@SimpleCache
def IsLLVMWrappedBitcode(filename):
  return IsLLVMBitcodeWrapperHeader(GetBitcodeMagic(filename))

def IsPNaClBitcodeHeader(data):
  return data[:4] == PNACL_BITCODE_MAGIC

@SimpleCache
def IsPNaClBitcode(filename):
  return IsPNaClBitcodeHeader(GetBitcodeMagic(filename))

def IsLLVMRawBitcodeHeader(data):
  return data[:4] == LLVM_BITCODE_MAGIC

@SimpleCache
def IsLLVMBitcode(filename):
  header = GetBitcodeMagic(filename)
  return IsLLVMRawBitcodeHeader(header) or IsLLVMBitcodeWrapperHeader(header)

@SimpleCache
def IsArchive(filename):
  return artools.IsArchive(filename)

@SimpleCache
def IsBitcodeArchive(filename):
  filetype = FileType(filename)
  return filetype == 'archive-bc'

@SimpleCache
def IsNativeArchive(filename):
  return IsArchive(filename) and not IsBitcodeArchive(filename)


@SimpleCache
def IsELF(filename):
  return elftools.IsELF(filename)

@SimpleCache
def GetELFType(filename):
  """ ELF type as determined by ELF metadata """
  assert(elftools.IsELF(filename))
  elfheader = elftools.GetELFHeader(filename)
  elf_type_map = {
    'EXEC': 'nexe',
    'REL' : 'o',
    'DYN' : 'so'
  }
  return elf_type_map[elfheader.type]


# Parses a linker script to determine additional ld arguments specified.
# Returns a list of linker arguments.
#
# For example, if the linker script contains
#
#     GROUP ( libc.so.6 libc_nonshared.a  AS_NEEDED ( ld-linux.so.2 ) )
#
# Then this function will return:
#
#     ['--start-group', '-l:libc.so.6', '-l:libc_nonshared.a',
#      '--as-needed', '-l:ld-linux.so.2', '--no-as-needed', '--end-group']
#
# Returns None on any parse error.
def ParseLinkerScript(filename):
  fp = driver_log.DriverOpen(filename, 'rb')

  ret = []
  stack = []
  expect = ''  # Expected next token
  while True:
    token = GetNextToken(fp)
    if token is None:
      # Tokenization error
      return None

    if not token:
      # EOF
      break

    if expect:
      if token == expect:
        expect = ''
        continue
      else:
        return None

    if not stack:
      if token == 'INPUT':
        expect = '('
        stack.append(token)
      elif token == 'GROUP':
        expect = '('
        ret.append('--start-group')
        stack.append(token)
      elif token == 'OUTPUT_FORMAT':
        expect = '('
        stack.append(token)
      elif token == 'EXTERN':
        expect = '('
        stack.append(token)
      elif token == ';':
        pass
      else:
        return None
    else:
      if token == ')':
        section = stack.pop()
        if section == 'AS_NEEDED':
          ret.append('--no-as-needed')
        elif section == 'GROUP':
          ret.append('--end-group')
      elif token == 'AS_NEEDED':
        expect = '('
        ret.append('--as-needed')
        stack.append('AS_NEEDED')
      elif stack[-1] == 'OUTPUT_FORMAT':
        # Ignore stuff inside OUTPUT_FORMAT
        pass
      elif stack[-1] == 'EXTERN':
        ret.append('--undefined=' + token)
      else:
        ret.append('-l:' + token)

  fp.close()
  return ret


# Get the next token from the linker script
# Returns: ''   for EOF.
#          None on error.
def GetNextToken(fp):
  token = ''
  while True:
    ch = fp.read(1)

    if not ch:
      break

    # Whitespace terminates a token
    # (but ignore whitespace before the token)
    if ch in (' ', '\t', '\n', '\r'):
      if token:
        break
      else:
        continue

    # ( and ) are tokens themselves (or terminate existing tokens)
    if ch in ('(',')'):
      if token:
        fp.seek(-1, os.SEEK_CUR)
        break
      else:
        token = ch
        break

    token += ch
    if token.endswith('/*'):
      if not ReadPastComment(fp, '*/'):
        return None
      token = token[:-2]

  return token

def ReadPastComment(fp, terminator):
  s = ''
  while True:
    ch = fp.read(1)
    if not ch:
      return False
    s += ch
    if s.endswith(terminator):
      break

  return True

def IsLinkerScript(filename):
  _, ext = os.path.splitext(filename)
  return (len(ext) > 0 and ext[1:] in ('o', 'so', 'a', 'po', 'pa', 'x')
          and not IsELF(filename)
          and not IsArchive(filename)
          and not IsLLVMBitcode(filename)
          and ParseLinkerScript(filename) is not None)


# If FORCED_FILE_TYPE is set, FileType() will return FORCED_FILE_TYPE for all
# future input files. This is useful for the "as" incarnation, which
# needs to accept files of any extension and treat them as ".s" (or ".ll")
# files. Also useful for gcc's "-x", which causes all files between the
# current -x and the next -x to be treated in a certain way.
FORCED_FILE_TYPE = None
def SetForcedFileType(t):
  global FORCED_FILE_TYPE
  FORCED_FILE_TYPE = t

def GetForcedFileType():
  return FORCED_FILE_TYPE

def ForceFileType(filename, newtype = None):
  if newtype is None:
    if FORCED_FILE_TYPE is None:
      return
    newtype = FORCED_FILE_TYPE
  FileType.OverrideValue(newtype, filename)

def ClearFileTypeCaches():
  """ Clear caches for all filetype functions (externally they must all be
      cleared together because they can call each other)
  """
  SimpleCache.ClearAllCaches()

# File Extension -> Type string
# TODO(pdox): Add types for sources which should not be preprocessed.
ExtensionMap = {
  'c'   : 'c',
  'i'   : 'c',    # C, but should not be preprocessed.

  'cc'  : 'c++',
  'cp'  : 'c++',
  'cxx' : 'c++',
  'cpp' : 'c++',
  'CPP' : 'c++',
  'c++' : 'c++',
  'C'   : 'c++',
  'ii'  : 'c++',  # C++, but should not be preprocessed.

  'h'   : 'c-header',
  'hpp' : 'c++-header',

  'm'   : 'objc',  # .m = "Objective-C source file"

  'll'  : 'll',
  'bc'  : 'po',
  'po'  : 'po',   # .po = "Portable object file"
  'pexe': 'pexe', # .pexe = "Portable executable"
  'asm' : 'S',
  'S'   : 'S',
  'sx'  : 'S',
  's'   : 's',
  'o'   : 'o',
  'os'  : 'o',
  'so'  : 'so',
  'nexe': 'nexe',
}

def IsSourceType(filetype):
  return filetype in ('c','c++','objc')

def IsHeaderType(filetype):
  return filetype in ('c-header', 'c++-header')

# The SimpleCache decorator is required for correctness, due to the
# ForceFileType mechanism.
@SimpleCache
def FileType(filename):
  # Auto-detect bitcode files, since we can't rely on extensions
  ext = filename.split('.')[-1]

  # TODO(pdox): We open and read the the first few bytes of each file
  #             up to 4 times, when we only need to do it once. The
  #             OS cache prevents us from hitting the disk, but this
  #             is still slower than it needs to be.
  if IsArchive(filename):
    return artools.GetArchiveType(filename)

  if elftools.IsELF(filename):
    return GetELFType(filename)

  # If this is LLVM bitcode, we don't have a good way of determining if it
  # is an object file or a non-finalized program, so just say 'po' for now.
  if IsLLVMBitcode(filename):
    return 'po'

  if IsPNaClBitcode(filename):
    # Although this file has the same extension as a native ".so", it actually
    # is in a portable format and should be handled slightly differently.
    if ext == 'so':
      return 'pll'
    return 'pexe'

  if IsLinkerScript(filename):
    return 'ldscript'

  # Use the file extension if it is recognized
  if ext in ExtensionMap:
    return ExtensionMap[ext]

  driver_log.Log.Fatal('%s: Unrecognized file type', filename)

# Map from GCC's -x file types and this driver's file types.
FILE_TYPE_MAP = {
    'c'                 : 'c',
    'c++'               : 'c++',
    'assembler'         : 's',
    'assembler-with-cpp': 'S',
    'c-header'          : 'c-header',
    'c++-header'        : 'c++-header',
}
FILE_TYPE_MAP_REVERSE = dict([reversed(_tmp) for _tmp in FILE_TYPE_MAP.items()])

def FileTypeToGCCType(filetype):
  return FILE_TYPE_MAP_REVERSE[filetype]

def GCCTypeToFileType(gcctype):
  if gcctype not in FILE_TYPE_MAP:
    driver_log.Log.Fatal('language "%s" not recognized' % gcctype)
  return FILE_TYPE_MAP[gcctype]
