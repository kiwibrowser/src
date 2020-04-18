# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for chromite.lib.patch."""

from __future__ import print_function

import copy
import contextlib
import itertools
import mock
import os
import shutil
import time

from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import gerrit
from chromite.lib import git
from chromite.lib import osutils
from chromite.lib import patch as cros_patch


site_config = config_lib.GetConfig()


_GetNumber = iter(itertools.count()).next

# Change-ID of a known open change in public gerrit.
GERRIT_OPEN_CHANGEID = '8366'
GERRIT_MERGED_CHANGEID = '3'
GERRIT_ABANDONED_CHANGEID = '2'

FAKE_PATCH_JSON = {
    'project': 'tacos/chromite',
    'branch': 'master',
    'id': 'Iee5c89d929f1850d7d4e1a4ff5f21adda800025f',
    'currentPatchSet': {
        'number': '2',
        'ref': gerrit.GetChangeRef(1112, 2),
        'revision': 'ff10979dd360e75ff21f5cf53b7f8647578785ef',
    },
    'number': '1112',
    'subject': 'chromite commit',
    'owner': {
        'name': 'Chromite Master',
        'email': 'chromite@chromium.org',
    },
    'url': 'https://chromium-review.googlesource.com/1112',
    'lastUpdated': 1311024529,
    'sortKey': '00166e8700001052',
    'open': True,
    'status': 'NEW',
}

# List of labels as seen in the top level desc by normal CrOS devs.
FAKE_LABELS_JSON = {
    'Code-Review': {
        'default_value': 0,
        'values': {
            ' 0': 'No score',
            '+1': 'Looks good to me, but someone else must approve',
            '+2': 'Looks good to me, approved',
            '-1': 'I would prefer that you did not submit this',
            '-2': 'Do not submit',
        },
    },
    'Commit-Queue': {
        'default_value': 0,
        'optional': True,
        'values': {
            ' 0': 'Not Ready',
            '+1': 'Ready',
            '+2': 'Sheriff only: Ready, allow in throttled tree',
            '+3': 'Ignore this label',
        },
    },
    'Trybot-Ready': {
        'default_value': 0,
        'optional': True,
        'values': {
            ' 0': 'Not Ready',
            '+1': 'Ready',
        },
    },
    'Verified': {
        'default_value': 0,
        'values': {
            ' 0': 'No score',
            '+1': 'Verified',
            '-1': 'Fails',
        },
    },
}

# List of label values as seen in the permitted_labels field.
# Note: The space before the 0 is intentional -- it's what gerrit returns.
FAKE_PERMITTED_LABELS_JSON = {
    'Code-Review': ['-2', '-1', ' 0', '+1', '+2'],
    'Commit-Queue': [' 0', '+1', '+2'],
    'Trybot-Ready': [' 0', '+1'],
    'Verified': ['-1', ' 0', '+1'],
}

# An account structure as seen in owners or reviewers lists.
FAKE_GERRIT_ACCT_JSON = {
    '_account_id': 991919291,
    'avatars': [
        {'height': 26, 'url': 'https://example.com/26/photo.jpg'},
        {'height': 32, 'url': 'https://example.com/32/photo.jpg'},
        {'height': 100, 'url': 'https://example.com/100/photo.jpg'},
    ],
    'email': 'happy-funky-duck@chromium.org',
    'name': 'Duckworth Duck',
}

# A valid change json result as returned by gerrit.
FAKE_CHANGE_JSON = {
    '_number': 8366,
    'branch': 'master',
    'change_id': 'I3a753d6bacfbe76e5675a6f2f7941fe520c095e5',
    'created': '2016-09-14 23:02:46.000000000',
    'current_revision': 'be54f9935bedb157b078eefa26fc1885b8264da6',
    'deletions': 1,
    'hashtags': [],
    'id': 'example%2Frepo~master~I3a753d6bacfbe76e5675a6f2f7941fe520c095e5',
    'insertions': 0,
    'labels': FAKE_LABELS_JSON,
    'mergeable': True,
    'owner': FAKE_GERRIT_ACCT_JSON,
    'permitted_labels': FAKE_PERMITTED_LABELS_JSON,
    'project': 'example/repo',
    'removable_reviewers': [],
    'reviewers': {},
    'revisions': {
        'be54f9935bedb157b078eefa26fc1885b8264da6': {
            '_number': 1,
            'commit': {
                'author': {
                    'date': '2016-09-14 22:54:07.000000000',
                    'email': 'vapier@chromium.org',
                    'name': 'Mike Frysinger',
                    'tz': -240,
                },
                'committer': {
                    'date': '2016-09-14 23:02:03.000000000',
                    'email': 'vapier@chromium.org',
                    'name': 'Mike Frysinger',
                    'tz': -240,
                },
                'message': ('my super great message\n\nChange-Id: '
                            'I3a753d6bacfbe76e5675a6f2f7941fe520c095e5\n'),
                'parents': [
                    {
                        'commit': '3d54362a9b010330bae2dde973fc5c3efc4e5f44',
                        'subject': 'some parent commit',
                    },
                ],
                'subject': 'my super great message',
            },
            'created': '2016-09-14 23:02:46.000000000',
            'fetch': {
                'http': {
                    'ref': 'refs/changes/15/8366/1',
                    'url': 'https://chromium.googlesource.com/example/repo',
                },
                'repo': {
                    'ref': 'refs/changes/15/8366/1',
                    'url': 'example/repo',
                },
                'rpc': {
                    'ref': 'refs/changes/15/8366/1',
                    'url': 'rpc://chromium/example/repo',
                },
                'sso': {
                    'ref': 'refs/changes/15/8366/1',
                    'url': 'sso://chromium/example/repo',
                },
            },
            'kind': 'REWORK',
            'ref': 'refs/changes/15/8366/1',
            'uploader': FAKE_GERRIT_ACCT_JSON,
        },
    },
    'status': 'NEW',
    'subject': 'my super great message',
    'submit_type': 'CHERRY_PICK',
    'updated': '2016-09-14 23:02:46.000000000',
}


