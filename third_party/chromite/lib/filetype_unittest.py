# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for the filetype.py module."""

from __future__ import print_function

import os
import stat

from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib import filetype
from chromite.lib import unittest_lib


class SplitShebangTest(cros_test_lib.TestCase):
  """Test the SplitShebang function."""

  def testSimpleCase(self):
    """Test a simple case."""
    self.assertEquals(('/bin/sh', ''), filetype.SplitShebang('#!/bin/sh'))

  def testCaseWithArguments(self):
    """Test a case with arguments."""
    self.assertEquals(('/bin/sh', '-i -c "ls"'),
                      filetype.SplitShebang('#!/bin/sh  -i -c "ls"'))

  def testCaseWithEndline(self):
    """Test a case finished with a newline char."""
    self.assertEquals(('/bin/sh', '-i'),
                      filetype.SplitShebang('#!/bin/sh  -i\n'))

  def testCaseWithSpaces(self):
    """Test a case with several spaces in the line."""
    self.assertEquals(('/bin/sh', '-i'),
                      filetype.SplitShebang('#!  /bin/sh  -i   \n'))

  def testInvalidCases(self):
    """Thes invalid cases."""
    self.assertRaises(ValueError, filetype.SplitShebang, '/bin/sh -i')
    self.assertRaises(ValueError, filetype.SplitShebang, '#!')
    self.assertRaises(ValueError, filetype.SplitShebang, '#!env python')


