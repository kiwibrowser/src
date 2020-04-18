#!/usr/bin/env python
# Copyright 2017 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Extract specific state entry from Swarming bots"""

import json
import logging
import optparse
import os
import subprocess
import sys
import urllib


CLIENT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(
    __file__.decode(sys.getfilesystemencoding()))))


def _run_json(cmd):
  """Runs cmd and calls process with the decoded json."""
  logging.info('Running %s', ' '.join(cmd))
  raw = subprocess.check_output(cmd, cwd=CLIENT_DIR)
  logging.info('- returned %d bytes', len(raw))
  return json.loads(raw)


def fetch_bots(swarming, dimensions):
  data = [('dimensions', d) for d in dimensions]
  cmd = [
    sys.executable, os.path.join(CLIENT_DIR, 'swarming.py'),
    'query', '-S', swarming, '--limit', '0',
    'bots/list?' + urllib.urlencode(data),
  ]
  bots = _run_json(cmd)[u'items']
  for b in bots:
    b[u'state'] = json.loads(b[u'state'])
  return bots


def main():
  parser = optparse.OptionParser(description=sys.modules['__main__'].__doc__)
  parser.add_option(
      '-S', '--swarming',
      metavar='URL', default=os.environ.get('SWARMING_SERVER', ''),
      help='Swarming server to use')

  group = optparse.OptionGroup(parser, 'Filtering')
  group.add_option(
      '-d', '--dimension', metavar='KEY:VALUE', dest='dimensions',
      action='append', default=[],
      help='Dimensions to filter on')
  group.add_option('-k', '--key', help='State key to print out')
  parser.add_option_group(group)

  parser.add_option(
      '-v', '--verbose', action='count', default=0, help='Log')
  options, args = parser.parse_args()
  logging.basicConfig(level=logging.DEBUG if options.verbose else logging.ERROR)
  if args:
    parser.error('Unsupported argument %s' % args)
  if not options.swarming:
    parser.error('--swarming is required')

  bots = fetch_bots(options.swarming, options.dimensions)
  if options.key:
    out = {
      bot[u'bot_id']: bot[u'state'].get(options.key, None)
      for bot in bots
    }
  else:
    out = {bot[u'bot_id']: bot[u'state'] for bot in bots}
  json.dump(out, sys.stdout, sort_keys=True, indent=2)
  return 0


if __name__ == '__main__':
  sys.exit(main())
