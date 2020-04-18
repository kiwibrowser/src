# -*- coding: utf-8 -*-
# Copyright 2017 Google Inc. All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish, dis-
# tribute, sublicense, and/or sell copies of the Software, and to permit
# persons to whom the Software is furnished to do so, subject to the fol-
# lowing conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
# ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
# SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
"""Tests for media helper functions and classes for GCS JSON API."""

from gslib.gcs_json_media import BytesTransferredContainer
from gslib.gcs_json_media import UploadCallbackConnectionClassFactory
import gslib.tests.testcase as testcase
import mock


class TestUploadCallbackConnection(testcase.GsUtilUnitTestCase):
  """Tests for the upload callback connection."""

  def setUp(self):
    super(TestUploadCallbackConnection, self).setUp()
    self.bytes_container = BytesTransferredContainer()
    self.class_factory = UploadCallbackConnectionClassFactory(
        self.bytes_container, buffer_size=50, total_size=100,
        progress_callback='Sample')
    self.instance = self.class_factory.GetConnectionClass()('host')

  @mock.patch('httplib.HTTPSConnection')
  def testHeaderDefaultBehavior(self, mock_conn):
    """Test the size modifier is correct under expected headers."""
    mock_conn.putheader.return_value = None
    self.instance.putheader('content-encoding', 'gzip')
    self.instance.putheader('content-length', '10')
    self.instance.putheader('content-range', 'bytes 0-104/*')
    # Ensure the modifier is as expected.
    self.assertAlmostEqual(self.instance.size_modifier, 10.5)

  @mock.patch('httplib.HTTPSConnection')
  def testHeaderIgnoreWithoutGzip(self, mock_conn):
    """Test that the gzip content-encoding is required to modify size."""
    mock_conn.putheader.return_value = None
    self.instance.putheader('content-length', '10')
    self.instance.putheader('content-range', 'bytes 0-99/*')
    # Ensure the modifier is unchanged.
    self.assertAlmostEqual(self.instance.size_modifier, 1.0)

  @mock.patch('httplib.HTTPSConnection')
  def testHeaderAlternativeRangeFormats(self, mock_conn):
    """Test alternative content-range header formats."""
    mock_conn.putheader.return_value = None
    expected_values = [
        # Test 'bytes %s-%s/*'
        ('bytes 0-99/*', 10.0),
        # Test 'bytes */%s'
        ('bytes */100', 1.0),
        # Test 'bytes %s-%s/%s'
        ('bytes 0-99/100', 10.0),]
    for (header, expected) in expected_values:
      self.instance = self.class_factory.GetConnectionClass()('host')
      self.instance.putheader('content-encoding', 'gzip')
      self.instance.putheader('content-length', '10')
      self.instance.putheader('content-range', header)
      # Ensure the modifier is as expected.
      self.assertAlmostEqual(self.instance.size_modifier, expected)

  @mock.patch('httplib.HTTPSConnection')
  def testHeaderParseFailure(self, mock_conn):
    """Test incorrect header values do not raise exceptions."""
    mock_conn.putheader.return_value = None
    self.instance.putheader('content-encoding', 'gzip')
    self.instance.putheader('content-length', 'bytes 10')
    self.instance.putheader('content-range', 'not a number')
    # Ensure the modifier is unchanged.
    self.assertAlmostEqual(self.instance.size_modifier, 1.0)

  @mock.patch('gslib.progress_callback.ProgressCallbackWithTimeout')
  @mock.patch('httplib2.HTTPSConnectionWithTimeout')
  def testSendDefaultBehavior(self, mock_conn, mock_callback):
    mock_conn.send.return_value = None
    self.instance.size_modifier = 2
    self.instance.processed_initial_bytes = True
    self.instance.callback_processor = mock_callback
    # Send 10 bytes of data.
    sample_data = b'0123456789'
    self.instance.send(sample_data)
    # Ensure the data is fully sent since the buffer size is 50 bytes.
    self.assertTrue(mock_conn.send.called)
    (_, sent_data), _ = mock_conn.send.call_args_list[0]
    self.assertEqual(sent_data, sample_data)
    # Ensure the progress callback is correctly scaled.
    self.assertTrue(mock_callback.Progress.called)
    [sent_bytes], _ = mock_callback.Progress.call_args_list[0]
    self.assertEqual(sent_bytes, 20)
