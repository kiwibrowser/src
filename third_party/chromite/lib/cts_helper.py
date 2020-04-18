# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""CTS/GTS-related helper functions/classes."""
from __future__ import print_function

import os
import glob
import gzip
import shutil

from chromite.lib import constants
from chromite.lib import cros_logging as logging

# TODO(pwang): Move CTS upload logic from autotest to here for consistency.

# Autotest test to collect list of CTS tests
TEST_LIST_COLLECTOR = 'tradefed-run-collect-tests-only'


def isCtsTest(test_name):
  """Check if the test is CTS/GTS tests."""
  for cts_prefix in constants.DEFAULT_CTS_TEST_XML_MAP.keys():
    if test_name.startswith(cts_prefix):
      return True
  return False


def getXMLPattern(test_name):
  """Return CTS result XML file pattern string."""
  for cts_prefix in constants.DEFAULT_CTS_TEST_XML_MAP.keys():
    resultXML = constants.DEFAULT_CTS_TEST_XML_MAP[cts_prefix]
    if test_name.startswith(cts_prefix):
      return resultXML
  return None


def getXMLGZFiles(test_name, test_folder):
  """Get CTS/GTS XML file in gz format within test_folder.

  Args:
    test_name: name of the test.
    test_folder: path to test folder.

  Returns:
    A list of xml.gz files or []
  """
  if not isCtsTest(test_name):
    return []
  _D = '[0-9][0-9]'
  _TIMESTAMP_PATTERN = '%s%s.%s.%s_%s.%s.%s' % (_D, _D, _D, _D, _D, _D, _D)
  xml_pattern = getXMLPattern(test_name)
  xml_files = glob.glob(os.path.join(test_folder, '*', 'results', '*',
                                     _TIMESTAMP_PATTERN, xml_pattern))
  files = []
  for xml_file in xml_files:
    xml_file_gz = '%s.gz' % xml_file
    with open(xml_file, 'r') as f_in, (
        gzip.open(xml_file_gz, 'w')) as f_out:
      shutil.copyfileobj(f_in, f_out)
    files.append(xml_file_gz)
  return files


def getApfeFiles(test_name, test_folder):
  """Get CTS/GTS Apfe file within test_folder.

  Args:
    test_name: name of the test.
    test_folder: path to test folder.

  Returns:
    A list of APFE files or []
  """
  if not isCtsTest(test_name):
    return []
  _D = '[0-9][0-9]'
  _TIMESTAMP_PATTERN = '%s%s.%s.%s_%s.%s.%s' % (_D, _D, _D, _D, _D, _D, _D)
  apfe_files = glob.glob(os.path.join(test_folder, '*', 'results', '*',
                                      '%s.zip' % _TIMESTAMP_PATTERN))
  return apfe_files


def _is_test_collector(package):
  """Returns true if the test run is just to collect list of CTS tests.

  Args:
    package: Autotest package name. e.g. cheets_CTS_N.CtsGraphicsTestCase

  Returns:
    Bool flag indicating a test package is CTS list generator or not.
  """
  return TEST_LIST_COLLECTOR in package


def uploadFiles(dir_entry, build, apfe_id, job_id, package, uploader,
                *args, **kwargs):
  """Upload CTS/GTS tests result to gs buckets.

  Args:
    dir_entry: path to the test folder.
    build: build name such as samus-release, or kevin-release.
    apfe_id: id number used for apfe upload, typically we use autotest parent
             job id.
    job_id: id number, such as autotest_job_id or builder_id.
    package: CTS package name.
    uploader: upload function to upload to gs
  """
  xml_files = getXMLGZFiles(package, dir_entry)
  logging.info('Uploading CTS/GTS xml files: %s', xml_files)
  for xml in xml_files:
    timestamp = os.path.basename(os.path.dirname(xml))
    gs_url = os.path.join(constants.DEFAULT_CTS_RESULTS_GSURI, package,
                          build + '-' + job_id + '_' + timestamp)
    uploader(gs_url, xml, *args, **kwargs)

  # Results produced by CTS test list collector are dummy results.
  # They don't need to be copied to APFE bucket which is mainly being used for
  # CTS APFE submission.
  if not _is_test_collector(package):
    apfe_files = getApfeFiles(package, dir_entry)
    logging.info('Uploading CTS/GTS apfe files: %s', apfe_files)
    for apfe in apfe_files:
      timestamp = os.path.splitext(os.path.basename(apfe))[0]
      gs_url = os.path.join(constants.DEFAULT_CTS_APFE_GSURI, build,
                            apfe_id, package, job_id + "_" + timestamp)
      uploader(gs_url, apfe, *args, **kwargs)
  else:
    logging.debug('%s is a CTS Test collector Autotest test run.', package)
    logging.debug('Skipping CTS results upload to APFE gs:// bucket.')
