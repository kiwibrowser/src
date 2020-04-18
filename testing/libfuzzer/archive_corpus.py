#!/usr/bin/python2
#
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Archive corpus file into zip and generate .d depfile.

Invoked by GN from fuzzer_test.gni.
"""

from __future__ import print_function
import argparse
import os
import sys
import warnings
import zipfile


def main():
  parser = argparse.ArgumentParser(description="Generate fuzzer config.")
  parser.add_argument('corpus_directories', metavar='corpus_dir', type=str,
                      nargs='+')
  parser.add_argument('--output', metavar='output_archive_name.zip',
                      required=True)
  args = parser.parse_args()

  corpus_files = []

  for directory in args.corpus_directories:
    if not os.path.exists(directory):
      raise Exception('The given seed_corpus directory (%s) does not exist.' %
                      directory)
    for (dirpath, _, filenames) in os.walk(directory):
      for filename in filenames:
        full_filename = os.path.join(dirpath, filename)
        corpus_files.append(full_filename)

  with zipfile.ZipFile(args.output, 'w') as z:
    # Turn warnings into errors to interrupt the build: crbug.com/653920.
    with warnings.catch_warnings():
      warnings.simplefilter("error")
      for i, corpus_file in enumerate(corpus_files):
        # To avoid duplication of filenames inside the archive, use numbers.
        arcname = '%016d' % i
        z.write(corpus_file, arcname)


if __name__ == '__main__':
  main()
