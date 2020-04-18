# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import csv
import logging
import os


def CollectCSVsFromDirectory(directory_path, file_output):
  """Collects recursively all .csv files from directory into one.

  Note: The list of CSV columns must be identical across all files.

  Args:
    directory_path: Path of the directory to collect from.
    file_output: File-like object to dump the CSV to.
  """
  # List CSVs.
  csv_list = []
  for root, _, files in os.walk(directory_path):
    for file_name in files:
      file_path = os.path.join(root, file_name)
      if os.path.abspath(file_path) == os.path.abspath(file_output.name):
        continue
      if file_name.endswith('.csv'):
        csv_list.append(os.path.join(root, file_name))
  if not csv_list:
    logging.error('No CSV files found in %s' % directory_path)
    return False

  # List rows.
  csv_list.sort()
  csv_field_names = None
  csv_rows = []
  for csv_file in csv_list:
    logging.info('collecting %s' % csv_file)
    with open(csv_file) as csvfile:
      reader = csv.DictReader(csvfile)
      if csv_field_names is None:
        csv_field_names = reader.fieldnames
      else:
        assert reader.fieldnames == csv_field_names, (
            'Different field names in: {}'.format(csv_file))
      for row in reader:
        csv_rows.append(row)

  # Export rows.
  writer = csv.DictWriter(file_output, fieldnames=csv_field_names)
  writer.writeheader()
  for row in csv_rows:
    writer.writerow(row)
  return True