class GitRepoPatchTestCase(cros_test_lib.TempDirTestCase):
  """Helper TestCase class for writing test cases."""

  # No mock bits are to be used in this class's tests.
  # This needs to actually validate git output, and git behaviour, rather
  # than test our assumptions about git's behaviour/output.

  patch_kls = cros_patch.GitRepoPatch

  COMMIT_TEMPLATE = """\
commit abcdefgh

Author: Fake person
Date:  Tue Oct 99

I am the first commit.

%(extra)s

%(change-id)s
"""

  # Boolean controlling whether the target class natively knows its
  # ChangeId; only GerritPatches do.
  has_native_change_id = False

  DEFAULT_TRACKING = (
      'refs/remotes/%s/master' % site_config.params.EXTERNAL_REMOTE)

  def _CreateSourceRepo(self, path):
    """Generate a new repo with a single commit."""
    tmp_path = '%s-tmp' % path
    os.mkdir(path)
    os.mkdir(tmp_path)
    self._run(['git', 'init', '--separate-git-dir', path], cwd=tmp_path)

    # Add an initial commit then wipe the working tree.
    self._run(['git', 'commit', '--allow-empty', '-m', 'initial commit'],
              cwd=tmp_path)
    shutil.rmtree(tmp_path)

  def setUp(self):
    # Create an empty repo to work from.
    self.source = os.path.join(self.tempdir, 'source.git')
    self._CreateSourceRepo(self.source)
    self.default_cwd = os.path.join(self.tempdir, 'unwritable')
    self.original_cwd = os.getcwd()
    os.mkdir(self.default_cwd)
    os.chdir(self.default_cwd)
    # Disallow write so as to smoke out any invalid writes to
    # cwd.
    os.chmod(self.default_cwd, 0o500)

  def tearDown(self):
    if hasattr(self, 'original_cwd'):
      os.chdir(self.original_cwd)

  def _MkPatch(self, source, sha1, ref='refs/heads/master', **kwargs):
    # This arg is used by inherited versions of _MkPatch. Pop it to make this
    # _MkPatch compatible with them.
    kwargs.pop('suppress_branch', None)
    return self.patch_kls(source, 'chromiumos/chromite', ref,
                          '%s/master' % site_config.params.EXTERNAL_REMOTE,
                          kwargs.pop('remote',
                                     site_config.params.EXTERNAL_REMOTE),
                          sha1=sha1, **kwargs)

  def _run(self, cmd, cwd=None):
    # Note that cwd is intentionally set to a location the user can't write
    # to; this flushes out any bad usage in the tests that would work by
    # fluke of being invoked from w/in a git repo.
    if cwd is None:
      cwd = self.default_cwd
    return cros_build_lib.RunCommand(
        cmd, cwd=cwd, print_cmd=False, capture_output=True).output.strip()

  def _GetSha1(self, cwd, refspec):
    return self._run(['git', 'rev-list', '-n1', refspec], cwd=cwd)

  def _MakeRepo(self, name, clone, remote=None, alternates=True):
    path = os.path.join(self.tempdir, name)
    cmd = ['git', 'clone', clone, path]
    if alternates:
      cmd += ['--reference', clone]
    if remote is None:
      remote = site_config.params.EXTERNAL_REMOTE
    cmd += ['--origin', remote]
    self._run(cmd)
    return path

  def _MakeCommit(self, repo, commit=None):
    if commit is None:
      commit = 'commit at %s' % (time.time(),)
    self._run(['git', 'commit', '-a', '-m', commit], repo)
    return self._GetSha1(repo, 'HEAD')

  def CommitFile(self, repo, filename, content, commit=None, **kwargs):
    osutils.WriteFile(os.path.join(repo, filename), content)
    self._run(['git', 'add', filename], repo)
    sha1 = self._MakeCommit(repo, commit=commit)
    if not self.has_native_change_id:
      kwargs.pop('ChangeId', None)
    patch = self._MkPatch(repo, sha1, **kwargs)
    self.assertEqual(patch.sha1, sha1)
    return patch

  def _CommonGitSetup(self):
    git1 = self._MakeRepo('git1', self.source)
    git2 = self._MakeRepo('git2', self.source)
    patch = self.CommitFile(git1, 'monkeys', 'foon')
    return git1, git2, patch

  def MakeChangeId(self, how_many=1):
    l = [cros_patch.MakeChangeId() for _ in xrange(how_many)]
    if how_many == 1:
      return l[0]
    return l

  def CommitChangeIdFile(self, repo, changeid=None, extra=None,
                         filename='monkeys', content='flinging',
                         raw_changeid_text=None, **kwargs):
    template = self.COMMIT_TEMPLATE
    if changeid is None:
      changeid = self.MakeChangeId()
    if raw_changeid_text is None:
      raw_changeid_text = 'Change-Id: %s' % (changeid,)
    if extra is None:
      extra = ''
    commit = template % {'change-id': raw_changeid_text, 'extra': extra}

    return self.CommitFile(repo, filename, content, commit=commit,
                           ChangeId=changeid, **kwargs)


