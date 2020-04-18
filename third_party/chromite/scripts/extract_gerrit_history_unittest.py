# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Build database associating Gerrit change # with commit metadata."""

from __future__ import print_function

from chromite.lib import cros_test_lib
from chromite.scripts import extract_gerrit_history as egh

BASIC_MSG = """Force XXX to YYY

BUG=foobarium:4321
TEST=barfooium

Change-Id: I9a8e7b9efbfb8da3c127f14f30f386b25427e7c5
Reviewed-on: https://chromium-review.googlesource.com/421249
Commit-Ready: Foobi Barbi <foobi@barium.co.nz>
Tested-by: Foobi Barbi <foobi@barium.co.nz>
Revied-by: Foobi Barbi <foobi@barium.co.nz>
"""

MULTILINK_MSG = """UPSTREAM: common object embedded into various struct ....ns

for now - just move corresponding ->proc_inum instances over there

Acked-by: "Eric W. Biederman" <ebiederm@xmission.com>
Signed-off-by: Al Viro <viro@zeniv.linux.org.uk>

BUG=b:29259708
TEST=Built and booted on cyan

(cherry picked from commit 435d5f4bb2ccba3b791d9ef61d2590e30b8e806e)
Signed-off-by: Dmitry Torokhov <dtor@chromium.org>
Original-Change-Id: I3ae970b315aaeebb3389baff7163ee49ffdb421d
Reviewed-on: https://chromium-review.googlesource.com/353786
Reviewed-by: Dylan Reid <dgreid@chromium.org>
(cherry picked from commit 8f37c950efff5346373576b654ccf99f098a2927
 and updated to use include/wireless-4.2/net/... and dropped the rest)
Change-Id: I70dcbbceb880c28b82926c74491f570cc0756b87
Reviewed-on: https://chromium-review.googlesource.com/354450
Commit-Ready: Grant Grundler <grundler@chromium.org>
Tested-by: Grant Grundler <grundler@chromium.org>
Reviewed-by: Grant Grundler <grundler@chromium.org>
"""

_GERRIT_CHROMIUM_MSG = """cros-xauth: mini replacement for xauth

The xauth package depends on a bunch of libs that no one else does to
support legacy formats we don't care about.  Since we only need it to
write out a <100 byte file, implement the logic ourselves.

BUG=chromium-os:39422
TEST=`./cros-xauth foo` produced a file that looks right

Change-Id: I4c2de5effcde627fcd7d4e3063892091814f6a94
Reviewed-on: https://gerrit.chromium.org/gerrit/44402
Reviewed-by: Jorge Lucangeli Obes <jorgelo@chromium.org>
Reviewed-by: Kees Cook <keescook@chromium.org>
Commit-Queue: Mike Frysinger <vapier@chromium.org>
Tested-by: Mike Frysinger <vapier@chromium.org>
"""
# pylint: disable=protected-access
class CommitMessageParseTest(cros_test_lib.TestCase):
  """Tests that commit message parsing works as expected."""

  def testInvalidInputRaises(self):
    with self.assertRaises(ValueError):
      egh._ParseCommitMessage('Non-matching string')

  def testBasicMatch(self):
    self.assertEqual(('chromium-review.googlesource.com', '421249'),
                     egh._ParseCommitMessage(BASIC_MSG))

  def testMultiLinkMatch(self):
    self.assertEqual(('chromium-review.googlesource.com', '354450'),
                     egh._ParseCommitMessage(MULTILINK_MSG))

  def testOldGerritLink(self):
    self.assertEqual(('gerrit.chromium.org', '44402'),
                     egh._ParseCommitMessage(_GERRIT_CHROMIUM_MSG))
