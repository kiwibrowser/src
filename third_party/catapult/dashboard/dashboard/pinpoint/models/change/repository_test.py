# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from dashboard.common import namespaced_stored_object
from dashboard.common import testing_common
from dashboard.pinpoint.models.change import repository


_CHROMIUM_URL = 'https://chromium.googlesource.com/chromium/src'


class RepositoryTest(testing_common.TestCase):

  def setUp(self):
    super(RepositoryTest, self).setUp()

    self.SetCurrentUser('internal@chromium.org', is_admin=True)

    namespaced_stored_object.Set('repositories', {
        'chromium': {'repository_url': _CHROMIUM_URL},
    })
    namespaced_stored_object.Set('repository_urls_to_names', {
        _CHROMIUM_URL: 'chromium',
    })

  def testRepositoryUrl(self):
    self.assertEqual(repository.RepositoryUrl('chromium'), _CHROMIUM_URL)

  def testRepositoryUrlRaisesWithUnknownName(self):
    with self.assertRaises(KeyError):
      repository.RepositoryUrl('not chromium')

  def testRepository(self):
    self.assertEqual(repository.Repository(_CHROMIUM_URL + '.git'), 'chromium')

  def testRepositoryRaisesWithUnknownUrl(self):
    with self.assertRaises(KeyError):
      repository.Repository('https://chromium.googlesource.com/nonexistent/repo')

  def testAddRepository(self):
    name = repository.Repository('https://example/repo',
                                 add_if_missing=True)
    self.assertEqual(name, 'repo')

    self.assertEqual(repository.RepositoryUrl('repo'), 'https://example/repo')
    self.assertEqual(repository.Repository('https://example/repo'), 'repo')

  def testAddRepositoryRaisesWithDuplicateName(self):
    with self.assertRaises(AssertionError):
      repository.Repository('https://example/chromium', add_if_missing=True)
