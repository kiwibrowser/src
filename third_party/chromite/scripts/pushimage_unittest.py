# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for pushimage.py"""

from __future__ import print_function

import mock
import os

from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import gs
from chromite.lib import gs_unittest
from chromite.lib import osutils
from chromite.lib import partial_mock
from chromite.lib import signing
from chromite.scripts import pushimage


class InputInsnsTest(cros_test_lib.MockTestCase):
  """Tests for InputInsns"""

  def setUp(self):
    self.StartPatcher(gs_unittest.GSContextMock())

  def testBasic(self):
    """Simple smoke test"""
    insns = pushimage.InputInsns('test.board')
    insns.GetInsnFile('recovery')
    self.assertEqual(insns.GetChannels(), ['dev', 'canary'])
    self.assertEqual(insns.GetKeysets(), ['stumpy-mp-v3'])

  def testGetInsnFile(self):
    """Verify various inputs result in right insns path"""
    testdata = (
        ('UPPER_CAPS', 'UPPER_CAPS'),
        ('recovery', 'test.board'),
        ('firmware', 'test.board.firmware'),
        ('factory', 'test.board.factory'),
    )
    insns = pushimage.InputInsns('test.board')
    for image_type, filename in testdata:
      ret = insns.GetInsnFile(image_type)
      self.assertEqual(os.path.basename(ret), '%s.instructions' % (filename))

  def testSplitCfgField(self):
    """Verify splitting behavior behaves"""
    testdata = (
        ('', []),
        ('a b c', ['a', 'b', 'c']),
        ('a, b', ['a', 'b']),
        ('a,b', ['a', 'b']),
        ('a,\tb', ['a', 'b']),
        ('a\tb', ['a', 'b']),
    )
    for val, exp in testdata:
      ret = pushimage.InputInsns.SplitCfgField(val)
      self.assertEqual(ret, exp)

  def testOutputInsnsBasic(self):
    """Verify output instructions are sane"""
    exp_content = """[insns]
channel = dev canary
keyset = stumpy-mp-v3
chromeos_shell = false
ensure_no_password = true
firmware_update = true
security_checks = true
create_nplusone = true

[general]
"""

    insns = pushimage.InputInsns('test.board')
    self.assertEqual(insns.GetAltInsnSets(), [None])
    m = self.PatchObject(osutils, 'WriteFile')
    insns.OutputInsns('/bogus', {}, {})
    self.assertTrue(m.called)
    content = m.call_args_list[0][0][1]
    self.assertEqual(content.rstrip(), exp_content.rstrip())

  def testOutputInsnsReplacements(self):
    """Verify output instructions can be updated"""
    exp_content = """[insns]
channel = dev
keyset = batman
chromeos_shell = false
ensure_no_password = true
firmware_update = true
security_checks = true
create_nplusone = true

[general]
board = board
config_board = test.board
"""
    sect_insns = {
        'channel': 'dev',
        'keyset': 'batman',
    }
    sect_general = {
        'config_board': 'test.board',
        'board': 'board',
    }

    insns = pushimage.InputInsns('test.board')
    m = self.PatchObject(osutils, 'WriteFile')
    insns.OutputInsns('/a/file', sect_insns, sect_general)
    self.assertTrue(m.called)
    content = m.call_args_list[0][0][1]
    self.assertEqual(content.rstrip(), exp_content.rstrip())

  def testOutputInsnsMergeAlts(self):
    """Verify handling of alternative insns.xxx sections"""
    TEMPLATE_CONTENT = """[insns]
channel = %(channel)s
chromeos_shell = false
ensure_no_password = true
firmware_update = true
security_checks = true
create_nplusone = true
override = sect_insns
keyset = %(keyset)s
%(extra)s
[general]
board = board
config_board = test.board
"""

    exp_alts = ['insns.one', 'insns.two', 'insns.hotsoup']
    exp_fields = {
        'one': {'channel': 'dev canary', 'keyset': 'OneKeyset', 'extra': ''},
        'two': {'channel': 'best', 'keyset': 'TwoKeyset', 'extra': ''},
        'hotsoup': {
            'channel': 'dev canary',
            'keyset': 'ColdKeyset',
            'extra': 'soup = cheddar\n',
        },
    }

    # Make sure this overrides the insn sections.
    sect_insns = {
        'override': 'sect_insns',
    }
    sect_insns_copy = sect_insns.copy()
    sect_general = {
        'config_board': 'test.board',
        'board': 'board',
    }

    insns = pushimage.InputInsns('test.multi')
    self.assertEqual(insns.GetAltInsnSets(), exp_alts)
    m = self.PatchObject(osutils, 'WriteFile')

    for alt in exp_alts:
      m.reset_mock()
      insns.OutputInsns('/a/file', sect_insns, sect_general, insns_merge=alt)
      self.assertEqual(sect_insns, sect_insns_copy)
      self.assertTrue(m.called)
      content = m.call_args_list[0][0][1]
      exp_content = TEMPLATE_CONTENT % exp_fields[alt[6:]]
      self.assertEqual(content.rstrip(), exp_content.rstrip())


