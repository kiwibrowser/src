#!/usr/bin/python
# Copyright 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob
import os
import sys
from collections import namedtuple
from optparse import OptionParser

Config = namedtuple('Config', 'linkfiles linkfileglobs ' +
                              'rmfiles rmfileglobs no_rmfileglobs')

configs = {
  ('164.gzip', 'train'):
    Config(linkfiles=[],
           linkfileglobs=[],
           rmfiles=[],
           rmfileglobs=['*.out'],
           no_rmfileglobs=[]),
  ('164.gzip', 'ref'):
    Config(linkfiles=[],
           linkfileglobs=[],
           rmfiles=[],
           rmfileglobs=['*.out'],
           no_rmfileglobs=[]),
  ('175.vpr', 'train'):
    Config(linkfiles=[],
           linkfileglobs=['data/train/input/*'],
           rmfiles=[],
           rmfileglobs=['*.out', '*.in'],
           no_rmfileglobs=[]),
  ('175.vpr', 'ref'):
    Config(linkfiles=[],
           linkfileglobs=['data/ref/input/*'],
           rmfiles=[],
           rmfileglobs=['*.out', '*.in'],
           no_rmfileglobs=[]),
  ('176.gcc', 'train'):
    Config(linkfiles=[],
           linkfileglobs=[],
           rmfiles=[],
           rmfileglobs=['*.out', '*.s'],
           no_rmfileglobs=['*.sz.s']),
  ('176.gcc', 'ref'):
    Config(linkfiles=[],
           linkfileglobs=[],
           rmfiles=[],
           rmfileglobs=['*.out', '*.s'],
           no_rmfileglobs=['*.sz.s']),
  ('177.mesa', 'train'):
    Config(linkfiles=[],
           linkfileglobs=['data/train/input/*'],
           rmfiles=['mesa.log', 'mesa.ppm', 'mesa.in', 'numbers'],
           rmfileglobs=['*.out'],
           no_rmfileglobs=[]),
  ('177.mesa', 'ref'):
    Config(linkfiles=[],
           linkfileglobs=['data/ref/input/*'],
           rmfiles=['mesa.log', 'mesa.ppm', 'mesa.in', 'numbers'],
           rmfileglobs=['*.out'],
           no_rmfileglobs=[]),
  ('179.art', 'train'):
    Config(linkfiles=[],
           linkfileglobs=['data/train/input/*'],
           rmfiles=['a10.img', 'c756hel.in', 'hc.img'],
           rmfileglobs=['*.out', '*.err'],
           no_rmfileglobs=[]),
  ('179.art', 'ref'):
    Config(linkfiles=[],
           linkfileglobs=['data/ref/input/*'],
           rmfiles=['a10.img', 'c756hel.in', 'hc.img'],
           rmfileglobs=['*.out', '*.err'],
           no_rmfileglobs=[]),
  ('181.mcf', 'train'):
    Config(linkfiles=[],
           linkfileglobs=[],
           rmfiles=[],
           rmfileglobs=['*.out'],
           no_rmfileglobs=[]),
  ('181.mcf', 'ref'):
    Config(linkfiles=[],
           linkfileglobs=[],
           rmfiles=[],
           rmfileglobs=['*.out'],
           no_rmfileglobs=[]),
  ('183.equake', 'train'):
    Config(linkfiles=[],
           linkfileglobs=['data/train/input/*'],
           rmfiles=['inp.in', 'inp.out'],
           rmfileglobs=['*.out'],
           no_rmfileglobs=[]),
  ('183.equake', 'ref'):
    Config(linkfiles=[],
           linkfileglobs=['data/ref/input/*'],
           rmfiles=['inp.in', 'inp.out'],
           rmfileglobs=['*.out'],
           no_rmfileglobs=[]),
  ('186.crafty', 'train'):
    Config(linkfiles=[],
           linkfileglobs=[],
           rmfiles=[],
           rmfileglobs=['*.out'],
           no_rmfileglobs=[]),
  ('186.crafty', 'ref'):
    Config(linkfiles=[],
           linkfileglobs=[],
           rmfiles=[],
           rmfileglobs=['*.out'],
           no_rmfileglobs=[]),
  ('188.ammp', 'train'):
    Config(linkfiles=[],
           linkfileglobs=['data/train/input/*'],
           rmfiles=['all.new.ammp', 'ammp.in', 'new.tether', 'ammp.out'],
           rmfileglobs=['*.out'],
           no_rmfileglobs=[]),
  ('188.ammp', 'ref'):
    Config(linkfiles=[],
           linkfileglobs=['data/ref/input/*'],
           rmfiles=['all.init.ammp', 'ammp.in', 'ammp.out'],
           rmfileglobs=['*.out', 'init_cond.run.[123]'],
           no_rmfileglobs=[]),
  ('197.parser', 'train'):
    Config(linkfiles=['data/all/input/words', 'data/all/input/2.1.dict'],
           linkfileglobs=[],
           rmfiles=['words', '2.1.dict'],
           rmfileglobs=['*.out'],
           no_rmfileglobs=[]),
  ('197.parser', 'ref'):
    Config(linkfiles=['data/all/input/words', 'data/all/input/2.1.dict'],
           linkfileglobs=[],
           rmfiles=['words', '2.1.dict'],
           rmfileglobs=['*.out'],
           no_rmfileglobs=[]),
  ('252.eon', 'train'):
    Config(linkfiles=[],
           linkfileglobs=['data/train/input/*'],
           rmfiles=['materials', 'spectra.dat', 'eon.dat'],
           rmfileglobs=['*.out', 'chair.*', 'pixel_*'],
           no_rmfileglobs=[]),
  ('252.eon', 'ref'):
    Config(linkfiles=[],
           linkfileglobs=['data/ref/input/*'],
           rmfiles=['materials', 'spectra.dat', 'eon.dat'],
           rmfileglobs=['*.out', 'chair.*', 'pixel_*'],
           no_rmfileglobs=[]),
  ('253.perlbmk', 'train'):
    Config(linkfiles=['data/all/input/lib', 'data/train/input/dictionary'],
           linkfileglobs=['data/all/input/[bl]enums'],
           rmfiles=['lib', 'dictionary'],
           rmfileglobs=['*.out', '*.rc', '*enums'],
           no_rmfileglobs=[]),
  ('253.perlbmk', 'ref'):
    Config(linkfiles=['data/all/input/lib',
                      'data/all/input/cpu2000_mhonarc.rc'],
           linkfileglobs=['data/all/input/[bl]enums'],
           rmfiles=['lib', 'dictionary'],
           rmfileglobs=['*.out', '*.rc', '*enums'],
           no_rmfileglobs=[]),
  ('254.gap', 'train'):
    Config(linkfiles=[],
           linkfileglobs=[],
           rmfiles=[],
           rmfileglobs=['*.out'],
           no_rmfileglobs=[]),
  ('254.gap', 'ref'):
    Config(linkfiles=[],
           linkfileglobs=[],
           rmfiles=[],
           rmfileglobs=['*.out'],
           no_rmfileglobs=[]),
  ('255.vortex', 'train'):
    Config(linkfiles=[],
           linkfileglobs=['data/train/input/*'],
           rmfiles=[],
           rmfileglobs=['*.out'],
           no_rmfileglobs=[]),
  ('255.vortex', 'ref'):
    Config(linkfiles=[],
           linkfileglobs=['data/ref/input/*'],
           rmfiles=[],
           rmfileglobs=['*.out', '*endian*', 'persons.*'],
           no_rmfileglobs=[]),
  ('256.bzip2', 'train'):
    Config(linkfiles=[],
           linkfileglobs=[],
           rmfiles=[],
           rmfileglobs=['*.out', '*endian*', 'persons.*'],
           no_rmfileglobs=[]),
  ('256.bzip2', 'ref'):
    Config(linkfiles=[],
           linkfileglobs=[],
           rmfiles=[],
           rmfileglobs=['*.out'],
           no_rmfileglobs=[]),
  ('300.twolf', 'train'):
    Config(linkfiles=[],
           linkfileglobs=['data/train/input/*'],
           rmfiles=[],
           rmfileglobs=['*.out', 'train.*', 'ref.*', 'test.*'],
           no_rmfileglobs=[]),
  ('300.twolf', 'ref'):
    Config(linkfiles=[],
           linkfileglobs=['data/ref/input/*'],
           rmfiles=[],
           rmfileglobs=['*.out', 'train.*', 'ref.*', 'test.*'],
           no_rmfileglobs=[]),
}

