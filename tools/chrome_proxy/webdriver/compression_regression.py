# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import json
import math
import subprocess
import time

import common
from common import TestDriver
from common import IntegrationTest
from decorators import NotAndroid
from decorators import Slow

# The maximum number of data points that will be saved.
MAX_DATA_POINTS = 365

# The number of days in the past to compute the average of for regression
# alerting.
ALERT_WINDOW = 31

# The amount of tolerable difference a single data point can be away from the
# average without alerting. This is a percentage from 0.0 to 1.0 inclusive.
# 3% was chosen because over the course of the first month no change was seen to
# 8 decimal places. Erring on the more sensitive side to begin with is also
# better so we get a better feel for the timing and degree of regressions.
THRESHOLD = 0.03

# The format to use when recording dates in the DATA_FILE
DATE_FORMAT = '%Y-%m-%d'

# The persistant storage for compression data is kept in Google Storage with
# this bucket name.
BUCKET = 'chrome_proxy_compression'

# The data file name in the Google Storage bucket, above. The data file is also
# saved locally under the same name.
DATA_FILE = 'compression_data.json'

class CompressionRegression(IntegrationTest):
  """This class is responsible for alerting the Chrome Proxy team to regression
  in the compression metrics of the proxy. At present, this class will simply
  gather data and save it to a Google Storage bucket. Once enough data has been
  gathered to form a reasonable model, alerting will be added to check for
  regression.

  Before running the test, this class will fetch the JSON data file from Google
  Storage in a subprocess and store it on the local disk with the same file
  name. The data is then read from that file. After running the test, if the
  data has changed the file will be uploaded back to Google Storage.

  The JSON data object and data dict object used widely in this class has the
  following structure:
  {
    "2017-02-29": {
      "html": 0.314,
      "jpg": 0.1337,
      "png": 0.1234,
      "mp4": 0.9876
    }
  }
  where keys are date stamps in the form "YYYY-MM-DD", and each key in the child
  object is the resource type with its compression value.

  Also frequently referenced is the compression_average dict object, which
  contains the compression data just now gathered from Chrome in
  getCurrentCompressionMetrics(). That object has the following structure:
  {
    "test/html": 0.314,
    "image/jpg": 0.1337,
    "image/png": 0.1234,
    "video/mp4": 0.9876
  }
  where keys are the content type with its compression value.

  Due to the complexity of several methods in this class, a number of local
  unit tests can be found at the bottom of this file.

  Please note that while this test uses the IntegrationTest framework, it is
  classified as a regression test.
  """

  @Slow
  @NotAndroid
  def testCompression(self):
      """This function is the main test function for regression compression
      checking and facilitates the test with all of the helper functions'
      behavior.
      """
      compression_average = self.getCurrentCompressionMetricsWithRetry()
      self.fetchFromGoogleStorage()
      data = {}
      with open(DATA_FILE, 'r') as data_fp:
        data = json.load(data_fp)
      if self.updateDataObject(compression_average, data):
        with open(DATA_FILE, 'w') as data_fp:
          json.dump(data, data_fp)
        self.uploadToGoogleStorage()

  def getCurrentCompressionMetricsWithRetry(self, max_attempts=10):
    """This function allows some number of attempts to be tried to fetch
    compressed responses. Sometimes, the proxy will not have compressed results
    available immediately, especially for video resources.

    Args:
      max_attempts: the maximum number of attempts to try to fetch compressed
        resources.
    Returns:
      a dict object mapping resource type to compression
    """
    attempts = 0
    while attempts < max_attempts:
      try:
        return self.getCurrentCompressionMetrics()
      except Exception as e:
        attempts += 1
        time.sleep(2)
    if attempts >= max_attempts:
      raise Exception("Didn't get good response after %d attempts" % attempts)

  def getCurrentCompressionMetrics(self):
    """This function uses the ChromeDriver framework to open Chrome and navigate
    to a number of static resources of different types, like jpg, png, mp4, gif,
    html. Multiple resources of a single type are supported. This function will
    check that each resource was fetched via the Chrome Proxy, and then compute
    the compression as a percentage from the Content-Length and OFCL in
    Chrome-Proxy headers where compression = 1 - (cl / ofcl). The function will
    then return the average compression for each of the resource types.

    Returns:
      a dict object mapping resource type to compression
    """
    def AddToCompression(compression, key, value):
      if key in compression:
        compression[key].append(value)
      else:
        compression[key] = [value]
    with TestDriver() as t:
      t.AddChromeArg('--enable-spdy-proxy-auth')
      t.AddChromeArg('--data-reduction-proxy-server-experiments-disabled')
      t.LoadURL('http://check.googlezip.net/metrics/local.gif')
      t.LoadURL('http://check.googlezip.net/metrics/local.png')
      t.LoadURL('http://check.googlezip.net/metrics/local.jpg')
      t.LoadURL(
        'http://check.googlezip.net/cacheable/video/buck_bunny_tiny.html')
      compression = {}
      for response in t.GetHTTPResponses():
        # Check that the response was proxied.
        self.assertHasChromeProxyViaHeader(response)
        # Compute compression metrics.
        cl = response.response_headers['content-length']
        ofcl = getChromeProxyOFCL(response)
        content_type = response.response_headers['content-type']
        compression_rate = 1.0 - (float(cl) / float(ofcl))
        if 'html' in response.response_headers['content-type']:
          AddToCompression(compression, 'html', compression_rate)
        else:
          resource = response.url[response.url.rfind('/'):]
          AddToCompression(compression, resource[resource.rfind('.') + 1:],
            compression_rate)
      # Compute the average compression for each resource type.
      compression_average = {}
      for resource_type in compression:
        compression_average[resource_type] = (sum(compression[resource_type]) /
          float(len(compression[resource_type])))
      return compression_average

    # Returns the ofcl value in chrome-proxy header.
    def getChromeProxyOFCL(self, response):
      self.assertIn('chrome-proxy', response.response_headers)
      chrome_proxy_header = response.response_headers['chrome-proxy']
      self.assertIn('ofcl=', chrome_proxy_header)
      return chrome_proxy_header.split('ofcl=', 1)[1].split(',', 1)[0]


  def updateDataObject(self, compression_average, data,
      today=datetime.date.today()):
    """This function handles the updating of the data object when new data is
    available. Given the existing data object, the results of the
    getCurrentCompressionMetrics() func, and a date object, it will check if
    data exists for today. If it does, the method will do nothing and return
    False. Otherwise, it will update the data object with the compression data.
    If needed, it will also find the least recent entry in the data object and
    remove it.

    Args:
      compression_average: the compression data from
        getCurrentCompressionMetrics()
      data: all saved results from previous runs
      today: a date object, specifiable here for testing purposes.
    Returns:
      True iff the data object was changed
    """
    datestamp = today.strftime(DATE_FORMAT)
    # Check if this data has already been recorded.
    if datestamp in data:
      return False
    # Append new data, removing the least recent if needed.
    data[datestamp] = compression_average
    if len(data) > MAX_DATA_POINTS:
      min_date = None
      for date_str in data:
        date = datetime.date(*[int(d) for d in date_str.split('-')])
        if min_date == None or date < min_date:
          min_date = date
      del data[min_date.strftime(DATE_FORMAT)]
    return True

  def checkForRegression(self, data, compression_average):
    """This function checks whether the current data point in
    compression_average falls outside an allowable tolerance for the last
    ALERT_WINDOW days of data points in data. If so, an expection will be
    rasied.

    Args:
      data: all saved results from previous runs
      compression_average: the most recent data point
    """
    # Restructure data to be easily summable.
    data_sum_rt = {}
    for date in sorted(data, reverse=True):
      for resource_type in data[date]:
        if resource_type not in data_sum_rt:
          data_sum_rt[resource_type] = []
        data_sum_rt[resource_type].append(data[date][resource_type])
    # Compute average over ALERT_WINDOW if there is enough data points.
    # Data average will contain average compression ratios (eg: 1 - cl / ofcl).
    data_average = {}
    for resource_type in data_sum_rt:
      if len(data_sum_rt[resource_type]) >= ALERT_WINDOW:
        data_average[resource_type] = sum(
          data_sum_rt[resource_type][:ALERT_WINDOW]) / ALERT_WINDOW
    # Check regression, raising an exception if anything is detected.
    for resource_type in compression_average:
      if resource_type in data_average:
        expected = data_average[resource_type]
        actual = compression_average[resource_type]
        # Going over the max is ok, better compression is better.
        min_allowable = expected - THRESHOLD
        if actual < min_allowable:
          raise Exception('%s compression has regressed to %f' %
            (resource_type, actual))

  def uploadToGoogleStorage(self):
    """This function uses the gsutil command to upload the local data file to
    Google Storage.
    """
    gs_location = 'gs://%s/%s' % (BUCKET, DATA_FILE)
    cmd = ['gsutil', 'cp', DATA_FILE, gs_location]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    if proc.returncode:
      raise Exception('Uploading to Google Storage failed! output: %s %s' %
        (stdout, stderr))

  def fetchFromGoogleStorage(self):
    """This function uses the gsutil command to fetch the local data file from
    Google Storage.
    """
    gs_location = 'gs://%s/%s' % (BUCKET, DATA_FILE)
    cmd = ['gsutil', 'cp', gs_location, DATA_FILE]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    if proc.returncode:
      raise Exception('Fetching to Google Storage failed! output: %s %s' %
        (stdout, stderr))

  def test0UpdateDataObject_NoUpdate(self):
    """This unit test asserts that the updateDataObject() function doesn't
    update the data object when today is already contained in the data object.
    """
    data = { '2017-02-06': {'hello': 'world'}}
    new_data = {'Benoit': 'Mandelbrot'}
    test_day = datetime.date(2017, 02, 06)
    changed = self.updateDataObject(new_data, data, today=test_day)
    self.assertFalse(changed, "No data should have been recorded!")

  def test0UpdateDataObject_Update(self):
    """This unit test asserts that the updateDataObject() function updates the
    data object when there is new data available, also removing the least recent
    data point.
    """
    start_date = datetime.date(2017, 2, 6)
    data = {}
    for i in range(MAX_DATA_POINTS):
      date_obj = start_date + datetime.timedelta(days=i)
      datestamp = date_obj.strftime(DATE_FORMAT)
      data[datestamp] = {'hello': 'world'}
    new_data = {'Benoit': 'Mandelbrot'}
    test_day = datetime.date(2017, 02, 06) + datetime.timedelta(
      days=(MAX_DATA_POINTS))
    changed = self.updateDataObject(new_data, data, today=test_day)
    self.assertTrue(changed, "Data should have been recorded!")
    self.assertNotIn('2017-02-06', data)
    self.assertIn(test_day.strftime(DATE_FORMAT), data)

  def test0CheckForRegressionAverageRecent(self):
    """Make sure the checkForRegression() uses only the most recent data points.
    """
    data = {}
    start_date = datetime.date(2017, 2, 6)
    for i in range(2 * ALERT_WINDOW):
      date_obj = start_date + datetime.timedelta(days=i)
      datestamp = date_obj.strftime(DATE_FORMAT)
      data[datestamp] = {'mp4': 0.1}
    start_date = datetime.date(2017, 2, 6) + datetime.timedelta(days=(2 *
      ALERT_WINDOW))
    for i in range(ALERT_WINDOW):
      date_obj = start_date + datetime.timedelta(days=i)
      datestamp = date_obj.strftime(DATE_FORMAT)
      data[datestamp] = {'mp4': 0.9}
    # Expect no exception since the most recent data should have been used.
    self.checkForRegression(data, {'mp4': 0.9})

  def test0CheckForRegressionOnlySufficientData(self):
    """Make sure the checkForRegression() only checks resource types that have
    at least ALERT_WINDOW many data points.
    """
    data = {}
    start_date = datetime.date(2017, 2, 6)
    for i in range(2 * ALERT_WINDOW):
      date_obj = start_date + datetime.timedelta(days=i)
      datestamp = date_obj.strftime(DATE_FORMAT)
      data[datestamp] = {'mp4': 0.1}
    start_date = datetime.date(2017, 2, 6) + datetime.timedelta(days=(2 *
      ALERT_WINDOW))
    for i in range(ALERT_WINDOW):
      date_obj = start_date + datetime.timedelta(days=i)
      datestamp = date_obj.strftime(DATE_FORMAT)
      data[datestamp] = {'mp4': 0.9}
    for i in range(ALERT_WINDOW / 2):
      date_obj = start_date + datetime.timedelta(days=i)
      datestamp = date_obj.strftime(DATE_FORMAT)
      data[datestamp] = {'html': 0.3}
    # Expect no exception since the html should have been ignored for not having
    # enough data points.
    self.checkForRegression(data, {'mp4': 0.9, 'html': 0.1})

  def test0CheckForRegressionMismatchResourceTypes(self):
    """Make sure resource types that appear in only one of compression_average,
    data are not used or expected.
    """
    # Check using an extra resource type in the compression_average object.
    data = {}
    start_date = datetime.date(2017, 2, 6)
    for i in range(ALERT_WINDOW):
      date_obj = start_date + datetime.timedelta(days=i)
      datestamp = date_obj.strftime(DATE_FORMAT)
      data[datestamp] = {'mp4': 0.9}
    self.checkForRegression(data, {'mp4': 0.9, 'html': 0.2})
    # Check using an extra resource type in the data object.
    data = {}
    start_date = datetime.date(2017, 2, 6)
    for i in range(ALERT_WINDOW):
      date_obj = start_date + datetime.timedelta(days=i)
      datestamp = date_obj.strftime(DATE_FORMAT)
      data[datestamp] = {'mp4': 0.9, 'html': 0.2}
    self.checkForRegression(data, {'mp4': 0.9})

  def test0CheckForRegressionNoAlert(self):
    """Make sure checkForRegression does not alert when a new data point falls
    on the threshold.
    """
    data = {}
    start_date = datetime.date(2017, 2, 6)
    for i in range(ALERT_WINDOW):
      date_obj = start_date + datetime.timedelta(days=i)
      datestamp = date_obj.strftime(DATE_FORMAT)
      data[datestamp] = {'mp4': 0.9}
    self.checkForRegression(data, {'mp4': (0.9 - THRESHOLD)})

  def test0CheckForRegressionAlert(self):
    """Make sure checkForRegression does alert when a new data point falls
    outside of the threshold.
    """
    data = {}
    start_date = datetime.date(2017, 2, 6)
    for i in range(ALERT_WINDOW):
      date_obj = start_date + datetime.timedelta(days=i)
      datestamp = date_obj.strftime(DATE_FORMAT)
      data[datestamp] = {'mp4': 0.9}
    self.assertRaises(Exception, self.checkForRegression, data, {'mp4':
      (0.9 - THRESHOLD - 0.01)})


if __name__ == '__main__':
  IntegrationTest.RunAllTests()
