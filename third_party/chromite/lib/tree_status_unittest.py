# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test suite for tree_status.py"""

from __future__ import print_function

import mock
import time
import urllib

from chromite.lib import config_lib_unittest
from chromite.lib import constants
from chromite.lib import cros_test_lib
from chromite.lib import timeout_util
from chromite.lib import tree_status


# pylint: disable=protected-access


class TestTreeStatus(cros_test_lib.MockTestCase):
  """Tests TreeStatus method in cros_build_lib."""

  status_url = 'https://chromiumos-status.appspot.com/current?format=json'

  def setUp(self):
    mock_site_config = config_lib_unittest.MockSiteConfig()
    # Add a couple other builders so we have more than one.
    mock_site_config.Add('x86-generic-paladin')
    mock_site_config.Add('arm-generic-paladin')
    self.site_config = mock_site_config

  def _TreeStatusFile(self, message, general_state):
    """Returns a file-like object with the status message writtin in it."""
    json = '{"message": "%s", "general_state": "%s"}' % (message, general_state)
    return mock.MagicMock(json=json, getcode=lambda: 200, read=lambda: json)

  def _SetupMockTreeStatusResponses(self,
                                    final_tree_status='Tree is open.',
                                    final_general_state=constants.TREE_OPEN,
                                    rejected_tree_status='Tree is closed.',
                                    rejected_general_state=
                                    constants.TREE_CLOSED,
                                    rejected_status_count=0,
                                    retries_500=0,
                                    output_final_status=True):
    """Mocks out urllib.urlopen commands to simulate a given tree status.

    Args:
      final_tree_status: The final value of tree status that will be returned
        by urlopen.
      final_general_state: The final value of 'general_state' that will be
        returned by urlopen.
      rejected_tree_status: An intermediate value of tree status that will be
        returned by urlopen and retried upon.
      rejected_general_state: An intermediate value of 'general_state' that
        will be returned by urlopen and retried upon.
      rejected_status_count: The number of times urlopen will return the
        rejected state.
      retries_500: The number of times urlopen will fail with a 500 code.
      output_final_status: If True, the status given by final_tree_status and
        final_general_state will be the last status returned by urlopen. If
        False, final_tree_status will never be returned, and instead an
        unlimited number of times rejected_response will be returned.
    """
    final_response = self._TreeStatusFile(final_tree_status,
                                          final_general_state)
    rejected_response = self._TreeStatusFile(rejected_tree_status,
                                             rejected_general_state)

    error_500_response = mock.MagicMock(getcode=lambda: 500)
    return_value = [error_500_response] * retries_500

    if output_final_status:
      return_value += [rejected_response] * rejected_status_count
      return_value += [final_response]
    else:
      return_value += [rejected_response] * 10

    self.PatchObject(urllib, 'urlopen', autospec=True,
                     side_effect=return_value)

  def testTreeIsOpen(self):
    """Tests that we return True is the tree is open."""
    self._SetupMockTreeStatusResponses(rejected_status_count=5,
                                       retries_500=5)
    self.assertTrue(tree_status.IsTreeOpen(status_url=self.status_url,
                                           period=0))

  def testTreeIsClosed(self):
    """Tests that we return false is the tree is closed."""
    self._SetupMockTreeStatusResponses(output_final_status=False)
    self.assertFalse(tree_status.IsTreeOpen(status_url=self.status_url,
                                            period=0.1))

  def testTreeIsThrottled(self):
    """Tests that we return True if the tree is throttled."""
    self._SetupMockTreeStatusResponses(
        final_tree_status='Tree is throttled (flaky bug on flaky builder)',
        final_general_state=constants.TREE_THROTTLED)
    self.assertTrue(tree_status.IsTreeOpen(status_url=self.status_url,
                                           throttled_ok=True))

  def testTreeIsThrottledNotOk(self):
    """Tests that we respect throttled_ok"""
    self._SetupMockTreeStatusResponses(
        rejected_tree_status='Tree is throttled (flaky bug on flaky builder)',
        rejected_general_state=constants.TREE_THROTTLED,
        output_final_status=False)
    self.assertFalse(tree_status.IsTreeOpen(status_url=self.status_url,
                                            period=0.1))

  def testWaitForStatusOpen(self):
    """Tests that we can wait for a tree open response."""
    self._SetupMockTreeStatusResponses()
    self.assertEqual(tree_status.WaitForTreeStatus(status_url=self.status_url),
                     constants.TREE_OPEN)


  def testWaitForStatusThrottled(self):
    """Tests that we can wait for a tree open response."""
    self._SetupMockTreeStatusResponses(
        final_general_state=constants.TREE_THROTTLED)
    self.assertEqual(tree_status.WaitForTreeStatus(status_url=self.status_url,
                                                   throttled_ok=True),
                     constants.TREE_THROTTLED)

  def testWaitForStatusFailure(self):
    """Tests that we can wait for a tree open response."""
    self._SetupMockTreeStatusResponses(output_final_status=False)
    self.assertRaises(timeout_util.TimeoutError,
                      tree_status.WaitForTreeStatus,
                      status_url=self.status_url,
                      period=0.1)

  def testGetStatusDictParsesMessage(self):
    """Tests that _GetStatusDict parses message correctly."""
    self._SetupMockTreeStatusResponses(
        final_tree_status='Tree is throttled (foo canary: taco investigating)',
        final_general_state=constants.TREE_OPEN)
    data = tree_status._GetStatusDict(self.status_url)
    self.assertEqual(data[tree_status.TREE_STATUS_MESSAGE],
                     'foo canary: taco investigating')

  def testGetStatusDictEmptyMessage(self):
    """Tests that _GetStatusDict stores an empty string for unknown format."""
    self._SetupMockTreeStatusResponses(
        final_tree_status='Tree is throttled. foo canary -> crbug.com/bar',
        final_general_state=constants.TREE_OPEN)
    data = tree_status._GetStatusDict(self.status_url)
    self.assertEqual(data[tree_status.TREE_STATUS_MESSAGE], '')

  def testGetStatusDictRawMessage(self):
    """Tests that _GetStatusDict stores raw message if requested."""
    self._SetupMockTreeStatusResponses(final_tree_status='Tree is open (taco).',
                                       final_general_state=constants.TREE_OPEN)
    data = tree_status._GetStatusDict(self.status_url, raw_message=True)
    self.assertEqual(data[tree_status.TREE_STATUS_MESSAGE],
                     'Tree is open (taco).')

  def testGetExperimentalBuilders(self):
    """Tests that GetExperimentalBuilders parses out EXPERIMENTAL-BUILDERS=."""
    self._SetupMockTreeStatusResponses(
        final_tree_status=(
            'Tree is open (EXPERIMENTAL=amd64-generic-paladin).'),
        final_general_state=constants.TREE_OPEN)
    builders = tree_status.GetExperimentalBuilders(self.status_url)
    self.assertItemsEqual(builders, ['amd64-generic-paladin'])

    self._SetupMockTreeStatusResponses(
        final_tree_status=('Tree is open '
                           '(EXPERIMENTAL=amd64-generic-paladin '
                           'EXPERIMENTAL=arm-generic-paladin).'),
        final_general_state=constants.TREE_OPEN)
    builders = tree_status.GetExperimentalBuilders(self.status_url)
    self.assertItemsEqual(
        builders,
        ['amd64-generic-paladin', 'arm-generic-paladin'])

    # Builders not in the site config are filtered out.
    self._SetupMockTreeStatusResponses(
        final_tree_status=(
            'Tree is open (EXPERIMENTAL=foo-generic-paladin).'),
        final_general_state=constants.TREE_OPEN)
    builders = tree_status.GetExperimentalBuilders(self.status_url)
    self.assertEqual(builders, [])

    # Case insensitive.
    self._SetupMockTreeStatusResponses(
        final_tree_status=(
            'Tree is open (experimental=amd64-generic-paladin).'),
        final_general_state=constants.TREE_OPEN)
    builders = tree_status.GetExperimentalBuilders(self.status_url)
    self.assertItemsEqual(builders, ['amd64-generic-paladin'])

  def testGetExperimentalBuildersTimeout(self):
    """Tests timeout behavior of GetExperimentalBuilders."""
    with mock.patch.object(tree_status, '_GetStatusDict') as m:
      m.side_effect = lambda _: time.sleep(10)
      with self.assertRaises(timeout_util.TimeoutError):
        tree_status.GetExperimentalBuilders(self.status_url, timeout=1)

  def testUpdateTreeStatusWithEpilogue(self):
    """Tests that epilogue is appended to the message."""
    with mock.patch.object(tree_status, '_UpdateTreeStatus') as m:
      tree_status.UpdateTreeStatus(
          constants.TREE_CLOSED, 'failure', announcer='foo',
          epilogue='bar')
      m.assert_called_once_with(mock.ANY, 'Tree is closed (foo: failure | bar)')

  def testUpdateTreeStatusWithoutEpilogue(self):
    """Tests that the tree status message is created as expected."""
    with mock.patch.object(tree_status, '_UpdateTreeStatus') as m:
      tree_status.UpdateTreeStatus(
          constants.TREE_CLOSED, 'failure', announcer='foo')
      m.assert_called_once_with(mock.ANY, 'Tree is closed (foo: failure)')

  def testUpdateTreeStatusUnknownStatus(self):
    """Tests that the exception is raised on unknown tree status."""
    with mock.patch.object(tree_status, '_UpdateTreeStatus'):
      self.assertRaises(tree_status.InvalidTreeStatus,
                        tree_status.UpdateTreeStatus, 'foostatus', 'failure')

  def testThrottlesTreeOnWithBuildNumberAndType(self):
    """Tests that tree is throttled with the build number in the message."""
    self._SetupMockTreeStatusResponses(final_tree_status='Tree is open (taco)',
                                       final_general_state=constants.TREE_OPEN)
    with mock.patch.object(tree_status, '_UpdateTreeStatus') as m:
      tree_status.ThrottleOrCloseTheTree('foo', 'failure', buildnumber=1234,
                                         internal=True)
      m.assert_called_once_with(mock.ANY,
                                'Tree is throttled (foo-i-1234: failure)')

  def testThrottlesTreeOnWithBuildNumberAndPublicType(self):
    """Tests that tree is throttled with the build number in the message."""
    self._SetupMockTreeStatusResponses(final_tree_status='Tree is open (taco)',
                                       final_general_state=constants.TREE_OPEN)
    with mock.patch.object(tree_status, '_UpdateTreeStatus') as m:
      tree_status.ThrottleOrCloseTheTree('foo', 'failure', buildnumber=1234,
                                         internal=False)
      m.assert_called_once_with(mock.ANY,
                                'Tree is throttled (foo-p-1234: failure)')

  def testThrottlesTreeOnOpen(self):
    """Tests that ThrottleOrCloseTheTree throttles the tree if tree is open."""
    self._SetupMockTreeStatusResponses(final_tree_status='Tree is open (taco)',
                                       final_general_state=constants.TREE_OPEN)
    with mock.patch.object(tree_status, '_UpdateTreeStatus') as m:
      tree_status.ThrottleOrCloseTheTree('foo', 'failure')
      m.assert_called_once_with(mock.ANY, 'Tree is throttled (foo: failure)')

  def testThrottlesTreeOnThrottled(self):
    """Tests ThrottleOrCloseTheTree throttles the tree if tree is throttled."""
    self._SetupMockTreeStatusResponses(
        final_tree_status='Tree is throttled (taco)',
        final_general_state=constants.TREE_THROTTLED)
    with mock.patch.object(tree_status, '_UpdateTreeStatus') as m:
      tree_status.ThrottleOrCloseTheTree('foo', 'failure')
      # Also make sure that previous status message is included.
      m.assert_called_once_with(mock.ANY,
                                'Tree is throttled (foo: failure | taco)')

  def testClosesTheTreeOnClosed(self):
    """Tests ThrottleOrCloseTheTree closes the tree if tree is closed."""
    self._SetupMockTreeStatusResponses(
        final_tree_status='Tree is closed (taco)',
        final_general_state=constants.TREE_CLOSED)
    with mock.patch.object(tree_status, '_UpdateTreeStatus') as m:
      tree_status.ThrottleOrCloseTheTree('foo', 'failure')
      m.assert_called_once_with(mock.ANY,
                                'Tree is closed (foo: failure | taco)')

  def testClosesTheTreeOnMaintenance(self):
    """Tests ThrottleOrCloseTheTree closes the tree if tree is closed."""
    self._SetupMockTreeStatusResponses(
        final_tree_status='Tree is under maintenance (taco)',
        final_general_state=constants.TREE_MAINTENANCE)
    with mock.patch.object(tree_status, '_UpdateTreeStatus') as m:
      tree_status.ThrottleOrCloseTheTree('foo', 'failure')
      m.assert_called_once_with(
          mock.ANY,
          'Tree is under maintenance (foo: failure | taco)')

  def testDiscardUpdateFromTheSameAnnouncer(self):
    """Tests we don't include messages from the same announcer."""
    self._SetupMockTreeStatusResponses(
        final_tree_status='Tree is throttled (foo: failure | bar: taco)',
        final_general_state=constants.TREE_THROTTLED)
    with mock.patch.object(tree_status, '_UpdateTreeStatus') as m:
      tree_status.ThrottleOrCloseTheTree('foo', 'failure')
      # Also make sure that previous status message is included.
      m.assert_called_once_with(mock.ANY,
                                'Tree is throttled (foo: failure | bar: taco)')


