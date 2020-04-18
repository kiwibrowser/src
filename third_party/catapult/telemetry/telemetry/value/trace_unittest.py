# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import codecs
import json
import mock
import os
import shutil
import tempfile
import unittest

from telemetry import story
from telemetry import page as page_module
from telemetry.value import trace
from tracing_build import html2trace
from tracing.trace_data import trace_data


class TestBase(unittest.TestCase):

  def setUp(self):
    story_set = story.StorySet(base_dir=os.path.dirname(__file__))
    story_set.AddStory(
        page_module.Page('http://www.bar.com/', story_set, story_set.base_dir,
                         name='http://www.bar.com/'))
    story_set.AddStory(
        page_module.Page('http://www.baz.com/', story_set, story_set.base_dir,
                         name='http://www.baz.com/'))
    story_set.AddStory(
        page_module.Page('http://www.foo.com/', story_set, story_set.base_dir,
                         name='http://www.foo.com/'))
    self.story_set = story_set

  @property
  def pages(self):
    return self.story_set.stories


class TestSet(object):
  """ A test set that represents a set that contains any key. """

  def __contains__(self, key):
    return True


class TestDefaultDict(object):
  """ A test default dict that represents a dictionary that contains any key
  with value |default_value|. """

  def __init__(self, default_value):
    self._default_value = default_value
    self._test_set = TestSet()

  def __contains__(self, key):
    return key in self._test_set

  def __getitem__(self, key):
    return self._default_value

  def keys(self):
    return self._test_set


class ValueTest(TestBase):
  def testRepr(self):
    v = trace.TraceValue(
        self.pages[0], trace_data.CreateTraceDataFromRawData([{'test': 1}]),
        important=True, description='desc')

    self.assertEquals('TraceValue(http://www.bar.com/, trace)', str(v))

  @mock.patch('telemetry.value.trace.cloud_storage.Insert')
  def testAsDictWhenTraceSerializedAndUploaded(self, insert_mock):
    tempdir = tempfile.mkdtemp()
    try:
      file_path = os.path.join(tempdir, 'test.html')
      v = trace.TraceValue(
          None, trace_data.CreateTraceDataFromRawData([{'test': 1}]),
          file_path=file_path,
          upload_bucket=trace.cloud_storage.PUBLIC_BUCKET,
          remote_path='a.html',
          cloud_url='http://example.com/a.html')
      fh = v.Serialize()
      cloud_url = v.UploadToCloud()
      d = v.AsDict()
      self.assertEqual(d['file_id'], fh.id)
      self.assertEqual(d['cloud_url'], cloud_url)
      insert_mock.assert_called_with(
          trace.cloud_storage.PUBLIC_BUCKET,
          'a.html',
          file_path)
    finally:
      shutil.rmtree(tempdir)

  @mock.patch('telemetry.value.trace.cloud_storage.Insert')
  def testAsDictWhenTraceIsNotSerializedAndUploaded(self, insert_mock):
    test_temp_file = tempfile.NamedTemporaryFile(delete=False)
    try:
      v = trace.TraceValue(
          None, trace_data.CreateTraceDataFromRawData([{'test': 1}]),
          upload_bucket=trace.cloud_storage.PUBLIC_BUCKET,
          remote_path='a.html',
          cloud_url='http://example.com/a.html')
      cloud_url = v.UploadToCloud()
      d = v.AsDict()
      self.assertEqual(d['cloud_url'], cloud_url)
      insert_mock.assert_called_with(
          trace.cloud_storage.PUBLIC_BUCKET,
          'a.html',
          v.filename)
    finally:
      if os.path.exists(test_temp_file.name):
        test_temp_file.close()
        os.remove(test_temp_file.name)

  def testFindTraceParts(self):
    raw_data = {
        'powerTraceAsString': 'BattOr Data',
        'traceEvents': [{'trace': 1}],
        'tabIds': 'Tab Data',
    }
    data = trace_data.CreateTraceDataFromRawData(raw_data)
    v = trace.TraceValue(None, data)
    tempdir = tempfile.mkdtemp()
    temp_path = os.path.join(tempdir, 'test.json')
    battor_seen = False
    chrome_seen = False
    tabs_seen = False
    try:
      with codecs.open(v.filename, mode='r', encoding='utf-8') as f:
        trace_files = html2trace.CopyTraceDataFromHTMLFilePath(f, temp_path)
      for f in trace_files:
        with open(f, 'r') as trace_file:
          d = trace_file.read()
          if d == raw_data['powerTraceAsString']:
            self.assertFalse(battor_seen)
            battor_seen = True
          elif d == json.dumps({'traceEvents': raw_data['traceEvents']}):
            self.assertFalse(chrome_seen)
            chrome_seen = True
          elif d == raw_data['tabIds']:
            self.assertFalse(tabs_seen)
            tabs_seen = True
      self.assertTrue(battor_seen)
      self.assertTrue(chrome_seen)
      self.assertTrue(tabs_seen)
    finally:
      shutil.rmtree(tempdir)
      os.remove(v.filename)


def _IsEmptyDir(path):
  return os.path.exists(path) and not os.listdir(path)


class NoLeakedTempfilesTests(TestBase):

  def setUp(self):
    super(NoLeakedTempfilesTests, self).setUp()
    self.temp_test_dir = tempfile.mkdtemp()
    self.actual_tempdir = trace.tempfile.tempdir
    trace.tempfile.tempdir = self.temp_test_dir

  def testNoLeakedTempFileOnImplicitCleanUp(self):
    with trace.TraceValue(
        None, trace_data.CreateTraceDataFromRawData([{'test': 1}])):
      pass
    self.assertTrue(_IsEmptyDir(self.temp_test_dir))

  def testNoLeakedTempFileWhenUploadingTrace(self):
    v = trace.TraceValue(
        None, trace_data.CreateTraceDataFromRawData([{'test': 1}]))
    v.CleanUp()
    self.assertTrue(_IsEmptyDir(self.temp_test_dir))

  def tearDown(self):
    super(NoLeakedTempfilesTests, self).tearDown()
    shutil.rmtree(self.temp_test_dir)
    trace.tempfile.tempdir = self.actual_tempdir
