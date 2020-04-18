# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Show the builder layout for CrOS waterfalls."""

from __future__ import print_function

import json
import sys

from chromite.lib import config_lib
from chromite.lib import commandline


def _FormatText(data, out):
  """Formatter function for text output."""
  output = lambda *a: print(*a, file=out)

  for waterfall in sorted(data.iterkeys()):
    layout = data[waterfall]
    if not layout:
      continue

    output('== %s ==' % (waterfall,))
    for board in sorted(layout.iterkeys()):
      board_layout = layout[board]
      children = board_layout.get('children', ())
      if not children:
        output('%(name)s (%(buildslave_type)s)' % board_layout)
      else:
        output('[%(name)s] (%(buildslave_type)s)' % board_layout)
      for child in sorted(board_layout.get('children', ())):
        output('  %s' % (child,))
    output()


def _FormatJson(data, out):
  """Formatter function for JSON output."""
  json.dump(data, out, sort_keys=True)


_FORMATTERS = {
    'text': _FormatText,
    'json': _FormatJson,
}


def _ParseArguments(argv):
  parser = commandline.ArgumentParser(description=__doc__)

  parser.add_argument('--format', default='text',
                      choices=sorted(_FORMATTERS.iterkeys()),
                      help='Choose output format.')
  opts = parser.parse_args(argv)
  opts.format = _FORMATTERS[opts.format]
  opts.Freeze()
  return opts


def main(argv):
  opts = _ParseArguments(argv)

  site_config = config_lib.GetConfig()

  layout = {}
  for config_name, config in site_config.iteritems():
    active_waterfall = config['active_waterfall']
    if not active_waterfall:
      continue

    waterfall_layout = layout.setdefault(active_waterfall, {})
    board_layout = waterfall_layout[config_name] = {
        'name': config_name,
        'buildslave_type': config['buildslave_type'],
    }

    children = config['child_configs']
    if children:
      board_layout['children'] = [c['name'] for c in children]
  opts.format(layout, sys.stdout)