# pylint: disable=protected-access
class TestGitRepoPatch(GitRepoPatchTestCase):
  """Unittests for git patch related methods."""

  def testGetDiffStatus(self):
    git1, _, patch1 = self._CommonGitSetup()
    # Ensure that it can work on the first commit, even if it
    # doesn't report anything (no delta; it's the first files).
    patch1 = self._MkPatch(git1, self._GetSha1(git1, self.DEFAULT_TRACKING))
    self.assertEqual({}, patch1.GetDiffStatus(git1))
    patch2 = self.CommitFile(git1, 'monkeys', 'blah')
    self.assertEqual({'monkeys': 'M'}, patch2.GetDiffStatus(git1))
    git.RunGit(git1, ['mv', 'monkeys', 'monkeys2'])
    patch3 = self._MkPatch(git1, self._MakeCommit(git1, commit='mv'))
    self.assertEqual({'monkeys': 'D', 'monkeys2': 'A'},
                     patch3.GetDiffStatus(git1))
    patch4 = self.CommitFile(git1, 'monkey2', 'blah')
    self.assertEqual({'monkey2': 'A'}, patch4.GetDiffStatus(git1))

  def testFetch(self):
    _, git2, patch = self._CommonGitSetup()
    patch.Fetch(git2)
    self.assertEqual(patch.sha1, self._GetSha1(git2, 'FETCH_HEAD'))
    # Verify reuse; specifically that Fetch doesn't actually run since
    # the rev is already available locally via alternates.
    patch.project_url = '/dev/null'
    git3 = self._MakeRepo('git3', git2)
    patch.Fetch(git3)
    self.assertEqual(patch.sha1, self._GetSha1(git3, patch.sha1))

  def testFetchFirstPatchInSeries(self):
    git1, git2, patch = self._CommonGitSetup()
    self.CommitFile(git1, 'monkeys', 'foon2')
    patch.Fetch(git2)

  def testFetchWithoutSha1(self):
    git1, git2, _ = self._CommonGitSetup()
    patch2 = self.CommitFile(git1, 'monkeys', 'foon2')
    sha1, patch2.sha1 = patch2.sha1, None
    patch2.Fetch(git2)
    self.assertEqual(sha1, patch2.sha1)

  def testParentless(self):
    git1 = self._MakeRepo('git1', self.source)
    patch1 = self._MkPatch(git1, self._GetSha1(git1, 'HEAD'))
    self.assertRaises2(cros_patch.PatchNoParents, patch1.Apply, git1,
                       self.DEFAULT_TRACKING, check_attrs={'inflight': False})

  def testNoParentOrAlreadyApplied(self):
    git1 = self._MakeRepo('git1', self.source)
    patch1 = self._MkPatch(git1, self._GetSha1(git1, 'HEAD'))
    self.assertRaises2(cros_patch.PatchNoParents, patch1.Apply, git1,
                       self.DEFAULT_TRACKING, check_attrs={'inflight': False})
    patch2 = self.CommitFile(git1, 'monkeys', 'rule')
    self.assertRaises2(cros_patch.PatchIsEmpty, patch2.Apply, git1,
                       self.DEFAULT_TRACKING, check_attrs={'inflight': True})

  def testGetNoParents(self):
    git1 = self._MakeRepo('git1', self.source)
    sha1 = self._GetSha1(git1, 'HEAD')
    patch = self._MkPatch(self.source, sha1)
    self.assertEquals(patch._GetParents(git1), [])

  def testGet1Parent(self):
    git1 = self._MakeRepo('git1', self.source)
    patch1 = self.CommitFile(git1, 'foo', 'foo')
    patch2 = self.CommitFile(git1, 'bar', 'bar')
    self.assertEquals(patch2._GetParents(git1), [patch1.sha1])

  def testGet2Parents(self):
    # Prepare a merge commit, then test that its two parents are correctly
    # calculated.
    git1 = self._MakeRepo('git1', self.source)
    patch_common = self.CommitFile(git1, 'foo', 'foo')

    patch_right = self.CommitFile(git1, 'bar', 'bar')

    git.RunGit(git1, ['reset', '--hard', patch_common.sha1])
    patch_left = self.CommitFile(git1, 'baz', 'baz')

    git.RunGit(git1, ['merge', patch_right.sha1])
    sha1 = self._GetSha1(git1, 'HEAD')
    patch_merge = self._MkPatch(self.source, sha1, suppress_branch=True)

    self.assertEquals(patch_merge._GetParents(git1),
                      [patch_left.sha1, patch_right.sha1])

  def testIsAncestor(self):
    git1 = self._MakeRepo('git1', self.source)
    patch1 = self.CommitFile(git1, 'foo', 'foo')
    patch2 = self.CommitFile(git1, 'bar', 'bar')
    self.assertTrue(patch1._IsAncestorOf(git1, patch1))
    self.assertTrue(patch1._IsAncestorOf(git1, patch2))
    self.assertFalse(patch2._IsAncestorOf(git1, patch1))

  def testFromSha1(self):
    git1 = self._MakeRepo('git1', self.source)
    patch1 = self.CommitFile(git1, 'foo', 'foo')
    patch2 = self.CommitFile(git1, 'bar', 'bar')
    patch2_from_sha1 = patch1._FromSha1(patch2.sha1)
    patch2_from_sha1.Fetch(git1)
    patch2.Fetch(git1)
    self.assertEqual(patch2.tree_hash, patch2_from_sha1.tree_hash)

  def testValidateMerge(self):
    git1 = self._MakeRepo('git1', self.source)

    # Prepare history like this:
    #   * E (upstream)
    # * | D (merge being handled)
    # |\|
    # * | C
    # | * B
    # |/
    # *   A
    #
    # D is valid to merge into E.
    A = self.CommitFile(git1, 'A', 'A')
    B = self.CommitFile(git1, 'B', 'B')
    E = self.CommitFile(git1, 'E', 'E')
    git.RunGit(git1, ['reset', '--hard', A.sha1])
    C = self.CommitFile(git1, 'C', 'C')
    git.RunGit(git1, ['merge', B.sha1])
    sha1 = self._GetSha1(git1, 'HEAD')
    D = self._MkPatch(self.source, sha1, suppress_branch=True)

    D._ValidateMergeCommit(git1, E.sha1, [C.sha1, B.sha1])

  def testValidateMergeFailure(self):
    git1 = self._MakeRepo('git1', self.source)
    # *     F (merge being handled)
    # |\
    # | *   E
    # * |   D
    # | | * C (upstream)
    # | |/
    # | *   B
    # |/
    # *     A
    #
    # F is not valid to merge into E.
    A = self.CommitFile(git1, 'A', 'A')
    B = self.CommitFile(git1, 'B', 'B')
    C = self.CommitFile(git1, 'C', 'C')
    git.RunGit(git1, ['reset', '--hard', B.sha1])
    E = self.CommitFile(git1, 'E', 'E')
    git.RunGit(git1, ['reset', '--hard', A.sha1])
    D = self.CommitFile(git1, 'D', 'D')
    git.RunGit(git1, ['merge', E.sha1])
    sha1 = self._GetSha1(git1, 'HEAD')
    F = self._MkPatch(self.source, sha1, suppress_branch=True)

    with self.assertRaises(cros_patch.NonMainlineMerge):
      F._ValidateMergeCommit(git1, C.sha1, [D.sha1, E.sha1])

  def testDeleteEbuildTwice(self):
    """Test that double-deletes of ebuilds are flagged as conflicts."""
    # Create monkeys.ebuild for testing.
    git1 = self._MakeRepo('git1', self.source)
    patch1 = self.CommitFile(git1, 'monkeys.ebuild', 'rule')
    git.RunGit(git1, ['rm', 'monkeys.ebuild'])
    patch2 = self._MkPatch(git1, self._MakeCommit(git1, commit='rm'))

    # Delete an ebuild that does not exist in TOT.
    check_attrs = {'inflight': False, 'files': ('monkeys.ebuild',)}
    self.assertRaises2(cros_patch.EbuildConflict, patch2.Apply, git1,
                       self.DEFAULT_TRACKING, check_attrs=check_attrs)

    # Delete an ebuild that exists in TOT, but does not exist in the current
    # patch series.
    check_attrs['inflight'] = True
    self.assertRaises2(cros_patch.EbuildConflict, patch2.Apply, git1,
                       patch1.sha1, check_attrs=check_attrs)

  def testCleanlyApply(self):
    _, git2, patch = self._CommonGitSetup()
    # Clone git3 before we modify git2; else we'll just wind up
    # cloning its master.
    git3 = self._MakeRepo('git3', git2)
    patch.Apply(git2, self.DEFAULT_TRACKING)
    # Verify reuse; specifically that Fetch doesn't actually run since
    # the object is available in alternates.  testFetch partially
    # validates this; the Apply usage here fully validates it via
    # ensuring that the attempted Apply goes boom if it can't get the
    # required sha1.
    patch.project_url = '/dev/null'
    patch.Apply(git3, self.DEFAULT_TRACKING)

  def testFailsApply(self):
    _, git2, patch1 = self._CommonGitSetup()
    patch2 = self.CommitFile(git2, 'monkeys', 'not foon')
    # Note that Apply creates it's own branch, resetting to master
    # thus we have to re-apply (even if it looks stupid, it's right).
    patch2.Apply(git2, self.DEFAULT_TRACKING)
    self.assertRaises2(cros_patch.ApplyPatchException,
                       patch1.Apply, git2, self.DEFAULT_TRACKING,
                       exact_kls=True, check_attrs={'inflight': True})

  def testTrivial(self):
    _, git2, patch1 = self._CommonGitSetup()
    # Throw in a bunch of newlines so that content-merging would work.
    content = 'not foon%s' % ('\n' * 100)
    patch1 = self._MkPatch(git2, self._GetSha1(git2, 'HEAD'))
    patch1 = self.CommitFile(git2, 'monkeys', content)
    git.RunGit(
        git2, ['update-ref', self.DEFAULT_TRACKING, patch1.sha1])
    patch2 = self.CommitFile(git2, 'monkeys', '%sblah' % content)
    patch3 = self.CommitFile(git2, 'monkeys', '%sblahblah' % content)
    # Get us a back to the basic, then derive from there; this is used to
    # verify that even if content merging works, trivial is flagged.
    self.CommitFile(git2, 'monkeys', 'foon')
    patch4 = self.CommitFile(git2, 'monkeys', content)
    patch5 = self.CommitFile(git2, 'monkeys', '%sfoon' % content)
    # Reset so we derive the next changes from patch1.
    git.RunGit(git2, ['reset', '--hard', patch1.sha1])
    patch6 = self.CommitFile(git2, 'blah', 'some-other-file')
    self.CommitFile(git2, 'monkeys',
                    '%sblah' % content.replace('not', 'bot'))

    self.assertRaises2(cros_patch.PatchIsEmpty,
                       patch1.Apply, git2, self.DEFAULT_TRACKING, trivial=True,
                       check_attrs={'inflight': False, 'trivial': False})

    # Now test conflicts since we're still at ToT; note that this is an actual
    # conflict because the fuzz anchors have changed.
    self.assertRaises2(cros_patch.ApplyPatchException,
                       patch3.Apply, git2, self.DEFAULT_TRACKING, trivial=True,
                       check_attrs={'inflight': False, 'trivial': False},
                       exact_kls=True)

    # Now test trivial conflict; this would've merged fine were it not for
    # trivial.
    self.assertRaises2(cros_patch.PatchIsEmpty,
                       patch4.Apply, git2, self.DEFAULT_TRACKING, trivial=True,
                       check_attrs={'inflight': False, 'trivial': False},
                       exact_kls=True)

    # Move us into inflight testing.
    patch2.Apply(git2, self.DEFAULT_TRACKING, trivial=True)

    # Repeat the tests from above; should still be the same.
    self.assertRaises2(cros_patch.PatchIsEmpty,
                       patch4.Apply, git2, self.DEFAULT_TRACKING, trivial=True,
                       check_attrs={'inflight': False, 'trivial': False})

    # Actual conflict merge conflict due to inflight; non trivial induced.
    self.assertRaises2(cros_patch.ApplyPatchException,
                       patch5.Apply, git2, self.DEFAULT_TRACKING, trivial=True,
                       check_attrs={'inflight': True, 'trivial': False},
                       exact_kls=True)

    self.assertRaises2(cros_patch.PatchIsEmpty,
                       patch1.Apply, git2, self.DEFAULT_TRACKING, trivial=True,
                       check_attrs={'inflight': False})

    self.assertRaises2(cros_patch.ApplyPatchException,
                       patch5.Apply, git2, self.DEFAULT_TRACKING, trivial=True,
                       check_attrs={'inflight': True, 'trivial': False},
                       exact_kls=True)

    # And this should apply without issue, despite the differing history.
    patch6.Apply(git2, self.DEFAULT_TRACKING, trivial=True)

  def _assertLookupAliases(self, remote):
    git1 = self._MakeRepo('git1', self.source)
    patch = self.CommitChangeIdFile(git1, remote=remote)
    prefix = '*' if patch.internal else ''
    vals = [patch.sha1, getattr(patch, 'gerrit_number', None),
            getattr(patch, 'original_sha1', None)]
    # Append full Change-ID if it exists.
    if patch.project and patch.tracking_branch and patch.change_id:
      vals.append('%s~%s~%s' % (
          patch.project, patch.tracking_branch, patch.change_id))
    vals = [x for x in vals if x is not None]
    self.assertEqual(set(prefix + x for x in vals), set(patch.LookupAliases()))

  def testExternalLookupAliases(self):
    self._assertLookupAliases(site_config.params.EXTERNAL_REMOTE)

  def testInternalLookupAliases(self):
    self._assertLookupAliases(site_config.params.INTERNAL_REMOTE)

  def _CheckPaladin(self, repo, master_id, ids, extra):
    patch = self.CommitChangeIdFile(
        repo, master_id, extra=extra,
        filename='paladincheck', content=str(_GetNumber()))
    deps = patch.PaladinDependencies(repo)
    # Assert that our parsing unique'ifies the results.
    self.assertEqual(len(deps), len(set(deps)))
    # Verify that we have the correct dependencies.
    dep_ids = []
    dep_ids += [(dep.remote, dep.change_id) for dep in deps
                if dep.change_id is not None]
    dep_ids += [(dep.remote, dep.gerrit_number) for dep in deps
                if dep.gerrit_number is not None]
    dep_ids += [(dep.remote, dep.sha1) for dep in deps
                if dep.sha1 is not None]
    for input_id in ids:
      change_tuple = cros_patch.StripPrefix(input_id)
      self.assertIn(change_tuple, dep_ids)

    return patch

  def testPaladinDependencies(self):
    git1 = self._MakeRepo('git1', self.source)
    cid1, cid2, cid3, cid4 = self.MakeChangeId(4)
    # Verify it handles nonexistant CQ-DEPEND.
    self._CheckPaladin(git1, cid1, [], '')
    # Single key, single value.
    self._CheckPaladin(git1, cid1, [cid2],
                       'CQ-DEPEND=%s' % cid2)
    # Single key, gerrit number.
    self._CheckPaladin(git1, cid1, ['123'],
                       'CQ-DEPEND=%s' % 123)
    # Single key, gerrit number.
    self._CheckPaladin(git1, cid1, ['123456'],
                       'CQ-DEPEND=%s' % 123456)
    # Single key, gerrit number; ensure it
    # cuts off before a million changes (this
    # is done to avoid collisions w/ sha1 when
    # we're using shortened versions).
    self.assertRaises(cros_patch.BrokenCQDepends,
                      self._CheckPaladin, git1, cid1,
                      ['123456789'], 'CQ-DEPEND=%s' % '123456789')
    # Single key, gerrit number, internal.
    self._CheckPaladin(git1, cid1, ['*123'],
                       'CQ-DEPEND=%s' % '*123')
    # Ensure SHA1's aren't allowed.
    sha1 = '0' * 40
    self.assertRaises(cros_patch.BrokenCQDepends,
                      self._CheckPaladin, git1, cid1,
                      [sha1], 'CQ-DEPEND=%s' % sha1)

    # Single key, multiple values
    self._CheckPaladin(git1, cid1, [cid2, '1223'],
                       'CQ-DEPEND=%s %s' % (cid2, '1223'))
    # Dumb comma behaviour
    self._CheckPaladin(git1, cid1, [cid2, cid3],
                       'CQ-DEPEND=%s, %s,' % (cid2, cid3))
    # Multiple keys.
    self._CheckPaladin(git1, cid1, [cid2, '*245', cid4],
                       'CQ-DEPEND=%s, %s\nCQ-DEPEND=%s' % (cid2, '*245', cid4))

    # Ensure it goes boom on invalid data.
    self.assertRaises(cros_patch.BrokenCQDepends, self._CheckPaladin,
                      git1, cid1, [], 'CQ-DEPEND=monkeys')
    self.assertRaises(cros_patch.BrokenCQDepends, self._CheckPaladin,
                      git1, cid1, [], 'CQ-DEPEND=%s monkeys' % (cid2,))
    # Validate numeric is allowed.
    self._CheckPaladin(git1, cid1, [cid2, '1'], 'CQ-DEPEND=1 %s' % cid2)
    # Validate that it unique'ifies the results.
    self._CheckPaladin(git1, cid1, ['1'], 'CQ-DEPEND=1 1')

    # Invalid syntax
    self.assertRaises(cros_patch.BrokenCQDepends, self._CheckPaladin,
                      git1, cid1, [], 'CQ-DEPENDS=1')
    self.assertRaises(cros_patch.BrokenCQDepends, self._CheckPaladin,
                      git1, cid1, [], 'CQ_DEPEND=1')
    self.assertRaises(cros_patch.BrokenCQDepends, self._CheckPaladin,
                      git1, cid1, [], ' CQ-DEPEND=1')

  def testChangeIdMetadata(self):
    """Verify Change-Id is set in git metadata."""
    git1, git2, _ = self._CommonGitSetup()
    changeid = 'I%s' % ('1'.rjust(40, '0'))
    patch = self.CommitChangeIdFile(git1, changeid=changeid, change_id=changeid,
                                    raw_changeid_text='')
    patch.change_id = changeid
    patch.Fetch(git1)
    self.assertIn('Change-Id: %s\n' % changeid, patch.commit_message)
    patch = self.CommitChangeIdFile(git2, changeid=changeid, change_id=changeid)
    patch.Fetch(git2)
    self.assertEqual(patch.change_id, changeid)
    self.assertIn('Change-Id: %s\n' % changeid, patch.commit_message)


