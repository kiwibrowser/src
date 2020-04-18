#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of the pnacl driver.

This tests various driver_tools / driver_log utility functions.
"""

from driver_env import env
import driver_log
import driver_temps
import driver_tools
import pathtools

import os
import struct
import tempfile
import unittest


class TestTempNamegenFileWipe(unittest.TestCase):

  def setUp(self):
    self.tempfiles = []

  def tearDown(self):
    # Just in case the test fails, try to wipe the temp files
    # ourselves.
    for t in self.tempfiles:
      if os.path.exists(t.name):
        os.remove(t.name)

  def nameGenTemps(self):
    temp_out = pathtools.normalize(tempfile.NamedTemporaryFile().name)
    temp_in1 = pathtools.normalize(tempfile.NamedTemporaryFile().name)
    temp_in2 = pathtools.normalize(tempfile.NamedTemporaryFile().name)
    namegen = driver_tools.TempNameGen([temp_in1, temp_in2],
                                       temp_out)
    t_gen_out = namegen.TempNameForOutput('pexe')
    t_gen_in = namegen.TempNameForInput(temp_in1, 'bc')

    # Touch/create them (append to not truncate).
    fp = driver_log.DriverOpen(t_gen_out, 'a')
    self.assertTrue(os.path.exists(t_gen_out))
    self.tempfiles.append(fp)
    fp.close()

    fp2 = driver_log.DriverOpen(t_gen_in, 'a')
    self.assertTrue(os.path.exists(t_gen_in))
    self.tempfiles.append(fp2)
    fp2.close()
    return t_gen_out, t_gen_in


  def test_NamegenGetsWiped(self):
    """Test that driver-generated temp files can get wiped
    (no path canonicalization problems, etc.).
    This assumes that the default option is to not "-save-temps".
    """
    t_gen_out, t_gen_in = self.nameGenTemps()
    # Now wipe!
    driver_temps.TempFiles.wipe()
    # They are gone!
    self.assertFalse(os.path.exists(t_gen_out))
    self.assertFalse(os.path.exists(t_gen_in))


  def test_SaveTempsNotWiped(self):
    """Test that driver-generated temp files don't get wiped w/ "-save-temps".
    """
    env.push()
    env.set('SAVE_TEMPS', '1')
    t_gen_out, t_gen_in = self.nameGenTemps()
    # Now wipe!
    driver_temps.TempFiles.wipe()
    env.pop()
    # They are *not* gone.
    self.assertTrue(os.path.exists(t_gen_out))
    self.assertTrue(os.path.exists(t_gen_in))
    # However, tearDown() should clean them up since we registered them
    # with the test harness's own tempfiles list.


# Note: We cannot use NamedTemporaryFile unfortunately, because it is not
# ensured that the created temp file can be reopened by its name. So,
# instead, we define our own temporay file. Client must not delete the file.
class NamedTemporaryFile(object):
  def __init__(self, suffix):
    desc, path = tempfile.mkstemp(
        prefix='pnacl_driver_tools_test_', suffix=suffix)
    os.close(desc)
    self._path = path

  def __del__(self):
    self._close()

  def __enter__(self):
    return self._path

  def __exit__(self, exc_type, exc_val, exc_tb):
    try:
      self._close()
    except:
      # We raise the exception for close(), iff the with-block is successfully
      # finished.
      if exc_type is None:
        raise
    return False  # re-raise the passed exception if necessary.

  def _close(self):
    if self._path:
      os.remove(self._path)
      self._path = None


class DriverToolsTest(unittest.TestCase):

  def setUp(self):
    env.push()

  def tearDown(self):
    env.pop()

  def _WriteDummyElfHeader(self, stream):
    header = struct.pack(
        '<4sBBBBB7xHH',
        '\177ELF',  # ELF magic
        1,  # ei_class = ELFCLASS32
        1,  # ei_data = ELFDATA2LSB
        1,  # ei_version = EV_CURRENT
        3,  # ei_osabi = ELFOSABI_LINUX (=ELFOSABI_GNU)
        7,  # ei_abiversion = NACL ABI version
        1,  # e_type = ET_REL
        3)  # e_machine = EM_386
    # Append dummy values as the rest of the ELF header. The size of
    # Elf32_Ehdr is 52.
    header += '\0' * (52 - len(header))
    stream.write(header)

  def _WriteDummyArHeader(self, stream):
    stream.write('!<arch>\n')

  def _WriteDummyArFileHeader(self, stream, name, filesize):
    stream.writelines([
        (name + '/').ljust(16),
        '0'.rjust(12),  # Dummy timestamp
        '0'.rjust(6),  # Dummy ownerid
        '0'.rjust(6),  # Dummy groupid
        '666'.rjust(8),  # Dummy filemode (rw-rw-rw-)
        str(filesize).rjust(10),
        '\x60\x0A'])

  def test_ArchMerge_ObjectFile(self):
    with NamedTemporaryFile(suffix='.o') as path:
      with open(path, 'wb') as stream:
        self._WriteDummyElfHeader(stream)

      # First, ARCH is not set.
      self.assertIsNone(driver_tools.GetArch())

      # After ArchMerge(), ARCH is set.
      driver_tools.ArchMerge(path, True)
      self.assertEquals('X8632', driver_tools.GetArch())

      # The ArchMerge() with the file of the same architecture shouldn't fail.
      driver_tools.ArchMerge(path, True)
      self.assertEquals('X8632', driver_tools.GetArch())

  def test_ArchMerge_ArchiveFile(self):
    with NamedTemporaryFile(suffix='.a') as path:
      # Create dummy archive file.
      with open(path, 'wb') as stream:
        self._WriteDummyArHeader(stream)
        # 52 is the length of dummy ELF header size.
        self._WriteDummyArFileHeader(stream, 'file', filesize=52)
        self._WriteDummyElfHeader(stream)

      # First, ARCH is not set.
      self.assertIsNone(driver_tools.GetArch())

      # After ArchMerge(), ARCH is set.
      driver_tools.ArchMerge(path, True)
      self.assertEquals('X8632', driver_tools.GetArch())

      # The ArchMerge() with the file of the same architecture shouldn't fail.
      driver_tools.ArchMerge(path, True)
      self.assertEquals('X8632', driver_tools.GetArch())

  def test_ArchMerge_Inconsistent(self):
    with NamedTemporaryFile(suffix='.o') as path:
      # Write dummy ELF header for X8632.
      with open(path, 'wb') as stream:
        self._WriteDummyElfHeader(stream)

      # Set ARCH to ARM.
      driver_tools.SetArch('arm')

      # Calling ArchMerge() with must_match=False warns mismatching of the
      # architecture, but not a fatal error.
      driver_tools.ArchMerge(path, False)

      # ARCH is not modified.
      self.assertEquals('ARM', driver_tools.GetArch())

      # Calling ArchMerge() with must_match=True causes a fatal error.
      with self.assertRaises(SystemExit):
        driver_tools.ArchMerge(path, True)

  def test_ArchMerge_NonSfi(self):
    with NamedTemporaryFile(suffix='.o') as path:
      # Write dummy ELF header for X8632.
      with open(path, 'wb') as stream:
        self._WriteDummyElfHeader(stream)

      # Set ARCH to X8632_NONSFI.
      driver_tools.SetArch('x86-32-nonsfi')

      # The binary format for _NONSFI architecture is compatible with the one
      # for SFI.
      driver_tools.ArchMerge(path, True)

      # ARCH is not modified.
      self.assertEquals('X8632_NONSFI', driver_tools.GetArch())


if __name__ == '__main__':
  unittest.main()
