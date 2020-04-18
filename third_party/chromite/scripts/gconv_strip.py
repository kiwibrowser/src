# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to remove unused gconv charset modules from a build."""

from __future__ import print_function

import ahocorasick
import glob
import lddtree
import operator
import os
import stat

from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import osutils


# Path pattern to search for the gconv-modules file.
GCONV_MODULES_PATH = 'usr/*/gconv/gconv-modules'

# Sticky modules. These charsets modules are always included even if they
# aren't used. You can specify any charset name as supported by 'iconv_open',
# for example, 'LATIN1' or 'ISO-8859-1'.
STICKY_MODULES = ('UTF-16', 'UTF-32', 'UNICODE')

# List of function names (symbols) known to use a charset as a parameter.
GCONV_SYMBOLS = (
    # glibc
    'iconv_open',
    'iconv',
    # glib
    'g_convert',
    'g_convert_with_fallback',
    'g_iconv',
    'g_locale_to_utf8',
    'g_get_charset',
)


class GconvModules(object):
  """Class to manipulate the gconv/gconv-modules file and referenced modules.

  This class parses the contents of the gconv-modules file installed by glibc
  which provides the definition of the charsets supported by iconv_open(3). It
  allows to load the current gconv-modules file and rewrite it to include only
  a subset of the supported modules, removing the other modules.

  Each charset is involved on some transformation between that charset and an
  internal representation. This transformation is defined on a .so file loaded
  dynamically with dlopen(3) when the charset defined in this file is requested
  to iconv_open(3).

  See the comments on gconv-modules file for syntax details.
  """

  def __init__(self, gconv_modules_file):
    """Initialize the class.

    Args:
      gconv_modules_file: Path to gconv/gconv-modules file.
    """
    self._filename = gconv_modules_file

    # An alias map of charsets. The key (fromcharset) is the alias name and
    # the value (tocharset) is the real charset name. We also support a value
    # that is an alias for another charset.
    self._alias = {}

    # The modules dict goes from charset to module names (the filenames without
    # the .so extension). Since several transformations involving the same
    # charset could be defined in different files, the values of this dict are
    # a set of module names.
    self._modules = {}

  def Load(self):
    """Load the charsets from gconv-modules."""
    for line in open(self._filename):
      line = line.split('#', 1)[0].strip()
      if not line: # Comment
        continue

      lst = line.split()
      if lst[0] == 'module':
        _, fromset, toset, filename = lst[:4]
        for charset in (fromset, toset):
          charset = charset.rstrip('/')
          mods = self._modules.get(charset, set())
          mods.add(filename)
          self._modules[charset] = mods
      elif lst[0] == 'alias':
        _, fromset, toset = lst
        fromset = fromset.rstrip('/')
        toset = toset.rstrip('/')
        # Warn if the same charset is defined as two different aliases.
        if self._alias.get(fromset, toset) != toset:
          logging.error('charset "%s" already defined as "%s".', fromset,
                        self._alias[fromset])
        self._alias[fromset] = toset
      else:
        cros_build_lib.Die('Unknown line: %s', line)

    logging.debug('Found %d modules and %d alias in %s', len(self._modules),
                  len(self._alias), self._filename)
    charsets = sorted(self._alias.keys() + self._modules.keys())
    # Remove the 'INTERNAL' charset from the list, since it is not a charset
    # but an internal representation used to convert to and from other charsets.
    if 'INTERNAL' in charsets:
      charsets.remove('INTERNAL')
    return charsets

  def Rewrite(self, used_charsets, dry_run=False):
    """Rewrite gconv-modules file with only the used charsets.

    Args:
      used_charsets: A list of used charsets. This should be a subset of the
                     list returned by Load().
      dry_run: Whether this function should not change any file.
    """

    # Compute the used modules.
    used_modules = set()
    for charset in used_charsets:
      while charset in self._alias:
        charset = self._alias[charset]
      used_modules.update(self._modules[charset])
    unused_modules = reduce(set.union, self._modules.values()) - used_modules

    modules_dir = os.path.dirname(self._filename)

    all_modules = set.union(used_modules, unused_modules)
    # The list of charsets that depend on a given library. For example,
    # libdeps['libCNS.so'] is the set of all the modules that require that
    # library. These libraries live in the same directory as the modules.
    libdeps = {}
    for module in all_modules:
      deps = lddtree.ParseELF(os.path.join(modules_dir, '%s.so' % module),
                              modules_dir, [])
      if not 'needed' in deps:
        continue
      for lib in deps['needed']:
        # Ignore the libs without a path defined (outside the modules_dir).
        if deps['libs'][lib]['path']:
          libdeps[lib] = libdeps.get(lib, set()).union([module])

    used_libdeps = set(lib for lib, deps in libdeps.iteritems()
                       if deps.intersection(used_modules))
    unused_libdeps = set(libdeps).difference(used_libdeps)

    logging.debug('Used modules: %s', ', '.join(sorted(used_modules)))
    logging.debug('Used dependency libs: %s, '.join(sorted(used_libdeps)))

    unused_size = 0
    for module in sorted(unused_modules):
      module_path = os.path.join(modules_dir, '%s.so' % module)
      unused_size += os.lstat(module_path).st_size
      logging.debug('rm %s', module_path)
      if not dry_run:
        os.unlink(module_path)

    unused_libdeps_size = 0
    for lib in sorted(unused_libdeps):
      lib_path = os.path.join(modules_dir, lib)
      unused_libdeps_size += os.lstat(lib_path).st_size
      logging.debug('rm %s', lib_path)
      if not dry_run:
        os.unlink(lib_path)

    logging.info('Done. Using %d gconv modules. Removed %d unused modules'
                 ' (%.1f KiB) and %d unused dependencies (%.1f KiB)',
                 len(used_modules), len(unused_modules), unused_size / 1024.,
                 len(unused_libdeps), unused_libdeps_size / 1024.)

    # Recompute the gconv-modules file with only the included gconv modules.
    result = []
    for line in open(self._filename):
      lst = line.split('#', 1)[0].strip().split()

      if not lst:
        result.append(line)  # Keep comments and copyright headers.
      elif lst[0] == 'module':
        _, _, _, filename = lst[:4]
        if filename in used_modules:
          result.append(line)  # Used module
      elif lst[0] == 'alias':
        _, charset, _ = lst
        charset = charset.rstrip('/')
        while charset in self._alias:
          charset = self._alias[charset]
        if used_modules.intersection(self._modules[charset]):
          result.append(line)  # Alias to an used module
      else:
        cros_build_lib.Die('Unknown line: %s', line)

    if not dry_run:
      osutils.WriteFile(self._filename, ''.join(result))


