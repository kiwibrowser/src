#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Recipes for NativeClient toolchain packages.

The real entry plumbing is in toolchain_main.py.
"""

import fnmatch
import os
import optparse
import process
import shutil
import StringIO
import sys

def Mkdir(path):
  if path[-1] == '/':
    path = path[:-1]
  head, tail = os.path.split(path)

  if not head:
    head = '.'

  if not os.path.islink(path) and not os.path.exists(path):
    if not os.path.exists(head):
      Mkdir(head)
    os.mkdir(path)


def Rmdir(path):
  if os.path.exists(path):
    print "Removing: " + path
    shutil.rmtree(path)


def Symlink(srcpath, dstpath):
  if os.path.islink(dstpath):
    linkinfo = os.readlink(dstpath)
    if linkinfo == srcpath:
      return False
  Rmdir(dstpath)
  os.symlink(srcpath, dstpath)
  return True


def AcceptMatch(name, paterns, filters):
  for pat in filters:
    if fnmatch.fnmatch(name, pat):
      return False
  for pat in paterns:
    if fnmatch.fnmatch(name, pat):
      return True
  return False


def UpdateText(dstpath, text):
  if os.path.exists(dstpath):
    old = open(dstpath, 'r').read()
    if old == text:
      return False

  Mkdir(os.path.dirname(dstpath))
  with open(dstpath, 'w') as f:
    f.write(text)
  return True


def UpdateFile(srcpath, dstpath, verbose=False):
  if verbose:
    print '%s -> %s\n' % (dstpath, srcpath)
  shutil.copy(srcpath, dstpath)


def NeedsUpdate(src, dst):
  if not os.path.exists(dst):
    return True

  stime = os.path.getmtime(src)
  dtime = os.path.getmtime(dst)
  return (stime > dtime)


def CopyOrLinkNewer(src, dst):
  if not NeedsUpdate(src, dst):
    return False

  if os.path.islink(src):
    linkinfo = os.readlink(src)
    if os.path.islink(dst):
      if os.readlink(dst) == linkinfo:
        return False
      os.remove(dst)
    os.symlink(linkinfo, dst)
  else:
    UpdateFile(src, dst)
  return True


def UpdateFromTo(src, dst, paterns=['*'], filters=[]):
  if os.path.isfile(src):
    if not AcceptMatch(src, paterns, filters):
      return False

    Mkdir(os.path.dirname(dst))
    return CopyOrLinkNewer(src, dst)

  if not os.path.isdir(src):
    print "SRC does not exist, skipping: " + src
    return False

  pathlen = len(os.path.abspath(src))
  modified = False
  path_offs = len(src)
  for root, dirs, files in os.walk(src, followlinks=False):
    relroot = root[path_offs+1:]
    dstdir = os.path.join(dst, relroot)
    srcdir = root

    # Don't travel down symlinks
    if os.path.islink(root):
      continue

    # Don't travel down filtered directories
    if not AcceptMatch(srcdir, paterns=['*'], filters=filters):
      continue

    Mkdir(dstdir)
    for filename in files:
      srcpath = os.path.join(srcdir, filename)
      dstpath = os.path.join(dstdir, filename)
      if not AcceptMatch(srcpath, paterns, filters):
        continue
      if CopyOrLinkNewer(srcpath, dstpath):
        print "  %s -> %s" % (srcpath, dstpath)
        modified = True

    for filename in dirs:
      srcpath = os.path.abspath(os.path.join(root, filename))
      dstrel = os.path.abspath(os.path.join(root, filename))[pathlen+1:]
      dstpath = os.path.join(dst, dstrel)
      if os.path.islink(srcpath):
        if CopyOrLinkNewer(srcpath, dstpath):
          modified = True
          print "  %s -> %s" % (srcpath, dstpath)
  if modified:
    print "Update From To %s -> %s" % (src, dst)
  return modified
