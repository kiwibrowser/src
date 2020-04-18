# -*- coding: utf-8 -*-
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the archive_lib module."""

from __future__ import print_function

import multiprocessing

from chromite.lib import metadata_lib
from chromite.lib import cros_test_lib
from chromite.lib import parallel


class MetadataTest(cros_test_lib.TestCase):
  """Tests the correctness of various metadata methods."""

  def testGetDict(self):
    starting_dict = {
        'key1': 1,
        'key2': '2',
        'cl_actions': [('a', 1), ('b', 2)],
        'board-metadata': {
            'board-1': {'info': 432},
        },
    }
    metadata = metadata_lib.CBuildbotMetadata(starting_dict)
    ending_dict = metadata.GetDict()
    self.assertEqual(starting_dict, ending_dict)

  def testGetValue(self):
    """Test GetValue."""
    starting_dict = {
        'key1': 1,
        'key2': '2'
    }
    metadata = metadata_lib.CBuildbotMetadata(starting_dict)
    self.assertEqual(metadata.GetValue('key1'), 1)
    self.assertEqual(metadata.GetValue('key2'), '2')
    self.assertRaises(KeyError, metadata.GetValue, 'key3')

  def testGetValueWithDefault(self):
    """Test GetValueWithDefault."""
    starting_dict = {
        'key1': 1,
        'key2': '2'
    }
    metadata = metadata_lib.CBuildbotMetadata(starting_dict)
    self.assertEqual(metadata.GetValueWithDefault('key1'), 1)
    self.assertEqual(metadata.GetValueWithDefault('key2', 2), '2')
    self.assertEqual(metadata.GetValueWithDefault('key3', 3), 3)

  def testUpdateKeyDictWithDict(self):
    expected_dict = {str(x): x for x in range(20)}
    m = multiprocessing.Manager()
    metadata = metadata_lib.CBuildbotMetadata(multiprocess_manager=m)

    metadata.UpdateKeyDictWithDict('my_dict', expected_dict)

    self.assertEqual(expected_dict, metadata.GetDict()['my_dict'])

  def testExtendKeyListWithList(self):
    """Test ExtendKeyListWithList."""
    m = multiprocessing.Manager()
    metadata = metadata_lib.CBuildbotMetadata(multiprocess_manager=m)

    expected_list = [str(x) for x in range(20)]
    metadata.ExtendKeyListWithList('my_list', expected_list)
    self.assertEqual(expected_list, metadata.GetDict()['my_list'])

    sub_list_1 = [str(x) for x in range(0, 10)]
    sub_list_2 = [str(x) for x in range(10, 20)]
    metadata.ExtendKeyListWithList('my_list_2', sub_list_1)
    metadata.ExtendKeyListWithList('my_list_2', sub_list_2)
    self.assertEqual(expected_list, metadata.GetDict()['my_list_2'])

  def testUpdateKeyDictWithDictMultiprocess(self):
    expected_dict = {str(x): x for x in range(20)}
    m = multiprocessing.Manager()
    metadata = metadata_lib.CBuildbotMetadata(multiprocess_manager=m)

    with parallel.BackgroundTaskRunner(metadata.UpdateKeyDictWithDict) as q:
      for k, v in expected_dict.iteritems():
        q.put(['my_dict', {k: v}])

    self.assertEqual(expected_dict, metadata.GetDict()['my_dict'])


  def testUpdateBoardMetadataWithEmptyDict(self):
    metadata = metadata_lib.CBuildbotMetadata()
    metadata.UpdateBoardDictWithDict('someboard', {})
    self.assertEqual(metadata.GetDict()['board-metadata']['someboard'], {})


  def testUpdateBoardMetadataWithMultiprocessDict(self):
    starting_dict = {
        'key1': 1,
        'key2': '2',
        'cl_actions': [('a', 1), ('b', 2)],
        'board-metadata': {
            'board-1': {'info': 432},
        },
    }

    m = multiprocessing.Manager()
    metadata = metadata_lib.CBuildbotMetadata(metadata_dict=starting_dict,
                                              multiprocess_manager=m)

    # pylint: disable=no-member
    update_dict = m.dict()
    update_dict['my_key'] = 'some value'
    metadata.UpdateBoardDictWithDict('board-1', update_dict)

    self.assertEqual(metadata.GetDict()['board-metadata']['board-1']['my_key'],
                     'some value')

  def testMultiprocessSafety(self):
    m = multiprocessing.Manager()
    metadata = metadata_lib.CBuildbotMetadata(multiprocess_manager=m)
    key_dict = {'key1': 1, 'key2': 2}
    starting_dict = {
        'key1': 1,
        'key2': '2',
        'key3': key_dict,
        'cl_actions': [('a', 1), ('b', 2)],
        'board-metadata': {
            'board-1': {'info': 432},
        },
    }

    # Test that UpdateWithDict is process-safe
    parallel.RunParallelSteps([lambda: metadata.UpdateWithDict(starting_dict)])
    ending_dict = metadata.GetDict()
    self.assertEqual(starting_dict, ending_dict)

    # Test that UpdateKeyDictWithDict is process-safe
    parallel.RunParallelSteps([lambda: metadata.UpdateKeyDictWithDict(
        'key3', key_dict)])
    ending_dict = metadata.GetDict()
    self.assertEqual(starting_dict, ending_dict)

    # Test that RecordCLAction is process-safe
    fake_change = metadata_lib.GerritPatchTuple(12345, 1, False)
    fake_action = ('asdf,')
    parallel.RunParallelSteps([lambda: metadata.RecordCLAction(fake_change,
                                                               fake_action)])
    ending_dict = metadata.GetDict()
    # Assert that an action was recorded.
    self.assertEqual(len(starting_dict['cl_actions']) + 1,
                     len(ending_dict['cl_actions']))

  def testPerBoardDict(self):
    starting_per_board_dict = {
        'board-1': {'kubrick': 2001,
                    'bergman': 'persona',
                    'hitchcock': 'vertigo'},
        'board-2': {'kubrick': ['barry lyndon', 'dr. strangelove'],
                    'bergman': 'the seventh seal'}
    }

    starting_dict = {'board-metadata': starting_per_board_dict}

    m = multiprocessing.Manager()
    metadata = metadata_lib.CBuildbotMetadata(metadata_dict=starting_dict,
                                              multiprocess_manager=m)

    extra_per_board_dict = {
        'board-1': {'kurosawa': 'rashomon',
                    'coen brothers': 'fargo'},
        'board-3': {'hitchcock': 'north by northwest',
                    'coen brothers': 'the big lebowski'}
    }

    expected_dict = starting_per_board_dict

    # Write each per board key-value pair to metadata in a separate process.
    with parallel.BackgroundTaskRunner(metadata.UpdateBoardDictWithDict) as q:
      for board, board_dict in extra_per_board_dict.iteritems():
        expected_dict.setdefault(board, {}).update(board_dict)
        for k, v in board_dict.iteritems():
          q.put([board, {k: v}])

    self.assertEqual(expected_dict, metadata.GetDict()['board-metadata'])
