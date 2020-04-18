#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Wraps gclient calls with annotated output.

Note that you will have to use -- to stop option parsing for gclient flags.

To run `gclient sync --gclientfile=.gclient` and annotate got_v8_revision:
  `annotated_gclient.py --revision-mapping='{"src/v8": "got_v8_revision"}' --
      sync --gclientfile=.gclient`
"""

import contextlib
import json
import optparse
import os
import subprocess
import sys
import tempfile


@contextlib.contextmanager
def temp_filename(suffix='', prefix='tmp'):
  output_fd, output_file = tempfile.mkstemp(suffix=suffix, prefix=prefix)
  os.close(output_fd)

  yield output_file

  try:
    os.remove(output_file)
  except OSError as e:
    print 'Error cleaning up temp file %s: %s' % (output_file, e)


def parse_got_revision(filename, revision_mapping):
  result = {}
  with open(filename) as f:
    data = json.load(f)

  for path, info in data['solutions'].iteritems():
    # gclient json paths always end with a slash
    path = path.rstrip('/')
    if path in revision_mapping:
      propname = revision_mapping[path]
      result[propname] = info['revision']

  return result


def emit_buildprops(got_revisions):
  for prop, revision in got_revisions.iteritems():
    print '@@@SET_BUILD_PROPERTY@%s@%s@@@' % (prop, json.dumps(revision))


def main():
  parser = optparse.OptionParser(
      description=('Runs gclient and annotates the output with any '
                   'got_revisions.'))
  parser.add_option('--revision-mapping', default='{}',
                    help='json dict of directory-to-property mappings.')
  parser.add_option('--suffix', default='gclient',
                    help='tempfile suffix')
  opts, args = parser.parse_args()

  revision_mapping = json.loads(opts.revision_mapping)

  if not args:
    parser.error('Must provide arguments to gclient.')

  if any(a.startswith('--output-json') for a in args):
    parser.error('Can\'t call annotated_gclient with --output-json.')

  with temp_filename(opts.suffix) as f:
    cmd = ['gclient']
    cmd.extend(args)
    cmd.extend(['--output-json', f])
    p = subprocess.Popen(cmd)
    p.wait()

    if p.returncode == 0:
      revisions = parse_got_revision(f, revision_mapping)
      emit_buildprops(revisions)
    return p.returncode


if __name__ == '__main__':
  sys.exit(main())
