# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import json
import mock
import os
import shutil
import tempfile
import unittest

from py_utils import cloud_storage  # pylint: disable=import-error

from telemetry.page import page
from telemetry.wpr import archive_info


class MockPage(page.Page):
  def __init__(self, url, name=None, platform_specific=False):
    super(MockPage, self).__init__(url, None, name=name)
    self._platform_specific = platform_specific

page1 = MockPage('http://www.foo.com/', 'Foo')
page2 = MockPage('http://www.bar.com/', 'Bar', True)
page3 = MockPage('http://www.baz.com/', 'Baz', platform_specific=True)
pageNew1 = MockPage('http://www.new.com/', 'New')
pageNew2 = MockPage('http://www.newer.com/', 'Newer', True)
recording1 = 'data_001.wprgo'
recording2 = 'data_002.wprgo'
recording3 = 'data_003.wprgo'
recording4 = 'data_004.wprgo'
recording5 = 'data_005.wprgo'
_DEFAULT_PLATFORM = archive_info._DEFAULT_PLATFORM

default_archives_info_contents_dict = {
    "platform_specific": True,
    "archives": {
        "Foo": {
            _DEFAULT_PLATFORM: recording1
        },
        "Bar": {
            _DEFAULT_PLATFORM: recording2
        },
        "Baz": {
            _DEFAULT_PLATFORM: recording1,
            "win": recording2,
            "mac": recording3,
            "linux": recording4,
            "android": recording5
        }
    }
}

default_archive_info_contents = json.dumps(default_archives_info_contents_dict)
default_wpr_files = [
    'data_001.wprgo', 'data_002.wprgo', 'data_003.wprgo', 'data_004.wprgo',
    'data_005.wprgo']
_BASE_ARCHIVE = {
    u'platform_specific': True,
    u'description': (u'Describes the Web Page Replay archives for a'
                     u' story set. Don\'t edit by hand! Use record_wpr for'
                     u' updating.'),
    u'archives': {},
}


