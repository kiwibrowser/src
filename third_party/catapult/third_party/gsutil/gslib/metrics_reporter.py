# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Script for reporting metrics."""

import logging
import os
import pickle
import random
import string
import sys

# pylint:disable=g-import-not-at-top
try:
  from gslib.util import GetNewHttp
  from gslib.util import ConfigureCertsFile
except:  # pylint: disable=bare-except
  # Some environments import their own version of standard Python libraries
  # which might cause the import of gslib.util to fail.  Try this alternative
  # import in such cases.
  try:
    # Fall back to httplib (no proxy) if we can't import libraries normally.
    import httplib

    def GetNewHttp():
      """Returns an httplib-based metrics reporter."""

      class HttplibReporter(object):

        def __init__(self):
          pass

        # pylint: disable=invalid-name
        def request(self, endpoint, method=None, body=None,
                    headers=None):
          # Strip 'https://'
          https_con = httplib.HTTPSConnection(endpoint[8:].split('/')[0])
          https_con.request(method, endpoint, body=body,
                            headers=headers)
          response = https_con.getresponse()
          # Return status like an httplib2 response.
          return ({'status': response.status},)
        # pylint: enable=invalid-name

      return HttplibReporter()

    def ConfigureCertsFile():
      pass
  except:
    sys.exit(0)

LOG_FILE_PATH = os.path.expanduser(os.path.join('~', '.gsutil/metrics.log'))


def ReportMetrics(metrics_file_path, log_level, log_file_path=None):
  """Sends the specified anonymous usage event to the given analytics endpoint.

  Args:
      metrics_file_path: str, File with pickled metrics (list of tuples).
      log_level: int, The logging level of gsutil's root logger.
      log_file_path: str, The file that this module should write its logs to.
        This parameter is intended for use by tests that need to evaluate the
        contents of the file at this path.

  """
  logger = logging.getLogger()
  if log_file_path is not None:
    # Use a separate logger so that we don't add another handler to the default
    # module-level logger. This is intended to prevent multiple calls from tests
    # running in parallel from writing output to the same file.
    new_name = '%s.%s' % (
        logger.name,
        ''.join(random.choice(string.ascii_lowercase) for _ in range(8)))
    logger = logging.getLogger(new_name)

  handler = logging.FileHandler(log_file_path or LOG_FILE_PATH, mode='w')
  logger.addHandler(handler)
  logger.setLevel(log_level)

  with open(metrics_file_path, 'rb') as metrics_file:
    metrics = pickle.load(metrics_file)
  os.remove(metrics_file_path)

  ConfigureCertsFile()
  http = GetNewHttp()

  for metric in metrics:
    try:
      headers = {'User-Agent': metric.user_agent}
      response = http.request(metric.endpoint,
                              method=metric.method,
                              body=metric.body,
                              headers=headers)
      logger.debug(metric)
      logger.debug('RESPONSE: %s', response[0]['status'])
    except Exception as e:  # pylint: disable=broad-except
      logger.debug(e)
