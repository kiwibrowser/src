# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

bisect_builds = __import__('bisect-builds')


class BisectTest(unittest.TestCase):

  patched = []
  max_rev = 10000

  def monkey_patch(self, obj, name, new):
    self.patched.append((obj, name, getattr(obj, name)))
    setattr(obj, name, new)

  def clear_patching(self):
    for obj, name, old in self.patched:
      setattr(obj, name, old)
    self.patched = []

  def setUp(self):
    self.monkey_patch(bisect_builds.DownloadJob, 'Start', lambda *args: None)
    self.monkey_patch(bisect_builds.DownloadJob, 'Stop', lambda *args: None)
    self.monkey_patch(bisect_builds.DownloadJob, 'WaitFor', lambda *args: None)
    self.monkey_patch(bisect_builds, 'RunRevision', lambda *args: (0, "", ""))
    self.monkey_patch(bisect_builds.PathContext, 'ParseDirectoryIndex',
                      lambda *args: range(self.max_rev))

  def tearDown(self):
    self.clear_patching()

  def bisect(self, good_rev, bad_rev, evaluate):
    return bisect_builds.Bisect(good_rev=good_rev,
                                bad_rev=bad_rev,
                                evaluate=evaluate,
                                num_runs=1,
                                official_builds=False,
                                platform='linux',
                                profile=None,
                                try_args=())

  def testBisectConsistentAnswer(self):
    self.assertEqual(self.bisect(1000, 100, lambda *args: 'g'), (100, 101))
    self.assertEqual(self.bisect(100, 1000, lambda *args: 'b'), (100, 101))
    self.assertEqual(self.bisect(2000, 200, lambda *args: 'b'), (1999, 2000))
    self.assertEqual(self.bisect(200, 2000, lambda *args: 'g'), (1999, 2000))


if __name__ == '__main__':
  unittest.main()
