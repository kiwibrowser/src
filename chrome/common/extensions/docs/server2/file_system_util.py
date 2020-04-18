# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import posixpath

def CreateURLsFromPaths(file_system, directory, urlprefix):
  '''Yields a tuple (url, prefix) for every file in |directory|, where the URL
  is given relative to |urlprefix|.
  '''
  for root, _, files in file_system.Walk(directory):
    for f in files:
      url = posixpath.join(urlprefix, root, f)
      yield url, posixpath.join(directory, root, f)
