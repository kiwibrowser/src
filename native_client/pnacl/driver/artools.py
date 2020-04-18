#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Tool for reading archive (.a) files
# For information about the archive file format, see:
#   http://en.wikipedia.org/wiki/Ar_(Unix)

import driver_log
import elftools
import pathtools

# See above link to wiki entry on archive format.
AR_MAGIC = '!<arch>\n'
# Thin archives are like normal archives except that there are only
# indirect references to each member (the data is not embedded).
# See manpage for a description of this.
THIN_MAGIC = '!<thin>\n'

# filetype.IsArchive calls this IsArchive. Top-level tools should prefer
# filetype.IsArchive, both for consistency (i.e., all checks for file type come
# from that library), and because its results are cached.
def IsArchive(filename):
  fp = driver_log.DriverOpen(filename, "rb")
  magic = fp.read(len(AR_MAGIC))
  fp.close()
  return magic in [AR_MAGIC, THIN_MAGIC]


def GetMemberFilename(member, strtab_data):
  """ Get the real filename of the archive member. """
  if not member.is_long_name:
    return member.name.strip()
  else:
    # GNU style long filenames are /[index]
    # where index is a position within the strtab_data.
    # Filter out non-digits
    name = ''.join([c for c in member.name if c.isdigit()])
    name_index = int(name)
    name_data = strtab_data[name_index:]
    name_data = name_data.split('\n', 2)[0]
    assert (name_data.endswith('/'))
    return name_data[:-1]


def GetThinArchiveData(archive_filename, member, strtab_data):
  # Get member's filename (relative to the archive) and open the member
  # ourselves to check the data.
  member_filename = GetMemberFilename(member, strtab_data)
  member_filename = pathtools.join(
      pathtools.dirname(pathtools.abspath(archive_filename)),
      member_filename)
  member_fp = driver_log.DriverOpen(member_filename, 'rb')
  data = member_fp.read(member.size)
  member_fp.close()
  return data


def GetArchiveType(filename):
  fp = driver_log.DriverOpen(filename, "rb")

  # Read the archive magic header
  magic = fp.read(len(AR_MAGIC))
  assert(magic in [AR_MAGIC, THIN_MAGIC])

  # Find a regular file or symbol table
  empty_file = True
  found_type = ''
  strtab_data = ''
  while not found_type:
    member = MemberHeader(fp)
    if member.error == 'EOF':
      break
    elif member.error:
      driver_log.Log.Fatal("%s: %s", filename, member.error)

    empty_file = False

    if member.is_regular_file:
      if not magic == THIN_MAGIC:
        data = fp.read(member.size)
      else:
        # For thin archives, do not read the data section.
        # We instead must get at the member indirectly.
        data = GetThinArchiveData(filename, member, strtab_data)

      if data.startswith('BC'):
        found_type = 'archive-bc'
      else:
        elf_header = elftools.DecodeELFHeader(data, filename)
        if elf_header:
          found_type = 'archive-%s' % elf_header.arch
    elif member.is_strtab:
      # We need the strtab data to get long filenames.
      data = fp.read(member.size)
      strtab_data = data
      continue
    else:
      # Other symbol tables we can just skip ahead.
      data = fp.read(member.size)
      continue

  if empty_file:
    # Empty archives are treated as bitcode ones.
    found_type = 'archive-bc'
  elif not found_type:
    driver_log.Log.Fatal("%s: Unable to determine archive type", filename)

  fp.close()
  return found_type


class MemberHeader(object):
  def __init__(self, fp):
    self.error = ''
    header = fp.read(60)
    if len(header) == 0:
      self.error = "EOF"
      return

    if len(header) != 60:
      self.error = 'Short count reading archive member header'
      return

    self.name = header[0:16]
    self.size = header[48:48 + 10]
    self.fmag = header[58:60]

    if self.fmag != '`\n':
      self.error = 'Invalid archive member header magic string %s' % header
      return

    self.size = int(self.size)

    self.is_svr4_symtab = (self.name == '/               ')
    self.is_llvm_symtab = (self.name == '#_LLVM_SYM_TAB_#')
    self.is_bsd4_symtab = (self.name == '__.SYMDEF SORTED')
    self.is_strtab      = (self.name == '//              ')
    self.is_regular_file = not (self.is_svr4_symtab or
                                self.is_llvm_symtab or
                                self.is_bsd4_symtab or
                                self.is_strtab)

    # BSD style long names (not supported)
    if self.name.startswith('#1/'):
      self.error = "BSD-style long file names not supported"
      return

    # If it's a GNU long filename, note this.  We use this for thin archives.
    self.is_long_name = (self.is_regular_file and self.name.startswith('/'))

    if self.is_regular_file and not self.is_long_name:
      # Filenames end with '/' and are padded with spaces up to 16 bytes
      self.name = self.name.strip()[:-1]
