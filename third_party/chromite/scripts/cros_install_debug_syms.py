# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Install debug symbols for specified packages.

Only reinstall the debug symbols if they are not already installed to save time.

The debug symbols are packaged outside of the prebuilt package in a
.debug.tbz2 archive when FEATURES=separatedebug is set (by default on
builders). On local machines, separatedebug is not set and the debug symbols
are part of the prebuilt package.
"""

from __future__ import print_function

import argparse
import os
import pickle
import sys
import tempfile
import urlparse

from chromite.lib import binpkg
from chromite.lib import cache
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.lib import path_util
from chromite.lib import gs

if cros_build_lib.IsInsideChroot():
  from portage import create_trees


DEBUG_SYMS_EXT = '.debug.tbz2'


# We cache the package indexes. When the format of what we store changes,
# bump the cache version to avoid problems.
CACHE_VERSION = '1'


class DebugSymbolsInstaller(object):
  """Container for enviromnent objects, needed to make multiprocessing work.

  This also redirects stdout to null when stdout_to_null=True to avoid
  polluting the output with portage QA warnings.
  """
  _old_stdout = None
  _null = None

  def __init__(self, vartree, gs_context, sysroot, stdout_to_null):
    self._vartree = vartree
    self._gs_context = gs_context
    self._sysroot = sysroot
    self._stdout_to_null = stdout_to_null

  def __enter__(self):
    if self._stdout_to_null:
      self._old_stdout = sys.stdout
      self._null = open(os.devnull, 'w')
      sys.stdout = self._null
    return self

  def __exit__(self, _exc_type, _exc_val, _exc_tb):
    if self._stdout_to_null:
      sys.stdout = self._old_stdout
      self._null.close()

  def Install(self, cpv, url):
    """Install the debug symbols for |cpv|.

    This will install the debug symbols tarball in PKGDIR so that it can be
    used later.

    Args:
      cpv: the cpv of the package to build. This assumes that the cpv is
        installed in the sysroot.
      url: url of the debug symbols archive. This could be a Google Storage url
        or a local path.
    """
    archive = os.path.join(self._vartree.settings['PKGDIR'],
                           cpv + DEBUG_SYMS_EXT)
    # GsContext does not understand file:// scheme so we need to extract the
    # path ourselves.
    parsed_url = urlparse.urlsplit(url)
    if not parsed_url.scheme or parsed_url.scheme == 'file':
      url = parsed_url.path

    if not os.path.isfile(archive):
      self._gs_context.Copy(url, archive, debug_level=logging.DEBUG)

    with osutils.TempDir(sudo_rm=True) as tempdir:
      cros_build_lib.SudoRunCommand(['tar', '-I', 'bzip2 -q', '-xf', archive,
                                     '-C', tempdir], quiet=True)

      with open(self._vartree.getpath(cpv, filename='CONTENTS'),
                'a') as content_file:
        # Merge the content of the temporary dir into the sysroot.
        # pylint: disable=protected-access
        link = self._vartree.dbapi._dblink(cpv)
        link.mergeme(tempdir, self._sysroot, content_file, None, '', {}, None)


def ParseArgs(argv):
  """Parse arguments and initialize field.

  Args:
    argv: arguments passed to the script.
  """
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('--board', help='Board name (required).', required=True)
  parser.add_argument('--all', dest='all', action='store_true',
                      help='Install the debug symbols for all installed '
                      'packages', default=False)
  parser.add_argument('packages', nargs=argparse.REMAINDER,
                      help='list of packages that need the debug symbols.')

  advanced = parser.add_argument_group('Advanced options')
  advanced.add_argument('--nocachebinhost', dest='cachebinhost', default=True,
                        action='store_false', help="Don't cache the list of"
                        " files contained in binhosts. (Default: cache)")
  advanced.add_argument('--clearcache', dest='clearcache', action='store_true',
                        default=False, help='Clear the binhost cache.')
  advanced.add_argument('--jobs', default=None, type=int,
                        help='Number of processes to run in parallel.')

  options = parser.parse_args(argv)
  options.Freeze()

  if options.all and options.packages:
    cros_build_lib.Die('Cannot use --all with a list of packages')
  return options


def ShouldGetSymbols(cpv, vardb, remote_symbols):
  """Return True if the symbols for cpv are available and are not installed.

  We try to check if the symbols are installed before checking availability as
  a GS request is more expensive than checking locally.

  Args:
    cpv: cpv of the package
    vardb: a vartree dbapi
    remote_symbols: a mapping from cpv to debug symbols url

  Returns:
    True if |cpv|'s debug symbols are not installed and are available
  """
  features, contents = vardb.aux_get(cpv, ['FEATURES', 'CONTENTS'])

  return ('separatedebug' in features and not '/usr/lib/debug/' in contents
          and cpv in remote_symbols)


def RemoteSymbols(vartree, binhost_cache=None):
  """Get the cpv to debug symbols mapping.

  If several binhost contain debug symbols for the same cpv, keep only the
  highest priority one.

  Args:
    vartree: a vartree
    binhost_cache: a cache containing the cpv to debug symbols url for all
      known binhosts. None if we are not caching binhosts.

  Returns:
    a dictionary mapping the cpv to a remote debug symbols gsurl.
  """
  symbols_mapping = {}
  for binhost in vartree.settings['PORTAGE_BINHOST'].split():
    if binhost:
      symbols_mapping.update(ListBinhost(binhost, binhost_cache))
  return symbols_mapping


def GetPackageIndex(binhost, binhost_cache=None):
  """Get the packages index for |binhost|.

  If a cache is provided, use it to a cache remote packages index.

  Args:
    binhost: a portage binhost, local, google storage or http.
    binhost_cache: a cache for the remote packages index.

  Returns:
    A PackageIndex object.
  """
  key = binhost.split('://')[-1]
  key = key.rstrip('/').split('/')

  if binhost_cache and binhost_cache.Lookup(key).Exists():
    with open(binhost_cache.Lookup(key).path) as f:
      return pickle.load(f)

  pkgindex = binpkg.GrabRemotePackageIndex(binhost)
  if pkgindex and binhost_cache:
    # Only cache remote binhosts as local binhosts can change.
    with tempfile.NamedTemporaryFile(delete=False) as temp_file:
      pickle.dump(pkgindex, temp_file)
      temp_file.file.close()
      binhost_cache.Lookup(key).Assign(temp_file.name)
  elif pkgindex is None:
    urlparts = urlparse.urlsplit(binhost)
    if urlparts.scheme not in ('file', ''):
      # Don't fail the build on network errors. Print a warning message and
      # continue.
      logging.warning('Could not get package index %s' % binhost)
      return None

    binhost = urlparts.path
    if not os.path.isdir(binhost):
      raise ValueError('unrecognized binhost format for %s.')
    pkgindex = binpkg.GrabLocalPackageIndex(binhost)

  return pkgindex


def ListBinhost(binhost, binhost_cache=None):
  """Return the cpv to debug symbols mapping for a given binhost.

  List the content of the binhost to extract the cpv to debug symbols
  mapping. If --cachebinhost is set, we cache the result to avoid the
  cost of gsutil every time.

  Args:
    binhost: a portage binhost, local or on google storage.
    binhost_cache: a cache containing mappings cpv to debug symbols url for a
      given binhost (None if we don't want to cache).

  Returns:
    A cpv to debug symbols url mapping.
  """

  symbols = {}
  pkgindex = GetPackageIndex(binhost, binhost_cache)
  if pkgindex is None:
    return symbols

  for p in pkgindex.packages:
    if p.get('DEBUG_SYMBOLS') == 'yes':
      path = p.get('PATH', p['CPV'] + '.tbz2')
      base_url = pkgindex.header.get('URI', binhost)
      symbols[p['CPV']] = os.path.join(base_url,
                                       path.replace('.tbz2', DEBUG_SYMS_EXT))

  return symbols


def GetMatchingCPV(package, vardb):
  """Return the cpv of the installed package matching |package|.

  Args:
    package: package name
    vardb: a vartree dbapi

  Returns:
    The cpv of the installed package whose name matchex |package|.
  """
  matches = vardb.match(package)
  if not matches:
    cros_build_lib.Die('Could not find package %s' % package)
  if len(matches) != 1:
    cros_build_lib.Die('Ambiguous package name: %s.\n'
                       'Matching: %s' % (package, ' '.join(matches)))
  return matches[0]


def main(argv):
  options = ParseArgs(argv)

  if not cros_build_lib.IsInsideChroot():
    raise commandline.ChrootRequiredError()

  if os.geteuid() != 0:
    cros_build_lib.SudoRunCommand(sys.argv)
    return

  # sysroot must have a trailing / as the tree dictionary produced by
  # create_trees in indexed with a trailing /.
  sysroot = cros_build_lib.GetSysroot(options.board) + '/'
  trees = create_trees(target_root=sysroot, config_root=sysroot)

  vartree = trees[sysroot]['vartree']

  cache_dir = os.path.join(path_util.FindCacheDir(),
                           'cros_install_debug_syms-v' + CACHE_VERSION)

  if options.clearcache:
    osutils.RmDir(cache_dir, ignore_missing=True)

  binhost_cache = None
  if options.cachebinhost:
    binhost_cache = cache.DiskCache(cache_dir)

  boto_file = vartree.settings['BOTO_CONFIG']
  if boto_file:
    os.environ['BOTO_CONFIG'] = boto_file

  gs_context = gs.GSContext()
  symbols_mapping = RemoteSymbols(vartree, binhost_cache)

  if options.all:
    to_install = vartree.dbapi.cpv_all()
  else:
    to_install = [GetMatchingCPV(p, vartree.dbapi) for p in options.packages]

  to_install = [p for p in to_install
                if ShouldGetSymbols(p, vartree.dbapi, symbols_mapping)]

  if not to_install:
    logging.info('nothing to do, exit')
    return

  with DebugSymbolsInstaller(vartree, gs_context, sysroot,
                             not options.debug) as installer:
    args = [(p, symbols_mapping[p]) for p in to_install]
    parallel.RunTasksInProcessPool(installer.Install, args,
                                   processes=options.jobs)

  logging.debug('installation done, updating packages index file')
  packages_dir = os.path.join(sysroot, 'packages')
  packages_file = os.path.join(packages_dir, 'Packages')
  # binpkg will set DEBUG_SYMBOLS automatically if it detects the debug symbols
  # in the packages dir.
  pkgindex = binpkg.GrabLocalPackageIndex(packages_dir)
  with open(packages_file, 'w') as p:
    pkgindex.Write(p)
