# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Adapted from portage/getbinpkg.py -- Portage binary-package helper functions
# Copyright 2003-2004 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2

"""Helpers dealing with binpkg Packages index files"""

from __future__ import print_function

import collections
import cStringIO
import operator
import os
import tempfile
import time
import urllib2

from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import gs
from chromite.lib import parallel


TWO_WEEKS = 60 * 60 * 24 * 7 * 2
HTTP_FORBIDDEN_CODES = (401, 403)
HTTP_NOT_FOUND_CODES = (404, 410)

_Package = collections.namedtuple('_Package', ['mtime', 'uri', 'debug_symbols'])

class PackageIndex(object):
  """A parser for the Portage Packages index file.

  The Portage Packages index file serves to keep track of what packages are
  included in a tree. It contains the following sections:
    1) The header. The header tracks general key/value pairs that don't apply
       to any specific package. E.g., it tracks the base URL of the packages
       file, and the number of packages included in the file. The header is
       terminated by a blank line.
    2) The body. The body is a list of packages. Each package contains a list
       of key/value pairs. Packages are either terminated by a blank line or
       by the end of the file. Every package has a CPV entry, which serves as
       a unique identifier for the package.
  """

  def __init__(self):
    """Constructor."""

    # The header tracks general key/value pairs that don't apply to any
    # specific package. E.g., it tracks the base URL of the packages.
    self.header = {}

    # A list of packages (stored as a list of dictionaries).
    self.packages = []

    # Whether or not the PackageIndex has been modified since the last time it
    # was written.
    self.modified = False

  def _PopulateDuplicateDB(self, db, expires):
    """Populate db with SHA1 -> URL mapping for packages.

    Args:
      db: Dictionary to populate with SHA1 -> URL mapping for packages.
      expires: The time at which prebuilts expire from the binhost.
    """

    uri = gs.CanonicalizeURL(self.header['URI'])
    for pkg in self.packages:
      cpv, sha1, mtime = pkg['CPV'], pkg.get('SHA1'), pkg.get('MTIME')
      oldpkg = db.get(sha1, _Package(0, None, False))
      if sha1 and mtime and int(mtime) > max(expires, oldpkg.mtime):
        path = pkg.get('PATH', cpv + '.tbz2')
        db[sha1] = _Package(int(mtime),
                            '%s/%s' % (uri.rstrip('/'), path),
                            pkg.get('DEBUG_SYMBOLS') == 'yes')

  def _ReadPkgIndex(self, pkgfile):
    """Read a list of key/value pairs from the Packages file into a dictionary.

    Both header entries and package entries are lists of key/value pairs, so
    they can both be read by this function. Entries can be terminated by empty
    lines or by the end of the file.

    This function will read lines from the specified file until it encounters
    the a blank line or the end of the file.

    Keys and values in the Packages file are separated by a colon and a space.
    Keys may contain capital letters, numbers, and underscores, but may not
    contain colons. Values may contain any character except a newline. In
    particular, it is normal for values to contain colons.

    Lines that have content, and do not contain a valid key/value pair, are
    ignored. This is for compatibility with the Portage package parser, and
    to allow for future extensions to the Packages file format.

    All entries must contain at least one key/value pair. If the end of the
    fils is reached, an empty dictionary is returned.

    Args:
      pkgfile: A python file object.

    Returns:
      The dictionary of key-value pairs that was read from the file.
    """
    d = {}
    for line in pkgfile:
      line = line.rstrip('\n')
      if not line:
        assert d, 'Packages entry must contain at least one key/value pair'
        break
      line = line.split(': ', 1)
      if len(line) == 2:
        k, v = line
        d[k] = v
    return d

  def _WritePkgIndex(self, pkgfile, entry):
    """Write header entry or package entry to packages file.

    The keys and values will be separated by a colon and a space. The entry
    will be terminated by a blank line.

    Args:
      pkgfile: A python file object.
      entry: A dictionary of the key/value pairs to write.
    """
    lines = ['%s: %s' % (k, v) for k, v in sorted(entry.items()) if v]
    pkgfile.write('%s\n\n' % '\n'.join(lines))

  def _ReadHeader(self, pkgfile):
    """Read header of packages file.

    Args:
      pkgfile: A python file object.
    """
    assert not self.header, 'Should only read header once.'
    self.header = self._ReadPkgIndex(pkgfile)

  def _ReadBody(self, pkgfile):
    """Read body of packages file.

    Before calling this function, you must first read the header (using
    _ReadHeader).

    Args:
      pkgfile: A python file object.
    """
    assert self.header, 'Should read header first.'
    assert not self.packages, 'Should only read body once.'

    # Read all of the sections in the body by looping until we reach the end
    # of the file.
    while True:
      d = self._ReadPkgIndex(pkgfile)
      if not d:
        break
      if 'CPV' in d:
        self.packages.append(d)

  def Read(self, pkgfile):
    """Read the entire packages file.

    Args:
      pkgfile: A python file object.
    """
    self._ReadHeader(pkgfile)
    self._ReadBody(pkgfile)

  def RemoveFilteredPackages(self, filter_fn):
    """Remove packages which match filter_fn.

    Args:
      filter_fn: A function which operates on packages. If it returns True,
                 the package should be removed.
    """

    filtered = [p for p in self.packages if not filter_fn(p)]
    if filtered != self.packages:
      self.modified = True
      self.packages = filtered

  def ResolveDuplicateUploads(self, pkgindexes):
    """Point packages at files that have already been uploaded.

    For each package in our index, check if there is an existing package that
    has already been uploaded to the same base URI, and that is no older than
    two weeks. If so, point that package at the existing file, so that we don't
    have to upload the file.

    Args:
      pkgindexes: A list of PackageIndex objects containing info about packages
        that have already been uploaded.

    Returns:
      A list of the packages that still need to be uploaded.
    """
    db = {}
    now = int(time.time())
    expires = now - TWO_WEEKS
    base_uri = gs.CanonicalizeURL(self.header['URI'])
    for pkgindex in pkgindexes:
      if gs.CanonicalizeURL(pkgindex.header['URI']) == base_uri:
        # pylint: disable=W0212
        pkgindex._PopulateDuplicateDB(db, expires)

    uploads = []
    base_uri = self.header['URI']
    for pkg in self.packages:
      sha1 = pkg.get('SHA1')
      dup = db.get(sha1)

      # If the debug symbols are available locally but are not available in the
      # remote binhost, re-upload them.
      # Note: this should never happen as we would have pulled the debug symbols
      # from said binhost.
      if (sha1 and dup and dup.uri.startswith(base_uri)
          and (pkg.get('DEBUG_SYMBOLS') != 'yes' or dup.debug_symbols)):
        pkg['PATH'] = dup.uri[len(base_uri):].lstrip('/')
        pkg['MTIME'] = str(dup.mtime)

        if dup.debug_symbols:
          pkg['DEBUG_SYMBOLS'] = 'yes'
      else:
        pkg['MTIME'] = str(now)
        uploads.append(pkg)
    return uploads

  def SetUploadLocation(self, base_uri, path_prefix):
    """Set upload location to base_uri + path_prefix.

    Args:
      base_uri: Base URI for all packages in the file. We set
        self.header['URI'] to this value, so all packages must live under
        this directory.
      path_prefix: Path prefix to use for all current packages in the file.
        This will be added to the beginning of the path for every package.
    """
    self.header['URI'] = base_uri
    for pkg in self.packages:
      path = pkg['CPV'] + '.tbz2'
      pkg['PATH'] = '%s/%s' % (path_prefix.rstrip('/'), path)

  def Write(self, pkgfile):
    """Write a packages file to disk.

    If 'modified' flag is set, the TIMESTAMP and PACKAGES fields in the header
    will be updated before writing to disk.

    Args:
      pkgfile: A python file object.
    """
    if self.modified:
      self.header['TIMESTAMP'] = str(long(time.time()))
      self.header['PACKAGES'] = str(len(self.packages))
      self.modified = False
    self._WritePkgIndex(pkgfile, self.header)
    for metadata in sorted(self.packages, key=operator.itemgetter('CPV')):
      self._WritePkgIndex(pkgfile, metadata)

  def WriteToNamedTemporaryFile(self):
    """Write pkgindex to a temporary file.

    Args:
      pkgindex: The PackageIndex object.

    Returns:
      A temporary file containing the packages from pkgindex.
    """
    f = tempfile.NamedTemporaryFile(prefix='chromite.binpkg.pkgidx.')
    self.Write(f)
    f.flush()
    f.seek(0)
    return f


