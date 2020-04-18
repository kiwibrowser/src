# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Classes to manage HWTest result."""

from __future__ import print_function

import collections
import os

from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import git
from chromite.lib import patch as cros_patch


HWTEST_RESULT_COLUMNS = ['id', 'build_id', 'test_name', 'status']

_hwTestResult = collections.namedtuple('_hwTestResult',
                                       HWTEST_RESULT_COLUMNS)


class HWTestResult(_hwTestResult):
  """Class to present HWTest status."""

  @classmethod
  def FromReport(cls, build_id, test_name, status):
    """Get a HWTestResult instance from a result report."""
    return HWTestResult(None, build_id, test_name, status)

  @classmethod
  def FromEntry(cls, entry):
    """Get a HWTestResult instance from cidb entry."""
    return HWTestResult(entry.id, entry.build_id, entry.test_name, entry.status)

  @classmethod
  def NormalizeTestName(cls, test_name):
    """Normalize test name.

    Normalization examples:
    Suite job -> None
    cheets_CTS.com.android.cts.dram -> cheets_CTS
    security_NetworkListeners -> security_NetworkListeners

    Args:
      test_name: The test name string to normalize.

    Returns:
      Test name after normalization.
    """
    if test_name == 'Suite job':
      return None

    names = test_name.split('.')
    return names[0]


class HWTestResultManager(object):
  """Class to manage HWTest results."""

  @classmethod
  def GetHWTestResultsFromCIDB(cls, db, build_ids, test_statues=None):
    """Get HWTest results for given builds from CIDB.

    Args:
      db: An instance of cidb.CIDBConnection.
      build_ids: A list of build_ids (strings) to get HWTest results.
      test_statues: A set of HWTest statuses (stirngs) to get. If not None,
        only return HWTests in test_statues.

    Returns:
      A list of HWTestResult instances.
    """
    results = db.GetHWTestResultsForBuilds(build_ids)

    if test_statues is None:
      return results

    return [x for x in results if x.status in test_statues]

  @classmethod
  def GetFailedHWTestsFromCIDB(cls, db, build_ids):
    """Get test names of failed HWTests from CIDB.

    Args:
      db: An instance of cidb.CIDBConnection
      build_ids: A list of build_ids (strings) to get failed HWTests.

    Returns:
      A list of normalized HWTest names (strings).
    """
    # TODO: probably only count 'fail' and exclude abort' and 'other' results?
    hwtest_results = cls.GetHWTestResultsFromCIDB(
        db, build_ids, test_statues=constants.HWTEST_STATUES_NOT_PASSED)

    failed_tests = set([HWTestResult.NormalizeTestName(result.test_name)
                        for result in hwtest_results])
    failed_tests.discard(None)

    logging.info('Found failed tests: %s ', failed_tests)
    return failed_tests

  @classmethod
  def GetFailedHwtestsAffectedByChange(cls, change, manifest, failed_hwtests):
    """Get failed HWtests which could be affected by the given change.

    Args:
      change: An instance of cros_patch.GerritPatch.
      manifest: An instance of ManifestCheckout.
      failed_hwtests: A list of normalized name of failed hwtests got from CIDB
        (see the return type of HWTestResultManager.GetFailedHWTestsFromCIDB).

    Returns:
      A list of failed hwtest names (strings) which are likely to be affected by
      the change.
    """
    assert failed_hwtests
    checkout = change.GetCheckout(manifest)

    assigned_failed_hwtests = set()
    if checkout:
      git_repo = checkout.GetPath(absolute=True)
      for path in change.GetDiffStatus(git_repo):
        root, _ = os.path.splitext(path)
        touched_dirs = root.split(os.sep)
        for touched_dir in touched_dirs:
          if touched_dir in failed_hwtests:
            logging.info('Change %s changed path %s which may be relevant to '
                         'the failed HWTest %s', change, path, touched_dir)
            assigned_failed_hwtests.add(touched_dir)

    return assigned_failed_hwtests

  @classmethod
  def FindHWTestFailureSuspects(cls, changes, build_root, failed_hwtests):
    """Find suspects for HWTest failures.

    Args:
      changes: A list of cros_patch.GerritPatch instances.
      build_root: The path to the build root.
      failed_hwtests: A list of names of failed hwtests got from CIDB (see the
        return type of HWTestResultManager.GetFailedHWTestsFromCIDB), or None.

    Returns:
      A pair of suspects and no_assignee_hwtests. suspects is a set of
      cros_patch.GerritPatch instances. no_assignee_hwtests is True when there
      are failed hwtests without assigned suspects; else False.
    """
    suspects = set()
    no_assignee_hwtests = False

    if not failed_hwtests:
      logging.info('No failed HWTests, skip finding HWTest failure suspects')
      return suspects, no_assignee_hwtests

    manifest = git.ManifestCheckout.Cached(build_root)
    hwtests_with_assignee = set()
    for change in changes:
      assigned = cls.GetFailedHwtestsAffectedByChange(
          change, manifest, failed_hwtests)
      if assigned:
        hwtests_with_assignee.update(assigned)
        suspects.add(change)

    if suspects:
      logging.info('Found suspects for HWTest failures: %s',
                   cros_patch.GetChangesAsString(suspects))

    hwtests_without_assignee = failed_hwtests - hwtests_with_assignee
    if hwtests_without_assignee:
      logging.info('Didn\'t find changes to blame for failed HWtests: %s',
                   hwtests_without_assignee)
      no_assignee_hwtests = True

    return suspects, no_assignee_hwtests
