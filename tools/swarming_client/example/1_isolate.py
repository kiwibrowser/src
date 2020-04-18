#!/usr/bin/env python
# coding=utf-8
# Copyright 2012 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Runs hello_🌐.py, through hello_🌐.isolated, locally in a temporary
directory.

The files are archived and fetched from the remote Isolate Server.
"""

import hashlib
import os
import shutil
import subprocess
import sys
import tempfile

# Pylint can't find common.py that's in the same directory as this file.
# pylint: disable=F0401
import common


def main():
  args = common.parse_args(use_isolate_server=True, use_swarming=False)
  tempdir = unicode(tempfile.mkdtemp(prefix=u'hello_world'))
  try:
    isolated_sha1 = common.archive(
        tempdir, args.isolate_server, args.verbose, args.which)

    common.note(
        'Downloading from %s and running in a temporary directory' %
        args.isolate_server)
    cachei = os.path.join(tempdir, u'cachei')
    cachen = os.path.join(tempdir, u'cachen')
    common.run(
        [
          'run_isolated.py',
          '--cache', cachei.encode('utf-8'),
          '--named-cache-root', cachen.encode('utf-8'),
          '--isolate-server', args.isolate_server,
          '--isolated', isolated_sha1,
          '--no-log',
          '--', args.which + u'.py', 'Dear 💩', '${ISOLATED_OUTDIR}',
        ], args.verbose)
    return 0
  except subprocess.CalledProcessError as e:
    return e.returncode
  finally:
    shutil.rmtree(tempdir)


if __name__ == '__main__':
  sys.exit(main())
