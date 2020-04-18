#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of the pnacl driver.

This tests that pnacl-translate options pass through to LLC and
Subzero correctly, and are overridden correctly.
"""

from driver_env import env
import driver_log
import driver_test_utils
import driver_tools
import filetype

import cStringIO
import os
import re
import sys
import unittest

class TestTranslateOptions(driver_test_utils.DriverTesterCommon):
  def setUp(self):
    super(TestTranslateOptions, self).setUp()
    driver_test_utils.ApplyTestEnvOverrides(env)
    self.platform = driver_test_utils.GetPlatformToTest()

  def getFakePexe(self, finalized=True):
    # Even --dry-run requires a file to exist, so make a fake pexe.
    # It even cares that the file is really bitcode.
    with self.getTemp(suffix='.ll', close=False) as t:
      with self.getTemp(suffix='.pexe') as p:
        t.write('''
define i32 @_start() {
  ret i32 0
}
''')
        t.close()
        driver_tools.RunDriver('pnacl-as', [t.name, '-o', p.name])
        if finalized:
          driver_tools.RunDriver('pnacl-finalize', [p.name])
          self.assertTrue(filetype.IsPNaClBitcode(p.name))
        else:
          self.assertTrue(filetype.IsLLVMBitcode(p.name))
        return p


  def checkCompileTranslateFlags(self, pexe, arch, flags,
                                 expected_flags, invert_match=False,
                                 expected_args=(),
                                 expected_output_file_count=None):
    ''' Given a |pexe| the |arch| for translation and additional pnacl-translate
    |flags|, check that the commandline for LLC really contains the
    |expected_flags|.  This ensures that the pnacl-translate script
    does not drop certain flags accidentally. '''
    # TODO(jvoung): Get rid of INHERITED_DRIVER_ARGS, which leaks across runs.
    env.set('INHERITED_DRIVER_ARGS', '')
    temp_output = self.getTemp()
    # Major hack to prevent DriverExit() from aborting the test.
    # The test will surely DriverLog.Fatal() because dry-run currently
    # does not handle anything that involves invoking a subprocess and
    # grepping the stdout/stderr since it never actually invokes
    # the subprocess. Unfortunately, pnacl-translate does grep the output of
    # the sandboxed LLC run, so we can only go that far with --dry-run.
    capture_out = cStringIO.StringIO()
    driver_log.Log.CaptureToStream(capture_out)
    backup_exit = sys.exit
    sys.exit = driver_test_utils.FakeExit
    self.assertRaises(driver_test_utils.DriverExitException,
                      driver_tools.RunDriver,
                      'pnacl-translate',
                      ['--pnacl-driver-verbose',
                       '--dry-run',
                       '-arch', arch,
                       pexe.name,
                       '-o', temp_output.name] + flags)
    driver_log.Log.ResetStreams()
    out = capture_out.getvalue()
    sys.exit = backup_exit
    for f in expected_flags:
      message = 'Searching for regex %s in %s' % (f, out)
      match = re.search(f, out, re.S)
      if not invert_match:
        self.assertTrue(match, msg=message)
      else:
        self.assertFalse(match, msg=message)

    # Check for some env vars that are passed to the sandboxed translator.
    args_got = re.findall(
        r'-E NACL_IRT_PNACL_TRANSLATOR_COMPILE_ARG_\d*=([^ ]*)', out)
    for arg in expected_args:
      self.assertIn(arg, args_got, repr(out))

    if expected_output_file_count is not None:
      output_files_got = re.findall(
          r'-E NACL_IRT_PNACL_TRANSLATOR_COMPILE_OUTPUT_\d*=([^ ]*)', out)
      self.assertEquals(len(output_files_got), expected_output_file_count)


  #### Individual tests.

  def test_no_overrides(self):
    if driver_test_utils.CanRunHost():
      pexe = self.getFakePexe()
      if self.platform == 'arm':
        expected_triple_cpu = ['-mtriple=arm.*', '-mcpu=cortex.*']
      elif self.platform == 'x86-32':
        expected_triple_cpu = ['-mtriple=i686.*', '-mcpu=pentium4m.*']
      elif self.platform == 'x86-64':
        expected_triple_cpu = ['-mtriple=x86_64.*', '-mcpu=x86-64.*']
      elif self.platform == 'mips':
        expected_triple_cpu = ['-mtriple=mips.*', '-mcpu=mips32.*']
      else:
        raise Exception('Unknown platform: "%s"' % self.platform)
      # Test that certain defaults are set, when no flags are given.
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          [],
          expected_triple_cpu + ['-bitcode-format=pnacl'])
      # Check that the driver sets the number of threads and modules
      # correctly (1 vs 4).
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          ['--pnacl-sb', '-split-module=1'],
          [' -E NACL_IRT_PNACL_TRANSLATOR_COMPILE_THREADS=1 '],
          expected_args=['-split-module=1'],
          expected_output_file_count=1)
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          ['--pnacl-sb', '-split-module=4'],
          [' -E NACL_IRT_PNACL_TRANSLATOR_COMPILE_THREADS=4 '],
          expected_args=['-split-module=4'],
          expected_output_file_count=4)
      # Check that -split-module=auto does something sensible (1, 2, 3, or 4).
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          ['--pnacl-sb', '-split-module=auto'],
          [' -E NACL_IRT_PNACL_TRANSLATOR_COMPILE_THREADS=[1234] '])
      # Check that omitting -split-module is the same as auto.
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          ['--pnacl-sb'],
          [' -E NACL_IRT_PNACL_TRANSLATOR_COMPILE_THREADS=[1234] '])
      # Check that -split-module=seq is the same as -split-module=1.
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          ['--pnacl-sb', '-split-module=seq'],
          [' -E NACL_IRT_PNACL_TRANSLATOR_COMPILE_THREADS=1 '],
          expected_args=['-split-module=1'],
          expected_output_file_count=1)
      # Check that we do not get streaming bitcode by default.
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          [],
          ['-streaming-bitcode'], invert_match=True)

  def test_subzero_flags(self):
    # Subzero only supports x86-32 for now.
    if self.platform != 'x86-32':
      return
    if driver_test_utils.CanRunHost():
      pexe = self.getFakePexe()
      # Test Subzero's default args. Assume default is -split-module=auto.
      # In that case # of modules is 1..4, and the only special param
      # is the optimization level.
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          ['--pnacl-sb', '--use-sz'],
          [' -E NACL_IRT_PNACL_TRANSLATOR_COMPILE_THREADS=[1234] '],
          expected_args=['-O2'],
          expected_output_file_count=1)
      # Similar, but with explicitly set -split-module=auto.
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          ['--pnacl-sb', '--use-sz', '-split-module=auto'],
          [' -E NACL_IRT_PNACL_TRANSLATOR_COMPILE_THREADS=[1234] '],
          expected_args=['-O2'],
          expected_output_file_count=1)
      # Similar, but with explicitly set split-module=seq.
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          ['--pnacl-sb', '--use-sz', '-split-module=seq'],
          [' -E NACL_IRT_PNACL_TRANSLATOR_COMPILE_THREADS=[1234] '],
          expected_args=['-O2'],
          expected_output_file_count=1)
      # Test that we can bump the thread count up (e.g., up to 4).
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          ['--pnacl-sb', '--use-sz', '-split-module=4'],
          [' -E NACL_IRT_PNACL_TRANSLATOR_COMPILE_THREADS=4 '],
          expected_args=['-O2'],
          expected_output_file_count=1)
      # Test that you get Om1 when you ask for O0.
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          ['-O0', '--pnacl-sb', '--use-sz'],
          [],
          expected_args=['-Om1'])

  def test_LLVMFile(self):
    if not driver_test_utils.CanRunHost():
      return
    pexe = self.getFakePexe(finalized=False)
    # For non-sandboxed pnacl-llc, the default -bitcode-format=llvm,
    # so there is nothing to really check.
    self.checkCompileTranslateFlags(
        pexe,
        self.platform,
        ['--allow-llvm-bitcode-input'],
        ['bitcode-format'], invert_match=True)
    # For sandboxed pnacl-llc, the default -bitcode-format=pnacl,
    # so we need to set -bitcode-format=llvm to read LLVM files.
    self.checkCompileTranslateFlags(
        pexe,
        self.platform,
        ['--pnacl-sb', '--allow-llvm-bitcode-input'],
        ['-bitcode-format=llvm'])
    # Check that we do not get streaming bitcode by default.
    self.checkCompileTranslateFlags(
        pexe,
        self.platform,
        ['--allow-llvm-bitcode-input'],
        ['-streaming-bitcode'], invert_match=True)
    # Test that we get streaming bitcode when we ask for it.
    self.checkCompileTranslateFlags(
        pexe,
        self.platform,
        ['--allow-llvm-bitcode-input', '-stream-bitcode'],
        ['-streaming-bitcode'])
    # ...even when using threading
    self.checkCompileTranslateFlags(
        pexe,
        self.platform,
        ['--allow-llvm-bitcode-input', '-stream-bitcode', '-split-module=4'],
        ['-streaming-bitcode', '-split-module=4'])

  def test_OverrideStreaming(self):
    if not driver_test_utils.CanRunHost():
      return
    pexe = self.getFakePexe()
    # Test that we get streaming bitcode when we ask for it.
    self.checkCompileTranslateFlags(
        pexe,
        self.platform,
        ['-stream-bitcode'],
        ['-streaming-bitcode'])
    # ...even when using threading.
    self.checkCompileTranslateFlags(
        pexe,
        self.platform,
        ['-stream-bitcode', '-split-module=4'],
        ['-streaming-bitcode', '-split-module=4'])

  def test_overrideO0(self):
    if driver_test_utils.CanRunHost():
      pexe = self.getFakePexe()
      # Test that you get O0 when you ask for O0.
      # You also get no frame pointer elimination.
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          ['-O0'],
          ['-O0', '-disable-fp-elim '])
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          ['-O0', '--pnacl-sb'],
          ['-O0.*-disable-fp-elim.*-mcpu=.*'])

  def test_overrideTranslateFast(self):
    if driver_test_utils.CanRunHost():
      pexe = self.getFakePexe()
      # Test that you get O0 when you ask for -translate-fast.
      # In this case... you don't get no frame pointer elimination.
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          ['-translate-fast'],
          ['-O0'])
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          ['-translate-fast', '--pnacl-sb'],
          ['-O0.*-mcpu=.*'])

  def test_overrideTLSUseCall(self):
    if driver_test_utils.CanRunHost():
      pexe = self.getFakePexe()
      # Test that you -mtls-use-call, etc. when you ask for it.
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          ['-mtls-use-call', '-fdata-sections', '-ffunction-sections'],
          ['-mtls-use-call', '-data-sections', '-function-sections'])
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          ['-mtls-use-call', '-fdata-sections', '-ffunction-sections',
           '--pnacl-sb'],
          ['-mtls-use-call.*-data-sections.*-function-sections'])

  def test_overrideMCPU(self):
    if driver_test_utils.CanRunHost():
      pexe = self.getFakePexe()
      if self.platform == 'arm':
        mcpu_pattern = '-mcpu=cortex-a15'
      elif self.platform == 'x86-32':
        mcpu_pattern = '-mcpu=atom'
      elif self.platform == 'x86-64':
        mcpu_pattern = '-mcpu=corei7'
      elif self.platform == 'mips':
        mcpu_pattern = '-mcpu=mips32'
      else:
        raise Exception('Unknown platform: "%s"' % self.platform)
      # Test that you get the -mcpu that you ask for.
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          [mcpu_pattern],
          [mcpu_pattern])
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          [mcpu_pattern, '--pnacl-sb'],
          [mcpu_pattern])

  def test_overrideMAttr(self):
    if driver_test_utils.CanRunHost():
      pexe = self.getFakePexe()
      if self.platform == 'arm':
        mattr_flags, mattr_pat = '-mattr=+hwdiv', r'-mattr=\+hwdiv'
      elif self.platform == 'x86-32' or self.platform == 'x86-64':
        mattr_flags, mattr_pat = '-mattr=+avx2,+sse41', r'-mattr=\+avx2,\+sse41'
      elif self.platform == 'mips':
        mattr_flags, mattr_pat = '-mattr=+fp64', r'-mattr=\+fp64'
      else:
        raise Exception('Unknown platform: "%s"' % self.platform)
      # Test that you get the -mattr=.* that you ask for.
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          [mattr_flags],
          [mattr_pat])
      self.checkCompileTranslateFlags(
          pexe,
          self.platform,
          [mattr_flags, '--pnacl-sb'],
          [mattr_pat + '.*-mcpu=.*'])
      # Same for subzero (only test supported architectures).
      if self.platform == 'x86-32':
        self.checkCompileTranslateFlags(
            pexe,
            self.platform,
            [mattr_flags, '--pnacl-sb', '--use-sz'],
            [mattr_pat])


if __name__ == '__main__':
  unittest.main()
