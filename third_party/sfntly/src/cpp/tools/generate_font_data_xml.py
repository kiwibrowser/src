#!/usr/bin/env python
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

"""Script to generate XML test data from fonts."""

import os
import argparse

from cmap_data_generator_xml import CMapDataGeneratorXML
from test_data_generator_xml import TestDataGeneratorXML
from utils import FixPath
from utils import GetFontList


def main():
  parser = argparse.ArgumentParser(description='Generates font test data.',
                                   formatter_class=
                                   argparse.ArgumentDefaultsHelpFormatter)
  parser.add_argument('--destination',
                      default=None,
                      help="""ouput folder for the generated files;
by default in the same location as the source file""")
  parser.add_argument('--source',
                      default='../data/fonts',
                      help='input folder for the font files')
  parser.add_argument('--num_mappings',
                      type=int,
                      default=10,
                      help='number of mappings per cmap')
  parser.add_argument('--name',
                      default='cmap_test_data',
                      help='base name of the generated test files')
  parser.add_argument('--fonts',
                      default='',
                      help="""space separated list of font files to use as
source data""")
  parser.add_argument('--font_dir',
                      default='.',
                      help="""base directory of the fonts in the generated
source""")
  parser.add_argument('--cmap_ids',
                      type=list,
                      default=[(3, 1), (1, 0)],
                      help="""(platform_id, encoding_id) pairs of cmaps to dump
(3, 1) corresponds to Windows BMP, a format 4 CMap and (1, 0) to Macintosh CMap,
either format 0 or 6""")
  args = parser.parse_args()

  if args.num_mappings <= 0:
    print 'Warning: num_mappings is <= 0; setting it to 1.'
    args.num_mappings = 1

  if args.destination is not None:
    try:
      os.stat(args.destination)
    except OSError:
      os.mkdir(args.destination)
    args.destination = FixPath(args.destination)

  args.source = FixPath(args.source)
  args.font_dir = FixPath(args.font_dir)
  if args.font_dir == '.':
    args.font_dir = ''

  if not args.fonts:
    args.fonts = GetFontList(args.source, ['.ttf', '.ttc', '.otf'])

  data_generator = TestDataGeneratorXML(
      [('cmap', CMapDataGeneratorXML(args.cmap_ids, args.num_mappings))],
      args.fonts,
      args.destination)

  data_generator.Generate()

if __name__ == '__main__':
  main()
