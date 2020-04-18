# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mock

from dashboard.common import namespaced_stored_object
from dashboard.common import testing_common
from dashboard.pinpoint.models.change import change
from dashboard.pinpoint.models.change import commit
from dashboard.pinpoint.models.change import patch


_CATAPULT_URL = ('https://chromium.googlesource.com/'
                 'external/github.com/catapult-project/catapult')
_CHROMIUM_URL = 'https://chromium.googlesource.com/chromium/src'


class _ChangeTest(testing_common.TestCase):

  def setUp(self):
    super(_ChangeTest, self).setUp()

    self.SetCurrentUser('internal@chromium.org', is_admin=True)

    namespaced_stored_object.Set('repositories', {
        'catapult': {'repository_url': _CATAPULT_URL},
        'chromium': {'repository_url': _CHROMIUM_URL},
        'another_repo': {'repository_url': 'https://another/url'},
    })
    namespaced_stored_object.Set('repository_urls_to_names', {
        _CATAPULT_URL: 'catapult',
        _CHROMIUM_URL: 'chromium',
    })



@mock.patch('dashboard.services.gitiles_service.CommitInfo',
            mock.MagicMock(side_effect=lambda x, y: {'commit': y}))
class ChangeTest(_ChangeTest):

  def testChange(self):
    base_commit = commit.Commit('chromium', 'aaa7336c821888839f759c6c0a36b56c')
    dep = commit.Commit('catapult', 'e0a2efbb3d1a81aac3c90041eefec24f066d26ba')
    p = patch.GerritPatch('https://codereview.com', 672011, '2f0d5c7')

    # Also test the deps conversion to frozenset.
    c = change.Change([base_commit, dep], p)

    self.assertEqual(c, change.Change((base_commit, dep), p))
    string = 'chromium@aaa7336 catapult@e0a2efb + 2f0d5c7'
    id_string = ('catapult@e0a2efbb3d1a81aac3c90041eefec24f066d26ba '
                 'chromium@aaa7336c821888839f759c6c0a36b56c + '
                 'https://codereview.com/672011/2f0d5c7')
    self.assertEqual(str(c), string)
    self.assertEqual(c.id_string, id_string)
    self.assertEqual(c.base_commit, base_commit)
    self.assertEqual(c.last_commit, dep)
    self.assertEqual(c.deps, (dep,))
    self.assertEqual(c.commits, (base_commit, dep))
    self.assertEqual(c.patch, p)

  def testUpdate(self):
    old_commit = commit.Commit('chromium', 'aaaaaaaa')
    dep_a = commit.Commit('catapult', 'e0a2efbb')
    change_a = change.Change((old_commit, dep_a))

    new_commit = commit.Commit('chromium', 'bbbbbbbb')
    dep_b = commit.Commit('another_repo', 'e0a2efbb')
    p = patch.GerritPatch('https://codereview.com', 672011, '2f0d5c7')
    change_b = change.Change((dep_b, new_commit), p)

    expected = change.Change((new_commit, dep_a, dep_b), p)
    self.assertEqual(change_a.Update(change_b), expected)

  def testUpdateWithMultiplePatches(self):
    base_commit = commit.Commit('chromium', 'aaa7336c821888839f759c6c0a36b56c')
    p = patch.GerritPatch('https://codereview.com', 672011, '2f0d5c7')
    c = change.Change((base_commit,), p)
    with self.assertRaises(NotImplementedError):
      c.Update(c)

  @mock.patch('dashboard.pinpoint.models.change.patch.GerritPatch.AsDict')
  @mock.patch('dashboard.pinpoint.models.change.commit.Commit.AsDict')
  def testAsDict(self, commit_as_dict, patch_as_dict):
    commit_as_dict.side_effect = (
        {'git_hash': 'aaa7336c82'}, {'git_hash': 'e0a2efbb3d'})
    patch_as_dict.return_value = {'revision': '2f0d5c7'}
    commits = (commit.Commit('chromium', 'aaa7336c82'),
               commit.Commit('catapult', 'e0a2efbb3d'))
    p = patch.GerritPatch('https://codereview.com', 672011, '2f0d5c7')
    c = change.Change(commits, p)

    expected = {
        'commits': [
            {'git_hash': 'aaa7336c82'},
            {'git_hash': 'e0a2efbb3d'},
        ],
        'patch': {'revision': '2f0d5c7'},
    }
    self.assertEqual(c.AsDict(), expected)

  @mock.patch('dashboard.services.gitiles_service.CommitInfo')
  def testFromDictWithJustOneCommit(self, _):
    c = change.Change.FromDict({
        'commits': [{'repository': 'chromium', 'git_hash': 'aaa7336'}],
    })

    expected = change.Change((commit.Commit('chromium', 'aaa7336'),))
    self.assertEqual(c, expected)

  @mock.patch('dashboard.services.gerrit_service.GetChange')
  def testFromDictWithAllFields(self, get_change):
    get_change.return_value = {
        'id': 'repo~branch~id',
        'revisions': {'2f0d5c7': {}}
    }

    c = change.Change.FromDict({
        'commits': (
            {'repository': 'chromium', 'git_hash': 'aaa7336'},
            {'repository': 'catapult', 'git_hash': 'e0a2efb'},
        ),
        'patch': {
            'server': 'https://codereview.com',
            'change': 672011,
            'revision': '2f0d5c7',
        },
    })

    commits = (commit.Commit('chromium', 'aaa7336'),
               commit.Commit('catapult', 'e0a2efb'))
    p = patch.GerritPatch('https://codereview.com', 'repo~branch~id', '2f0d5c7')
    expected = change.Change(commits, p)
    self.assertEqual(c, expected)


