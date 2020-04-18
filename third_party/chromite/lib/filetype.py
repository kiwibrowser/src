# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""File type decoding class for Chromium OS rootfs file bucketing.

This file decodes the type of file based on the contents, filename and other
metadata. The result is a string that represents the file type and subtypes
of the file, separated by slashes (/). The first level is one of the following:
"text", "binary" and "inode". The first two refer to the contents of the file
for regular files, while the third one is used for special files such as
directories, symlinks, block devices, etc.

The file type can have more than one level, for example "binary/elf/static",
"binary/image/png", or "text/conf". See the filetype_unittest.py file for more
examples.

The purpose of this module is to provide a file type that splits the contents
of a Chromium OS build in small buckets, partitioning cases where other standard
classifications keep in the same set.
"""

from __future__ import print_function

import itertools
import magic
import mmap
import os
import re
import stat

from chromite.lib import parseelf


# The buffer size we would use to read files from the disk.
FILE_BUFFER_SIZE = 32 * 1024


def SplitShebang(header):
  """Splits a shebang (#!) into command and arguments.

  Args:
    header: The first line of a shebang file, for example
        "#!/usr/bin/env -uPWD python foo.py\n". The referenced command must be
        an absolute path with optionally some arguments.

  Returns:
    A tuple of strings (command, args) where the first string is the called
    and the second is the list of arguments as passed in the header.

  Raises:
    ValueError if the passed header is not a valid shebang line.
  """
  m = re.match(r'#!\s*(/[a-z/0-9\.-]+)\s*(.*)$', header)
  if m:
    return m.group(1), m.group(2).strip()
  raise ValueError('shebang (#!) line expected')


class FileTypeDecoder(object):
  """Class to help decode the type of a file.

  This class implements a single GetType() method that decodes the type of a
  file based on the contents and metadata. This class holds some global data
  shared between several calls to that method.
  """

  # Whitelist of mime types and their mapping to file type.
  MIME_TYPE_MAPPING = {
      'application/x-gzip': 'binary/compressed/gzip',
      'application/x-bzip2': 'binary/compressed/bzip2',
      'application/x-xz': 'binary/compressed/xz',

      # Goobuntu magic database returns 'gzip' instead of 'x-gzip'. This
      # supports running dep_tracker outside the chroot for development.
      'application/gzip': 'binary/compressed/gzip',
  }

  def __init__(self, root='/'):
    """Initializes the internal state.

    Args:
      root: Path to the root directory where all the files live. This will be
      assumed as the root directory for absolute symlinks.
    """
    self._root = root
    self._mime = magic.open(magic.MIME_TYPE)
    self._mime.load()

  def __del__(self):
    self._mime.close()

  def GetType(self, rel_path, st=None, elf=None):
    """Return the file type of the passed file.

    Does a best-effort attempt to infer the file type of the passed file. If
    only rel_path is provided, the stat_struct information and parsed ELF data
    will be computed. If the information is already available, such as if the
    ELF file is already parsed, passing st and elf will speed up the file
    detection.

    Args:
      rel_path: The path to the file, used to detect the filetype from the
          contents of the file.
      st: The stat_result struct of the file.
      elf: The result of parseelf.ParseELF().

    Returns:
      A string with the file type classified in categories separated by /. For
      example, a dynamic library will return 'binary/elf/dynamic-so'. If the
      type can't be inferred it returns None.
    """
    # Analysis based on inode data.
    if st is None:
      st = os.lstat(os.path.join(self._root, rel_path))
    if stat.S_ISDIR(st.st_mode):
      return 'inode/directory'
    if stat.S_ISLNK(st.st_mode):
      return 'inode/symlink'
    if not stat.S_ISREG(st.st_mode):
      return 'inode/special'
    if st.st_size == 0:
      return 'inode/empty'

    # Analysis based on the ELF header and contents.
    if elf:
      return self._GetELFType(elf)

    # Analysis based on the file contents.
    try:
      with open(os.path.join(self._root, rel_path), 'rb') as fobj:
        fmap = mmap.mmap(fobj.fileno(), 0, prot=mmap.PROT_READ)
        result = self._GetTypeFromContent(rel_path, fobj, fmap)
        fmap.close()
        return result
    except IOError:
      return

  def _GetTypeFromContent(self, rel_path, fobj, fmap):
    """Return the file path based on the file contents.

    This helper function detect the file type based on the contents of the file.

    Args:
      rel_path: The path to the file, used to detect the filetype from the
          contents of the file.
      fobj: a file() object for random access to rel_path.
      fmap: a mmap object mapping the whole rel_path file for reading.
    """

    # Detect if the file is binary based on the presence of non-ASCII chars. We
    # include some the first 32 chars often used in text files but we exclude
    # the rest.
    ascii_chars = '\x07\x08\t\n\x0c\r\x1b' + ''.join(map(chr, range(32, 128)))
    is_binary = any(bool(chunk.translate(None, ascii_chars))
                    for chunk in iter(lambda: fmap.read(FILE_BUFFER_SIZE), ''))

    # We use the first part of the file in several checks.
    fmap.seek(0)
    first_kib = fmap.read(1024)

    # Binary files.
    if is_binary:
      # The elf argument was not passed, so compute it now if the file is an
      # ELF.
      if first_kib.startswith('\x7fELF'):
        return self._GetELFType(parseelf.ParseELF(self._root, rel_path,
                                                  parse_symbols=False))

      if first_kib.startswith('MZ\x90\0'):
        return 'binary/dos-bin'

      if len(first_kib) >= 512 and first_kib[510:512] == '\x55\xaa':
        return 'binary/bootsector/x86'

      # Firmware file depend on the technical details of the device they run on,
      # so there's no easy way to detect them. We use the filename to guess that
      # case.
      if '/firmware/' in rel_path and (
          rel_path.endswith('.fw') or
          rel_path[-4:] in ('.bin', '.cis', '.csp', '.dsp')):
        return 'binary/firmware'

      # TZif (timezone) files. See tzfile(5) for details.
      if (first_kib.startswith('TZif' + '\0' * 16) or
          first_kib.startswith('TZif2' + '\0' * 15) or
          first_kib.startswith('TZif3' + '\0' * 15)):
        return 'binary/tzfile'

      # Whitelist some binary mime types.
      fobj.seek(0)
      # _mime.descriptor() will close the passed file descriptor.
      mime_type = self._mime.descriptor(os.dup(fobj.fileno()))
      if mime_type.startswith('image/'):
        return 'binary/' + mime_type
      if mime_type in self.MIME_TYPE_MAPPING:
        return self.MIME_TYPE_MAPPING[mime_type]

      # Other binary files.
      return 'binary'

    # Text files.
    # Read the first couple of lines used in the following checks. This will
    # only read the required lines, with the '\n' char at the end of each line
    # except on the last one if it is not present on that line. At this point
    # we know that the file is not empty, so at least one line existst.
    fmap.seek(0)
    first_lines = list(itertools.islice(iter(fmap.readline, ''), 0, 10))
    head_line = first_lines[0]

    # #! or "shebangs". Only those files with a single line are considered
    # shebangs. Some files start with "#!" but are other kind of files, such
    # as python or bash scripts.
    try:
      prog_name, args = SplitShebang(head_line)
      if len(first_lines) == 1:
        return 'text/shebang'

      prog_name = os.path.basename(prog_name)
      args = args.split()
      if prog_name == 'env':
        # If "env" is called, we skip all the arguments passed to env (flags,
        # VAR=value) and treat the program name as the program to use.
        for i, arg in enumerate(args):
          if arg == '--' and (i + 1) < len(args):
            prog_name = args[i + 1]
            break
          if not arg or arg[0] == '-' or '=' in arg:
            continue
          prog_name = arg
          break

      # Strip the version number from comon programs like "python2.7".
      prog_name = prog_name.rstrip('0123456789-.')

      if prog_name in ('awk', 'bash', 'dash', 'ksh', 'perl', 'python', 'sh'):
        return 'text/script/' + prog_name
      # Other unknown script.
      return 'text/script'
    except ValueError:
      pass

    # PEM files.
    if head_line.strip() == '-----BEGIN CERTIFICATE-----':
      return 'text/pem/cert'
    if head_line.strip() == '-----BEGIN RSA PRIVATE KEY-----':
      return 'text/pem/rsa-private'

    # Linker script.
    if head_line.strip() == '/* GNU ld script':
      return 'text/ld-script'

    # Protobuf files.
    if rel_path.endswith('.proto'):
      return 'text/proto'

    if len(first_lines) == 1:
      if re.match(r'[0-9\.]+$', head_line):
        return 'text/oneline/number'
      return 'text/oneline'

    return 'text'

  @staticmethod
  def _GetELFType(elf):
    """Returns the file type for ELF files.

    Args:
      elf: The result of parseelf.ParseELF().
    """
    if elf['type'] == 'ET_REL':
      elf_type = 'object'
    elif (not '.dynamic' in elf['sections'] and
          not 'PT_DYNAMIC' in elf['segments']):
      elf_type = 'static'
    else:
      if elf['is_lib']:
        elf_type = 'dynamic-so'
      else:
        elf_type = 'dynamic-bin'
    return 'binary/elf/' + elf_type

  @classmethod
  def DecodeFile(cls, path):
    """Decodes the file type of the passed file.

    This function is a wrapper to the FileTypeDecoder class to decode the type
    of a single file. If you need to decode multiple files please use
    FileTypeDecoder class instead.

    Args:
      path: The path to the file or directory.

    Returns:
      A string with the decoded file type or None if it couldn't be decoded.
    """
    return cls('.').GetType(path)
