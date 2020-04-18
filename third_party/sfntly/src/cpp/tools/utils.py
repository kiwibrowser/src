# Copyright 2011 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limit

"""Common utility functions used by multiple scripts."""

import os


def GetFontList(path, exts, negate=False):
  """Recursively gets the list of files that from path such that."""
  # negate = False: files that match one of the extensions in exts.
  # negate = True: files that match no extension in exts.

  paths = []
  # for root, dirs, files in os.walk(path): makes the lint tool unhappy
  # because of dirs being unused :(
  for entry in os.walk(path):
    root = entry[0]
    files = entry[2]
    for path in files:
      has_ext_list = map(lambda ext: path[-len(ext):] == ext, exts)
      result = reduce(lambda a, h: a or h, has_ext_list, False)
      # normal: we want to include a file that matches at least one extension
      # negated: we want to include a file that matches no extension
      if negate != result:
        paths.append(os.path.join(root, path))
  return paths


def GetLevelList(path, max_level=1, negate=False):
  """Recursively gets the list of files that from path such that."""
  # negate = False: files that are at most |max_level|s deep.
  # negate = True: files that are more than |max_level|s deep.
  paths = []
  for entry in os.walk(path):
    root = entry[0]
    files = entry[2]
    for path in files:
      root_path = os.path.join(root, path)
      level = path.count(os.path.sep)
      if (not negate and level <= max_level) or (negate and level > max_level):
        paths.append(root_path)
  return paths


def FixPath(path):
  if path[-1] != '/':
    return path + '/'
  return path
