# -*- coding: utf-8 -*-
# Copyright 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Qemu is used to help with executing and debugging non-x86_64  binaries."""

from __future__ import print_function

import array
import errno
import os

from chromite.lib import cros_logging as logging
from chromite.lib import osutils


class Qemu(object):
  """Framework for running tests via qemu"""

  # The binfmt register format looks like:
  # :name:type:offset:magic:mask:interpreter:flags
  _REGISTER_FORMAT = r':%(name)s:M::%(magic)s:%(mask)s:%(interp)s:%(flags)s'

  # Require enough data to read the Ehdr of the ELF.
  _MIN_ELF_LEN = 64

  # Tuples of (magic, mask) for an arch.  Most only need to identify by the Ehdr
  # fields: e_ident (16 bytes), e_type (2 bytes), e_machine (2 bytes).
  _MAGIC_MASK = {
      'aarch64':
          (r'\x7f\x45\x4c\x46\x02\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x02\x00\xb7\x00',
           r'\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff'
           r'\xfe\xff\xff\xff'),
      'alpha':
          (r'\x7f\x45\x4c\x46\x02\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x02\x00\x26\x90',
           r'\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff'
           r'\xfe\xff\xff\xff'),
      'arm':
          (r'\x7f\x45\x4c\x46\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x02\x00\x28\x00',
           r'\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff'
           r'\xfe\xff\xff\xff'),
      'armeb':
          (r'\x7f\x45\x4c\x46\x01\x02\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x02\x00\x28',
           r'\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff'
           r'\xff\xfe\xff\xff'),
      'm68k':
          (r'\x7f\x45\x4c\x46\x01\x02\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x02\x00\x04',
           r'\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff'
           r'\xff\xfe\xff\xff'),
      # For mips targets, we need to scan e_flags.  But first we have to skip:
      # e_version (4 bytes), e_entry/e_phoff/e_shoff (4 or 8 bytes).
      'mips':
          (r'\x7f\x45\x4c\x46\x01\x02\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x02\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x00\x00\x00\x00\x00\x10\x00',
           r'\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff'
           r'\xff\xfe\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x00\x00\x00\x00\x00\xf0\x20'),
      'mipsel':
          (r'\x7f\x45\x4c\x46\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x02\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x00\x00\x00\x00\x10\x00\x00',
           r'\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff'
           r'\xfe\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x00\x00\x00\x20\xf0\x00\x00'),
      'mipsn32':
          (r'\x7f\x45\x4c\x46\x01\x02\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x02\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x00\x00\x00\x00\x00\x00\x20',
           r'\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff'
           r'\xff\xfe\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x00\x00\x00\x00\x00\xf0\x20'),
      'mipsn32el':
          (r'\x7f\x45\x4c\x46\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x02\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x00\x00\x00\x20\x00\x00\x00',
           r'\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff'
           r'\xfe\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x00\x00\x00\x20\xf0\x00\x00'),
      'mips64':
          (r'\x7f\x45\x4c\x46\x02\x02\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x02\x00\x08',
           r'\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff'
           r'\xff\xfe\xff\xff'),
      'mips64el':
          (r'\x7f\x45\x4c\x46\x02\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x02\x00\x08\x00',
           r'\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff'
           r'\xfe\xff\xff\xff'),
      'ppc':
          (r'\x7f\x45\x4c\x46\x01\x02\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x02\x00\x14',
           r'\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff'
           r'\xff\xfe\xff\xff'),
      'sparc':
          (r'\x7f\x45\x4c\x46\x01\x02\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x02\x00\x12',
           r'\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff'
           r'\xff\xfe\xff\xff'),
      'sparc64':
          (r'\x7f\x45\x4c\x46\x02\x02\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x02\x00\x2b',
           r'\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff'
           r'\xff\xfe\xff\xff'),
      's390x':
          (r'\x7f\x45\x4c\x46\x02\x02\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x02\x00\x16',
           r'\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff'
           r'\xff\xfe\xff\xff'),
      'sh4':
          (r'\x7f\x45\x4c\x46\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x02\x00\x2a\x00',
           r'\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff'
           r'\xfe\xff\xff\xff'),
      'sh4eb':
          (r'\x7f\x45\x4c\x46\x01\x02\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
           r'\x00\x02\x00\x2a',
           r'\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff'
           r'\xff\xfe\xff\xff'),
  }

  _BINFMT_PATH = '/proc/sys/fs/binfmt_misc'
  _BINFMT_REGISTER_PATH = os.path.join(_BINFMT_PATH, 'register')

  def __init__(self, sysroot, arch=None):
    if arch is None:
      arch = self.DetectArch(None, sysroot)
    self.arch = arch
    self.sysroot = sysroot
    self.name = 'qemu-%s' % self.arch
    self.build_path = os.path.join('/build', 'bin', self.name)
    self.binfmt_path = os.path.join(self._BINFMT_PATH, self.name)

  @classmethod
  def DetectArch(cls, prog, sysroot):
    """Figure out which qemu wrapper is best for this target"""
    def MaskMatches(bheader, bmagic, bmask):
      """Apply |bmask| to |bheader| and see if it matches |bmagic|

      The |bheader| array may be longer than the |bmask|; in which case we
      will only compare the number of bytes that |bmask| takes up.
      """
      # This algo is what the kernel uses.
      return all(((header_byte ^ magic_byte) & mask_byte) == 0x00
                 for header_byte, magic_byte, mask_byte in
                 zip(bheader[0:len(bmask)], bmagic, bmask))

    if prog is None:
      # Common when doing a global setup.
      prog = '/'

    for path in (prog, '/sbin/ldconfig', '/bin/sh', '/bin/dash', '/bin/bash'):
      path = os.path.join(sysroot, path.lstrip('/'))
      if os.path.islink(path) or not os.path.isfile(path):
        continue

      # Read the header of the ELF first.
      matched_arch = None
      with open(path, 'rb') as f:
        header = f.read(cls._MIN_ELF_LEN)
        if len(header) == cls._MIN_ELF_LEN:
          bheader = array.array('B', header)

          # Walk all the magics and see if any of them match this ELF.
          for arch, magic_mask in cls._MAGIC_MASK.items():
            magic = magic_mask[0].decode('string_escape')
            bmagic = array.array('B', magic)
            mask = magic_mask[1].decode('string_escape')
            bmask = array.array('B', mask)

            if MaskMatches(bheader, bmagic, bmask):
              # Make sure we do not have ambiguous magics as this will
              # also confuse the kernel when it tries to find a match.
              if not matched_arch is None:
                raise ValueError('internal error: multiple masks matched '
                                 '(%s & %s)' % (matched_arch, arch))
              matched_arch = arch

      if not matched_arch is None:
        return matched_arch

  @staticmethod
  def inode(path):
    """Return the inode for |path| (or -1 if it doesn't exist)"""
    try:
      return os.stat(path).st_ino
    except OSError as e:
      if e.errno == errno.ENOENT:
        return -1
      raise

  def Install(self, sysroot=None):
    """Install qemu into |sysroot| safely"""
    if sysroot is None:
      sysroot = self.sysroot

    # Copying strategy:
    # Compare /usr/bin/qemu inode to /build/$board/build/bin/qemu; if
    # different, hard link to a temporary file, then rename temp to target.
    # This should ensure that once $QEMU_SYSROOT_PATH exists it will always
    # exist, regardless of simultaneous test setups.
    paths = (
        ('/usr/bin/%s' % self.name,
         sysroot + self.build_path),
        ('/usr/bin/qemu-binfmt-wrapper',
         sysroot + self.build_path + '-binfmt-wrapper'),
    )

    for src_path, sysroot_path in paths:
      src_path = os.path.normpath(src_path)
      sysroot_path = os.path.normpath(sysroot_path)
      if self.inode(sysroot_path) != self.inode(src_path):
        # Use hardlinks so that the process is atomic.
        temp_path = '%s.%s' % (sysroot_path, os.getpid())
        os.link(src_path, temp_path)
        os.rename(temp_path, sysroot_path)
        # Clear out the temp path in case it exists (another process already
        # swooped in and created the target link for us).
        try:
          os.unlink(temp_path)
        except OSError as e:
          if e.errno != errno.ENOENT:
            raise

  @classmethod
  def GetRegisterBinfmtStr(cls, arch, name, interp):
    """Get the string used to pass to the kernel for registering the format

    Args:
      arch: The architecture to get the register string
      name: The name to use for registering
      interp: The name for the interpreter

    Returns:
      A string ready to pass to the register file
    """
    magic, mask = cls._MAGIC_MASK[arch]

    # We need to decode the escape sequences as the kernel has a limit on
    # the register string (256 bytes!).  However, we can't decode two chars:
    # NUL bytes (since the kernel uses strchr and friends) and colon bytes
    # (since we use that as the field separator).
    # TODO: Once this lands, and we drop support for older kernels, we can
    # probably drop this workaround too.  https://lkml.org/lkml/2014/9/1/181
    magic = magic.decode('string_escape')
    mask = mask.decode('string_escape')

    # Further way of data packing: if the mask and magic use 0x00 for the same
    # byte, then turn the magic into something else.  This way the magic can
    # be written in raw form, but the mask will still cancel it out.
    magic = ''.join([
        '!' if (magic_byte == '\x00' and mask_byte == '\x00') else magic_byte
        for magic_byte, mask_byte in zip(magic, mask)
    ])

    # New repack the bytes.
    def _SemiEncode(s):
      return s.replace('\x00', r'\x00').replace(':', '\x3a')
    magic = _SemiEncode(magic)
    mask = _SemiEncode(mask)

    return cls._REGISTER_FORMAT % {
        'name': name,
        'magic': magic,
        'mask': mask,
        'interp': '%s-binfmt-wrapper' % interp,
        'flags': 'POC',
    }

  def RegisterBinfmt(self):
    """Make sure qemu has been registered as a format handler

    Prep the binfmt handler. First mount if needed, then unregister any bad
    mappings, and then register our mapping.

    There may still be some race conditions here where one script
    de-registers and another script starts executing before it gets
    re-registered, however it should be rare.
    """
    if not os.path.exists(self._BINFMT_REGISTER_PATH):
      osutils.Mount('binfmt_misc', self._BINFMT_PATH, 'binfmt_misc', 0)

    if os.path.exists(self.binfmt_path):
      interp = 'interpreter %s\n' % self.build_path
      for line in osutils.ReadFile(self.binfmt_path):
        if line == interp:
          break
      else:
        osutils.WriteFile(self.binfmt_path, '-1')

    if not os.path.exists(self.binfmt_path):
      register = self.GetRegisterBinfmtStr(self.arch, self.name,
                                           self.build_path)
      try:
        osutils.WriteFile(self._BINFMT_REGISTER_PATH, register)
      except IOError:
        logging.error('error: attempted to register: (len:%i) %s',
                      len(register), register)