class TestGerritFetchOnlyPatch(cros_test_lib.MockTestCase):
  """Test of GerritFetchOnlyPatch."""

  def testFromAttrDict(self):
    """Test whether FromAttrDict can handle with commit message."""
    attr_dict_without_msg = {
        cros_patch.ATTR_PROJECT_URL: 'https://host/chromite/tacos',
        cros_patch.ATTR_PROJECT: 'chromite/tacos',
        cros_patch.ATTR_REF: 'refs/changes/11/12345/4',
        cros_patch.ATTR_BRANCH: 'master',
        cros_patch.ATTR_REMOTE: 'cros-internal',
        cros_patch.ATTR_COMMIT: '7181e4b5e182b6f7d68461b04253de095bad74f9',
        cros_patch.ATTR_CHANGE_ID: 'I47ea30385af60ae4cc2acc5d1a283a46423bc6e1',
        cros_patch.ATTR_GERRIT_NUMBER: '12345',
        cros_patch.ATTR_PATCH_NUMBER: '4',
        cros_patch.ATTR_OWNER_EMAIL: 'foo@chromium.org',
        cros_patch.ATTR_FAIL_COUNT: 1,
        cros_patch.ATTR_PASS_COUNT: 1,
        cros_patch.ATTR_TOTAL_FAIL_COUNT: 3}

    attr_dict_with_msg = {
        cros_patch.ATTR_PROJECT_URL: 'https://host/chromite/tacos',
        cros_patch.ATTR_PROJECT: 'chromite/tacos',
        cros_patch.ATTR_REF: 'refs/changes/11/12345/4',
        cros_patch.ATTR_BRANCH: 'master',
        cros_patch.ATTR_REMOTE: 'cros-internal',
        cros_patch.ATTR_COMMIT: '7181e4b5e182b6f7d68461b04253de095bad74f9',
        cros_patch.ATTR_CHANGE_ID: 'I47ea30385af60ae4cc2acc5d1a283a46423bc6e1',
        cros_patch.ATTR_GERRIT_NUMBER: '12345',
        cros_patch.ATTR_PATCH_NUMBER: '4',
        cros_patch.ATTR_OWNER_EMAIL: 'foo@chromium.org',
        cros_patch.ATTR_FAIL_COUNT: 1,
        cros_patch.ATTR_PASS_COUNT: 1,
        cros_patch.ATTR_TOTAL_FAIL_COUNT: 3,
        cros_patch.ATTR_COMMIT_MESSAGE: 'commit message'}

    self.PatchObject(cros_patch.GitRepoPatch, '_AddFooters',
                     return_value='commit message')

    result_1 = (cros_patch.GerritFetchOnlyPatch.
                FromAttrDict(attr_dict_without_msg).commit_message)
    result_2 = (cros_patch.GerritFetchOnlyPatch.
                FromAttrDict(attr_dict_with_msg).commit_message)
    self.assertEqual(None, result_1)
    self.assertEqual('commit message', result_2)

  def testGetAttributeDict(self):
    """Test Whether GetAttributeDict can get the commit message properly."""
    change = cros_patch.GerritFetchOnlyPatch(
        'https://host/chromite/tacos',
        'chromite/tacos',
        'refs/changes/11/12345/4',
        'master',
        'cros-internal',
        '7181e4b5e182b6f7d68461b04253de095bad74f9',
        'I47ea30385af60ae4cc2acc5d1a283a46423bc6e1',
        '12345',
        '4',
        'foo@chromium.org',
        1,
        1,
        3)

    expected = {
        cros_patch.ATTR_PROJECT_URL: 'https://host/chromite/tacos',
        cros_patch.ATTR_PROJECT: 'chromite/tacos',
        cros_patch.ATTR_REF: 'refs/changes/11/12345/4',
        cros_patch.ATTR_BRANCH: 'master',
        cros_patch.ATTR_REMOTE: 'cros-internal',
        cros_patch.ATTR_COMMIT: '7181e4b5e182b6f7d68461b04253de095bad74f9',
        cros_patch.ATTR_CHANGE_ID: 'I47ea30385af60ae4cc2acc5d1a283a46423bc6e1',
        cros_patch.ATTR_GERRIT_NUMBER: '12345',
        cros_patch.ATTR_PATCH_NUMBER: '4',
        cros_patch.ATTR_OWNER_EMAIL: 'foo@chromium.org',
        cros_patch.ATTR_FAIL_COUNT: '1',
        cros_patch.ATTR_PASS_COUNT: '1',
        cros_patch.ATTR_TOTAL_FAIL_COUNT: '3',
        cros_patch.ATTR_COMMIT_MESSAGE: None}
    self.assertEqual(change.GetAttributeDict(), expected)

    self.PatchObject(cros_patch.GitRepoPatch, '_AddFooters',
                     return_value='commit message')
    change.commit_message = 'commit message'
    expected[cros_patch.ATTR_COMMIT_MESSAGE] = 'commit message'
    self.assertEqual(change.GetAttributeDict(), expected)


