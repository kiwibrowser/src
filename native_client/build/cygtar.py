#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob
import hashlib
import optparse
import os
import posixpath
import shutil
import subprocess
import stat
import sys
import tarfile

"""A Cygwin aware version compress/extract object.

This module supports creating and unpacking a tarfile on all platforms.  For
Cygwin, Mac, and Linux, it will use the standard tarfile implementation.  For
Win32 it will detect Cygwin style symlinks as it archives and convert them to
symlinks.

For Win32, it is unfortunate that os.stat does not return a FileID in the ino
field which would allow us to correctly determine which files are hardlinks, so
instead we assume that any files in the archive that are an exact match are
hardlinks to the same data.

We know they are not Symlinks because we are using Cygwin style symlinks only,
which appear to Win32 a normal file.

All paths stored and retrieved from a TAR file are expected to be POSIX style,
Win32 style paths will be rejected.

NOTE:
  All paths represent by the tarfile and all API functions are POSIX style paths
  except for CygTar.Add which assumes a Native path.
"""


def ToNativePath(native_path):
  """Convert to a posix style path if this is win32."""
  if sys.platform == 'win32':
    return native_path.replace('/', '\\')
  return native_path


def IsCygwinSymlink(symtext):
  """Return true if the provided text looks like a Cygwin symlink."""
  return symtext[:12] == '!<symlink>\xff\xfe'


def SymDatToPath(symtext):
  """Convert a Cygwin style symlink data to a relative path."""
  return ''.join([ch for ch in symtext[12:] if ch != '\x00'])


def PathToSymDat(filepath):
  """Convert a filepath to cygwin style symlink data."""
  symtag = '!<symlink>\xff\xfe'
  unipath = ''.join([ch + '\x00' for ch in filepath])
  strterm = '\x00\x00'
  return symtag + unipath + strterm


def CreateWin32Link(filepath, targpath, verbose):
  """Create a link on Win32 if possible

  Uses mklink to create a link (hardlink or junction) if possible. On failure,
  it will assume mklink is unavailible and copy the file instead. Future calls
  will not attempt to use mklink."""

  targ_is_dir = os.path.isdir(targpath)

  call_mklink = False
  if targ_is_dir and CreateWin32Link.try_junction:
    # Creating a link to a directory will fail, but a junction (which is more
    # like a symlink) will work.
    mklink_flag = '/J'
    call_mklink = True
  elif not targ_is_dir and CreateWin32Link.try_hardlink:
    mklink_flag = '/H'
    call_mklink = True

  # Assume an error, if subprocess succeeds, then it should return 0
  err = 1
  if call_mklink:
    try:
      cmd = ['cmd', '/C', 'mklink %s %s %s' % (
              mklink_flag, ToNativePath(filepath), ToNativePath(targpath))]
      err = subprocess.call(cmd,
          stdout = open(os.devnull, 'wb'),
          stderr = open(os.devnull, 'wb'))
    except EnvironmentError:
      if targ_is_dir:
        CreateWin32Link.try_junction = False
      else:
        CreateWin32Link.try_hardlink = False

  # If we failed to create a link, then just copy it.  We wrap this in a
  # retry for Windows which often has stale file lock issues.
  if err or not os.path.exists(filepath):
    if targ_is_dir and verbose:
      print 'Failed to create junction %s -> %s. Copying instead.\n' % (
          filepath, targpath)

    for cnt in range(1,4):
      try:
        if targ_is_dir:
          shutil.copytree(targpath, filepath)
        else:
          shutil.copyfile(targpath, filepath)
        return False
      except EnvironmentError:
        if verbose:
          print 'Try %d: Failed hardlink %s -> %s\n' % (cnt, filepath, targpath)
    if verbose:
      print 'Giving up.'

CreateWin32Link.try_hardlink = True
CreateWin32Link.try_junction = True



def ComputeFileHash(filepath):
  """Generate a sha1 hash for the file at the given path."""
  sha1 = hashlib.sha1()
  with open(filepath, 'rb') as fp:
    sha1.update(fp.read())
  return sha1.hexdigest()


def ReadableSizeOf(num):
  """Convert to a human readable number."""
  if num < 1024.0:
    return '[%5dB]' % num
  for x in ['B','K','M','G','T']:
     if num < 1024.0:
       return '[%5.1f%s]' % (num, x)
     num /= 1024.0
  return '[%dT]' % int(num)


