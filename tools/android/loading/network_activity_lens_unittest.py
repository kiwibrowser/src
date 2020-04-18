# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import copy
import unittest

from network_activity_lens import NetworkActivityLens
import test_utils


class NetworkActivityLensTestCase(unittest.TestCase):
  def testTimeline(self):
    timing_dict = {
        'requestTime': 1.2,
        'dnsStart': 20, 'dnsEnd': 30,
        'connectStart': 50, 'connectEnd': 60,
        'sendStart': 70, 'sendEnd': 80,
        'receiveHeadersEnd': 90,
        'loadingFinished': 100}
    request = test_utils.MakeRequestWithTiming(1, 2, timing_dict)
    lens = self._NetworkActivityLens([request])
    start_end_times = lens.uploaded_bytes_timeline[0]
    expected_start_times = [
        1220., 1230., 1250., 1260., 1270., 1280., 1290., 1300.]
    self.assertListEqual(expected_start_times, start_end_times)
    timing_dict = copy.copy(timing_dict)
    timing_dict['requestTime'] += .005
    second_request = test_utils.MakeRequestWithTiming(1, 2, timing_dict)
    lens = self._NetworkActivityLens([request, second_request])
    start_end_times = lens.uploaded_bytes_timeline[0]
    expected_start_times = sorted(
        expected_start_times + [x + 5. for x in expected_start_times])
    for (expected, actual) in zip(expected_start_times, start_end_times):
      self.assertAlmostEquals(expected, actual)

  def testTransferredBytes(self):
    timing_dict = {
        'requestTime': 1.2,
        'dnsStart': 20, 'dnsEnd': 30,
        'connectStart': 50, 'connectEnd': 60,
        'sendStart': 70, 'sendEnd': 80,
        'receiveHeadersEnd': 90,
        'loadingFinished': 100}
    request = test_utils.MakeRequestWithTiming(1, 2, timing_dict)
    request.request_headers = {'a': 'b'}
    request.response_headers = {'c': 'def'}
    lens = self._NetworkActivityLens([request])
    # Upload
    upload_timeline = lens.uploaded_bytes_timeline
    self.assertEquals(1270, upload_timeline[0][4])
    self.assertEquals(1280, upload_timeline[0][5])
    self.assertEquals(0, upload_timeline[1][4])
    self.assertEquals(2, upload_timeline[1][5])
    self.assertEquals(0, upload_timeline[1][6])
    upload_rate = lens.upload_rate_timeline
    self.assertEquals(2 / 10e-3, upload_rate[1][4])
    self.assertEquals(0, upload_rate[1][5])
    # Download
    download_timeline = lens.downloaded_bytes_timeline
    download_rate = lens.download_rate_timeline
    self.assertEquals(1280, download_timeline[0][5])
    self.assertEquals(1290, download_timeline[0][6])
    self.assertEquals(0, download_timeline[1][5])
    self.assertEquals(4, download_timeline[1][6])
    self.assertEquals(0, download_timeline[1][7])
    download_rate = lens.download_rate_timeline
    self.assertEquals(4 / 10e-3, download_rate[1][5])
    self.assertEquals(0, download_rate[1][6])
    self.assertAlmostEquals(4, lens.total_download_bytes)

  def testLongRequest(self):
    timing_dict = {
        'requestTime': 1200,
        'dnsStart': 20, 'dnsEnd': 30,
        'connectStart': 50, 'connectEnd': 60,
        'sendStart': 70, 'sendEnd': 80,
        'receiveHeadersEnd': 90,
        'loadingFinished': 100}
    request = test_utils.MakeRequestWithTiming(1, 2, timing_dict)
    request.response_headers = {}
    timing_dict = {
        'requestTime': 1200,
        'dnsStart': 2, 'dnsEnd': 3,
        'connectStart': 5, 'connectEnd': 6,
        'sendStart': 7, 'sendEnd': 8,
        'receiveHeadersEnd': 10,
        'loadingFinished': 1000}
    long_request = test_utils.MakeRequestWithTiming(1, 2, timing_dict)
    long_request.response_headers = {}
    long_request.encoded_data_length = 1000
    lens = self._NetworkActivityLens([request, long_request])
    (timestamps, downloaded_bytes) = lens.downloaded_bytes_timeline
    (_, download_rate) = lens.download_rate_timeline
    start_receive = (long_request.start_msec
                     + long_request.timing.receive_headers_end)
    end_receive = (long_request.start_msec
                   + long_request.timing.loading_finished)
    self.assertEquals(1000, downloaded_bytes[-1])
    for (index, timestamp) in enumerate(timestamps):
      if start_receive < timestamp < end_receive:
        self.assertAlmostEqual(1000 / 990e-3, download_rate[index])
        self.assertEquals(0, downloaded_bytes[index])
    self.assertEquals(1000, downloaded_bytes[-1])

  def testDownloadedBytesAt(self):
    timing_dict = {
        'requestTime': 1.2,
        'dnsStart': 20, 'dnsEnd': 30,
        'connectStart': 50, 'connectEnd': 60,
        'sendStart': 70, 'sendEnd': 80,
        'receiveHeadersEnd': 90,
        'loadingFinished': 100}
    request = test_utils.MakeRequestWithTiming(1, 2, timing_dict)
    lens = self._NetworkActivityLens([request])
    # See testTransferredBytes for key events times. We test around events at
    # the start, middle and end of the data transfer as well as for the
    # interpolation.
    self.assertEquals(0, lens.DownloadedBytesAt(1219))
    self.assertEquals(0, lens.DownloadedBytesAt(1220))
    self.assertEquals(0, lens.DownloadedBytesAt(1225))
    self.assertEquals(0, lens.DownloadedBytesAt(1280))
    self.assertEquals(1.6, lens.DownloadedBytesAt(1281))
    self.assertEquals(8, lens.DownloadedBytesAt(1285))
    self.assertEquals(14.4, lens.DownloadedBytesAt(1289))
    self.assertEquals(16, lens.DownloadedBytesAt(1290))
    self.assertEquals(16, lens.DownloadedBytesAt(1291))
    self.assertEquals(16, lens.DownloadedBytesAt(1295))
    self.assertEquals(16, lens.DownloadedBytesAt(1300))
    self.assertEquals(16, lens.DownloadedBytesAt(1400))

  def _NetworkActivityLens(self, requests):
    trace = test_utils.LoadingTraceFromEvents(requests)
    return NetworkActivityLens(trace)


if __name__ == '__main__':
  unittest.main()