class TestGetOptionLinesFromCommitMessage(cros_test_lib.TestCase):
  """Tests of GetOptionFromCommitMessage."""

  _M1 = """jabberwocky: by Lewis Carroll

'Twas brillig, and the slithy toves
did gyre and gimble in the wabe.
"""

  _M2 = """jabberwocky: by Lewis Carroll

All mimsy were the borogroves,
And the mome wraths outgrabe.
jabberwocky: Charles Lutwidge Dodgson
"""

  _M3 = """jabberwocky: by Lewis Carroll

He took his vorpal sword in hand:
Long time the manxome foe he sought
jabberwocky:
"""

  _M4 = """the poem continues...

jabberwocky: O frabjuous day!
jabberwocky: Calloh! Callay!
"""

  def testNoMessage(self):
    o = cros_patch.GetOptionLinesFromCommitMessage('', 'jabberwocky:')
    self.assertEqual(None, o)

  def testNoOption(self):
    o = cros_patch.GetOptionLinesFromCommitMessage(self._M1, 'jabberwocky:')
    self.assertEqual(None, o)

  def testYesOption(self):
    o = cros_patch.GetOptionLinesFromCommitMessage(self._M2, 'jabberwocky:')
    self.assertEqual(['Charles Lutwidge Dodgson'], o)

  def testEmptyOption(self):
    o = cros_patch.GetOptionLinesFromCommitMessage(self._M3, 'jabberwocky:')
    self.assertEqual([], o)

  def testMultiOption(self):
    o = cros_patch.GetOptionLinesFromCommitMessage(self._M4, 'jabberwocky:')
    self.assertEqual(['O frabjuous day!', 'Calloh! Callay!'], o)


class TestApplyAgainstManifest(GitRepoPatchTestCase,
                               cros_test_lib.MockTestCase):
  """Test applying a patch against a manifest"""

  MANIFEST_TEMPLATE = """\
<?xml version="1.0" encoding="UTF-8"?>
<manifest>
  <remote name="cros" />
  <default revision="refs/heads/master" remote="cros" />
  %(projects)s
</manifest>
"""

  def _CommonRepoSetup(self, *projects):
    basedir = self.tempdir
    repodir = os.path.join(basedir, '.repo')
    manifest_file = os.path.join(repodir, 'manifest.xml')
    proj_pieces = []
    for project in projects:
      proj_pieces.append('<project')
      for key, val in project.items():
        if key == 'path':
          val = os.path.relpath(os.path.realpath(val),
                                os.path.realpath(self.tempdir))
        proj_pieces.append(' %s="%s"' % (key, val))
      proj_pieces.append(' />\n  ')
    proj_str = ''.join(proj_pieces)
    content = self.MANIFEST_TEMPLATE % {'projects': proj_str}
    os.mkdir(repodir)
    osutils.WriteFile(manifest_file, content)
    return basedir

  def testApplyAgainstManifest(self):
    git1, git2, _ = self._CommonGitSetup()

    readme_text = 'Dummy README text.'
    readme1 = self.CommitFile(git1, 'README', readme_text)
    readme_text += ' Even more dummy README text.'
    readme2 = self.CommitFile(git1, 'README', readme_text)
    readme_text += ' Even more README text.'
    readme3 = self.CommitFile(git1, 'README', readme_text)

    git1_proj = {
        'path': git1,
        'name': 'chromiumos/chromite',
        'revision': str(readme1.sha1),
        'upstream': 'refs/heads/master',
    }
    git2_proj = {
        'path': git2,
        'name': 'git2',
    }
    basedir = self._CommonRepoSetup(git1_proj, git2_proj)

    self.PatchObject(git.ManifestCheckout, '_GetManifestsBranch',
                     return_value=None)
    manifest = git.ManifestCheckout(basedir)

    readme2.ApplyAgainstManifest(manifest)
    readme3.ApplyAgainstManifest(manifest)

    # Verify that both readme2 and readme3 are on the patch branch.
    cmd = ['git', 'log', '--format=%T',
           '%s..%s' % (readme1.sha1, constants.PATCH_BRANCH)]
    trees = self._run(cmd, git1).splitlines()
    self.assertEqual(trees, [str(readme3.tree_hash), str(readme2.tree_hash)])