class WprArchiveInfoTest(unittest.TestCase):
  def setUp(self):
    self.tmp_dir = tempfile.mkdtemp()
    # Set file for the metadata.
    self.story_set_archive_info_file = os.path.join(
        self.tmp_dir, 'info.json')

  def tearDown(self):
    shutil.rmtree(self.tmp_dir)

  def createArchiveInfo(
      self, archive_data=default_archive_info_contents,
      cloud_storage_bucket=cloud_storage.PUBLIC_BUCKET, wpr_files=None):

    # Cannot set lists as a default parameter, so doing it this way.
    if wpr_files is None:
      wpr_files = default_wpr_files

    with open(self.story_set_archive_info_file, 'w') as f:
      f.write(archive_data)

    assert isinstance(wpr_files, list)
    for wpr_file in wpr_files:
      assert isinstance(wpr_file, basestring)
      with open(os.path.join(self.tmp_dir, wpr_file), 'w') as f:
        f.write(archive_data)
    return archive_info.WprArchiveInfo.FromFile(
        self.story_set_archive_info_file, cloud_storage_bucket)

  def testInitNotPlatformSpecific(self):
    with open(self.story_set_archive_info_file, 'w') as f:
      f.write('{}')
    with self.assertRaises(AssertionError):
      self.createArchiveInfo(archive_data='{}')


  @mock.patch('telemetry.wpr.archive_info.cloud_storage.GetIfChanged')
  def testDownloadArchivesIfNeededAllOrOneNeeded(self, get_mock):
    test_archive_info = self.createArchiveInfo()

    test_archive_info.DownloadArchivesIfNeeded()
    expected_recordings = [
        recording1, recording1, recording2, recording2, recording3, recording4,
        recording5
    ]
    expected_calls = [
        mock.call(os.path.join(self.tmp_dir, r), cloud_storage.PUBLIC_BUCKET)
        for r in expected_recordings
    ]
    get_mock.assert_has_calls(expected_calls, any_order=True)

  @mock.patch('telemetry.wpr.archive_info.cloud_storage.GetIfChanged')
  def testDownloadArchivesIfNeededNonDefault(self, get_mock):
    data = {
        'platform_specific': True,
        'archives': {
            'http://www.baz.com/': {
                _DEFAULT_PLATFORM: 'data_001.wprgo',
                'win': 'data_002.wprgo',
                'linux': 'data_004.wprgo',
                'mac': 'data_003.wprgo',
                'android': 'data_005.wprgo'},
            'Foo': {_DEFAULT_PLATFORM: 'data_003.wprgo'},
            'Bar': {_DEFAULT_PLATFORM: 'data_002.wprgo'}
        }
    }
    test_archive_info = self.createArchiveInfo(
        archive_data=json.dumps(data, separators=(',', ': ')))

    test_archive_info.DownloadArchivesIfNeeded(target_platforms=['linux'])
    expected_calls = [
        mock.call(os.path.join(self.tmp_dir, r), cloud_storage.PUBLIC_BUCKET)
        for r in (recording1, recording2, recording3, recording4)
    ]
    get_mock.assert_has_calls(expected_calls, any_order=True)

  @mock.patch('telemetry.wpr.archive_info.cloud_storage.GetIfChanged')
  def testDownloadArchivesIfNeededNoBucket(self, get_mock):
    test_archive_info = self.createArchiveInfo(cloud_storage_bucket=None)

    test_archive_info.DownloadArchivesIfNeeded()
    self.assertEqual(get_mock.call_count, 0)

  def testWprFilePathForStoryDefault(self):
    test_archive_info = self.createArchiveInfo()
    self.assertEqual(
        test_archive_info.WprFilePathForStory(page1),
        os.path.join(self.tmp_dir, recording1))
    self.assertEqual(
        test_archive_info.WprFilePathForStory(page2),
        os.path.join(self.tmp_dir, recording2))
    self.assertEqual(
        test_archive_info.WprFilePathForStory(page3),
        os.path.join(self.tmp_dir, recording1))

  def testWprFilePathForStoryMac(self):
    test_archive_info = self.createArchiveInfo()
    self.assertEqual(test_archive_info.WprFilePathForStory(page1, 'mac'),
                     os.path.join(self.tmp_dir, recording1))
    self.assertEqual(test_archive_info.WprFilePathForStory(page2, 'mac'),
                     os.path.join(self.tmp_dir, recording2))
    self.assertEqual(test_archive_info.WprFilePathForStory(page3, 'mac'),
                     os.path.join(self.tmp_dir, recording3))

  def testWprFilePathForStoryWin(self):
    test_archive_info = self.createArchiveInfo()
    self.assertEqual(test_archive_info.WprFilePathForStory(page1, 'win'),
                     os.path.join(self.tmp_dir, recording1))
    self.assertEqual(test_archive_info.WprFilePathForStory(page2, 'win'),
                     os.path.join(self.tmp_dir, recording2))
    self.assertEqual(test_archive_info.WprFilePathForStory(page3, 'win'),
                     os.path.join(self.tmp_dir, recording2))

  def testWprFilePathForStoryAndroid(self):
    test_archive_info = self.createArchiveInfo()
    self.assertEqual(test_archive_info.WprFilePathForStory(page1, 'android'),
                     os.path.join(self.tmp_dir, recording1))
    self.assertEqual(test_archive_info.WprFilePathForStory(page2, 'android'),
                     os.path.join(self.tmp_dir, recording2))
    self.assertEqual(test_archive_info.WprFilePathForStory(page3, 'android'),
                     os.path.join(self.tmp_dir, recording5))

  def testWprFilePathForStoryLinux(self):
    test_archive_info = self.createArchiveInfo()
    self.assertEqual(test_archive_info.WprFilePathForStory(page1, 'linux'),
                     os.path.join(self.tmp_dir, recording1))
    self.assertEqual(test_archive_info.WprFilePathForStory(page2, 'linux'),
                     os.path.join(self.tmp_dir, recording2))
    self.assertEqual(test_archive_info.WprFilePathForStory(page3, 'linux'),
                     os.path.join(self.tmp_dir, recording4))

  def testWprFilePathForStoryBadStory(self):
    test_archive_info = self.createArchiveInfo()
    self.assertIsNone(test_archive_info.WprFilePathForStory(pageNew1))


  def testAddRecordedStoriesNoStories(self):
    test_archive_info = self.createArchiveInfo()
    old_data = test_archive_info._data.copy()
    test_archive_info.AddNewTemporaryRecording()
    test_archive_info.AddRecordedStories(None)
    self.assertDictEqual(old_data, test_archive_info._data)

  def assertWprFileDoesNotExist(self, file_name):
    sha_file = file_name + '.sha1'
    self.assertFalse(os.path.isfile(os.path.join(self.tmp_dir, sha_file)))
    self.assertFalse(os.path.isfile(os.path.join(self.tmp_dir, file_name)))

  def assertWprFileDoesExist(self, file_name):
    sha_file = file_name + '.sha1'
    self.assertTrue(os.path.isfile(os.path.join(self.tmp_dir, sha_file)))
    self.assertTrue(os.path.isfile(os.path.join(self.tmp_dir, file_name)))

  @mock.patch(
      'telemetry.wpr.archive_info.cloud_storage.CalculateHash',
      return_value='filehash')
  def testAddRecordedStoriesDefault(self, hash_mock):
    test_archive_info = self.createArchiveInfo()
    self.assertWprFileDoesNotExist('data_006.wprgo')

    new_temp_recording = os.path.join(self.tmp_dir, 'recording.wprgo')
    expected_archive_file_path = os.path.join(self.tmp_dir, 'data_006.wprgo')

    with open(new_temp_recording, 'w') as f:
      f.write('wpr data')

    test_archive_info.AddNewTemporaryRecording(new_temp_recording)
    test_archive_info.AddRecordedStories([page2, page3])

    with open(self.story_set_archive_info_file, 'r') as f:
      archive_file_contents = json.load(f)

    expected_archive_contents = _BASE_ARCHIVE.copy()
    expected_archive_contents['archives'] = {
        page1.name: {
            _DEFAULT_PLATFORM: recording1
        },
        page2.name: {
            _DEFAULT_PLATFORM: 'data_006.wprgo'
        },
        page3.name: {
            _DEFAULT_PLATFORM: u'data_006.wprgo',
            'linux': recording4,
            'mac': recording3,
            'win': recording2,
            'android': recording5
        }
    }

    self.assertDictEqual(expected_archive_contents, archive_file_contents)
    # Ensure the saved JSON does not contain trailing spaces.
    with open(self.story_set_archive_info_file, 'rU') as f:
      for line in f:
        self.assertFalse(line.rstrip('\n').endswith(' '))
    self.assertWprFileDoesExist('data_006.wprgo')
    self.assertEquals(hash_mock.call_count, 1)
    hash_mock.assert_called_with(expected_archive_file_path)

  @mock.patch(
      'telemetry.wpr.archive_info.cloud_storage.CalculateHash',
      return_value='filehash')
  def testAddRecordedStoriesNotDefault(self, hash_mock):
    test_archive_info = self.createArchiveInfo()
    self.assertWprFileDoesNotExist('data_006.wprgo')
    new_temp_recording = os.path.join(self.tmp_dir, 'recording.wprgo')
    expected_archive_file_path = os.path.join(self.tmp_dir, 'data_006.wprgo')

    with open(new_temp_recording, 'w') as f:
      f.write('wpr data')
    test_archive_info.AddNewTemporaryRecording(new_temp_recording)
    test_archive_info.AddRecordedStories([page2, page3],
                                         target_platform='android')

    with open(self.story_set_archive_info_file, 'r') as f:
      archive_file_contents = json.load(f)

    expected_archive_contents = _BASE_ARCHIVE.copy()
    expected_archive_contents['archives'] = {
        page1.name: {
            _DEFAULT_PLATFORM: recording1
        },
        page2.name: {
            _DEFAULT_PLATFORM: recording2,
            'android': 'data_006.wprgo'
        },
        page3.name: {
            _DEFAULT_PLATFORM: recording1,
            'linux': recording4,
            'mac': recording3,
            'win': recording2,
            'android': 'data_006.wprgo'
        },
    }

    self.assertDictEqual(expected_archive_contents, archive_file_contents)
    # Ensure the saved JSON does not contain trailing spaces.
    with open(self.story_set_archive_info_file, 'rU') as f:
      for line in f:
        self.assertFalse(line.rstrip('\n').endswith(' '))
    self.assertWprFileDoesExist('data_006.wprgo')
    self.assertEqual(hash_mock.call_count, 1)
    hash_mock.assert_called_with(expected_archive_file_path)

  def testAddRecordedStoriesNewPage(self):
    test_archive_info = self.createArchiveInfo()
    self.assertWprFileDoesNotExist('data_006.wprgo')
    self.assertWprFileDoesNotExist('data_007.wprgo')
    new_temp_recording = os.path.join(self.tmp_dir, 'recording.wprgo')
    expected_archive_file_path1 = os.path.join(self.tmp_dir, 'data_006.wprgo')
    expected_archive_file_path2 = os.path.join(self.tmp_dir, 'data_007.wprgo')
    hash_dictionary = {
        expected_archive_file_path1: 'filehash',
        expected_archive_file_path2: 'filehash2'
    }

    def hash_side_effect(file_path):
      return hash_dictionary[file_path]

    with mock.patch('telemetry.wpr.archive_info.cloud_storage.CalculateHash',
                    side_effect=hash_side_effect) as hash_mock:

      with open(new_temp_recording, 'w') as f:
        f.write('wpr data')
      test_archive_info.AddNewTemporaryRecording(new_temp_recording)
      test_archive_info.AddRecordedStories([pageNew1])

      with open(new_temp_recording, 'w') as f:
        f.write('wpr data2')
      test_archive_info.AddNewTemporaryRecording(new_temp_recording)
      test_archive_info.AddRecordedStories([pageNew2],
                                           target_platform='android')

      with open(self.story_set_archive_info_file, 'r') as f:
        archive_file_contents = json.load(f)

      expected_archive_contents = _BASE_ARCHIVE.copy()
      expected_archive_contents['archives'] = {
          page1.name: {
              _DEFAULT_PLATFORM: recording1
          },
          page2.name: {
              _DEFAULT_PLATFORM: recording2,
          },
          page3.name: {
              _DEFAULT_PLATFORM: recording1,
              'linux': recording4,
              'mac': recording3,
              'win': recording2,
              'android': recording5
          },
          pageNew1.name: {
              _DEFAULT_PLATFORM: 'data_006.wprgo'
          },
          pageNew2.name: {
              _DEFAULT_PLATFORM: 'data_007.wprgo',
              'android': 'data_007.wprgo'
          }
      }

      self.assertDictEqual(expected_archive_contents, archive_file_contents)
      # Ensure the saved JSON does not contain trailing spaces.
      with open(self.story_set_archive_info_file, 'rU') as f:
        for line in f:
          self.assertFalse(line.rstrip('\n').endswith(' '))
      self.assertWprFileDoesExist('data_006.wprgo')
      self.assertWprFileDoesExist('data_007.wprgo')
      self.assertEqual(hash_mock.call_count, 2)
      hash_mock.assert_any_call(expected_archive_file_path1)
      hash_mock.assert_any_call(expected_archive_file_path2)
