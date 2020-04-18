# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.util import command_line


_DEPRECATED_MESSAGE = ('\nERROR: This command has been removed.'
  '\n\nPlease visit https://chromium.googlesource.com/chromium/'
  'src/+/master/docs/speed/perf_trybots.md for up-to-date information on '
  'running perf try jobs.\n\n')


class Trybot(command_line.ArgParseCommand):
  """Run telemetry perf benchmark on trybot."""

  usage = 'botname benchmark_name [<benchmark run options>]'

  def __init__(self):
    pass

  def Run(self, options, extra_args=None):
    print _DEPRECATED_MESSAGE

    return 0