class TestLocalPatchGit(GitRepoPatchTestCase):
  """Test Local patch handling."""

  patch_kls = cros_patch.LocalPatch

  def setUp(self):
    self.sourceroot = os.path.join(self.tempdir, 'sourceroot')

  def _MkPatch(self, source, sha1, ref='refs/heads/master', **kwargs):
    remote = kwargs.pop('remote', site_config.params.EXTERNAL_REMOTE)
    return self.patch_kls(source, 'chromiumos/chromite', ref,
                          '%s/master' % remote, remote, sha1, **kwargs)

  def testUpload(self):
    def ProjectDirMock(_sourceroot):
      return git1

    git1, git2, patch = self._CommonGitSetup()

    git2_sha1 = self._GetSha1(git2, 'HEAD')

    patch.ProjectDir = ProjectDirMock
    # First suppress carbon copy behaviour so we verify pushing plain works.
    sha1 = patch.sha1
    patch._GetCarbonCopy = lambda: sha1  # pylint: disable=protected-access
    patch.Upload(git2, 'refs/testing/test1')
    self.assertEqual(self._GetSha1(git2, 'refs/testing/test1'),
                     patch.sha1)

    # Enable CarbonCopy behaviour; verify it lands a different
    # sha1.  Additionally verify it didn't corrupt the patch's sha1 locally.
    del patch._GetCarbonCopy
    patch.Upload(git2, 'refs/testing/test2')
    self.assertNotEqual(self._GetSha1(git2, 'refs/testing/test2'),
                        patch.sha1)
    self.assertEqual(patch.sha1, sha1)
    # Ensure the carbon creation didn't damage the target repo.
    self.assertEqual(self._GetSha1(git1, 'HEAD'), sha1)

    # Ensure we didn't damage the target repo's state at all.
    self.assertEqual(git2_sha1, self._GetSha1(git2, 'HEAD'))
    # Ensure the content is the same.
    base = ['git', 'show']
    self.assertEqual(
        self._run(base + ['refs/testing/test1:monkeys'], git2),
        self._run(base + ['refs/testing/test2:monkeys'], git2))
    base = ['git', 'log', '--format=%B', '-n1']
    self.assertEqual(
        self._run(base + ['refs/testing/test1'], git2),
        self._run(base + ['refs/testing/test2'], git2))


class UploadedLocalPatchTestCase(GitRepoPatchTestCase):
  """Test uploading of local git patches."""

  PROJECT = 'chromiumos/chromite'
  ORIGINAL_BRANCH = 'original_branch'
  ORIGINAL_SHA1 = 'ffffffff'.ljust(40, '0')

  patch_kls = cros_patch.UploadedLocalPatch

  def _MkPatch(self, source, sha1, ref='refs/heads/master', **kwargs):
    return self.patch_kls(source, self.PROJECT, ref,
                          '%s/master' % site_config.params.EXTERNAL_REMOTE,
                          self.ORIGINAL_BRANCH,
                          kwargs.pop('original_sha1', self.ORIGINAL_SHA1),
                          kwargs.pop('remote',
                                     site_config.params.EXTERNAL_REMOTE),
                          carbon_copy_sha1=sha1, **kwargs)


class TestUploadedLocalPatch(UploadedLocalPatchTestCase):
  """Test uploading of local git patches."""

  def testStringRepresentation(self):
    _, _, patch = self._CommonGitSetup()
    str_rep = str(patch).split(':')
    for element in [self.PROJECT, self.ORIGINAL_BRANCH, self.ORIGINAL_SHA1[:8]]:
      self.assertTrue(element in str_rep,
                      msg="Couldn't find %s in %s" % (element, str_rep))