class MidpointTest(_ChangeTest):

  def setUp(self):
    super(MidpointTest, self).setUp()

    patcher = mock.patch('dashboard.services.gitiles_service.CommitRange')
    self.addCleanup(patcher.stop)
    commit_range = patcher.start()
    def _CommitRange(repository_url, first_git_hash, last_git_hash):
      del repository_url
      first_git_hash = int(first_git_hash)
      last_git_hash = int(last_git_hash)
      return [{'commit': x} for x in xrange(last_git_hash, first_git_hash, -1)]
    commit_range.side_effect = _CommitRange

    patcher = mock.patch('dashboard.services.gitiles_service.FileContents')
    self.addCleanup(patcher.stop)
    file_contents = patcher.start()
    def _FileContents(repository_url, git_hash, path):
      del path
      if repository_url != _CHROMIUM_URL:
        return 'deps = {}'
      if git_hash <= 4:  # DEPS roll at chromium@5
        return 'deps = {"chromium/catapult": "%s@0"}' % (_CATAPULT_URL + '.git')
      else:
        return 'deps = {"chromium/catapult": "%s@9"}' % _CATAPULT_URL
    file_contents.side_effect = _FileContents

  def testDifferingPatch(self):
    change_a = change.Change((commit.Commit('chromium', '0e57e2b'),))
    change_b = change.Change(
        (commit.Commit('chromium', 'babe852'),),
        patch=patch.GerritPatch('https://codereview.com', 672011, '2f0d5c7'))
    with self.assertRaises(commit.NonLinearError):
      change.Change.Midpoint(change_a, change_b)

  def testDifferingRepository(self):
    change_a = change.Change((commit.Commit('chromium', '0e57e2b'),))
    change_b = change.Change((commit.Commit('another_repo', 'babe852'),))
    with self.assertRaises(commit.NonLinearError):
      change.Change.Midpoint(change_a, change_b)

  def testDifferingCommitCount(self):
    change_a = change.Change((commit.Commit('chromium', 0),))
    change_b = change.Change((commit.Commit('chromium', 9),
                              commit.Commit('another_repo', 'babe852')))
    with self.assertRaises(commit.NonLinearError):
      change.Change.Midpoint(change_a, change_b)

  def testSameChange(self):
    change_a = change.Change((commit.Commit('chromium', 0),))
    change_b = change.Change((commit.Commit('chromium', 0),))
    with self.assertRaises(commit.NonLinearError):
      change.Change.Midpoint(change_a, change_b)

  def testAdjacentWithNoDepsRoll(self):
    change_a = change.Change((commit.Commit('chromium', 0),))
    change_b = change.Change((commit.Commit('chromium', 1),))
    with self.assertRaises(commit.NonLinearError):
      change.Change.Midpoint(change_a, change_b)

  def testAdjacentWithDepsRoll(self):
    change_a = change.Change((commit.Commit('chromium', 4),))
    change_b = change.Change((commit.Commit('chromium', 5),))
    expected = change.Change((commit.Commit('chromium', 4),
                              commit.Commit('catapult', 4)))
    self.assertEqual(change.Change.Midpoint(change_a, change_b), expected)

  def testNotAdjacent(self):
    change_a = change.Change((commit.Commit('chromium', 0),))
    change_b = change.Change((commit.Commit('chromium', 9),))
    self.assertEqual(change.Change.Midpoint(change_a, change_b),
                     change.Change((commit.Commit('chromium', 4),)))

  def testDepsRollLeft(self):
    change_a = change.Change((commit.Commit('chromium', 4),))
    change_b = change.Change((commit.Commit('chromium', 4),
                              commit.Commit('catapult', 4)))
    expected = change.Change((commit.Commit('chromium', 4),
                              commit.Commit('catapult', 2)))
    self.assertEqual(change.Change.Midpoint(change_a, change_b), expected)

  def testDepsRollRight(self):
    change_a = change.Change((commit.Commit('chromium', 4),
                              commit.Commit('catapult', 4)))
    change_b = change.Change((commit.Commit('chromium', 5),))
    expected = change.Change((commit.Commit('chromium', 4),
                              commit.Commit('catapult', 6)))
    self.assertEqual(change.Change.Midpoint(change_a, change_b), expected)

  def testAdjacentWithDepsRollAndDepAlreadyOverridden(self):
    change_a = change.Change((commit.Commit('chromium', 4),))
    change_b = change.Change((commit.Commit('chromium', 5),
                              commit.Commit('catapult', 4)))
    expected = change.Change((commit.Commit('chromium', 4),
                              commit.Commit('catapult', 2)))
    self.assertEqual(change.Change.Midpoint(change_a, change_b), expected)
