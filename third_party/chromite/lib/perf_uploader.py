# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Uploads performance data to the performance dashboard.

The performance dashboard is owned by Chrome team and is available here:
https://chromeperf.appspot.com/
Users must be logged in with an @google.com account to view perf data there.

For more information on sending data to the dashboard, see:
http://dev.chromium.org/developers/testing/sending-data-to-the-performance-dashboard

Note: This module started off from the autotest/tko/perf_uploader.py but has
been extended significantly since.
"""

from __future__ import print_function

import collections
import httplib
import json
import math
import os
import re
import string
import urllib
import urllib2

from chromite.lib import cros_logging as logging
from chromite.lib import osutils
from chromite.lib import retry_util


# Clearly mark perf values coming from chromite by default.
_DEFAULT_TEST_PREFIX = 'cbuildbot.'
_DEFAULT_PLATFORM_PREFIX = 'cros-'
_ROOT_DIR = os.path.dirname(os.path.abspath(__file__))
_PRESENTATION_CONFIG_FILE = os.path.join(_ROOT_DIR,
                                         'perf_dashboard_config.json')

LOCAL_DASHBOARD_URL = 'http://localhost:8080'
STAGE_DASHBOARD_URL = 'https://chrome-perf.googleplex.com'
DASHBOARD_URL = 'https://chromeperf.appspot.com'

_MAX_DESCRIPTION_LENGTH = 256
_MAX_UNIT_LENGTH = 32

# Format for Chrome and Chrome OS version strings.
_VERSION_REGEXP = r'^(\d+)\.(\d+)\.(\d+)\.(\d+)$'

class PerfUploadingError(Exception):
  """A class to wrap errors in this module.

  This exception class has two attributes: value and orig_exc. "value" is what
  was used to create this exception while "orig_exc" is the optional original
  exception that is wrapped by this exception.
  """

  def __init__(self, value, orig_exc=None):
    super(PerfUploadingError, self).__init__(value)
    self.orig_exc = orig_exc

  def __str__(self):
    r = super(PerfUploadingError, self).__str__()
    if self.orig_exc:
      r += '\ncaused by: %s' % str(self.orig_exc)
    return r


PerformanceValue = collections.namedtuple(
    'PerformanceValue',
    'description value units higher_is_better graph stdio_uri')


def OutputPerfValue(filename, description, value, units,
                    higher_is_better=True, graph=None, stdio_uri=None):
  """Record a measured performance value in an output file.

  This is originally from autotest/files/client/common_lib/test.py.

  The output file will subsequently be parsed by ImageTestStage to have the
  information sent to chromeperf.appspot.com.

  Args:
    filename: A path to the output file. Data will be appended to this file.
    description: A string describing the measured perf value. Must
      be maximum length 256, and may only contain letters, numbers,
      periods, dashes, and underscores.  For example:
      "page_load_time", "scrolling-frame-rate".
    value: A number representing the measured perf value, or a list of
      measured values if a test takes multiple measurements. Measured perf
      values can be either ints or floats.
    units: A string describing the units associated with the measured perf
      value(s). Must be maximum length 32, and may only contain letters,
      numbers, periods, dashes, and uderscores. For example: "msec", "fps".
    higher_is_better: A boolean indicating whether or not a higher measured
      perf value is considered better. If False, it is assumed that a "lower"
      measured value is better.
    graph: A string indicating the name of the graph on which the perf value
      will be subsequently displayed on the chrome perf dashboard. This
      allows multiple metrics to be grouped together on the same graph.
      Default to None, perf values should be graphed individually on separate
      graphs.
    stdio_uri: A URL relevant to this data point (e.g. the buildbot log).
  """
  def ValidateString(param_name, value, max_len):
    if len(value) > max_len:
      raise ValueError('%s must be at most %d characters.', param_name, max_len)

    allowed_chars = string.ascii_letters + string.digits + '-._'
    if not set(value).issubset(set(allowed_chars)):
      raise ValueError(
          '%s may only contain letters, digits, hyphens, periods, and '
          'underscores. Its current value is %s.',
          param_name, value
      )

  ValidateString('description', description, _MAX_DESCRIPTION_LENGTH)
  ValidateString('units', units, _MAX_UNIT_LENGTH)

  entry = {
      'description': description,
      'value': value,
      'units': units,
      'higher_is_better': higher_is_better,
      'graph': graph,
      'stdio_uri': stdio_uri,
  }

  data = (json.dumps(entry), '\n')
  osutils.WriteFile(filename, data, 'a')


def LoadPerfValues(filename):
  """Return a list of PerformanceValue objects from |filename|."""
  lines = osutils.ReadFile(filename).splitlines()
  entries = []
  for line in lines:
    entry = json.loads(line)
    entries.append(PerformanceValue(**entry))
  return entries


def _AggregateIterations(perf_values):
  """Aggregate same measurements from multiple iterations.

  Each perf measurement may exist multiple times across multiple iterations
  of a test.  Here, the results for each unique measured perf metric are
  aggregated across multiple iterations.

  Args:
    perf_values: A list of PerformanceValue objects.

  Returns:
    A dictionary mapping each unique measured perf value (keyed by tuple of
      its description and graph name) to information about that perf value
      (in particular, the value is a list of values for each iteration).
  """
  aggregated_data = {}
  for perf_value in perf_values:
    key = (perf_value.description, perf_value.graph)
    try:
      aggregated_entry = aggregated_data[key]
    except KeyError:
      aggregated_entry = {
          'units': perf_value.units,
          'higher_is_better': perf_value.higher_is_better,
          'graph': perf_value.graph,
          'value': [],
      }
      aggregated_data[key] = aggregated_entry
    # Note: the stddev will be recomputed later when the results
    # from each of the multiple iterations are averaged together.
    aggregated_entry['value'].append(perf_value.value)
  return aggregated_data


def _MeanAndStddev(data, precision=4):
  """Computes mean and standard deviation from a list of numbers.

  Args:
    data: A list of numeric values.
    precision: The integer number of decimal places to which to
      round the results.

  Returns:
    A 2-tuple (mean, standard_deviation), in which each value is
      rounded to |precision| decimal places.
  """
  n = len(data)
  if n == 0:
    raise ValueError('Cannot compute mean and stddev of an empty list.')
  if n == 1:
    return round(data[0], precision), 0

  mean = math.fsum(data) / n
  # Divide by n-1 to compute "sample standard deviation".
  variance = math.fsum((elem - mean) ** 2 for elem in data) / (n - 1)
  return round(mean, precision), round(math.sqrt(variance), precision)


def _ComputeAvgStddev(perf_data):
  """Compute average and standard deviations as needed for perf measurements.

  For any perf measurement that exists in multiple iterations (has more than
  one measured value), compute the average and standard deviation for it and
  then store the updated information in the dictionary (in place).

  Args:
    perf_data: A dictionary of measured perf data as computed by
      _AggregateIterations(), except each "value" is now a single value, not
      a list of values.
  """
  for perf in perf_data.itervalues():
    perf['value'], perf['stddev'] = _MeanAndStddev(perf['value'])
  return perf_data


PresentationInfo = collections.namedtuple(
    'PresentationInfo',
    'master_name test_name')


def _GetPresentationInfo(test_name):
  """Get presentation info for |test_name| from config file.

  Args:
    test_name: The test name.

  Returns:
    A PresentationInfo object for this test.
  """
  infos = osutils.ReadFile(_PRESENTATION_CONFIG_FILE)
  infos = json.loads(infos)
  for info in infos:
    if info['test_name'] == test_name:
      try:
        return PresentationInfo(**info)
      except:
        raise PerfUploadingError('No master found for %s' % test_name)

  raise PerfUploadingError('No presentation config found for %s' % test_name)


def _FormatForUpload(perf_data, platform_name, presentation_info, revision=None,
                     cros_version=None, chrome_version=None, test_prefix=None,
                     platform_prefix=None):
  """Formats perf data suitably to upload to the perf dashboard.

  The perf dashboard expects perf data to be uploaded as a
  specially-formatted JSON string.  In particular, the JSON object must be a
  dictionary with key "data", and value being a list of dictionaries where
  each dictionary contains all the information associated with a single
  measured perf value: master name, bot name, test name, perf value, units,
  and build version numbers.

  See also google3/googleclient/chrome/speed/dashboard/add_point.py for the
  server side handler.

  Args:
    platform_name: The string name of the platform.
    perf_data: A dictionary of measured perf data. This is keyed by
      (description, graph name) tuple.
    presentation_info: A PresentationInfo object of the given test.
    revision: The raw X-axis value; normally it represents a VCS repo, but may
      be any monotonic increasing value integer.
    cros_version: A string identifying Chrome OS version e.g. '6052.0.0'.
    chrome_version: A string identifying Chrome version e.g. '38.0.2091.2'.
    test_prefix: Arbitrary string to automatically prefix to the test name.
      If None, then 'cbuildbot.' is used to guarantee namespacing.
    platform_prefix: Arbitrary string to automatically prefix to
      |platform_name|. If None, then 'cros-' is used to guarantee namespacing.

  Returns:
    A dictionary containing the formatted information ready to upload
      to the performance dashboard.
  """
  if test_prefix is None:
    test_prefix = _DEFAULT_TEST_PREFIX
  if platform_prefix is None:
    platform_prefix = _DEFAULT_PLATFORM_PREFIX

  dash_entries = []
  for (desc, graph), data in perf_data.iteritems():
    # Each perf metric is named by a path that encodes the test name,
    # a graph name (if specified), and a description.  This must be defined
    # according to rules set by the Chrome team, as implemented in:
    # chromium/tools/build/scripts/slave/results_dashboard.py.
    desc = desc.replace('/', '_')
    test_name = test_prefix + presentation_info.test_name
    test_parts = [test_name, desc]
    if graph:
      test_parts.insert(1, graph)
    test_path = '/'.join(test_parts)

    supp_cols = {'a_default_rev': 'r_cros_version'}
    if data.get('stdio_uri'):
      supp_cols['a_stdio_uri'] = data['stdio_uri']
    if cros_version is not None:
      supp_cols['r_cros_version'] = cros_version
    if chrome_version is not None:
      supp_cols['r_chrome_version'] = chrome_version

    new_dash_entry = {
        'master': presentation_info.master_name,
        'bot': platform_prefix + platform_name,
        'test': test_path,
        'value': data['value'],
        'error': data['stddev'],
        'units': data['units'],
        'higher_is_better': data['higher_is_better'],
        'supplemental_columns': supp_cols,
    }
    if revision is not None:
      new_dash_entry['revision'] = revision

    dash_entries.append(new_dash_entry)

  json_string = json.dumps(dash_entries)
  return {'data': json_string}


def _SendToDashboard(data_obj, dashboard=DASHBOARD_URL):
  """Sends formatted perf data to the perf dashboard.

  Args:
    data_obj: A formatted data object as returned by _FormatForUpload().
    dashboard: The dashboard to upload data to.

  Raises:
    PerfUploadingError if an exception was raised when uploading.
  """
  upload_url = os.path.join(dashboard, 'add_point')
  encoded = urllib.urlencode(data_obj)
  req = urllib2.Request(upload_url, encoded)
  try:
    urllib2.urlopen(req)
  except urllib2.HTTPError as e:
    raise PerfUploadingError('HTTPError: %d %s for JSON %s\n' %
                             (e.code, e.msg, data_obj['data']), e)
  except urllib2.URLError as e:
    raise PerfUploadingError('URLError: %s for JSON %s\n' %
                             (str(e.reason), data_obj['data']), e)
  except httplib.HTTPException as e:
    raise PerfUploadingError(
        'HTTPException for JSON %s\n' % data_obj['data'], e)


def _ComputeRevisionFromVersions(chrome_version, cros_version):
  """Computes the point ID to use, from Chrome and Chrome OS version numbers.

  For ChromeOS row data, data values are associated with both a Chrome
  version number and a ChromeOS version number (unlike for Chrome row data
  that is associated with a single revision number).  This function takes
  both version numbers as input, then computes a single, unique integer ID
  from them, which serves as a 'fake' revision number that can uniquely
  identify each ChromeOS data point, and which will allow ChromeOS data points
  to be sorted by Chrome version number, with ties broken by ChromeOS version
  number.

  To compute the integer ID, we take the portions of each version number that
  serve as the shortest unambiguous names for each (as described here:
  http://www.chromium.org/developers/version-numbers).  We then force each
  component of each portion to be a fixed width (padded by zeros if needed),
  concatenate all digits together (with those coming from the Chrome version
  number first), and convert the entire string of digits into an integer.
  We ensure that the total number of digits does not exceed that which is
  allowed by AppEngine NDB for an integer (64-bit signed value).

  For example:
    Chrome version: 27.0.1452.2 (shortest unambiguous name: 1452.2)
    ChromeOS version: 27.3906.0.0 (shortest unambiguous name: 3906.0.0)
    concatenated together with padding for fixed-width columns:
        ('01452' + '002') + ('03906' + '000' + '00') = '014520020390600000'
    Final integer ID: 14520020390600000

  Args:
    chrome_version: The Chrome version number as a string.
    cros_version: The ChromeOS version number as a string.

  Returns:
    A unique integer ID associated with the two given version numbers.
  """
  # Number of digits to use from each part of the version string for Chrome
  # and Chrome OS versions when building a point ID out of these two versions.
  chrome_version_col_widths = [0, 0, 5, 3]
  cros_version_col_widths = [0, 5, 3, 2]

  def get_digits_from_version(version_num, column_widths):
    if re.match(_VERSION_REGEXP, version_num):
      computed_string = ''
      version_parts = version_num.split('.')
      for i, version_part in enumerate(version_parts):
        if column_widths[i]:
          computed_string += version_part.zfill(column_widths[i])
      return computed_string
    else:
      return None

  chrome_digits = get_digits_from_version(
      chrome_version, chrome_version_col_widths)
  cros_digits = get_digits_from_version(
      cros_version, cros_version_col_widths)
  if not chrome_digits or not cros_digits:
    return None
  result_digits = chrome_digits + cros_digits
  max_digits = sum(chrome_version_col_widths + cros_version_col_widths)
  if len(result_digits) > max_digits:
    return None
  return int(result_digits)


def _RetryIfServerError(perf_exc):
  """Exception handler to retry an upload if error code is 5xx.

  Args:
    perf_exc: The exception from _SendToDashboard.

  Returns:
    True if the cause of |perf_exc| is HTTP 5xx error.
  """
  return (isinstance(perf_exc.orig_exc, urllib2.HTTPError) and
          perf_exc.orig_exc.code >= 500)


def UploadPerfValues(perf_values, platform_name, test_name, revision=None,
                     cros_version=None, chrome_version=None,
                     dashboard=DASHBOARD_URL, master_name=None,
                     test_prefix=None, platform_prefix=None, dry_run=False):
  """Uploads any perf data associated with a test to the perf dashboard.

  Note: If |revision| is used, then |cros_version| & |chrome_version| are not
  necessary.  Conversely, if |revision| is not used, then |cros_version| and
  |chrome_version| must both be specified.

  Args:
    perf_values: List of PerformanceValue objects.
    platform_name: A string identifying platform e.g. 'x86-release'. 'cros-'
      will be prepended to |platform_name| internally, by _FormatForUpload.
    test_name: A string identifying the test
    revision: The raw X-axis value; normally it represents a VCS repo, but may
      be any monotonic increasing value integer.
    cros_version: A string identifying Chrome OS version e.g. '6052.0.0'.
    chrome_version: A string identifying Chrome version e.g. '38.0.2091.2'.
    dashboard: The dashboard to upload data to.
    master_name: The "master" field to use; by default it is looked up in the
      perf_dashboard_config.json database.
    test_prefix: Arbitrary string to automatically prefix to the test name.
      If None, then 'cbuildbot.' is used to guarantee namespacing.
    platform_prefix: Arbitrary string to automatically prefix to
      |platform_name|. If None, then 'cros-' is used to guarantee namespacing.
    dry_run: Do everything but upload the data to the server.
  """
  if not perf_values:
    return

  # Aggregate values from multiple iterations together.
  perf_data = _AggregateIterations(perf_values)

  # Compute averages and standard deviations as needed for measured perf
  # values that exist in multiple iterations.  Ultimately, we only upload a
  # single measurement (with standard deviation) for every unique measured
  # perf metric.
  _ComputeAvgStddev(perf_data)

  # Format the perf data for the upload, then upload it.
  if revision is None:
    # No "revision" field, calculate one. Chrome and CrOS fields must be given.
    cros_version = chrome_version[:chrome_version.find('.') + 1] + cros_version
    revision = _ComputeRevisionFromVersions(chrome_version, cros_version)
  try:
    if master_name is None:
      presentation_info = _GetPresentationInfo(test_name)
    else:
      presentation_info = PresentationInfo(master_name, test_name)
    formatted_data = _FormatForUpload(perf_data, platform_name,
                                      presentation_info,
                                      revision=revision,
                                      cros_version=cros_version,
                                      chrome_version=chrome_version,
                                      test_prefix=test_prefix,
                                      platform_prefix=platform_prefix)
    if dry_run:
      logging.debug('UploadPerfValues: skipping upload due to dry-run')
    else:
      retry_util.GenericRetry(_RetryIfServerError, 3, _SendToDashboard,
                              formatted_data, dashboard=dashboard)
  except PerfUploadingError:
    logging.exception('Error when uploading perf data to the perf '
                      'dashboard for test %s.', test_name)
    raise
  else:
    logging.info('Successfully uploaded perf data to the perf '
                 'dashboard for test %s.', test_name)
