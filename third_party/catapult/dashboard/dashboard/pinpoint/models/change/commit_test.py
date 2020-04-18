# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mock

from dashboard.common import namespaced_stored_object
from dashboard.common import testing_common
from dashboard.pinpoint.models.change import commit


_CHROMIUM_URL = 'https://chromium.googlesource.com/chromium/src'
_GITILES_COMMIT_INFO = {
    'author': {'email': 'author@chromium.org'},
    'commit': 'aaa7336',
    'committer': {'time': 'Fri Jan 01 00:01:00 2016'},
    'message': 'Subject.\n\n'
               'Commit message.\n'
               'Cr-Commit-Position: refs/heads/master@{#437745}',
}


class _CommitTest(testing_common.TestCase):

  def setUp(self):
    super(_CommitTest, self).setUp()

    self.SetCurrentUser('internal@chromium.org', is_admin=True)

    namespaced_stored_object.Set('repositories', {
        'chromium': {'repository_url': _CHROMIUM_URL},
    })
    namespaced_stored_object.Set('repository_urls_to_names', {
        _CHROMIUM_URL: 'chromium',
    })


class CommitTest(_CommitTest):

  def testCommit(self):
    c = commit.Commit('chromium', 'aaa7336c821888839f759c6c0a36')

    other_commit = commit.Commit(u'chromium', u'aaa7336c821888839f759c6c0a36')
    self.assertEqual(c, other_commit)
    self.assertEqual(str(c), 'chromium@aaa7336')
    self.assertEqual(c.id_string, 'chromium@aaa7336c821888839f759c6c0a36')
    self.assertEqual(c.repository, 'chromium')
    self.assertEqual(c.git_hash, 'aaa7336c821888839f759c6c0a36')
    self.assertEqual(c.repository_url,
                     'https://chromium.googlesource.com/chromium/src')

  @mock.patch('dashboard.services.gitiles_service.FileContents')
  def testDeps(self, file_contents):
    file_contents.return_value = """
vars = {
  'chromium_git': 'https://chromium.googlesource.com',
  'webrtc_git': 'https://webrtc.googlesource.com',
  'webrtc_rev': 'deadbeef',
}
deps = {
  'src/v8': Var('chromium_git') + '/v8/v8.git' + '@' + 'c092edb',
  'src/third_party/lighttpd': {
    'url': Var('chromium_git') + '/deps/lighttpd.git' + '@' + '9dfa55d',
    'condition': 'checkout_mac or checkout_win',
  },
  'src/third_party/webrtc': {
    'url': '{webrtc_git}/src.git',
    'revision': '{webrtc_rev}',
  },
  'src/third_party/intellij': {
    'packages': [{
      'package': 'chromium/third_party/intellij',
      'version': 'version:12.0-cr0',
    }],
    'condition': 'checkout_android',
    'dep_type': 'cipd',
  },
}
deps_os = {
  'win': {
    'src/third_party/cygwin':
      Var('chromium_git') + '/chromium/deps/cygwin.git' + '@' + 'c89e446',
  }
}
    """

    c = commit.Commit('chromium', 'aaa7336')
    expected = frozenset((
        ('https://chromium.googlesource.com/chromium/deps/cygwin', 'c89e446'),
        ('https://chromium.googlesource.com/deps/lighttpd', '9dfa55d'),
        ('https://chromium.googlesource.com/v8/v8', 'c092edb'),
        ('https://webrtc.googlesource.com/src', 'deadbeef'),
    ))
    self.assertEqual(c.Deps(), expected)

  @mock.patch('dashboard.services.gitiles_service.CommitInfo')
  def testAsDict(self, commit_info):
    commit_info.return_value = _GITILES_COMMIT_INFO

    c = commit.Commit('chromium', 'aaa7336')
    expected = {
        'repository': 'chromium',
        'git_hash': 'aaa7336',
        'url': _CHROMIUM_URL + '/+/aaa7336',
        'subject': 'Subject.',
        'author': 'author@chromium.org',
        'time': 'Fri Jan 01 00:01:00 2016',
        'commit_position': 437745,
    }
    self.assertEqual(c.AsDict(), expected)

  def testFromDepNewRepo(self):
    c = commit.Commit.FromDep(commit.Dep('https://new/repository/url.git', 'git_hash'))
    self.assertEqual(c, commit.Commit('url', 'git_hash'))

  def testFromDepExistingRepo(self):
    c = commit.Commit.FromDep(commit.Dep(_CHROMIUM_URL, 'git_hash'))
    self.assertEqual(c, commit.Commit('chromium', 'git_hash'))

  @mock.patch('dashboard.services.gitiles_service.CommitInfo',
              mock.MagicMock(side_effect=lambda x, y: {'commit': y}))
  def testFromDict(self):
    c = commit.Commit.FromDict({
        'repository': 'chromium',
        'git_hash': 'aaa7336',
    })

    expected = commit.Commit('chromium', 'aaa7336')
    self.assertEqual(c, expected)

  @mock.patch('dashboard.services.gitiles_service.CommitInfo',
              mock.MagicMock(side_effect=lambda x, y: {'commit': y}))
  def testFromDictWithRepositoryUrl(self):
    c = commit.Commit.FromDict({
        'repository': 'https://chromium.googlesource.com/chromium/src',
        'git_hash': 'aaa7336',
    })

    expected = commit.Commit('chromium', 'aaa7336')
    self.assertEqual(c, expected)

  @mock.patch('dashboard.services.gitiles_service.CommitInfo',
              mock.MagicMock(return_value={'commit': 'aaa7336'}))
  def testFromDictResolvesHEAD(self):
    c = commit.Commit.FromDict({
        'repository': 'https://chromium.googlesource.com/chromium/src',
        'git_hash': 'HEAD',
    })

    expected = commit.Commit('chromium', 'aaa7336')
    self.assertEqual(c, expected)

  def testFromDictFailureFromUnknownRepo(self):
    with self.assertRaises(KeyError):
      commit.Commit.FromDict({
          'repository': 'unknown repo',
          'git_hash': 'git hash',
      })

  @mock.patch('dashboard.services.gitiles_service.CommitInfo')
  def testFromDictFailureFromUnknownCommit(self, commit_info):
    commit_info.side_effect = KeyError()

    with self.assertRaises(KeyError):
      commit.Commit.FromDict({
          'repository': 'chromium',
          'git_hash': 'unknown git hash',
      })


