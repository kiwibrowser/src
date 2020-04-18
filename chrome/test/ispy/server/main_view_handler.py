# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Request handler to serve the main_view page."""

import jinja2
import json
import os
import re
import sys
import webapp2

import ispy_api
from common import constants
from common import ispy_utils

import gs_bucket
import views

JINJA = jinja2.Environment(
    loader=jinja2.FileSystemLoader(os.path.dirname(views.__file__)),
    extensions=['jinja2.ext.autoescape'])


class MainViewHandler(webapp2.RequestHandler):
  """Request handler to serve the main_view page."""

  def get(self):
    """Handles a get request to the main_view page.

    If the test_run parameter is specified, then a page displaying all of
    the failed runs in the test_run will be shown. Otherwise a view listing
    all of the test_runs available for viewing will be displayed.
    """
    test_run = self.request.get('test_run')
    bucket = gs_bucket.GoogleCloudStorageBucket(constants.BUCKET)
    ispy = ispy_utils.ISpyUtils(bucket)
    # Load the view.
    if test_run:
      self._GetForTestRun(test_run, ispy)
      return
    self._GetAllTestRuns(ispy)

  def _GetAllTestRuns(self, ispy):
    """Renders a list view of all of the test_runs available in GS.

    Args:
      ispy: An instance of ispy_api.ISpyApi.
    """
    template = JINJA.get_template('list_view.html')
    data = {}
    max_keys = 1000
    marker = 'failures/%s' % self.request.get('marker')
    test_runs = list([path.split('/')[1] for path in
       ispy.GetAllPaths('failures/', max_keys=max_keys,
                        marker=marker, delimiter='/')])
    base_url = '/?test_run=%s'
    next_url = '/?marker=%s' % test_runs[-1]
    data['next_url'] = next_url
    data['links'] = [(test_run, base_url % test_run) for test_run in test_runs]
    self.response.write(template.render(data))

  def _GetForTestRun(self, test_run, ispy):
    """Renders a sorted list of failure-rows for a given test_run.

    This method will produce a list of failure-rows that are sorted
    in descending order by number of different pixels.

    Args:
      test_run: The name of the test_run to render failure rows from.
      ispy: An instance of ispy_api.ISpyApi.
    """
    paths = set([path for path in ispy.GetAllPaths('failures/' + test_run)
                 if path.endswith('actual.png')])
    can_rebaseline = ispy_api.ISpyApi(
        ispy.cloud_bucket).CanRebaselineToTestRun(test_run)
    rows = [self._CreateRow(test_run, path, ispy) for path in paths]

    # Function that sorts by the different_pixels field in the failure-info.
    def _Sorter(a, b):
      return cmp(b['percent_different'],
                 a['percent_different'])
    template = JINJA.get_template('main_view.html')
    self.response.write(
        template.render({'comparisons': sorted(rows, _Sorter),
                         'test_run': test_run,
                         'can_rebaseline': can_rebaseline}))

  def _CreateRow(self, test_run, path, ispy):
    """Creates one failure-row.

    This method builds a dictionary with the data necessary to display a
    failure in the main_view html template.

    Args:
      test_run: The name of the test_run the failure is in.
      path: A path to the failure's actual.png file.
      ispy: An instance of ispy_api.ISpyApi.

    Returns:
      A dictionary with fields necessary to render a failure-row
        in the main_view html template.
    """
    res = {}
    res['expectation'] = path.lstrip('/').split('/')[2]
    res['test_run'] = test_run
    res['info'] = json.loads(ispy.cloud_bucket.DownloadFile(
        ispy_utils.GetFailurePath(res['test_run'], res['expectation'],
                                  'info.txt')))
    expected = ispy_utils.GetExpectationPath(
        res['expectation'], 'expected.png')
    diff = ispy_utils.GetFailurePath(test_run, res['expectation'], 'diff.png')
    res['percent_different'] = res['info']['fraction_different'] * 100
    res['expected_path'] = expected
    res['diff_path'] = diff
    res['actual_path'] = path
    res['expected'] = ispy.cloud_bucket.GetImageURL(expected)
    res['diff'] = ispy.cloud_bucket.GetImageURL(diff)
    res['actual'] = ispy.cloud_bucket.GetImageURL(path)
    return res