class CygTar(object):
  """ CygTar is an object which represents a Win32 and Cygwin aware tarball."""
  def __init__(self, filename, mode='r', verbose=False):
    self.size_map = {}
    self.file_hashes = {}
    # Set errorlevel=1 so that fatal errors actually raise!
    if 'r' in mode:
      self.read_file = open(filename, 'rb')
      self.read_filesize = os.path.getsize(filename)
      self.tar = tarfile.open(mode=mode, fileobj=self.read_file, errorlevel=1)
    else:
      self.read_file = None
      self.read_filesize = 0
      self.tar = tarfile.open(filename, mode=mode, errorlevel=1)
    self.verbose = verbose

  def __DumpInfo(self, tarinfo):
    """Prints information on a single object in the tarball."""
    typeinfo = '?'
    lnk = ''
    if tarinfo.issym():
      typeinfo = 'S'
      lnk = '-> ' + tarinfo.linkname
    if tarinfo.islnk():
      typeinfo = 'H'
      lnk = '-> ' + tarinfo.linkname
    if tarinfo.isdir():
      typeinfo = 'D'
    if tarinfo.isfile():
      typeinfo = 'F'
    reable_size = ReadableSizeOf(tarinfo.size)
    print '%s %s : %s %s' % (reable_size, typeinfo, tarinfo.name, lnk)
    return tarinfo

  def __AddFile(self, tarinfo, fileobj=None):
    """Add a file to the archive."""
    if self.verbose:
      self.__DumpInfo(tarinfo)
    self.tar.addfile(tarinfo, fileobj)

  def __AddLink(self, tarinfo, linktype, linkpath):
    """Add a Win32 symlink or hardlink to the archive."""
    tarinfo.linkname = linkpath
    tarinfo.type = linktype
    tarinfo.size = 0
    self.__AddFile(tarinfo)

  def Add(self, filepath, prefix=None):
    """Add path filepath to the archive which may be Native style.

    Add files individually recursing on directories.  For POSIX we use
    tarfile.addfile directly on symlinks and hardlinks.  For files, we must
    check if they are duplicates which we convert to hardlinks or symlinks
    which we convert from a file to a symlink in the tarfile.  All other files
    are added as a standard file.
    """

    # At this point tarinfo.name will contain a POSIX style path regardless
    # of the original filepath.
    tarinfo = self.tar.gettarinfo(filepath)
    if prefix:
      tarinfo.name = posixpath.join(prefix, tarinfo.name)

    if sys.platform == 'win32':
      # On win32 os.stat() always claims that files are world writable
      # which means that unless we remove this bit here we end up with
      # world writables files in the archive, which is almost certainly
      # not intended.
      tarinfo.mode &= ~stat.S_IWOTH
      tarinfo.mode &= ~stat.S_IWGRP

      # If we want cygwin to be able to extract this archive and use
      # executables and dll files we need to mark all the archive members as
      # executable.  This is essentially what happens anyway when the
      # archive is extracted on win32.
      tarinfo.mode |= stat.S_IXUSR | stat.S_IXOTH | stat.S_IXGRP

    # If this a symlink or hardlink, add it
    if tarinfo.issym() or tarinfo.islnk():
      tarinfo.size = 0
      self.__AddFile(tarinfo)
      return True

    # If it's a directory, then you want to recurse into it
    if tarinfo.isdir():
      self.__AddFile(tarinfo)
      native_files = glob.glob(os.path.join(filepath, '*'))
      for native_file in native_files:
        if not self.Add(native_file, prefix): return False
      return True

    # At this point we only allow addition of "FILES"
    if not tarinfo.isfile():
      print 'Failed to add non real file: %s' % filepath
      return False

    # Now check if it is a Cygwin style link disguised as a file.
    # We go ahead and check on all platforms just in case we are tar'ing a
    # mount shared with windows.
    if tarinfo.size <= 524:
      with open(filepath) as fp:
        symtext = fp.read()
      if IsCygwinSymlink(symtext):
        self.__AddLink(tarinfo, tarfile.SYMTYPE, SymDatToPath(symtext))
        return True

    # Otherwise, check if its a hardlink by seeing if it matches any unique
    # hash within the list of hashed files for that file size.
    nodelist = self.size_map.get(tarinfo.size, [])

    # If that size bucket is empty, add this file, no need to get the hash until
    # we get a bucket collision for the first time..
    if not nodelist:
      self.size_map[tarinfo.size] = [filepath]
      with open(filepath, 'rb') as fp:
        self.__AddFile(tarinfo, fp)
      return True

    # If the size collides with anything, we'll need to check hashes.  We assume
    # no hash collisions for SHA1 on a given bucket, since the number of files
    # in a bucket over possible SHA1 values is near zero.
    newhash = ComputeFileHash(filepath)
    self.file_hashes[filepath] = newhash

    for oldname in nodelist:
      oldhash = self.file_hashes.get(oldname, None)
      if not oldhash:
        oldhash = ComputeFileHash(oldname)
        self.file_hashes[oldname] = oldhash

      if oldhash == newhash:
        self.__AddLink(tarinfo, tarfile.LNKTYPE, oldname)
        return True

    # Otherwise, we missed, so add it to the bucket for this size
    self.size_map[tarinfo.size].append(filepath)
    with open(filepath, 'rb') as fp:
      self.__AddFile(tarinfo, fp)
    return True

  def Extract(self):
    """Extract the tarfile to the current directory."""
    if self.verbose:
      sys.stdout.write('|' + ('-' * 48) + '|\n')
      sys.stdout.flush()
      dots_outputted = 0

    win32_symlinks = {}
    for m in self.tar:
      if self.verbose:
        cnt = self.read_file.tell()
        curdots = cnt * 50 / self.read_filesize
        if dots_outputted < curdots:
          for dot in xrange(dots_outputted, curdots):
            sys.stdout.write('.')
          sys.stdout.flush()
          dots_outputted = curdots

      # For hardlinks in Windows, we try to use mklink, and instead copy on
      # failure.
      if m.islnk() and sys.platform == 'win32':
        CreateWin32Link(m.name, m.linkname, self.verbose)
      # On Windows we treat symlinks as if they were hard links.
      # Proper Windows symlinks supported by everything can be made with
      # mklink, but only by an Administrator.  The older toolchains are
      # built with Cygwin, so they could use Cygwin-style symlinks; but
      # newer toolchains do not use Cygwin, and nothing else on the system
      # understands Cygwin-style symlinks, so avoid them.
      elif m.issym() and sys.platform == 'win32':
        # For a hard link, the link target (m.linkname) always appears
        # in the archive before the link itself (m.name), so the links
        # can just be made on the fly.  However, a symlink might well
        # appear in the archive before its target file, so there would
        # not yet be any file to hard-link to.  Hence, we have to collect
        # all the symlinks and create them in dependency order at the end.
        linkname = m.linkname
        if not posixpath.isabs(linkname):
          linkname = posixpath.join(posixpath.dirname(m.name), linkname)
        linkname = posixpath.normpath(linkname)
        win32_symlinks[posixpath.normpath(m.name)] = linkname
      # Otherwise, extract normally.
      else:
        self.tar.extract(m)

    win32_symlinks_left = win32_symlinks.items()
    while win32_symlinks_left:
      this_symlink = win32_symlinks_left.pop(0)
      name, linkname = this_symlink
      if linkname in win32_symlinks:
        # The target is itself a symlink not yet created.
        # Wait for it to come 'round on the guitar.
        win32_symlinks_left.append(this_symlink)
      else:
        del win32_symlinks[name]
        CreateWin32Link(name, linkname, self.verbose)

    if self.verbose:
      sys.stdout.write('\n')
      sys.stdout.flush()

  def List(self):
    """List the set of objects in the tarball."""
    for tarinfo in self.tar:
      self.__DumpInfo(tarinfo)

  def Close(self):
    self.tar.close()
    if self.read_file is not None:
      self.read_file.close()
      self.read_file = None
      self.read_filesize = 0


