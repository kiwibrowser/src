# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import csv
import json
import os
import tempfile

from telemetry.internal.results import output_formatter
from tracing.value import histograms_to_csv


def _ReadCsv(text):
  dicts = []
  header = None
  for row in csv.reader(text.split('\n')):
    if header is None:
      header = row
    elif row:
      dicts.append(collections.OrderedDict(zip(header, row)))
  return dicts


def _WriteCsv(dicts, fileobj):
  header = []
  for d in dicts:
    for k in d.iterkeys():
      if k not in header:
        header.append(k)
  rows = [header]
  for d in dicts:
    rows.append([d.get(k, '') for k in header])
  csv.writer(fileobj).writerows(rows)


class CsvOutputFormatter(output_formatter.OutputFormatter):
  def __init__(self, output_stream, reset_results=False):
    super(CsvOutputFormatter, self).__init__(output_stream)
    self._reset_results = reset_results

  def Format(self, page_test_results):
    histograms = page_test_results.AsHistogramDicts()
    file_descriptor, json_path = tempfile.mkstemp()
    os.close(file_descriptor)
    json.dump(histograms, file(json_path, 'w'))
    vinn_result = histograms_to_csv.HistogramsToCsv(json_path)
    dicts = _ReadCsv(vinn_result.stdout)

    self._output_stream.seek(0)
    if not self._reset_results:
      dicts += _ReadCsv(self._output_stream.read())
      self._output_stream.seek(0)
    _WriteCsv(dicts, self._output_stream)
    self._output_stream.truncate()