class FileTypeDecoderTest(cros_test_lib.TempDirTestCase):
  """Test the FileTypeDecoder class."""

  def testSpecialFiles(self):
    """Tests special files, such as symlinks, directories and named pipes."""
    somedir = os.path.join(self.tempdir, 'somedir')
    osutils.SafeMakedirs(somedir)
    self.assertEquals('inode/directory',
                      filetype.FileTypeDecoder.DecodeFile(somedir))

    a_fifo = os.path.join(self.tempdir, 'a_fifo')
    os.mknod(a_fifo, stat.S_IFIFO)
    self.assertEquals('inode/special',
                      filetype.FileTypeDecoder.DecodeFile(a_fifo))

    empty_file = os.path.join(self.tempdir, 'empty_file')
    osutils.WriteFile(empty_file, '')
    self.assertEquals('inode/empty',
                      filetype.FileTypeDecoder.DecodeFile(empty_file))

    a_link = os.path.join(self.tempdir, 'a_link')
    os.symlink('somewhere', a_link)
    self.assertEquals('inode/symlink',
                      filetype.FileTypeDecoder.DecodeFile(a_link))

  def testTextShebangFiles(self):
    """Test shebangs (#!) file decoding based on the executed path."""
    # If the file has only one line is considered a "shebang" rather than a
    # script.
    shebang = os.path.join(self.tempdir, 'shebang')
    osutils.WriteFile(shebang, "#!/bin/python --foo --bar\n")
    self.assertEquals('text/shebang',
                      filetype.FileTypeDecoder.DecodeFile(shebang))

    # A shebang with contents is considered a script.
    script = os.path.join(self.tempdir, 'script')
    osutils.WriteFile(script, "#!/bin/foobar --foo --bar\n\nexit 1\n")
    self.assertEquals('text/script',
                      filetype.FileTypeDecoder.DecodeFile(script))

    bash_script = os.path.join(self.tempdir, 'bash_script')
    osutils.WriteFile(bash_script,
                      "#!/bin/bash --debug\n# Copyright\nexit 42\n")
    self.assertEquals('text/script/bash',
                      filetype.FileTypeDecoder.DecodeFile(bash_script))

    pyscript = os.path.join(self.tempdir, 'pyscript')
    osutils.WriteFile(pyscript,
                      "#!/usr/bin/env PYTHONPATH=/foo python-2.7 -3\n# foo\n")
    self.assertEquals('text/script/python',
                      filetype.FileTypeDecoder.DecodeFile(pyscript))

    perlscript = os.path.join(self.tempdir, 'perlscript')
    osutils.WriteFile(perlscript, "#!/usr/local/bin/perl\n#\n")
    self.assertEquals('text/script/perl',
                      filetype.FileTypeDecoder.DecodeFile(perlscript))

  def testTextPEMFiles(self):
    """Test decoding various PEM files."""
    # A RSA private key (sample from update_engine unittest).
    some_cert = os.path.join(self.tempdir, 'some_cert')
    osutils.WriteFile(some_cert,
                      """-----BEGIN CERTIFICATE-----
MIIDJTCCAo6gAwIBAgIJAP6IycaMXlqsMA0GCSqGSIb3DQEBBQUAMIGLMQswCQYD
VQQGEwJVUzETMBEGA1UECBMKQ2FsaWZvcm5pYTETMBEGA1UEChMKR29vZ2xlIElu
YzESMBAGA1UECxMJQ2hyb21lIE9TMRgwFgYDVQQDEw9PcGVuU1NMIFRlc3QgQ0Ex
JDAiBgkqhkiG9w0BCQEWFXNlY3VyaXR5QGNocm9taXVtLm9yZzAgFw0xMjA1MTcx
OTQ1MjJaGA8yMTEyMDExNDE5NDUyMlowgZ0xCzAJBgNVBAYTAlVTMRMwEQYDVQQI
EwpDYWxpZm9ybmlhMRYwFAYDVQQHEw1Nb3VudGFpbiBWaWV3MRMwEQYDVQQKEwpH
b29nbGUgSW5jMRIwEAYDVQQLEwlDaHJvbWUgT1MxEjAQBgNVBAMTCTEyNy4wLjAu
MTEkMCIGCSqGSIb3DQEJARYVc2VjdXJpdHlAY2hyb21pdW0ub3JnMIGfMA0GCSqG
SIb3DQEBAQUAA4GNADCBiQKBgQC5bxzyvNJFDmyThIGoFoZkN3rlQB8QoR80rS1u
8pLyqW5Vk2A0pNOvcxPrUHAUTgWhikqzymz4a4XoLxat53H/t/XmRYwZ9GVNZocz
Q4naWxtPyPqIBosMLnWu6FHUVO1lTdvhC6Pjw2i1S9Rq3dMsANU1IER4NR8XM+v6
qBg1XQIDAQABo3sweTAJBgNVHRMEAjAAMCwGCWCGSAGG+EIBDQQfFh1PcGVuU1NM
IEdlbmVyYXRlZCBDZXJ0aWZpY2F0ZTAdBgNVHQ4EFgQU+biqy5tbRGlUNLsEtjmy
7v1eYqowHwYDVR0jBBgwFoAUC0E889mD7bn2fXcEtA3HvUimV+0wDQYJKoZIhvcN
AQEFBQADgYEA2fJxpGwDbnUF5Z3mvZ81/pk8fVQdJvV5U93EA29VE1SaFA5S5qYS
zP1Ue0MX+RqMLKjnH+E6yEoo+kYD9rzagnvORefbJeM92SiHgHPeSm8F1nQtGclj
p8izLBlcKgPHwQLKxELmbS/xvt4cyHaLSIy50lLrdJeKtXjqq4PbH3Y=
-----END CERTIFICATE-----
""")
    self.assertEquals('text/pem/cert',
                      filetype.FileTypeDecoder.DecodeFile(some_cert))

    # A RSA private key (sample from vboot_reference unittest).
    rsa_key = os.path.join(self.tempdir, 'rsa_key')
    osutils.WriteFile(rsa_key,
                      """-----BEGIN RSA PRIVATE KEY-----
MIICXAIBAAKBgQCdYBOJIJvGX9vC4E5XD1jb9zJ99FzR4G0n8HNyWy5ZKyy/hi80
ibXpy6QdWcm4wqTvmVjU+20sP4AgzKC65fKyFvvAHUiD4yGr1qWtg4YFUcBbUiXO
CQ66W3AC4g2Ju9C16AzMpBk043bQsUQvxILEumQqQ1VS33uM7Kq8dWpL6QIDAQAB
AoGAb12y1WIu+gWRtWkX5wHkRty6bWmEWbzwYcgFWlJuDQnBg9MICqy8/7Js85w7
ZLTRFQC2XRmDW0GggRVtVHUu9X2jwkHR9+TWza4xAtYcSwDl6VJTHX2ygptrG/n9
qPFinfvnpiP7b2WNjC53V3cnjg3m+1B5zrmFxsVLDMVLQhECQQDN7i1NWZFVNfYa
GT2GSgMpD0nPXA1HHUvFFgnI9xJkBCewHzega+PrrrpMKZZWLpc4YCm3PK9nI8Nk
EmJE5HwNAkEAw6OpiOgWdRaJWx3+XBsFOhz6K86xwV0NpVb6ocrBKU/t0OqP+gZh
B/YBDfwXPr2w5FCwozUs/MrBdoYR3WnsTQJABNn/pzrc+azzx1mg4XEM8gKyMnhw
t6QxDMugH2Pywvh2FuglX1orXHoZWYIBULZ4SZO6Z96+IyfsiocEWasoYQJBALZ/
onO7BM/+0Oz1osSq1Aps45Yf/0OAmW0mITDyIZR3IkJjvSEf+D3j5wHzqn91lmC1
QMFOpoO+ZBA7asjfuXUCQGmHgpC0BuD4S1QlcF0nrVHTG7Y8KZ18s9qPJS3csuGf
or10mrNRF3tyGy8e/sw88a74Q/6v/PgChZHmq6QjOOU=
-----END RSA PRIVATE KEY-----
""")
    self.assertEquals('text/pem/rsa-private',
                      filetype.FileTypeDecoder.DecodeFile(rsa_key))

  def testBinaryELFFiles(self):
    """Test decoding ELF files."""
    liba_so = os.path.join(self.tempdir, 'liba.so')
    unittest_lib.BuildELF(liba_so, ['func_a'])
    self.assertEquals('binary/elf/dynamic-so',
                      filetype.FileTypeDecoder.DecodeFile(liba_so))

    prog = os.path.join(self.tempdir, 'prog')
    unittest_lib.BuildELF(prog,
                          undefined_symbols=['func_a'],
                          used_libs=['a'],
                          executable=True)
    self.assertEquals('binary/elf/dynamic-bin',
                      filetype.FileTypeDecoder.DecodeFile(prog))

    prog_static = os.path.join(self.tempdir, 'prog_static')
    unittest_lib.BuildELF(prog_static, executable=True, static=True)
    self.assertEquals('binary/elf/static',
                      filetype.FileTypeDecoder.DecodeFile(prog_static))

  def testBinaryCompressedFiles(self):
    """Test decoding compressed files."""
    compressed = os.path.join(self.tempdir, 'compressed')

    # `echo hola | gzip -9`
    osutils.WriteFile(compressed,
                      '\x1f\x8b\x08\x00<\xce\x07T\x02\x03\xcb\xc8\xcfI\xe4\x02'
                      '\x00x\xad\xdb\xd1\x05\x00\x00\x00')
    self.assertEquals('binary/compressed/gzip',
                      filetype.FileTypeDecoder.DecodeFile(compressed))

    # `echo hola | bzip2 -9`
    osutils.WriteFile(compressed,
                      'BZh91AY&SY\xfa\xd4\xdb5\x00\x00\x01A\x00\x00\x10 D\xa0'
                      '\x00!\x83A\x9a\t\xa8qw$S\x85\t\x0f\xadM\xb3P')
    self.assertEquals('binary/compressed/bzip2',
                      filetype.FileTypeDecoder.DecodeFile(compressed))

    # `echo hola | xz -9`
    osutils.WriteFile(
        compressed,
        '\xfd7zXZ\x00\x00\x04\xe6\xd6\xb4F\x02\x00!\x01\x16\x00\x00\x00t/\xe5'
        '\xa3\x01\x00\x04hola\n\x00\x00\x00\x00\xdd\xb0\x00\xac6w~\x9d\x00\x01'
        '\x1d\x05\xb8-\x80\xaf\x1f\xb6\xf3}\x01\x00\x00\x00\x00\x04YZ')
    self.assertEquals('binary/compressed/xz',
                      filetype.FileTypeDecoder.DecodeFile(compressed))

  def testBinaryMiscFiles(self):
    """Test for various binary file formats."""
    # A timezone file.
    some_timezone = os.path.join(self.tempdir, 'some_timezone')
    osutils.WriteFile(
        some_timezone,
        'TZif2\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        '\x00\x00\x01\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        '\x00\x01\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00UTC\x00\x00\x00TZif2'
        '\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        '\x00\x01\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        '\x01\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00UTC\x00\x00\x00\nUTC0\n')
    self.assertEquals('binary/tzfile',
                      filetype.FileTypeDecoder.DecodeFile(some_timezone))

    # A x86 boot sector with just nops.
    bootsec = os.path.join(self.tempdir, 'bootsec')
    osutils.WriteFile(bootsec, '\x90' * 510 + '\x55\xaa')
    self.assertEquals('binary/bootsector/x86',
                      filetype.FileTypeDecoder.DecodeFile(bootsec))