def PrepareInput():
  """Prepares the current directory by removing old input/output files
  and symlinking input files.
  """
  parser = OptionParser()
  parser.add_option('--linkfile', action='append', dest='linkfiles',
                    default=[], help='Symlink a specific file')
  parser.add_option('--link', action='append', dest='linkfileglobs',
                    default=[], help='Symlink a pattern of files')
  parser.add_option('--rmfile', action='append', dest='rmfiles',
                    default=[], help='Remove a specific file')
  parser.add_option('--rm', action='append', dest='rmfileglobs',
                    default=[], help='Remove a pattern of files')
  parser.add_option('--no-rm', action='append', dest='no_rmfileglobs',
                    default=[], help='Exclude a pattern from --rm set')
  parser.add_option('--config', action='store', dest='config', nargs=2,
                    metavar='SpecDir train|ref',
                    help='Use predefined config for component SpecDir')
  parser.add_option('-v', '--verbose', action='store_true', dest='verbose',
                    default=False, help='Verbose file messages')
  parser.add_option('--gen-emcc-embed', action='store_true',
                    dest='gen_emcc_embed', default=False,
                    help='Generate --embed-file options for input files for ' +
                    'Wasm test')
  options, args = parser.parse_args()
  if args:
    parser.print_help()
    return 1
  linkfiles = options.linkfiles
  linkfileglobs = options.linkfileglobs
  rmfiles = options.rmfiles
  no_rmfiles = []
  rmfileglobs = options.rmfileglobs
  no_rmfileglobs = options.no_rmfileglobs
  if options.config in configs:
    if options.verbose:
      print 'Using config', options.config,
      print ' ==>', configs[options.config]
    linkfiles.extend(configs[options.config].linkfiles)
    linkfileglobs.extend(configs[options.config].linkfileglobs)
    rmfiles.extend(configs[options.config].rmfiles)
    rmfileglobs.extend(configs[options.config].rmfileglobs)
    no_rmfileglobs.extend(configs[options.config].no_rmfileglobs)
  for pattern in options.linkfileglobs:
    linkfiles.extend(glob.glob(pattern))
  for pattern in options.rmfileglobs:
    rmfiles.extend(glob.glob(pattern))
  for pattern in options.no_rmfileglobs:
    no_rmfiles.extend(glob.glob(pattern))

  # Wasm needs to embed input files into a js file
  if options.gen_emcc_embed:
    # Some benchmarks directly access input files in data/[config]/input
    # directory, while others create symlinks into the current directory.
    # When symlinks are created (=linkfileglobs is NOT empty), we embed those
    # files assuming they are in the current directory.
    # When symlinks are NOT created, we assume files in data/ will be accessed
    # directly, and embed those files in the subdirectories.
    input_dir = 'data/' + options.config[1] + '/input/*'
    opts = ''
    linkfiles_in_input_dir = []
    if input_dir not in configs[options.config].linkfileglobs:
      linkfiles_in_input_dir.extend(glob.glob(input_dir))
    for file in linkfiles_in_input_dir:
      opts += '--embed-file %s ' % file
    for file in linkfiles:
      opts += '--embed-file %s@%s ' % (file, os.path.basename(file))
    sys.stdout.write(opts)
    return 0

  for file in set(rmfiles) - set(no_rmfiles):
    try:
      if options.verbose:
        print 'Unlink', file
      os.unlink(file)
    except OSError:
      if options.verbose:
        print "Warning: couldn't remove", file
  for file in linkfiles:
    try:
      if options.verbose:
        print 'Symlink', file
      os.symlink(file, os.path.basename(file))
    except OSError:
      if options.verbose:
        print "Warning: couldn't symlink", file
  return 0

if __name__ == '__main__':
  sys.exit(PrepareInput())
