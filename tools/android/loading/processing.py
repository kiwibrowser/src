# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import os
import os.path
import sys

import loading_model
import loading_trace


def SitesFromDir(directory):
  """Extract sites from a data directory.

  Based on ./analyze.py fetch file name conventions. We assume each site
  corresponds to two files, <site>.json and <site>.json.cold, and that no other
  kind of file appears in the data directory.

  Args:
    directory: the directory to process.

  Returns:
    A list of sites as strings.

  """
  files = set(os.listdir(directory))
  assert files
  sites = []
  for f in files:
    if f.endswith('.png'):
      continue
    assert f.endswith('.json') or f.endswith('.json.cold'), f
    if f.endswith('.json'):
      assert f + '.cold' in files
      sites.append(f[:f.rfind('.json')])
    elif f.endswith('.cold'):
      assert f[:f.rfind('.cold')] in files
  sites.sort()
  return sites


def WarmGraph(datadir, site):
  """Return a loading model graph for the warm pull of site.

  Based on ./analyze.py fetch file name conventions.

  Args:
    datadir: the directory containing site JSON data.
    site: a site string.

  Returns:
    A loading model object.
  """
  with file(os.path.join(datadir, site + '.json')) as f:
    return loading_model.ResourceGraph(loading_trace.LoadingTrace.FromJsonDict(
        json.load(f)))


def ColdGraph(datadir, site):
  """Return a loading model graph for the cold pull of site.

  Based on ./analyze.py fetch file name conventions.

  Args:
    datadir: the directory containing site JSON data.
    site: a site string.

  Returns:
    A loading model object.
  """
  with file(os.path.join(datadir, site + '.json.cold')) as f:
    return loading_model.ResourceGraph(loading_trace.LoadingTrace.FromJsonDict(
        json.load(f)))
