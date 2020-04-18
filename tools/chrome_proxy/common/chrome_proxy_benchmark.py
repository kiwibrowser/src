# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common import chrome_proxy_measurements as measurements
from telemetry import benchmark


class ChromeProxyBenchmark(benchmark.Benchmark):
  @classmethod
  def AddCommandLineArgs(cls, parser):
    parser.add_option(
        '--extra-chrome-proxy-via-header',
        type='string', dest="extra_header",
        help='Adds an expected Via header for the Chrome-Proxy tests.')

  @classmethod
  def ProcessCommandLineArgs(cls, parser, args):
    if args.extra_header:
      measurements.ChromeProxyValidation.extra_via_header = args.extra_header