def Main(args):
  parser = optparse.OptionParser()
  # Modes
  parser.add_option('-c', '--create', help='Create a tarball.',
      action='store_const', const='c', dest='action', default='')
  parser.add_option('-x', '--extract', help='Extract a tarball.',
      action='store_const', const='x', dest='action')
  parser.add_option('-t', '--list', help='List sources in tarball.',
      action='store_const', const='t', dest='action')

  # Compression formats
  parser.add_option('-j', '--bzip2', help='Create a bz2 tarball.',
      action='store_const', const=':bz2', dest='format', default='')
  parser.add_option('-z', '--gzip', help='Create a gzip tarball.',
      action='store_const', const=':gz', dest='format', )
  # Misc
  parser.add_option('-v', '--verbose', help='Use verbose output.',
      action='store_true', dest='verbose', default=False)
  parser.add_option('-f', '--file', help='Name of tarball.',
      dest='filename', default='')
  parser.add_option('-C', '--directory', help='Change directory.',
      dest='cd', default='')
  parser.add_option('--prefix', help='Subdirectory prefix for all paths')

  options, args = parser.parse_args(args[1:])
  if not options.action:
    parser.error('Expecting compress or extract')
  if not options.filename:
    parser.error('Expecting a filename')

  if options.action in ['c'] and not args:
    parser.error('Expecting list of sources to add')
  if options.action in ['x', 't'] and args:
    parser.error('Unexpected source list on extract')

  if options.action == 'c':
    mode = 'w' + options.format
  else:
    mode = 'r'+ options.format

  tar = CygTar(options.filename, mode, verbose=options.verbose)
  if options.cd:
    os.chdir(options.cd)

  if options.action == 't':
    tar.List()
    return 0

  if options.action == 'x':
    tar.Extract()
    return 0

  if options.action == 'c':
    for filepath in args:
      if not tar.Add(filepath, options.prefix):
        return -1
    tar.Close()
    return 0

  parser.error('Missing action c, t, or x.')
  return -1


if __name__ == '__main__':
  sys.exit(Main(sys.argv))
