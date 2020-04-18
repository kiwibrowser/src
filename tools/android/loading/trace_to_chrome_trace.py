#! /usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Convert trace output for Chrome.

Takes a loading trace from 'analyze.py log_requests' and outputs a json file
that can be loaded by chrome's about:tracing..
"""

import argparse
import json

if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument('input')
  parser.add_argument('output')
  args = parser.parse_args()
  with file(args.output, 'w') as output_f, file(args.input) as input_f:
    events = json.load(input_f)['tracing_track']['events']
    json.dump({'traceEvents': events, 'metadata': {}}, output_f)
