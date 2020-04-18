# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to discover dependencies and other file information from a build.

Some files in the image are installed to provide some functionality, such as
chrome, shill or bluetoothd provide different functionality that can be
present or not on a given build. Many other files are dependencies from these
files that need to be present in the image for them to work. These dependencies
come from needed shared libraries, executed files and other configuration files
read.

This script currently discovers dependencies between ELF files for libraries
required at load time (libraries loaded by the dynamic linker) but not
libraries loaded at runtime with dlopen(). It also computes size and file type
in several cases to help understand the contents of the built image.
"""

from __future__ import print_function

import itertools
import json
import multiprocessing
import os
import stat

from chromite.lib import commandline
from chromite.lib import cros_logging as logging
from chromite.lib import filetype
from chromite.lib import parseelf
from chromite.lib import portage_util
from chromite.scripts import lddtree


# Regex to parse Gentoo atoms. This should match the following ebuild names,
# splitting the package name from the version.
# without version:
#   chromeos-base/tty
#   chromeos-base/libchrome-271506
#   sys-kernel/chromeos-kernel-3_8
# with version:
#   chromeos-base/tty-0.0.1-r4
#   chromeos-base/libchrome-271506-r5
#   sys-kernel/chromeos-kernel-3_8-3.8.11-r35
RE_EBUILD_WITHOUT_VERSION = r'^([a-z0-9\-]+/[a-zA-Z0-9\_\+\-]+)$'
RE_EBUILD_WITH_VERSION = (
    r'^=?([a-z0-9\-]+/[a-zA-Z0-9\_\+\-]+)\-([^\-]+(\-r\d+)?)$')


def ParseELFWithArgs(args):
  """Wrapper to parseelf.ParseELF accepting a single arg.

  This wrapper is required to use multiprocessing.Pool.map function.

  Returns:
    A 2-tuple with the passed relative path and the result of ParseELF(). On
    error, when ParseELF() returns None, this function returns None.
  """
  elf = parseelf.ParseELF(*args)
  if elf is None:
    return
  return args[1], elf


class DepTracker(object):
  """Tracks dependencies and file information in a root directory.

  This class computes dependencies and other information related to the files
  in the root image.
  """

  def __init__(self, root, jobs=1):
    root_st = os.lstat(root)
    if not stat.S_ISDIR(root_st.st_mode):
      raise Exception('root (%s) must be a directory' % root)
    self._root = root.rstrip('/') + '/'
    self._file_type_decoder = filetype.FileTypeDecoder(root)

    # A wrapper to the multiprocess map function. We avoid launching a pool
    # of processes when jobs is 1 so python exceptions kill the main process,
    # useful for debugging.
    if jobs > 1:
      self._pool = multiprocessing.Pool(jobs)
      self._imap = self._pool.map
    else:
      self._imap = itertools.imap

    self._files = {}
    self._ebuilds = {}

    # Mapping of rel_paths for symlinks and hardlinks. Hardlinks are assumed
    # to point to the lowest lexicographically file with the same inode.
    self._symlinks = {}
    self._hardlinks = {}

  def Init(self):
    """Generates the initial list of files."""
    # First iteration over all the files in root searching for symlinks and
    # non-regular files.
    seen_inodes = {}
    for basepath, _, filenames in sorted(os.walk(self._root)):
      for filename in sorted(filenames):
        full_path = os.path.join(basepath, filename)
        rel_path = full_path[len(self._root):]
        st = os.lstat(full_path)

        file_data = {
            'size': st.st_size,
        }
        self._files[rel_path] = file_data

        # Track symlinks.
        if stat.S_ISLNK(st.st_mode):
          link_path = os.readlink(full_path)
          # lddtree's normpath handles a little more cases than the os.path
          # version. In particular, it handles the '//' case.
          self._symlinks[rel_path] = (
              link_path.lstrip('/') if link_path and link_path[0] == '/' else
              lddtree.normpath(os.path.join(os.path.dirname(rel_path),
                                            link_path)))
          file_data['deps'] = {
              'symlink': [self._symlinks[rel_path]]
          }

        # Track hardlinks.
        if st.st_ino in seen_inodes:
          self._hardlinks[rel_path] = seen_inodes[st.st_ino]
          continue
        seen_inodes[st.st_ino] = rel_path

  def SaveJSON(self, filename):
    """Save the computed information to a JSON file.

    Args:
      filename: The destination JSON file.
    """
    data = {
        'files': self._files,
        'ebuilds': self._ebuilds,
    }
    json.dump(data, open(filename, 'w'))

  def ComputeEbuildDeps(self, sysroot):
    """Compute the dependencies between ebuilds and files.

    Iterates over the list of ebuilds in the database and annotates the files
    with the ebuilds they are in. For each ebuild installing a file in the root,
    also compute the direct dependencies. Stores the information internally.

    Args:
      sysroot: The path to the sysroot, for example "/build/link".
    """
    portage_db = portage_util.PortageDB(sysroot)
    if not os.path.exists(portage_db.db_path):
      logging.warning('PortageDB directory not found: %s', portage_db.db_path)
      return

    for pkg in portage_db.InstalledPackages():
      pkg_files = []
      pkg_size = 0
      cpf = '%s/%s' % (pkg.category, pkg.pf)
      for typ, rel_path in pkg.ListContents():
        # We ignore other entries like for example "dir".
        if not typ in (pkg.OBJ, pkg.SYM):
          continue
        # We ignore files installed in the SYSROOT that weren't copied to the
        # image.
        if not rel_path in self._files:
          continue
        pkg_files.append(rel_path)
        file_data = self._files[rel_path]
        if 'ebuild' in file_data:
          logging.warning('Duplicated entry for %s: %s and %',
                          rel_path, file_data['ebuild'], cpf)
        file_data['ebuild'] = cpf
        pkg_size += file_data['size']
      # Ignore packages that don't install any file.
      if not pkg_files:
        continue
      self._ebuilds[cpf] = {
          'size': pkg_size,
          'files': len(pkg_files),
          'atom': '%s/%s' % (pkg.category, pkg.package),
          'version': pkg.version,
      }
    # TODO(deymo): Parse dependencies between ebuilds.

  def ComputeELFFileDeps(self):
    """Computes the dependencies between files.

    Computes the dependencies between the files in the root directory passed
    during construction. The dependencies are inferred for ELF files.
    The list of dependencies for each file in the passed rootfs as a dict().
    The result's keys are the relative path of the files and the value of each
    file is a list of dependencies. A dependency is a tuple (dep_path,
    dep_type) where the dep_path is relative path from the passed root to the
    dependent file and dep_type is one the following strings stating how the
    dependency was discovered:
      'ldd': The dependent ELF file is listed as needed in the dynamic section.
      'symlink': The dependent file is a symlink to the depending.
    If there are dependencies of a given type whose target file wasn't
    determined, a tuple (None, dep_type) is included. This is the case for
    example is a program uses library that wasn't found.
    """
    ldpaths = lddtree.LoadLdpaths(self._root)

    # First iteration over all the files in root searching for symlinks and
    # non-regular files.
    parseelf_args = []
    for rel_path, file_data in self._files.iteritems():
      if rel_path in self._symlinks or rel_path in self._hardlinks:
        continue

      full_path = os.path.join(self._root, rel_path)
      st = os.lstat(full_path)
      if not stat.S_ISREG(st.st_mode):
        continue
      parseelf_args.append((self._root, rel_path, ldpaths))

    # Parallelize the ELF lookup step since it is quite expensive.
    elfs = dict(x for x in self._imap(ParseELFWithArgs, parseelf_args)
                if not x is None)

    for rel_path, elf in elfs.iteritems():
      file_data = self._files[rel_path]
      # Fill in the ftype if not set yet. We complete this value at this point
      # to avoid re-parsing the ELF file later.
      if not 'ftype' in file_data:
        ftype = self._file_type_decoder.GetType(rel_path, elf=elf)
        if ftype:
          file_data['ftype'] = ftype

      file_deps = file_data.get('deps', {})
      # Dependencies based on the result of ldd.
      for lib in elf.get('needed', []):
        lib_path = elf['libs'][lib]['path']
        if not 'ldd' in file_deps:
          file_deps['ldd'] = []
        file_deps['ldd'].append(lib_path)

      if file_deps:
        file_data['deps'] = file_deps

  def ComputeFileTypes(self):
    """Computes all the missing file type for the files in the root."""
    for rel_path, file_data in self._files.iteritems():
      if 'ftype' in file_data:
        continue
      ftype = self._file_type_decoder.GetType(rel_path)
      if ftype:
        file_data['ftype'] = ftype


def ParseArgs(argv):
  """Return parsed commandline arguments."""

  parser = commandline.ArgumentParser()
  parser.add_argument(
      '-j', '--jobs', type=int, default=multiprocessing.cpu_count(),
      help='number of simultaneous jobs.')
  parser.add_argument(
      '--sysroot', type='path', metavar='SYSROOT',
      help='parse portage DB for ebuild information from the provided sysroot.')
  parser.add_argument(
      '--json', type='path',
      help='store information in JSON file.')

  parser.add_argument(
      'root', type='path',
      help='path to the directory where the rootfs is mounted.')

  opts = parser.parse_args(argv)
  opts.Freeze()
  return opts


def main(argv):
  """Main function to start the script."""
  opts = ParseArgs(argv)
  logging.debug('Options are %s', opts)

  dt = DepTracker(opts.root, jobs=opts.jobs)
  dt.Init()

  dt.ComputeELFFileDeps()
  dt.ComputeFileTypes()

  if opts.sysroot:
    dt.ComputeEbuildDeps(opts.sysroot)

  if opts.json:
    dt.SaveJSON(opts.json)
