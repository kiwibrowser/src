# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Collection of tests to run on the rootfs of a built image.

This module should only be imported inside the chroot.
"""

from __future__ import print_function

import cStringIO
import collections
import itertools
import lddtree
import magic
import mimetypes
import os
import re
import stat
import unittest

from elftools.elf import elffile
from elftools.common import exceptions

from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import filetype
from chromite.lib import image_test_lib
from chromite.lib import osutils
from chromite.lib import parseelf

from chromite.cros.test import usergroup_baseline


class LocaltimeTest(image_test_lib.ImageTestCase):
  """Verify that /etc/localtime is a symlink to /var/lib/timezone/localtime.

  This is an example of an image test. The image is already mounted. The
  test can access rootfs via ROOT_A constant.
  """

  def TestLocaltimeIsSymlink(self):
    localtime_path = os.path.join(image_test_lib.ROOT_A, 'etc', 'localtime')
    self.assertTrue(os.path.islink(localtime_path))

  def TestLocaltimeLinkIsCorrect(self):
    localtime_path = os.path.join(image_test_lib.ROOT_A, 'etc', 'localtime')
    self.assertEqual('/var/lib/timezone/localtime',
                     os.readlink(localtime_path))


def _GuessMimeType(magic_obj, file_name):
  """Guess a file's mimetype base on its extension and content.

  File extension is favored over file content to reduce noise.

  Args:
    magic_obj: A loaded magic instance.
    file_name: A path to the file.

  Returns:
    A mime type of |file_name|.
  """
  mime_type, _ = mimetypes.guess_type(file_name)
  if not mime_type:
    mime_type = magic_obj.file(file_name)
  return mime_type


class BlacklistTest(image_test_lib.ImageTestCase):
  """Verify that rootfs does not contain blacklisted items."""

  def TestBlacklistedDirectories(self):
    dirs = [os.path.join(image_test_lib.ROOT_A, 'usr', 'share', 'locale')]
    for d in dirs:
      self.assertFalse(os.path.isdir(d), 'Directory %s is blacklisted.' % d)

  def TestBlacklistedFileTypes(self):
    """Fail if there are files of prohibited types (such as C++ source code).

    The whitelist has higher precedence than the blacklist.
    """
    blacklisted_patterns = [re.compile(x) for x in [
        r'text/x-c\+\+',
        r'text/x-c',
    ]]
    whitelisted_patterns = [re.compile(x) for x in [
        r'.*/braille/.*',
        r'.*/brltty/.*',
        r'.*/etc/sudoers$',
        r'.*/dump_vpd_log$',
        r'.*\.conf$',
        r'.*/libnl/classid$',
        r'.*/locale/',
        r'.*/X11/xkb/',
        r'.*/chromeos-assets/',
        r'.*/udev/rules.d/',
        r'.*/firmware/ar3k/.*pst$',
        r'.*/etc/services',
        # Python reads this file at runtime to look up install features.
        r'.*/usr/include/python[\d\.]*/pyconfig.h$',
        r'.*/usr/share/dev-install/portage',
    ]]

    failures = []

    magic_obj = magic.open(magic.MAGIC_MIME_TYPE)
    magic_obj.load()
    for root, _, file_names in os.walk(image_test_lib.ROOT_A):
      for file_name in file_names:
        full_name = os.path.join(root, file_name)
        if os.path.islink(full_name) or not os.path.isfile(full_name):
          continue

        mime_type = _GuessMimeType(magic_obj, full_name)
        if (any(x.match(mime_type) for x in blacklisted_patterns) and not
            any(x.match(full_name) for x in whitelisted_patterns)):
          failures.append('File %s has blacklisted type %s.' %
                          (full_name, mime_type))
    magic_obj.close()

    self.assertFalse(failures, '\n'.join(failures))

  def TestValidInterpreter(self):
    """Fail if a script's interpreter is not found, or not executable.

    A script interpreter is anything after the #! sign, up to the end of line
    or the first space.
    """
    failures = []

    for root, _, file_names in os.walk(image_test_lib.ROOT_A):
      for file_name in file_names:
        full_name = os.path.join(root, file_name)
        file_stat = os.lstat(full_name)
        if (not stat.S_ISREG(file_stat.st_mode) or
            (file_stat.st_mode & 0111) == 0):
          continue

        with open(full_name, 'rb') as f:
          if f.read(2) != '#!':
            continue
          line = '#!' + f.readline().strip()

        try:
          # Ignore arguments to the interpreter.
          interp, _ = filetype.SplitShebang(line)
        except ValueError:
          failures.append('File %s has an invalid interpreter path: "%s".' %
                          (full_name, line))

        # Absolute path to the interpreter.
        interp = os.path.join(image_test_lib.ROOT_A, interp.lstrip('/'))
        # Interpreter could be a symlink. Resolve it.
        interp = osutils.ResolveSymlink(interp, image_test_lib.ROOT_A)
        if not os.path.isfile(interp):
          failures.append('File %s uses non-existing interpreter %s.' %
                          (full_name, interp))
        elif (os.stat(interp).st_mode & 0111) == 0:
          failures.append('Interpreter %s is not executable.' % interp)

    self.assertFalse(failures, '\n'.join(failures))


class LinkageTest(image_test_lib.ImageTestCase):
  """Verify that all binaries and libraries have proper linkage."""

  def setUp(self):
    osutils.MountDir(
        os.path.join(image_test_lib.STATEFUL, 'var_overlay'),
        os.path.join(image_test_lib.ROOT_A, 'var'),
        mount_opts=('bind', ),
    )

  def tearDown(self):
    osutils.UmountDir(
        os.path.join(image_test_lib.ROOT_A, 'var'),
        cleanup=False,
    )

  def _IsPackageMerged(self, package_name):
    cmd = [
        'portageq',
        'has_version',
        image_test_lib.ROOT_A,
        package_name
    ]
    ret = cros_build_lib.RunCommand(cmd, error_code_ok=True,
                                    combine_stdout_stderr=True,
                                    extra_env={'ROOT': image_test_lib.ROOT_A})
    if ret.returncode == 0:
      logging.info('Package is available: %s', package_name)
    else:
      logging.info('Package is not available: %s', package_name)
    return ret.returncode == 0

  def TestLinkage(self):
    """Find main executable binaries and check their linkage."""
    binaries = [
        'bin/sed',
    ]

    if self._IsPackageMerged('chromeos-base/chromeos-login'):
      binaries.append('sbin/session_manager')

    if self._IsPackageMerged('x11-base/xorg-server'):
      binaries.append('usr/bin/Xorg')

    # When chrome is built with USE="pgo_generate", rootfs chrome is actually a
    # symlink to a real binary which is in the stateful partition. So we do not
    # check for a valid chrome binary in that case.
    if not self._IsPackageMerged('chromeos-base/chromeos-chrome[pgo_generate]'):
      if self._IsPackageMerged('chromeos-base/chromeos-chrome[app_shell]'):
        binaries.append('opt/google/chrome/app_shell')
      elif self._IsPackageMerged('chromeos-base/chromeos-chrome'):
        binaries.append('opt/google/chrome/chrome')

    binaries = [os.path.join(image_test_lib.ROOT_A, x) for x in binaries]

    # Grab all .so files
    libraries = []
    for root, _, files in os.walk(image_test_lib.ROOT_A):
      for name in files:
        filename = os.path.join(root, name)
        if '.so' in filename:
          libraries.append(filename)

    ldpaths = lddtree.LoadLdpaths(image_test_lib.ROOT_A)
    for to_test in itertools.chain(binaries, libraries):
      # to_test could be a symlink, we need to resolve it relative to ROOT_A.
      while os.path.islink(to_test):
        link = os.readlink(to_test)
        if link.startswith('/'):
          to_test = os.path.join(image_test_lib.ROOT_A, link[1:])
        else:
          to_test = os.path.join(os.path.dirname(to_test), link)
      try:
        lddtree.ParseELF(to_test, root=image_test_lib.ROOT_A, ldpaths=ldpaths)
      except lddtree.exceptions.ELFError:
        continue
      except IOError as e:
        self.fail('Fail linkage test for %s: %s' % (to_test, e))


@unittest.expectedFailure
class FileSystemMetaDataTest(image_test_lib.ImageTestCase):
  """A test class to gather file system stats such as free inodes, blocks."""

  def TestStats(self):
    """Collect inodes and blocks usage."""
    # Find the loopback device that was mounted to ROOT_A.
    loop_device = None
    root_path = os.path.abspath(os.readlink(image_test_lib.ROOT_A))
    for mtab in osutils.IterateMountPoints():
      if mtab.destination == root_path:
        loop_device = mtab.source
        break
    self.assertTrue(loop_device, 'Cannot find loopback device for ROOT_A.')

    # Gather file system stats with tune2fs.
    cmd = [
        'tune2fs',
        '-l',
        loop_device
    ]
    # tune2fs produces output like this:
    #
    # tune2fs 1.42 (29-Nov-2011)
    # Filesystem volume name:   ROOT-A
    # Last mounted on:          <not available>
    # Filesystem UUID:          <none>
    # Filesystem magic number:  0xEF53
    # Filesystem revision #:    1 (dynamic)
    # ...
    #
    # So we need to ignore the first line.
    ret = cros_build_lib.SudoRunCommand(cmd, capture_output=True,
                                        extra_env={'LC_ALL': 'C'})
    fs_stat = dict(line.split(':', 1) for line in ret.output.splitlines()
                   if ':' in line)
    free_inodes = int(fs_stat['Free inodes'])
    free_blocks = int(fs_stat['Free blocks'])
    inode_count = int(fs_stat['Inode count'])
    block_count = int(fs_stat['Block count'])
    block_size = int(fs_stat['Block size'])

    sum_file_size = 0
    for root, _, filenames in os.walk(image_test_lib.ROOT_A):
      for file_name in filenames:
        full_name = os.path.join(root, file_name)
        file_stat = os.lstat(full_name)
        sum_file_size += file_stat.st_size

    metadata_size = (block_count - free_blocks) * block_size - sum_file_size

    self.OutputPerfValue('free_inodes_over_inode_count',
                         free_inodes * 100.0 / inode_count, 'percent',
                         graph='free_over_used_ratio')
    self.OutputPerfValue('free_blocks_over_block_count',
                         free_blocks * 100.0 / block_count, 'percent',
                         graph='free_over_used_ratio')
    self.OutputPerfValue('apparent_size', sum_file_size, 'bytes',
                         higher_is_better=False, graph='filesystem_stats')
    self.OutputPerfValue('metadata_size', metadata_size, 'bytes',
                         higher_is_better=False, graph='filesystem_stats')


class SymbolsTest(image_test_lib.ImageTestCase):
  """Tests related to symbols in ELF files."""

  def setUp(self):
    # Mapping of file name --> 2-tuple (import, export).
    self._known_symtabs = {}

  def _GetSymbols(self, file_name):
    """Return a 2-tuple (import, export) of an ELF file |file_name|.

    Import and export in the returned tuple are sets of strings (symbol names).
    """
    if file_name in self._known_symtabs:
      return self._known_symtabs[file_name]

    # We use cstringio here to obviate fseek/fread time in pyelftools.
    stream = cStringIO.StringIO(osutils.ReadFile(file_name))

    try:
      elf = elffile.ELFFile(stream)
    except exceptions.ELFError:
      raise ValueError('%s is not an ELF file.' % file_name)

    imp, exp = parseelf.ParseELFSymbols(elf)
    self._known_symtabs[file_name] = imp, exp
    return imp, exp

  def TestImportedSymbolsAreAvailable(self):
    """Ensure all ELF files' imported symbols are available in ROOT-A.

    In this test, we find all imported symbols and exported symbols from all
    ELF files on the system. This test will fail if the set of imported symbols
    is not a subset of exported symbols.

    This test DOES NOT simulate ELF loading. "TestLinkage" does that with
    `lddtree`.
    """
    # Import tables of files, keyed by file names.
    importeds = collections.defaultdict(set)
    # All exported symbols.
    exported = set()

    for root, _, filenames in os.walk(image_test_lib.ROOT_A):
      for filename in filenames:
        full_name = os.path.join(root, filename)
        if os.path.islink(full_name) or not os.path.isfile(full_name):
          continue

        try:
          imp, exp = self._GetSymbols(full_name)
        except (ValueError, IOError):
          continue
        else:
          importeds[full_name] = imp
          exported.update(exp)

    known_unsatisfieds = {
        'libthread_db-1.0.so': set([
            'ps_pdwrite', 'ps_pdread', 'ps_lgetfpregs', 'ps_lsetregs',
            'ps_lgetregs', 'ps_lsetfpregs', 'ps_pglobal_lookup', 'ps_getpid']),
    }

    failures = []
    for full_name, imported in importeds.iteritems():
      file_name = os.path.basename(full_name)
      missing = imported - exported - known_unsatisfieds.get(file_name, set())
      if missing:
        failures.append('File %s contains unsatisfied symbols: %r' %
                        (full_name, missing))
    self.assertFalse(failures, '\n'.join(failures))


class UserGroupTest(image_test_lib.ImageTestCase):
  """Tests users and groups in /etc/passwd and /etc/group."""

  @staticmethod
  def _validate_passwd(entry):
    """Check users that are not in the baseline.

    The user ID should match the group ID, and the user's home directory
    and shell should be invalid.
    """
    uid = entry.uid
    gid = entry.gid

    if uid != gid:
      logging.error('New user "%s" has uid %d and different gid %d',
                    entry.user, uid, gid)
      return False

    if entry.home != '/dev/null':
      logging.error('Expected /dev/null for new user "%s" home dir, got "%s"',
                    entry.user, entry.home)
      return False

    if entry.shell != '/bin/false':
      logging.error('Expected /bin/false for new user "%s" shell, got "%s"',
                    entry.user, entry.shell)
      return False

    return True

  @staticmethod
  def _validate_group(entry):
    """Check groups that are not in the baseline.

    Allow groups that have no users and groups with only the matching user.
    """
    group_name = entry.group
    users = entry.users

    # Groups with no users and groups with only the matching user are OK.
    if not users or users == {group_name}:
      return True

    logging.error('New group "%s" has users "%s"', group_name, users)
    return False

  @staticmethod
  def _match_passwd(expected, actual):
    """Match password, uid, gid, home, and shell."""
    matched = True

    if expected.encpasswd != actual.encpasswd:
      matched = False
      logging.error('Expected encrypted password "%s" for user "%s", got "%s".',
                    expected.encpasswd, expected.user, actual.encpasswd)

    if expected.uid != actual.uid:
      matched = False
      logging.error('Expected uid %d for user "%s", got %d.',
                    expected.uid, expected.user, actual.uid)

    if expected.gid != actual.gid:
      matched = False
      logging.error('Expected gid %d for user "%s", got %d.',
                    expected.gid, expected.user, actual.gid)

    if isinstance(expected.home, set):
      valid_home = actual.home in expected.home
    else:
      valid_home = actual.home == expected.home
    if not valid_home:
      matched = False
      logging.error('Expected home "%s" for user "%s", got "%s".',
                    expected.home, expected.user, actual.home)

    if isinstance(expected.shell, set):
      valid_shell = actual.shell in expected.shell
    else:
      valid_shell = actual.shell == expected.shell
    if not valid_shell:
      matched = False
      logging.error('Expected shell "%s" for user "%s", got "%s".',
                    expected.shell, expected.user, actual.shell)

    return matched

  @staticmethod
  def _match_group(expected, actual):
    """Match password, gid, and members."""
    matched = True

    if expected.encpasswd != actual.encpasswd:
      matched = False
      logging.error(
          'Expected encrypted password "%s" for group "%s", got "%s".',
          expected.encpasswd, expected.group, actual.encpasswd)

    if expected.gid != actual.gid:
      matched = False
      logging.error('Expected gid %d for group "%s", got %d.',
                    expected.gid, expected.group, actual.gid)

    if expected.users != actual.users:
      matched = False
      logging.error('Expected members "%s" for group "%s", got "%s".',
                    expected.users, expected.group, actual.users)

    return matched

  def _LoadPath(self, path):
    """Load the given passwd/group file.

    Args:
      path: Path to the file.

    Returns:
      A dict of passwd/group entries indexed by account name.
    """
    d = {}
    for line in osutils.ReadFile(path).splitlines():
      fields = line.split(':')
      if len(fields) == 7:
        # wpa:!:219:219::/dev/null:/bin/false
        entry = usergroup_baseline.UserEntry(user=fields[0],
                                             encpasswd=fields[1],
                                             uid=int(fields[2]),
                                             gid=int(fields[3]),
                                             home=fields[5],
                                             shell=fields[6])
        d[entry.user] = entry
      elif len(fields) == 4:
        # tty:!:5:power,brltty
        users = set()
        if fields[3]:
          users = set(fields[3].split(','))
        entry = usergroup_baseline.GroupEntry(group=fields[0],
                                              encpasswd=fields[1],
                                              gid=int(fields[2]),
                                              users=users)
        d[entry.group] = entry
      else:
        raise ValueError('Invalid baseline format "%s"' % line)

    return d

  def _LoadBaseline(self, basename):
    """Loads the passwd or group baseline."""
    d = None
    if 'passwd' in basename:
      d = usergroup_baseline.USER_BASELINE.copy()

      # Per-board baseline.
      if self._board and self._board in usergroup_baseline.USER_BOARD_BASELINES:
        d.update(usergroup_baseline.USER_BOARD_BASELINES[self._board])
    elif 'group' in basename:
      d = usergroup_baseline.GROUP_BASELINE.copy()
      # TODO(jorgelo): Merge this into the main baseline once:
      #     *Freon users are included in the main overlay.
      d.update(usergroup_baseline.GROUP_BASELINE_FREON)

      # Per-board baseline.
      if (self._board and
          self._board in usergroup_baseline.GROUP_BOARD_BASELINES):
        d.update(usergroup_baseline.GROUP_BOARD_BASELINES[self._board])
    else:
      raise ValueError('Invalid basename "%s"' % basename)

    return d

  def _CheckFile(self, basename):
    """Validates the passwd or group file."""
    match_func = getattr(self, '_match_%s' % basename)
    validate_func = getattr(self, '_validate_%s' % basename)

    expected_entries = self._LoadBaseline(basename)
    actual_entries = self._LoadPath(os.path.join(image_test_lib.ROOT_A,
                                                 'etc',
                                                 basename))

    success = True
    for entry, details in actual_entries.iteritems():
      if entry not in expected_entries:
        is_valid = validate_func(details)
        if not is_valid:
          logging.error('Unexpected %s entry for "%s".', basename, entry)

        success = success and is_valid
        continue

      expected = expected_entries[entry]
      match_res = match_func(expected, details)
      success = success and match_res

    missing = set(expected_entries.keys()) - set(actual_entries.keys())
    for m in missing:
      logging.info('Ignoring missing %s entry for "%s".', basename, m)

    self.assertTrue(success)

  def TestUsers(self):
    """Enforces a whitelist of known user IDs."""
    self._CheckFile('passwd')

  def TestGroups(self):
    """Enforces a whitelist of known group IDs."""
    self._CheckFile('group')


class CroshTest(image_test_lib.ImageTestCase):
  """Check crosh code."""

  # Base directory for crosh code.
  CROSH_DIR = 'usr/share/crosh'

  def TestUnknownModules(self):
    """Only permit a whitelist of known, allowed crosh modules on the system."""
    # Do *not* add modules to this list until they've been reviewed by security
    # or someone in the crosh/OWNERS list.  Insecure code here can easily cause
    # compromise of CrOS system security in verified mode.  It has happened.
    WHITELIST = {
        'dev.d': {'50-crosh.sh'},
        'extra.d': {'30-cups.sh'},
        'removable.d': {'50-crosh.sh'},
    }

    base_path = os.path.join(image_test_lib.ROOT_A, self.CROSH_DIR)
    for mod_dir, good_modules in WHITELIST.items():
      mod_path = os.path.join(base_path, mod_dir)
      if not os.path.exists(mod_path):
        continue

      found_modules = set(os.listdir(mod_path))
      unknown_modules = found_modules - good_modules
      self.assertEqual(set(), unknown_modules)