class MidpointTest(_CommitTest):

  @mock.patch('dashboard.services.gitiles_service.CommitRange')
  def testSuccess(self, commit_range):
    commit_range.return_value = [
        {'commit': 'babe852'},
        {'commit': 'b57345e'},
        {'commit': '949b36d'},
        {'commit': '1ef4789'},
    ]

    commit_a = commit.Commit('chromium', '0e57e2b')
    commit_b = commit.Commit('chromium', 'babe852')
    self.assertEqual(commit.Commit.Midpoint(commit_a, commit_b),
                     commit.Commit('chromium', '949b36d'))

  def testSameCommit(self):
    commit_a = commit.Commit('chromium', '0e57e2b')
    commit_b = commit.Commit('chromium', '0e57e2b')
    self.assertEqual(commit.Commit.Midpoint(commit_a, commit_b), commit_a)

  @mock.patch('dashboard.services.gitiles_service.CommitRange')
  def testAdjacentCommits(self, commit_range):
    commit_range.return_value = [{'commit': 'b57345e'}]

    commit_a = commit.Commit('chromium', '949b36d')
    commit_b = commit.Commit('chromium', 'b57345e')
    self.assertEqual(commit.Commit.Midpoint(commit_a, commit_b), commit_a)

  def testRaisesWithDifferingRepositories(self):
    commit_a = commit.Commit('chromium', '0e57e2b')
    commit_b = commit.Commit('not_chromium', 'babe852')
    with self.assertRaises(commit.NonLinearError):
      commit.Commit.Midpoint(commit_a, commit_b)

  @mock.patch('dashboard.services.gitiles_service.CommitRange')
  def testRaisesWithEmptyRange(self, commit_range):
    commit_range.return_value = []

    commit_b = commit.Commit('chromium', 'b57345e')
    commit_a = commit.Commit('chromium', '949b36d')
    with self.assertRaises(commit.NonLinearError):
      commit.Commit.Midpoint(commit_a, commit_b)
