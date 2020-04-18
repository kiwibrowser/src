#!/usr/bin/python
# Copyright (c) 2010 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Diff - Generate a unified diff of two sources or directories.

 The diff module provides a mechanism for running a diff a pair of
 sources or directories.  If there are any differences they are
 displayed as a unified diff and the module returns non zero.  If
 there are no differences the module returns zero.

 diff.py <opts> <dir1/file1> <dir2/file2>

 diff will load one source file, or multiple files if the sources are
 directories.  Only the top directory is used unless the recursive '-r'
 option is used.  Normally diff will quitely return zero if the files
 are the same.  Verbose '-v' will list all the files compared as well
 and the status of the diff.

 -a, --all : All sources of set 1 must be in set 2
 -h, --help : Display usage.
 -r, --recursive : Search subdirectories as well.
 -v, --verbose : Dump verbose information.

"""

import getopt
import os
import sys
import difflib


class SourceInfo(object):
  """ Contains one set of sources. """
  def __init__(self, name, recursive):
    self.files = []
    name = os.path.normpath(name)

    if not os.path.exists(name):
      print "Can not find", name
      usage()

    # If the source is a directory
    if os.path.isdir(name):
      self.path = name
      self.dir = True

      if recursive:
        # and we are searching recursively, get a set of
        # all files
        self.files = GetFileList(name)
      else:
        # otherwise just a set of files in this directory
        files = os.listdir(path)
        for file in files:
          if os.path.isfile(os.path.join(path, file)):
            self.files.append(file)
    else:
      # If not a directory, then just add the one file
      self.path = os.path.dirname(name)
      self.files.append(os.path.basename(name))
      self.dir = False

    if recursive and not self.dir:
      print "Source must be a directory to diff recusively."
      usage()


def GetFileList(path):
  """ Generates a list of files at a given path """
  files = []

  # Determine the length of the leading path
  skip = len(path.split(os.path.sep))

  for dirname, dirnames, filenames in os.walk(path):
    # Remove the leading path
    dirname = os.path.sep.join(dirname.split(os.path.sep)[skip:])

    for filename in filenames:
        path = os.path.join(dirname, filename)
        files.append(path)
  return files


def SourceIntersect(src1, src2):
  """ Returns the intersection of both source sets. """
  files = []
  src1set = set(src1.files)
  src2set = set(src2.files)

  for file1 in src1.files:
    if file1 in src2set:
      files.append(file1)
  return files


def ReadLines(path):
  """ Returns a list of lines of the file found at 'path'. """
  try:
    file = open(path, "r")
  except IOError, e:
    print "  ***I/O error({0}): {1} {2}".format(e[0], e[1], path)
    print
    raise

  lines = file.readlines()
  file.close()
  return lines


def Diff(file1, file2):
  """ Print the unified returing non zero if not equal."""
  foundDiff = 0
  try:
    lines1 = ReadLines(file1)
    lines2 = ReadLines(file2)
    diffs = difflib.unified_diff(lines1, lines2, fromfile=file1, tofile=file2)
    for diff in diffs:
      foundDiff = foundDiff + 1
      sys.stdout.write(diff)
    return foundDiff
  except IOError:
    return 1

def usage():
  """ Print the usage information. """
  print __doc__
  sys.exit(1)


def main(argv):
  verbose = False
  recursive = False
  all = False

  # Parse command-line arguments
  long_opts = ['all','help','recursive','verbose']
  opts, args = getopt.getopt(argv[1:], 'ahrv', long_opts)

  # Process options
  for k,v in opts:
    if k == '-h' or k == '--help':
      usage()
    if k == '-v' or k == '--verbose':
      verbose = True
    if k == '-r' or k == '--recursive':
      recursive = True
    if k == '-a' or k == '--all':
      all = True

  # Process sources
  if len(args) != 2:
    print "Expecting two sources (files or directories)."
    usage()

  src1 = SourceInfo(args[0], recursive)
  src2 = SourceInfo(args[1], recursive)

  if all:
    # Diff all files in the first set
    files = src1.files
  else:
    # Get a list of all matching files
    files = SourceIntersect(src1, src2)

  diffCnt = 0
  for file in files:
    path1 = os.path.join(src1.path, file)
    path2 = os.path.join(src2.path, file)
    if verbose:
      print "Compare %s and %s" % (path1, path2)
    diffCnt = diffCnt + Diff(path1, path2)

  if verbose:
    print "%d different file(s) or failure(s)." % diffCnt
  return diffCnt

if __name__ == '__main__':
  sys.exit(main(sys.argv))