class TestGerritPatch(TestGitRepoPatch):
  """Test Gerrit patch handling."""

  has_native_change_id = True

  class patch_kls(cros_patch.GerritPatch):
    """Test helper class to suppress pointing to actual gerrit."""
    # Suppress the behaviour pointing the project url at actual gerrit,
    # instead slaving it back to a local repo for tests.
    def __init__(self, *args, **kwargs):
      cros_patch.GerritPatch.__init__(self, *args, **kwargs)
      assert hasattr(self, 'patch_dict')
      self.project_url = self.patch_dict['_unittest_url_bypass']

  @property
  def test_json(self):
    return copy.deepcopy(FAKE_PATCH_JSON)

  def _MkPatch(self, source, sha1, ref='refs/heads/master', **kwargs):
    json = self.test_json
    remote = kwargs.pop('remote', site_config.params.EXTERNAL_REMOTE)
    url_prefix = kwargs.pop('url_prefix',
                            site_config.params.EXTERNAL_GERRIT_URL)
    suppress_branch = kwargs.pop('suppress_branch', False)
    change_id = kwargs.pop('ChangeId', None)
    if change_id is None:
      change_id = self.MakeChangeId()
    json.update(kwargs)
    change_num, patch_num = _GetNumber(), _GetNumber()
    # Note we intentionally use a gerrit like refspec here; we want to
    # ensure that none of our common code pathways puke on a non head/tag.
    refspec = gerrit.GetChangeRef(change_num + 1000, patch_num)
    json['currentPatchSet'].update(
        dict(number=patch_num, ref=refspec, revision=sha1))
    json['branch'] = os.path.basename(ref)
    json['_unittest_url_bypass'] = source
    json['id'] = change_id

    obj = self.patch_kls(json.copy(), remote, url_prefix)
    self.assertEqual(obj.patch_dict, json)
    self.assertEqual(obj.remote, remote)
    self.assertEqual(obj.url_prefix, url_prefix)
    self.assertEqual(obj.project, json['project'])
    self.assertEqual(obj.ref, refspec)
    self.assertEqual(obj.change_id, change_id)
    self.assertEqual(obj.id, '%s%s~%s~%s' % (
        site_config.params.CHANGE_PREFIX[remote], json['project'],
        json['branch'], change_id))

    # Now make the fetching actually work, if desired.
    if not suppress_branch:
      # Note that a push is needed here, rather than a branch; branch
      # will just make it under refs/heads, we want it literally in
      # refs/changes/
      self._run(['git', 'push', source, '%s:%s' % (sha1, refspec)], source)
    return obj

  def testApprovalTimestamp(self):
    """Test that the approval timestamp is correctly extracted from JSON."""
    repo = self._MakeRepo('git', self.source)
    for approvals, expected in [(None, 0), ([], 0), ([1], 1), ([1, 3, 2], 3)]:
      currentPatchSet = copy.deepcopy(FAKE_PATCH_JSON['currentPatchSet'])
      if approvals is not None:
        currentPatchSet['approvals'] = [{'grantedOn': x} for x in approvals]
      patch = self._MkPatch(repo, self._GetSha1(repo, self.DEFAULT_TRACKING),
                            currentPatchSet=currentPatchSet)
      msg = 'Expected %r, but got %r (approvals=%r)' % (
          expected, patch.approval_timestamp, approvals)
      self.assertEqual(patch.approval_timestamp, expected, msg)

  def _assertGerritDependencies(self,
                                remote=site_config.params.EXTERNAL_REMOTE):
    convert = str
    if remote == site_config.params.INTERNAL_REMOTE:
      convert = lambda val: '*%s' % (val,)
    git1 = self._MakeRepo('git1', self.source, remote=remote)
    patch = self._MkPatch(git1, self._GetSha1(git1, 'HEAD'), remote=remote)
    cid1, cid2 = '1', '2'

    # Test cases with no dependencies, 1 dependency, and 2 dependencies.
    self.assertEqual(patch.GerritDependencies(), [])
    patch.patch_dict['dependsOn'] = [{'number': cid1}]
    self.assertEqual(
        [cros_patch.AddPrefix(x, x.gerrit_number)
         for x in patch.GerritDependencies()],
        [convert(cid1)])
    patch.patch_dict['dependsOn'].append({'number': cid2})
    self.assertEqual(
        [cros_patch.AddPrefix(x, x.gerrit_number)
         for x in patch.GerritDependencies()],
        [convert(cid1), convert(cid2)])

  def testExternalGerritDependencies(self):
    self._assertGerritDependencies()

  def testInternalGerritDependencies(self):
    self._assertGerritDependencies(site_config.params.INTERNAL_REMOTE)

  def testReviewedOnMetadata(self):
    """Verify Change-Id and Reviewed-On are set in git metadata."""
    git1, _, patch = self._CommonGitSetup()
    patch.Apply(git1, self.DEFAULT_TRACKING)
    reviewed_on = '/'.join([site_config.params.EXTERNAL_GERRIT_URL,
                            patch.gerrit_number])
    self.assertIn('Reviewed-on: %s\n' % reviewed_on, patch.commit_message)

  def _MakeFooters(self):
    return (
        (),
        (('Footer-1', 'foo'),),
        (('Change-id', '42'),),
        (('Footer-1', 'foo'), ('Change-id', '42')),)

  def _MakeCommitMessages(self):
    headers = (
        'A standard commit message header',
        '',
        'Footer-1: foo',
        'Change-id: 42')

    bodies = (
        '',
        '\n',
        'Lots of comments\n about the commit\n' * 100)

    for header, body, preexisting in itertools.product(headers,
                                                       bodies,
                                                       self._MakeFooters()):
      yield '\n'.join((header,
                       body,
                       '\n'.join('%s: %s' for tag, ident in preexisting)))

  def testAddFooters(self):
    repo = self._MakeRepo('git', self.source)
    patch = self._MkPatch(repo, self._GetSha1(repo, 'HEAD'))
    approval = {'type': 'VRIF', 'value': '1', 'grantedOn': 1391733002}

    for msg in self._MakeCommitMessages():
      for footers in self._MakeFooters():
        ctx = contextlib.nested(
            mock.patch('chromite.lib.patch.FooterForApproval',
                       new=mock.Mock(side_effect=itertools.cycle(footers))),
            mock.patch.object(patch, '_approvals',
                              new=[approval] * len(footers)))

        with ctx:
          patch._commit_message = msg

          # Idempotence
          self.assertEqual(patch._AddFooters(msg),
                           patch._AddFooters(patch._AddFooters(msg)))

          # there may be pre-existing footers.  This asserts that we
          # can Get all of the footers after we Set them.
          self.assertFalse(bool(
              set(footers) -
              set(patch._GetFooters(patch._AddFooters(msg)))))

          if set(footers) - set(patch._GetFooters(msg)):
            self.assertNotEqual(msg, patch._AddFooters(msg))

  def testConvertQueryResults(self):
    """Verify basic ConvertQueryResults behavior."""
    self.maxDiff = None
    j = FAKE_CHANGE_JSON
    exp = {
        'project': j['project'],
        'url': 'https://host/#/c/8366/',
        'status': j['status'],
        'branch': j['branch'],
        'owner': {
            'username': 'Duckworth Duck',
            'name': 'Duckworth Duck',
            'email': 'happy-funky-duck@chromium.org',
        },
        'createdOn': 1473894166,
        'commitMessage': ('my super great message\n\nChange-Id: '
                          'I3a753d6bacfbe76e5675a6f2f7941fe520c095e5\n'),
        'currentPatchSet': {
            'approvals': [],
            'date': 1473894123,
            'draft': False,
            'number': '1',
            'ref': 'refs/changes/15/8366/1',
            'revision': j['current_revision'],
        },
        'dependsOn': [{'revision': '3d54362a9b010330bae2dde973fc5c3efc4e5f44'}],
        'subject': j['subject'],
        'id': j['change_id'],
        'lastUpdated': 1473894166,
        'number': str(j['_number']),
    }
    ret = cros_patch.GerritPatch.ConvertQueryResults(j, 'host')
    self.assertEqual(ret, exp)

  def testConvertQueryResultsProtoNoHttp(self):
    """Verify ConvertQueryResults handling of non-http protos."""
    j = copy.deepcopy(FAKE_CHANGE_JSON)
    fetch = j['revisions'][j['current_revision']]['fetch']
    fetch.pop('http', None)
    fetch.pop('https', None)
    # Mostly just verifying it still parses.
    ret = cros_patch.GerritPatch.ConvertQueryResults(j, 'host')
    self.assertNotEqual(ret, None)


class PrepareRemotePatchesTest(cros_test_lib.TestCase):
  """Test preparing remote patches."""

  def MkRemote(self,
               project='my/project', original_branch='my-local',
               ref='refs/tryjobs/elmer/patches', tracking_branch='master',
               internal=False):

    l = [project, original_branch, ref, tracking_branch,
         getattr(constants, ('%s_PATCH_TAG' %
                             ('INTERNAL' if internal else 'EXTERNAL')))]
    return ':'.join(l)

  def assertRemote(self, patch, project='my/project',
                   original_branch='my-local',
                   ref='refs/tryjobs/elmer/patches', tracking_branch='master',
                   internal=False):
    self.assertEqual(patch.project, project)
    self.assertEqual(patch.original_branch, original_branch)
    self.assertEqual(patch.ref, ref)
    self.assertEqual(patch.tracking_branch, tracking_branch)
    self.assertEqual(patch.internal, internal)

  def test(self):
    # Check handling of a single patch...
    patches = cros_patch.PrepareRemotePatches([self.MkRemote()])
    self.assertEqual(len(patches), 1)
    self.assertRemote(patches[0])

    # Check handling of a multiple...
    patches = cros_patch.PrepareRemotePatches(
        [self.MkRemote(), self.MkRemote(project='foon')])
    self.assertEqual(len(patches), 2)
    self.assertRemote(patches[0])
    self.assertRemote(patches[1], project='foon')

    # Ensure basic validation occurs:
    chunks = self.MkRemote().split(':')
    self.assertRaises(ValueError, cros_patch.PrepareRemotePatches,
                      ':'.join(chunks[:-1]))
    self.assertRaises(ValueError, cros_patch.PrepareRemotePatches,
                      ':'.join(chunks[:-1] + ['monkeys']))
    self.assertRaises(ValueError, cros_patch.PrepareRemotePatches,
                      ':'.join(chunks + [':']))


class PrepareLocalPatchesTests(cros_test_lib.RunCommandTestCase):
  """Test preparing local patches."""

  def setUp(self):
    self.path, self.project, self.branch = 'mydir', 'my/project', 'mybranch'
    self.tracking_branch = 'kernel'
    self.patches = ['%s:%s' % (self.project, self.branch)]
    self.manifest = mock.MagicMock()
    attrs = dict(tracking_branch=self.tracking_branch,
                 local_path=self.path,
                 remote='cros')
    checkout = git.ProjectCheckout(attrs)
    self.PatchObject(
        self.manifest, 'FindCheckouts', return_value=[checkout]
    )

  def PrepareLocalPatches(self, output):
    """Check the returned GitRepoPatchInfo against golden values."""
    output_obj = mock.MagicMock()
    output_obj.output = output
    self.PatchObject(cros_patch.LocalPatch, 'Fetch', return_value=output_obj)
    self.PatchObject(git, 'RunGit', return_value=output_obj)
    patch_info = cros_patch.PrepareLocalPatches(self.manifest, self.patches)[0]
    self.assertEquals(patch_info.project, self.project)
    self.assertEquals(patch_info.ref, self.branch)
    self.assertEquals(patch_info.tracking_branch, self.tracking_branch)

  def testBranchSpecifiedSuccessRun(self):
    """Test success with branch specified by user."""
    self.PrepareLocalPatches('12345'.rjust(40, '0'))

  def testBranchSpecifiedNoChanges(self):
    """Test when no changes on the branch specified by user."""
    self.assertRaises(SystemExit, self.PrepareLocalPatches, '')