def _RetryUrlOpen(url, tries=3):
  """Open the specified url, retrying if we run into temporary errors.

  We retry for both network errors and 5xx Server Errors. We do not retry
  for HTTP errors with a non-5xx code.

  Args:
    url: The specified url.
    tries: The number of times to try.

  Returns:
    The result of urllib2.urlopen(url).
  """
  for i in range(tries):
    try:
      return urllib2.urlopen(url)
    except urllib2.HTTPError as e:
      if i + 1 >= tries or e.code < 500:
        e.msg += ('\nwhile processing %s' % url)
        raise
      else:
        print('Cannot GET %s: %s' % (url, str(e)))
    except urllib2.URLError as e:
      if i + 1 >= tries:
        raise
      else:
        print('Cannot GET %s: %s' % (url, str(e)))
    print('Sleeping for 10 seconds before retrying...')
    time.sleep(10)


def GrabRemotePackageIndex(binhost_url):
  """Grab the latest binary package database from the specified URL.

  Args:
    binhost_url: Base URL of remote packages (PORTAGE_BINHOST).

  Returns:
    A PackageIndex object, if the Packages file can be retrieved. If the
    packages file cannot be retrieved, then None is returned.
  """
  url = '%s/Packages' % binhost_url.rstrip('/')
  pkgindex = PackageIndex()
  if binhost_url.startswith('http'):
    try:
      f = _RetryUrlOpen(url)
    except urllib2.HTTPError as e:
      if e.code in HTTP_FORBIDDEN_CODES:
        logging.PrintBuildbotStepWarnings()
        logging.error('Cannot GET %s: %s' % (url, str(e)))
        return None
      # Not found errors are normal if old prebuilts were cleaned out.
      if e.code in HTTP_NOT_FOUND_CODES:
        return None
      raise
  elif binhost_url.startswith('gs://'):
    try:
      gs_context = gs.GSContext()
      output = gs_context.Cat(url)
    except (cros_build_lib.RunCommandError, gs.GSNoSuchKey) as e:
      logging.PrintBuildbotStepWarnings()
      logging.error('Cannot GET %s: %s' % (url, str(e)))
      return None
    f = cStringIO.StringIO(output)
  else:
    return None
  pkgindex.Read(f)
  pkgindex.header.setdefault('URI', binhost_url)
  f.close()
  return pkgindex


