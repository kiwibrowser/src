#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from chroot_file_system import ChrootFileSystem
from file_system import StatInfo
from test_file_system import TestFileSystem


def _SortListValues(dict_):
  for value in dict_.itervalues():
    if isinstance(value, list):
      value.sort()
  return dict_


class ChrootFileSystemTest(unittest.TestCase):

  def setUp(self):
    self._test_fs = TestFileSystem({
      '404.html': '404.html contents',
      'apps': {
        'a11y.html': 'a11y.html contents',
        'about_apps.html': 'about_apps.html contents',
        'fakedir': {
          'file.html': 'file.html contents',
        },
      },
      'extensions': {
        'activeTab.html': 'activeTab.html contents',
        'alarms.html': 'alarms.html contents',
        'manifest': {
          'moremanifest': {
            'csp.html': 'csp.html contents',
            'usb.html': 'usb.html contents',
          },
          'sockets.html': 'sockets.html contents',
        },
      },
    })

  def testRead(self):
    for prefix in ('', '/'):
      for suffix in ('', '/'):
        chroot_fs = ChrootFileSystem(self._test_fs,
                                     prefix + 'extensions/manifest' + suffix)
        self.assertEqual({
          'moremanifest/usb.html': 'usb.html contents',
          '': ['moremanifest/', 'sockets.html', ],
          'moremanifest/': ['csp.html', 'usb.html'],
          'sockets.html': 'sockets.html contents',
        }, _SortListValues(chroot_fs.Read(
          ('moremanifest/usb.html', '', 'moremanifest/', 'sockets.html')
        ).Get()))

  def testEmptyRoot(self):
    chroot_fs = ChrootFileSystem(self._test_fs, '')
    self.assertEqual('404.html contents',
                     chroot_fs.ReadSingle('404.html').Get())

  def testStat(self):
    self._test_fs.IncrementStat('extensions/manifest/sockets.html', by=2)
    self._test_fs.IncrementStat('extensions/manifest/moremanifest/csp.html')
    for prefix in ('', '/'):
      for suffix in ('', '/'):
        chroot_fs = ChrootFileSystem(self._test_fs,
                                     prefix + 'extensions' + suffix)
        self.assertEqual(StatInfo('2', child_versions={
          'activeTab.html': '0',
          'alarms.html': '0',
          'manifest/': '2',
        }), chroot_fs.Stat(''))
        self.assertEqual(StatInfo('0'), chroot_fs.Stat('activeTab.html'))
        self.assertEqual(StatInfo('2', child_versions={
          'moremanifest/': '1',
          'sockets.html': '2',
        }), chroot_fs.Stat('manifest/'))
        self.assertEqual(StatInfo('2'), chroot_fs.Stat('manifest/sockets.html'))
        self.assertEqual(StatInfo('1', child_versions={
          'csp.html': '1',
          'usb.html': '0',
        }), chroot_fs.Stat('manifest/moremanifest/'))
        self.assertEqual(StatInfo('1'),
                         chroot_fs.Stat('manifest/moremanifest/csp.html'))
        self.assertEqual(StatInfo('0'),
                         chroot_fs.Stat('manifest/moremanifest/usb.html'))

  def testIdentity(self):
    chroot_fs1 = ChrootFileSystem(self._test_fs, '1')
    chroot_fs1b = ChrootFileSystem(self._test_fs, '1')
    chroot_fs2 = ChrootFileSystem(self._test_fs, '2')
    self.assertNotEqual(self._test_fs.GetIdentity(), chroot_fs1.GetIdentity())
    self.assertNotEqual(self._test_fs.GetIdentity(), chroot_fs2.GetIdentity())
    self.assertNotEqual(chroot_fs1.GetIdentity(), chroot_fs2.GetIdentity())
    self.assertEqual(chroot_fs1.GetIdentity(), chroot_fs1b.GetIdentity())


if __name__ == '__main__':
  unittest.main()