class TestFormatting(cros_test_lib.TestCase):
  """Test formatting of output."""

  VALID_CHANGE_ID = 'I47ea30385af60ae4cc2acc5d1a283a46423bc6e1'

  def _assertResult(self, functor, value, expected=None, raises=False,
                    **kwargs):
    if raises:
      self.assertRaises2(ValueError, functor, value,
                         msg='%s(%r) did not throw a ValueError'
                         % (functor.__name__, value), **kwargs)
    else:
      self.assertEqual(functor(value, **kwargs), expected,
                       msg='failed: %s(%r) != %r'
                       % (functor.__name__, value, expected))

  def _assertBad(self, functor, values, **kwargs):
    for value in values:
      self._assertResult(functor, value, raises=True, **kwargs)

  def _assertGood(self, functor, values, **kwargs):
    for value, expected in values:
      self._assertResult(functor, value, expected, **kwargs)

  def testGerritNumber(self):
    """Tests that we can pasre a Gerrit number."""
    self._assertGood(cros_patch.ParseGerritNumber,
                     [('12345',) * 2, ('12',) * 2, ('123',) * 2])

    self._assertBad(
        cros_patch.ParseGerritNumber,
        ['is', 'i1325', '01234567', '012345a', '**12345', '+123', '/0123'],
        error_ok=False)

  def testChangeID(self):
    """Tests that we can parse a change-ID."""
    self._assertGood(cros_patch.ParseChangeID, [(self.VALID_CHANGE_ID,) * 2])

    # Change-IDs too short/long, with unexpected characters in it.
    self._assertBad(
        cros_patch.ParseChangeID,
        ['is', '**i1325', 'i134'.ljust(41, '0'), 'I1234+'.ljust(41, '0'),
         'I123'.ljust(42, '0')],
        error_ok=False)

  def testSHA1(self):
    """Tests that we can parse a SHA1 hash."""
    self._assertGood(cros_patch.ParseSHA1,
                     [('1' * 40,) * 2,
                      ('a' * 40,) * 2,
                      ('1a7e034'.ljust(40, '0'),) * 2])

    self._assertBad(
        cros_patch.ParseSHA1,
        ['0abcg', 'Z', '**a', '+123', '1234ab' * 10],
        error_ok=False)

  def testFullChangeID(self):
    """Tests that we can parse a full change-ID."""
    change_id = self.VALID_CHANGE_ID
    self._assertGood(
        cros_patch.ParseFullChangeID,
        (('foo~bar~%s' % change_id,
          cros_patch.FullChangeId('foo', 'bar', change_id)),
         ('foo/bar/baz~refs/heads/_my-branch_~%s' % change_id,
          cros_patch.FullChangeId('foo/bar/baz', 'refs/heads/_my-branch_',
                                  change_id))))

  def testInvalidFullChangeID(self):
    """Should throw an error on bad inputs."""
    change_id = self.VALID_CHANGE_ID
    self._assertBad(
        cros_patch.ParseFullChangeID,
        ['foo', 'foo~bar', 'foo~bar~baz', 'foo~refs/bar~%s' % change_id],
        error_ok=False)

  def testParsePatchDeps(self):
    """Tests that we can parse the dependency specified by the user."""
    change_id = self.VALID_CHANGE_ID
    vals = ['CL:12345', 'project~branch~%s' % change_id, change_id,
            change_id[1:]]
    for val in vals:
      self.assertTrue(cros_patch.ParsePatchDep(val) is not None)

    self._assertBad(cros_patch.ParsePatchDep,
                    ['145462399', 'I47ea3', 'i47ea3'.ljust(41, '0')])


class MockPatchFactory(object):
  """Helper class to create patches or series of them, for unit tests."""

  def __init__(self, patch_mock=None):
    """Constructor for factory.

    patch_mock: Optional PatchMock instance.
    """
    self.patch_mock = patch_mock
    self._patch_counter = (itertools.count(1)).next

  def MockPatch(self, change_id=None, patch_number=None, is_merged=False,
                project='chromiumos/chromite',
                remote=site_config.params.EXTERNAL_REMOTE,
                tracking_branch='refs/heads/master', is_draft=False,
                approvals=(), commit_message=None):
    """Helper function to create mock GerritPatch objects."""
    if change_id is None:
      change_id = self._patch_counter()
    gerrit_number = str(change_id)
    change_id = hex(change_id)[2:].rstrip('L').lower()
    change_id = 'I%s' % change_id.rjust(40, '0')
    sha1 = hex(_GetNumber())[2:].rstrip('L').lower().rjust(40, '0')
    patch_number = (patch_number if patch_number is not None else _GetNumber())
    fake_url = 'http://foo/bar'
    if not approvals:
      approvals = [{'type': 'VRIF', 'value': '1', 'grantedOn': 1391733002},
                   {'type': 'CRVW', 'value': '2', 'grantedOn': 1391733002},
                   {'type': 'COMR', 'value': '1', 'grantedOn': 1391733002}]

    current_patch_set = {
        'number': patch_number,
        'revision': sha1,
        'draft': is_draft,
        'approvals': approvals,
    }
    patch_dict = {
        'currentPatchSet': current_patch_set,
        'id': change_id,
        'number': gerrit_number,
        'project': project,
        'branch': tracking_branch,
        'owner': {'email': 'elmer.fudd@chromium.org'},
        'remote': remote,
        'status': 'MERGED' if is_merged else 'NEW',
        'url': '%s/%s' % (fake_url, change_id),
    }

    patch = cros_patch.GerritPatch(patch_dict, remote, fake_url)
    patch.pass_count = 0
    patch.fail_count = 1
    patch.total_fail_count = 3
    patch.commit_message = commit_message

    return patch

  def GetPatches(self, how_many=1, always_use_list=False, **kwargs):
    """Get a sequential list of patches.

    Args:
      how_many: How many patches to return.
      always_use_list: Whether to use a list for a single item list.
      **kwargs: Keyword arguments for self.MockPatch.
    """
    patches = [self.MockPatch(**kwargs) for _ in xrange(how_many)]
    if self.patch_mock:
      for i, patch in enumerate(patches):
        self.patch_mock.SetGerritDependencies(patch, patches[:i + 1])
    if how_many == 1 and not always_use_list:
      return patches[0]
    return patches


class DependencyErrorTests(cros_test_lib.MockTestCase):
  """Tests for DependencyError."""

  def testGetRootError(self):
    """Test GetRootError on nested DependencyError."""
    p_1, p_2, p_3 = MockPatchFactory().GetPatches(how_many=3)
    ex_1 = cros_patch.ApplyPatchException(p_1)
    ex_2 = cros_patch.DependencyError(p_2, ex_1)
    ex_3 = cros_patch.DependencyError(p_3, ex_2)

    self.assertEqual(ex_3.GetRootError(), ex_1)

  def testGetRootErrorOnCircurlarError(self):
    """Test GetRootError on circular"""
    p_1, p_2, p_3 = MockPatchFactory().GetPatches(how_many=3)
    ex_1 = cros_patch.DependencyError(p_2, cros_patch.ApplyPatchException(p_1))
    ex_2 = cros_patch.DependencyError(p_2, ex_1)
    ex_3 = cros_patch.DependencyError(p_3, ex_2)
    ex_1.error = ex_3

    self.assertIsNone(ex_3.GetRootError())
