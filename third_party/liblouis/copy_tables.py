#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Copies the liblouis braille translation tables to a destination.'''

import liblouis_list_tables
import optparse
import os
import shutil


def LinkOrCopyFiles(sources, dest_dir):
  def LinkOrCopyOneFile(src, dst):
    if os.path.exists(dst):
      os.unlink(dst)
    try:
      os.link(src, dst)
    except:
      shutil.copy(src, dst)

  if not os.path.exists(dest_dir):
    os.makedirs(dest_dir)
  for source in sources:
    LinkOrCopyOneFile(source, os.path.join(dest_dir, os.path.basename(source)))


def WriteDepfile(depfile, infiles):
  stampfile = depfile + '.stamp'
  with open(stampfile, 'w'):
    os.utime(stampfile, None)
  content = '%s: %s' % (stampfile, ' '.join(infiles))
  open(depfile, 'w').write(content)



def main():
  parser = optparse.OptionParser(description=__doc__)
  parser.add_option('-D', '--directory', dest='directories',
                     action='append', help='Where to search for table files')
  parser.add_option('-e', '--extra_file', dest='extra_files', action='append',
                    default=[], help='Extra liblouis table file to process')
  parser.add_option('-d', '--dest_dir', action='store', metavar='DIR',
                    help=('Destination directory.  Used when translating ' +
                          'input paths to output paths and when copying '
                          'files.'))
  parser.add_option('--depfile', metavar='FILENAME',
                    help=('Store .d style dependencies in FILENAME and touch '
                          'FILENAME.stamp after copying the files'))
  options, args = parser.parse_args()

  if len(args) != 1:
    parser.error('Expecting exactly one argument')
  if not options.directories:
    parser.error('At least one --directory option must be specified')
  if not options.dest_dir:
    parser.error('At least one --dest_dir option must be specified')
  files = liblouis_list_tables.GetTableFiles(args[0], options.directories,
                                             options.extra_files)
  LinkOrCopyFiles(files, options.dest_dir)
  if options.depfile:
    WriteDepfile(options.depfile, files)


if __name__ == '__main__':
  main()