def GrabLocalPackageIndex(package_path):
  """Read a local packages file from disk into a PackageIndex() object.

  Args:
    package_path: Directory containing Packages file.

  Returns:
    A PackageIndex object.
  """
  packages_file = file(os.path.join(package_path, 'Packages'))
  pkgindex = PackageIndex()
  pkgindex.Read(packages_file)
  packages_file.close()

  # List all debug symbols available in package_path.
  symbols = set()
  for f in cros_build_lib.ListFiles(package_path):
    if f.endswith('.debug.tbz2'):
      f = os.path.relpath(f, package_path)[:-len('.debug.tbz2')]
      symbols.add(f)

  for p in pkgindex.packages:
    # If the Packages file has DEBUG_SYMBOLS set but no debug symbols are
    # found, unset it.
    p.pop('DEBUG_SYMBOLS', None)
    if p['CPV'] in symbols:
      p['DEBUG_SYMBOLS'] = 'yes'

  return pkgindex


def _DownloadURLs(urls, dest_dir):
  """Copy URLs into the specified |dest_dir|.

  Args:
    urls: List of URLs to fetch.
    dest_dir: Destination directory.
  """
  gs_ctx = gs.GSContext()
  cmd = ['cp'] + urls + [dest_dir]
  gs_ctx.DoCommand(cmd, parallel=len(urls) > 1)


def FetchTarballs(binhost_urls, pkgdir):
  """Prefetch the specified |binhost_urls| to the specified |pkgdir|.

  This function fetches the tarballs from the specified list of binhost
  URLs to disk. It does not populate the Packages file -- we leave that
  to Portage.

  Args:
    binhost_urls: List of binhost URLs to fetch.
    pkgdir: Location to store the fetched packages.
  """
  categories = {}
  for binhost_url in binhost_urls:
    pkgindex = GrabRemotePackageIndex(binhost_url)
    base_uri = pkgindex.header['URI']
    for pkg in pkgindex.packages:
      cpv = pkg['CPV']
      path = pkg.get('PATH', '%s.tbz2' % cpv)
      uri = '/'.join([base_uri, path])
      category = cpv.partition('/')[0]
      fetches = categories.setdefault(category, {})
      fetches[cpv] = uri

  with parallel.BackgroundTaskRunner(_DownloadURLs) as queue:
    for category, urls in categories.iteritems():
      category_dir = os.path.join(pkgdir, category)
      if not os.path.exists(category_dir):
        os.makedirs(category_dir)
      queue.put((urls.values(), category_dir))