def MultipleStringMatch(patterns, corpus):
  """Search a list of strings in a corpus string.

  Args:
    patterns: A list of strings.
    corpus: The text where to search for the strings.

  Returns:
    A list of Booleans stating whether each pattern string was found in the
    corpus or not.
  """
  tree = ahocorasick.KeywordTree()
  for word in patterns:
    tree.add(word)
  tree.make()

  result = [False] * len(patterns)
  for i, j in tree.findall(corpus):
    match = corpus[i:j]
    result[patterns.index(match)] = True

  return result


def GconvStrip(opts):
  """Process gconv-modules and remove unused modules.

  Args:
    opts: The command-line args passed to the script.

  Returns:
    The exit code number indicating whether the process succeeded.
  """
  root_st = os.lstat(opts.root)
  if not stat.S_ISDIR(root_st.st_mode):
    cros_build_lib.Die('root (%s) must be a directory.' % opts.root)

  # Detect the possible locations of the gconv-modules file.
  gconv_modules_files = glob.glob(os.path.join(opts.root, GCONV_MODULES_PATH))

  if not gconv_modules_files:
    logging.warning('gconv-modules file not found.')
    return 1

  # Only one gconv-modules files should be present, either on /usr/lib or
  # /usr/lib64, but not both.
  if len(gconv_modules_files) > 1:
    cros_build_lib.Die('Found several gconv-modules files.')

  gconv_modules_file = gconv_modules_files[0]
  logging.info('Searching for unused gconv files defined in %s',
               gconv_modules_file)

  gmods = GconvModules(gconv_modules_file)
  charsets = gmods.Load()

  # Use scanelf to search for all the binary files on the rootfs that require
  # or define the symbol iconv_open. We also include the binaries that define
  # it since there could be internal calls to it from other functions.
  files = set()
  for symbol in GCONV_SYMBOLS:
    cmd = ['scanelf', '--mount', '--quiet', '--recursive', '--format', '#s%F',
           '--symbol', symbol, opts.root]
    result = cros_build_lib.RunCommand(cmd, redirect_stdout=True,
                                       print_cmd=False)
    symbol_files = result.output.splitlines()
    logging.debug('Symbol %s found on %d files.', symbol, len(symbol_files))
    files.update(symbol_files)

  # The charsets are represented as nul-terminated strings in the binary files,
  # so we append the '\0' to each string. This prevents some false positives
  # when the name of the charset is a substring of some other string. It doesn't
  # prevent false positives when the charset name is the suffix of another
  # string, for example a binary with the string "DON'T DO IT\0" will match the
  # 'IT' charset. Empirical test on ChromeOS images suggests that only 4
  # charsets could fall in category.
  strings = [s + '\0' for s in charsets]
  logging.info('Will search for %d strings in %d files', len(strings),
               len(files))

  # Charsets listed in STICKY_MOUDLES are initialized as used. Note that those
  # strings should be listed in the gconv-modules file.
  unknown_sticky_modules = set(STICKY_MODULES) - set(charsets)
  if unknown_sticky_modules:
    logging.warning(
        'The following charsets were explicitly requested in STICKY_MODULES '
        'even though they don\'t exist: %s',
        ', '.join(unknown_sticky_modules))
  global_used = [charset in STICKY_MODULES for charset in charsets]

  for filename in files:
    used_filename = MultipleStringMatch(strings,
                                        osutils.ReadFile(filename, mode='rb'))

    global_used = map(operator.or_, global_used, used_filename)
    # Check the debug flag to avoid running an useless loop.
    if opts.debug and any(used_filename):
      logging.debug('File %s:', filename)
      for i in range(len(used_filename)):
        if used_filename[i]:
          logging.debug(' - %s', strings[i])

  used_charsets = [cs for cs, used in zip(charsets, global_used) if used]
  gmods.Rewrite(used_charsets, opts.dry_run)
  return 0


def ParseArgs(argv):
  """Return parsed commandline arguments."""

  parser = commandline.ArgumentParser()
  parser.add_argument(
      '--dry-run', action='store_true', default=False,
      help='process but don\'t modify any file.')
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

  return GconvStrip(opts)
