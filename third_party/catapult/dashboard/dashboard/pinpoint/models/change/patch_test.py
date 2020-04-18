# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import mock

from dashboard.pinpoint.models.change import patch


_GERRIT_CHANGE_INFO = {
    '_number': 658277,
    'id': 'repo~branch~id',
    'project': 'chromium/src',
    'subject': 'Subject',
    'current_revision': 'current revision',
    'revisions': {
        'current revision': {
            '_number': 5,
            'created': '2018-02-01 23:46:56.000000000',
            'uploader': {'email': 'author@example.org'},
            'fetch': {
                'http': {
                    'url': 'https://googlesource.com/chromium/src',
                    'ref': 'refs/changes/77/658277/5',
                },
            },
        },
        'other revision': {
            '_number': 4,
            'created': '2018-02-01 23:46:56.000000000',
            'uploader': {'email': 'author@example.org'},
            'fetch': {
                'http': {
                    'url': 'https://googlesource.com/chromium/src',
                    'ref': 'refs/changes/77/658277/4',
                },
            },
        },
    },
}


class FromDictTest(unittest.TestCase):

  @mock.patch('dashboard.services.gerrit_service.GetChange')
  def testFromDictGerrit(self, get_change):
    get_change.return_value = _GERRIT_CHANGE_INFO

    p = patch.FromDict('https://example.com/c/repo/+/658277')

    expected = patch.GerritPatch(
        'https://example.com', 'repo~branch~id', 'current revision')
    self.assertEqual(p, expected)

  @mock.patch('dashboard.services.gerrit_service.GetChange')
  def testFromDictGerritWithRevision(self, get_change):
    get_change.return_value = _GERRIT_CHANGE_INFO

    p = patch.FromDict('https://example.com/c/repo/+/658277/4')

    expected = patch.GerritPatch(
        'https://example.com', 'repo~branch~id', 'other revision')
    self.assertEqual(p, expected)

  def testFromDictBadUrl(self):
    with self.assertRaises(ValueError):
      patch.FromDict('https://example.com/not/a/gerrit/url')


class GerritPatchTest(unittest.TestCase):

  def testPatch(self):
    p = patch.GerritPatch('https://example.com', 672011, '2f0d5c7')

    other_patch = patch.GerritPatch(u'https://example.com', 672011, '2f0d5c7')
    self.assertEqual(p, other_patch)
    self.assertEqual(str(p), '2f0d5c7')
    self.assertEqual(p.id_string, 'https://example.com/672011/2f0d5c7')

  @mock.patch('dashboard.services.gerrit_service.GetChange')
  def testBuildParameters(self, get_change):
    get_change.return_value = _GERRIT_CHANGE_INFO

    p = patch.GerritPatch('https://example.com', 658277, 'current revision')
    expected = {
        'patch_gerrit_url': 'https://example.com',
        'patch_issue': 658277,
        'patch_project': 'chromium/src',
        'patch_ref': 'refs/changes/77/658277/5',
        'patch_repository_url': 'https://googlesource.com/chromium/src',
        'patch_set': 5,
        'patch_storage': 'gerrit',
    }
    self.assertEqual(p.BuildParameters(), expected)

  @mock.patch('dashboard.services.gerrit_service.GetChange')
  def testAsDict(self, get_change):
    get_change.return_value = _GERRIT_CHANGE_INFO

    p = patch.GerritPatch('https://example.com', 658277, 'current revision')
    expected = {
        'server': 'https://example.com',
        'change': 658277,
        'revision': 'current revision',
        'url': 'https://example.com/c/chromium/src/+/658277/5',
        'subject': 'Subject',
        'author': 'author@example.org',
        'time': '2018-02-01 23:46:56.000000000',
    }
    self.assertEqual(p.AsDict(), expected)

  @mock.patch('dashboard.services.gerrit_service.GetChange')
  def testFromDict(self, get_change):
    get_change.return_value = _GERRIT_CHANGE_INFO

    p = patch.GerritPatch.FromDict({
        'server': 'https://example.com',
        'change': 658277,
        'revision': 4,
    })

    expected = patch.GerritPatch(
        'https://example.com', 'repo~branch~id', 'other revision')
    self.assertEqual(p, expected)

  @mock.patch('dashboard.services.gerrit_service.GetChange')
  def testFromDictString(self, get_change):
    get_change.return_value = _GERRIT_CHANGE_INFO

    p = patch.GerritPatch.FromDict('https://example.com/c/repo/+/658277')

    expected = patch.GerritPatch(
        'https://example.com', 'repo~branch~id', 'current revision')
    self.assertEqual(p, expected)
