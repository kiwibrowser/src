# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import logging
import os

from py_utils import cloud_storage

from telemetry.internal.results import output_formatter

from tracing_build import vulcanize_histograms_viewer


class HtmlOutputFormatter(output_formatter.OutputFormatter):
  def __init__(self, output_stream, metadata, reset_results,
               upload_bucket=None):
    super(HtmlOutputFormatter, self).__init__(output_stream)
    self._metadata = metadata
    self._upload_bucket = upload_bucket
    self._reset_results = reset_results

  def Format(self, page_test_results):
    histograms = page_test_results.AsHistogramDicts()

    vulcanize_histograms_viewer.VulcanizeAndRenderHistogramsViewer(
        histograms, self._output_stream, self._reset_results)
    if self._upload_bucket:
      file_path = os.path.abspath(self._output_stream.name)
      remote_path = ('html-results/results-%s' %
                     datetime.datetime.now().strftime('%Y-%m-%d_%H-%M-%S'))
      try:
        url = cloud_storage.Insert(self._upload_bucket, remote_path, file_path)
        print 'View HTML results online at %s' % url
      except cloud_storage.PermissionError as e:
        logging.error('Cannot upload files to cloud storage due to '
                      ' permission error: %s' % e.message)