class MarkImageToBeSignedTest(gs_unittest.AbstractGSContextTest):
  """Tests for MarkImageToBeSigned()"""

  def setUp(self):
    # Minor optimization -- we call this for logging purposes in the main
    # code, but don't really care about it for testing.  It just slows us.
    self.PatchObject(cros_build_lib, 'MachineDetails', return_value='1234\n')

  def testBasic(self):
    """Simple smoke test"""
    tbs_base = 'gs://some-bucket'
    insns_path = 'chan/board/ver/file.instructions'
    tbs_file = '%s/tobesigned/90,chan,board,ver,file.instructions' % tbs_base
    ret = pushimage.MarkImageToBeSigned(self.ctx, tbs_base, insns_path, 90)
    self.assertEqual(ret, tbs_file)

  def testPriority(self):
    """Verify diff priority values get used correctly"""
    for prio, sprio in ((0, '00'), (9, '09'), (35, '35'), (99, '99')):
      ret = pushimage.MarkImageToBeSigned(self.ctx, '', '', prio)
      self.assertEquals(ret, '/tobesigned/%s,' % sprio)

  def testBadPriority(self):
    """Verify we reject bad priority values"""
    for prio in (-10, -1, 100, 91239):
      self.assertRaises(ValueError, pushimage.MarkImageToBeSigned, self.ctx,
                        '', '', prio)

  def testTbsUpload(self):
    """Make sure we actually try to upload the file"""
    pushimage.MarkImageToBeSigned(self.ctx, '', '', 50)
    self.gs_mock.assertCommandContains(['cp', '--'])


