# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helper script to classify the results of  a special task."""

from __future__ import print_function

import json
import os

from chromite.lib import classifier
from chromite.lib import commandline
from chromite.lib import cros_logging as logging
from chromite.lib import gs

def GetParser():
  """Creates the argparse parser."""
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('urls', type=str, nargs='*', action='store',
                      help='GS URL to special task logs')
  parser.add_argument('--input_json', '-i', type=str, action='store',
                      help='JSON file to use as input.')
  parser.add_argument('--key_path', '-k', type=str, action='append',
                      help='Key path to URLs, specifying all dict keys. '
                           'All lists are iterated, key path must end in '
                           'string of URL.')
  parser.add_argument('--find_suite_tasks', action='store_true', default=False,
                      help='Set key path to match find_suite_tasks output. '
                           'ie (special_tasks, next_entry, gs_url)')
  return parser


def ClassifySpecialTask(base_url):
  """Classify a single special task.

  Args:
    base_url: The GS URL to the special task directory.
  """
  ctx = gs.GSContext()
  STATUS_LOG = 'status.log'
  glob_url = os.path.join(base_url, '*', STATUS_LOG)
  try:
    files = ctx.LS(glob_url)
    content = ctx.Cat(files[0])
    result = classifier.ClassifyLabJobStatusResult(content.splitlines())
    logging.info('%s: %s', base_url, result)
  except gs.GSNoSuchKey:
    logging.warning('%s: %s not found', base_url, STATUS_LOG)


def TraverseTree(root, path, func):
  """Traverse a JSON tree.

  Allows traversing of the tree root, by iterating any lists encountered,
  or following dictionaries according to the keys specified by path.
  Leaf nodes execute a specified function.

  See unittest for examples of traversal.

  Args:
    root: The list/dict/str to traverse.
    path: A list of dictionary keys to traverse when encountered.
    func: A function to execute with leaf nodes.
  """
  if isinstance(root, list):
    for n in root:
      TraverseTree(n, path, func)
  elif path and len(path):
    if not isinstance(root, dict):
      logging.error('Unable to find key %s because not a dict, got %s',
                    path, type(root))
    elif path[0] in root:
      TraverseTree(root[path[0]], path[1:], func)
  else:
    if not isinstance(root, basestring):
      logging.error('Expecting string to classify, got: %s', type(root))
    else:
      func(root)


def main(argv):
  parser = GetParser()
  options = parser.parse_args(argv)

  key_path = options.key_path
  if options.find_suite_tasks:
    key_path = ['special_tasks', 'next_entry', 'gs_url']

  if options.input_json:
    with open(options.input_json) as f:
      json_file = json.load(f)
      TraverseTree(json_file, key_path, ClassifySpecialTask)

  for base_url in options.urls:
    ClassifySpecialTask(base_url)