class TestGettingSheriffEmails(cros_test_lib.MockTestCase):
  """Tests functions related to retrieving the sheriff's email address."""

  def testParsingSheriffEmails(self):
    """Tests parsing the raw data to get sheriff emails."""
    # Test parsing when there is only one sheriff.
    raw_line = "document.write('taco')"
    self.PatchObject(tree_status, '_OpenSheriffURL', return_value=raw_line)
    self.assertEqual(tree_status.GetSheriffEmailAddresses('chrome'),
                     ['taco@google.com'])

    # Test parsing when there are multiple sheriffs.
    raw_line = "document.write('taco, burrito')"
    self.PatchObject(tree_status, '_OpenSheriffURL', return_value=raw_line)
    self.assertEqual(tree_status.GetSheriffEmailAddresses('chrome'),
                     ['taco@google.com', 'burrito@google.com'])

    # Test parsing when sheriff is None.
    raw_line = "document.write('None (channel is sheriff)')"
    self.PatchObject(tree_status, '_OpenSheriffURL', return_value=raw_line)
    self.assertEqual(tree_status.GetSheriffEmailAddresses('chrome'), [])


class TestUrlConstruction(cros_test_lib.TestCase):
  """Tests functions that create URLs."""

  def testConstructLegolandBuildURL(self):
    """Tests generating Legoland build URLs."""
    output = tree_status.ConstructLegolandBuildURL('bbid')
    expected = ('http://cros-goldeneye/chromeos/healthmonitoring/'
                'buildDetails?buildbucketId=bbid')

    self.assertEqual(output, expected)