class PushImageTests(gs_unittest.AbstractGSContextTest):
  """Tests for PushImage()"""

  def setUp(self):
    self.mark_mock = self.PatchObject(pushimage, 'MarkImageToBeSigned')

  def testBasic(self):
    """Simple smoke test"""
    EXPECTED = {
        'canary': [
            ('gs://chromeos-releases/canary-channel/test.board-hi/5126.0.0/'
             'ChromeOS-recovery-R34-5126.0.0-test.board-hi.instructions')],
        'dev': [
            ('gs://chromeos-releases/dev-channel/test.board-hi/5126.0.0/'
             'ChromeOS-recovery-R34-5126.0.0-test.board-hi.instructions')],
    }
    with mock.patch.object(gs.GSContext, 'Exists', return_value=True):
      urls = pushimage.PushImage('/src', 'test.board', 'R34-5126.0.0',
                                 profile='hi')

    self.assertEqual(urls, EXPECTED)

  def testBasic_SignTypesEmptyList(self):
    """Tests PushImage behavior when |sign_types| is empty instead of None.

    As part of the buildbots, PushImage function always receives a tuple for
    |sign_types| argument.  This test checks the behavior for empty tuple.
    """
    EXPECTED = {
        'canary': [
            ('gs://chromeos-releases/canary-channel/test.board-hi/5126.0.0/'
             'ChromeOS-recovery-R34-5126.0.0-test.board-hi.instructions')],
        'dev': [
            ('gs://chromeos-releases/dev-channel/test.board-hi/5126.0.0/'
             'ChromeOS-recovery-R34-5126.0.0-test.board-hi.instructions')],
    }
    with mock.patch.object(gs.GSContext, 'Exists', return_value=True):
      urls = pushimage.PushImage('/src', 'test.board', 'R34-5126.0.0',
                                 profile='hi', sign_types=())

    self.assertEqual(urls, EXPECTED)

  def testBasic_RealBoardName(self):
    """Runs a simple smoke test using a real board name."""
    EXPECTED = {
        'canary': [
            ('gs://chromeos-releases/canary-channel/x86-alex/5126.0.0/'
             'ChromeOS-recovery-R34-5126.0.0-x86-alex.instructions')],
        'dev': [
            ('gs://chromeos-releases/dev-channel/x86-alex/5126.0.0/'
             'ChromeOS-recovery-R34-5126.0.0-x86-alex.instructions')],
    }
    with mock.patch.object(gs.GSContext, 'Exists', return_value=True):
      urls = pushimage.PushImage('/src', 'x86-alex', 'R34-5126.0.0')

    self.assertEqual(urls, EXPECTED)

  def testBasicMock(self):
    """Simple smoke test in mock mode"""
    with mock.patch.object(gs.GSContext, 'Exists', return_value=True):
      pushimage.PushImage('/src', 'test.board', 'R34-5126.0.0',
                          dry_run=True, mock=True)

  def testBadVersion(self):
    """Make sure we barf on bad version strings"""
    self.assertRaises(ValueError, pushimage.PushImage, '', '', 'asdf')

  def testNoInsns(self):
    """Boards w/out insn files should get skipped"""
    urls = pushimage.PushImage('/src', 'a bad bad board', 'R34-5126.0.0')
    self.assertEqual(self.gs_mock.call_count, 0)
    self.assertEqual(urls, None)

  def testSignTypesRecovery(self):
    """Only sign the requested recovery type"""
    EXPECTED = {
        'canary': [
            ('gs://chromeos-releases/canary-channel/test.board/5126.0.0/'
             'ChromeOS-recovery-R34-5126.0.0-test.board.instructions')],
        'dev': [
            ('gs://chromeos-releases/dev-channel/test.board/5126.0.0/'
             'ChromeOS-recovery-R34-5126.0.0-test.board.instructions')],
    }

    with mock.patch.object(gs.GSContext, 'Exists', return_value=True):
      urls = pushimage.PushImage('/src', 'test.board', 'R34-5126.0.0',
                                 sign_types=['recovery'])
    self.assertEqual(self.gs_mock.call_count, 26)
    self.assertTrue(self.mark_mock.called)
    self.assertEqual(urls, EXPECTED)

  def testSignTypesBase(self):
    """Only sign the requested recovery type"""
    EXPECTED = {
        'canary': [
            ('gs://chromeos-releases/canary-channel/test.board/5126.0.0/'
             'ChromeOS-base-R34-5126.0.0-test.board.instructions')],
        'dev': [
            ('gs://chromeos-releases/dev-channel/test.board/5126.0.0/'
             'ChromeOS-base-R34-5126.0.0-test.board.instructions')],
    }

    with mock.patch.object(gs.GSContext, 'Exists', return_value=True):
      urls = pushimage.PushImage('/src', 'test.board', 'R34-5126.0.0',
                                 sign_types=['base'])
    self.assertEqual(self.gs_mock.call_count, 28)
    self.assertTrue(self.mark_mock.called)
    self.assertEqual(urls, EXPECTED)

  def testSignTypesNone(self):
    """Verify nothing is signed when we request an unavailable type"""
    urls = pushimage.PushImage('/src', 'test.board', 'R34-5126.0.0',
                               sign_types=['nononononono'])
    self.assertEqual(self.gs_mock.call_count, 24)
    self.assertFalse(self.mark_mock.called)
    self.assertEqual(urls, {})

  def testGsError(self):
    """Verify random GS errors don't make us blow up entirely"""
    self.gs_mock.AddCmdResult(partial_mock.In('stat'), returncode=1,
                              output='gobblety gook\n')
    with cros_test_lib.LoggingCapturer('chromite'):
      self.assertRaises(pushimage.PushError, pushimage.PushImage, '/src',
                        'test.board', 'R34-5126.0.0')

  def testMultipleKeysets(self):
    """Verify behavior when processing an insn w/multiple keysets"""
    EXPECTED = {
        'canary': [
            ('gs://chromeos-releases/canary-channel/test.board/5126.0.0/'
             'ChromeOS-recovery-R34-5126.0.0-test.board.instructions'),
            ('gs://chromeos-releases/canary-channel/test.board/5126.0.0/'
             'ChromeOS-recovery-R34-5126.0.0-test.board-key2.instructions'),
            ('gs://chromeos-releases/canary-channel/test.board/5126.0.0/'
             'ChromeOS-recovery-R34-5126.0.0-test.board-key3.instructions'),
        ],
        'dev': [
            ('gs://chromeos-releases/dev-channel/test.board/5126.0.0/'
             'ChromeOS-recovery-R34-5126.0.0-test.board.instructions'),
            ('gs://chromeos-releases/dev-channel/test.board/5126.0.0/'
             'ChromeOS-recovery-R34-5126.0.0-test.board-key2.instructions'),
            ('gs://chromeos-releases/dev-channel/test.board/5126.0.0/'
             'ChromeOS-recovery-R34-5126.0.0-test.board-key3.instructions'),
        ],
    }
    with mock.patch.object(gs.GSContext, 'Exists', return_value=True):
      urls = pushimage.PushImage('/src', 'test.board', 'R34-5126.0.0',
                                 force_keysets=('key1', 'key2', 'key3'))
    self.assertEqual(urls, EXPECTED)

  def testForceChannel(self):
    """Verify behavior when user has specified custom channel"""
    EXPECTED = {
        'meep': [
            ('gs://chromeos-releases/meep-channel/test.board/5126.0.0/'
             'ChromeOS-recovery-R34-5126.0.0-test.board.instructions'),
        ],
    }
    with mock.patch.object(gs.GSContext, 'Exists', return_value=True):
      urls = pushimage.PushImage('/src', 'test.board', 'R34-5126.0.0',
                                 force_channels=('meep',))
    self.assertEqual(urls, EXPECTED)

  def testMultipleAltInsns(self):
    """Verify behavior when processing an insn w/multiple insn overlays"""
    EXPECTED = {
        'canary': [
            ('gs://chromeos-releases/canary-channel/test.multi/1.0.0/'
             'ChromeOS-recovery-R1-1.0.0-test.multi.instructions'),
            ('gs://chromeos-releases/canary-channel/test.multi/1.0.0/'
             'ChromeOS-recovery-R1-1.0.0-test.multi-TwoKeyset.instructions'),
            ('gs://chromeos-releases/canary-channel/test.multi/1.0.0/'
             'ChromeOS-recovery-R1-1.0.0-test.multi-ColdKeyset.instructions'),
        ],
        'dev': [
            ('gs://chromeos-releases/dev-channel/test.multi/1.0.0/'
             'ChromeOS-recovery-R1-1.0.0-test.multi.instructions'),
            ('gs://chromeos-releases/dev-channel/test.multi/1.0.0/'
             'ChromeOS-recovery-R1-1.0.0-test.multi-TwoKeyset.instructions'),
            ('gs://chromeos-releases/dev-channel/test.multi/1.0.0/'
             'ChromeOS-recovery-R1-1.0.0-test.multi-ColdKeyset.instructions'),
        ],
    }
    with mock.patch.object(gs.GSContext, 'Exists', return_value=True):
      urls = pushimage.PushImage('/src', 'test.multi', 'R1-1.0.0')
    self.assertEqual(urls, EXPECTED)


class MainTests(cros_test_lib.MockTestCase):
  """Tests for main()"""

  def setUp(self):
    self.PatchObject(pushimage, 'PushImage')

  def testBasic(self):
    """Simple smoke test"""
    pushimage.main(['--board', 'test.board', '/src', '--yes'])


def main(_argv):
  # Use our local copy of insns for testing as the main one is not
  # available in the public manifest.
  signing.INPUT_INSN_DIR = signing.TEST_INPUT_INSN_DIR

  # Run the tests.
  cros_test_lib.main(level='notice', module=__name__)
