# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Functions in this file relies on depot_tools been checked-out as a sibling
# of infra.git.

import re


def parse_revinfo(revinfo):
  """Parse the output of "gclient revinfo -a"

  Args:
    revinfo (str): string containing gclient stdout.

  Returns:
    revinfo_d (dict): <directory>: (URL, revision)
  """
  revision_expr = re.compile('(.*)@([^@]*)')

  revinfo_d = {}
  for line in revinfo.splitlines():
    if ':' not in line:
      continue

    # TODO: this fails when the file name contains a colon.
    path, line = line.split(':', 1)
    if '@' in line:
      url, revision = revision_expr.match(line).groups()
      revision = revision.strip()
    else:
      # Split at the last @
      url, revision = line.strip(), None

    path = path.strip()
    url = url.strip()
    revinfo_d[path] = {'source_url': url, 'revision': revision}
  return revinfo_d
