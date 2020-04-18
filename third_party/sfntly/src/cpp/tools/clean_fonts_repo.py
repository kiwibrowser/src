#!/usr/bin/python
# Copyright 2011 Google Inc. All Rights Reserved.

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limit

"""Script to clean up the fonts repo.

Recursively removes all files and folders except for .ttf, .ttc, .otf, OFL.txt,
from the source folder.
"""

import os
import argparse
from utils import GetFontList
from utils import GetLevelList


def main():
  parser = argparse.ArgumentParser(description="""Remove all files from the
font folders except for *.ttf, *.ttc, *.otf, OFL.txt.""",
                                   formatter_class=
                                   argparse.ArgumentDefaultsHelpFormatter)
  parser.add_argument('--source',
                      default='../data/fonts',
                      help='root folder for the font files')
  args = parser.parse_args()

  # Removing all files that are not what we want
  paths = GetFontList(args.source, ['.ttf', '.ttc', '.otf', 'OFL.txt'], True)
  map (os.remove, paths)
  try:
    map(lambda path: os.removedirs(os.path.dirname(path)), paths)
  except OSError:
    pass
  # Removing all empty folders that remain
  paths = GetLevelList(args.source, 4, True)
  print paths
  map(os.remove, paths)
  try:
    map(lambda path: os.removedirs(os.path.dirname(path)), paths)
  except OSError:
    pass

if __name__ == '__main__':
  main()
