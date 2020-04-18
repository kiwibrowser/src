# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Library containing utility functions used for Chrome-specific build tasks."""

from __future__ import print_function

import functools
import glob
import os
import re
import shlex
import shutil

from chromite.lib import failures_lib
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import osutils


# Taken from external/gyp.git/pylib.
def _NameValueListToDict(name_value_list):
  """Converts Name-Value list to dictionary.

  Takes an array of strings of the form 'NAME=VALUE' and creates a dictionary
  of the pairs.  If a string is simply NAME, then the value in the dictionary
  is set to True.  If VALUE can be converted to an integer, it is.
  """
  result = {}
  for item in name_value_list:
    tokens = item.split('=', 1)
    if len(tokens) == 2:
      # If we can make it an int, use that, otherwise, use the string.
      try:
        token_value = int(tokens[1])
      except ValueError:
        token_value = tokens[1]
      # Set the variable to the supplied value.
      result[tokens[0]] = token_value
    else:
      # No value supplied, treat it as a boolean and set it.
      result[tokens[0]] = True
  return result


def ProcessShellFlags(defines):
  """Validate and convert a string of shell style flags to a dictionary."""
  assert defines is not None
  return _NameValueListToDict(shlex.split(defines))


class Conditions(object):
  """Functions that return conditions used to construct Path objects.

  Condition functions returned by the public methods have signature
  f(gn_args, staging_flags). For descriptions of gn_args and
  staging_flags see docstring for StageChromeFromBuildDir().
  """

  @classmethod
  def _GnSetTo(cls, flag, value, gn_args, _staging_flags):
    val = gn_args.get(flag)
    return val == value

  @classmethod
  def _StagingFlagSet(cls, flag, _gn_args, staging_flags):
    return flag in staging_flags

  @classmethod
  def _StagingFlagNotSet(cls, flag, gn_args, staging_flags):
    return not cls._StagingFlagSet(flag, gn_args, staging_flags)

  @classmethod
  def GnSetTo(cls, flag, value):
    """Returns condition that tests a gn flag is set to a value."""
    return functools.partial(cls._GnSetTo, flag, value)

  @classmethod
  def StagingFlagSet(cls, flag):
    """Returns condition that tests a staging_flag is set."""
    return functools.partial(cls._StagingFlagSet, flag)

  @classmethod
  def StagingFlagNotSet(cls, flag):
    """Returns condition that tests a staging_flag is not set."""
    return functools.partial(cls._StagingFlagNotSet, flag)


class MultipleMatchError(failures_lib.StepFailure):
  """A glob pattern matches multiple files but a non-dir dest was specified."""


class MissingPathError(failures_lib.StepFailure):
  """An expected path is non-existant."""


class MustNotBeDirError(failures_lib.StepFailure):
  """The specified path should not be a directory, but is."""


class Copier(object):
  """File/directory copier.

  Provides destination stripping and permission setting functionality.
  """

  def __init__(self, strip_bin=None, strip_flags=None, default_mode=0o644,
               dir_mode=0o755, exe_mode=0o755):
    """Initialization.

    Args:
      strip_bin: Path to the program used to strip binaries.  If set to None,
                 binaries will not be stripped.
      strip_flags: A list of flags to pass to the |strip_bin| executable.
      default_mode: Default permissions to set on files.
      dir_mode: Mode to set for directories.
      exe_mode: Permissions to set on executables.
    """
    self.strip_bin = strip_bin
    self.strip_flags = strip_flags
    self.default_mode = default_mode
    self.dir_mode = dir_mode
    self.exe_mode = exe_mode

  @staticmethod
  def Log(src, dest, directory):
    sep = ' [d] -> ' if directory else ' -> '
    logging.debug('%s %s %s', src, sep, dest)

  def _CopyFile(self, src, dest, path):
    """Perform the copy.

    Args:
      src: The path of the file/directory to copy.
      dest: The exact path of the destination. Does nothing if it already
            exists.
      path: The Path instance containing copy operation modifiers (such as
            Path.exe, Path.strip, etc.)
    """
    assert not os.path.isdir(src), '%s: Not expecting a directory!' % src

    # This file has already been copied by an earlier Path.
    if os.path.exists(dest):
      return

    osutils.SafeMakedirs(os.path.dirname(dest), mode=self.dir_mode)
    if path.exe and self.strip_bin and path.strip and os.path.getsize(src) > 0:
      strip_flags = (['--strip-unneeded'] if self.strip_flags is None else
                     self.strip_flags)
      cros_build_lib.DebugRunCommand(
          [self.strip_bin] + strip_flags + ['-o', dest, src])
      shutil.copystat(src, dest)
    else:
      shutil.copy2(src, dest)

    mode = path.mode
    if mode is None:
      mode = self.exe_mode if path.exe else self.default_mode
    os.chmod(dest, mode)

  def Copy(self, src_base, dest_base, path, sloppy=False):
    """Copy artifact(s) from source directory to destination.

    Args:
      src_base: The directory to apply the src glob pattern match in.
      dest_base: The directory to copy matched files to.  |Path.dest|.
      path: A Path instance that specifies what is to be copied.
      sloppy: If set, ignore when mandatory artifacts are missing.

    Returns:
      A list of the artifacts copied.
    """
    copied_paths = []
    src = os.path.join(src_base, path.src)
    if not src.endswith('/') and os.path.isdir(src):
      raise MustNotBeDirError('%s must not be a directory\n'
                              'Aborting copy...' % (src,))
    paths = glob.glob(src)
    if not paths:
      if path.optional:
        logging.debug('%s does not exist and is optional.  Skipping.', src)
      elif sloppy:
        logging.warning('%s does not exist and is required.  Skipping anyway.',
                        src)
      else:
        msg = ('%s does not exist and is required.\n'
               'You can bypass this error with --sloppy.\n'
               'Aborting copy...' % src)
        raise MissingPathError(msg)
    elif len(paths) > 1 and path.dest and not path.dest.endswith('/'):
      raise MultipleMatchError(
          'Glob pattern %r has multiple matches, but dest %s '
          'is not a directory.\n'
          'Aborting copy...' % (path.src, path.dest))
    else:
      for p in paths:
        rel_src = os.path.relpath(p, src_base)
        if path.IsBlacklisted(rel_src):
          continue
        if path.dest is None:
          rel_dest = rel_src
        elif path.dest.endswith('/'):
          rel_dest = os.path.join(path.dest, os.path.basename(p))
        else:
          rel_dest = path.dest
        assert not rel_dest.endswith('/')
        dest = os.path.join(dest_base, rel_dest)

        copied_paths.append(p)
        self.Log(p, dest, os.path.isdir(p))
        if os.path.isdir(p):
          for sub_path in osutils.DirectoryIterator(p):
            rel_path = os.path.relpath(sub_path, p)
            sub_dest = os.path.join(dest, rel_path)
            if path.IsBlacklisted(rel_path):
              continue
            if sub_path.endswith('/'):
              osutils.SafeMakedirs(sub_dest, mode=self.dir_mode)
            else:
              self._CopyFile(sub_path, sub_dest, path)
        else:
          self._CopyFile(p, dest, path)

    return copied_paths


class Path(object):
  """Represents an artifact to be copied from build dir to staging dir."""

  DEFAULT_BLACKLIST = (r'(^|.*/)\.svn($|/.*)',)

  def __init__(self, src, exe=False, cond=None, dest=None, mode=None,
               optional=False, strip=True, blacklist=None):
    """Initializes the object.

    Args:
      src: The relative path of the artifact.  Can be a file or a directory.
           Can be a glob pattern.
      exe: Identifes the path as either being an executable or containing
           executables.  Executables may be stripped during copy, and have
           special permissions set.  We currently only support stripping of
           specified files and glob patterns that return files.  If |src| is a
           directory or contains directories, the content of the directory will
           not be stripped.
      cond: A condition (see Conditions class) to test for in deciding whether
            to process this artifact.
      dest: Name to give to the target file/directory.  Defaults to keeping the
            same name as the source.
      mode: The mode to set for the matched files, and the contents of matched
            directories.
      optional: Whether to enforce the existence of the artifact.  If unset, the
                script errors out if the artifact does not exist.  In 'sloppy'
                mode, the Copier class treats all artifacts as optional.
      strip: If |exe| is set, whether to strip the executable.
      blacklist: A list of path patterns to ignore during the copy. This gets
                 added to a default blacklist pattern.
    """
    self.src = src
    self.exe = exe
    self.cond = cond
    self.dest = dest
    self.mode = mode
    self.optional = optional
    self.strip = strip
    self.blacklist = self.DEFAULT_BLACKLIST
    if blacklist is not None:
      self.blacklist += tuple(blacklist)

  def IsBlacklisted(self, path):
    """Returns whether |path| is in the blacklist.

    A file in the blacklist is not copied over to the staging directory.

    Args:
      path: The path of a file, relative to the path of this Path object.
    """
    for pattern in self.blacklist:
      if re.match(pattern, path):
        return True
    return False

  def ShouldProcess(self, gn_args, staging_flags):
    """Tests whether this artifact should be copied."""
    if not gn_args and not staging_flags:
      return True
    if self.cond and isinstance(self.cond, list):
      for c in self.cond:
        if not c(gn_args, staging_flags):
          return False
    elif self.cond:
      return self.cond(gn_args, staging_flags)
    return True

_ENABLE_NACL = 'enable_nacl'
_IS_CHROME_BRANDED = 'is_chrome_branded'
_IS_COMPONENT_BUILD = 'is_component_build'

_HIGHDPI_FLAG = 'highdpi'
STAGING_FLAGS = (
    _HIGHDPI_FLAG,
)

_CHROME_SANDBOX_DEST = 'chrome-sandbox'
C = Conditions

# In the below Path lists, if two Paths both match a file, the earlier Path
# takes precedence.

# Files shared between all deployment types.
_COPY_PATHS_COMMON = (
    Path('chrome_sandbox', mode=0o4755, dest=_CHROME_SANDBOX_DEST),
    Path('icudtl.dat'),
    Path('libosmesa.so', exe=True, optional=True),
    # Do not strip the nacl_helper_bootstrap binary because the binutils
    # objcopy/strip mangles the ELF program headers.
    Path('nacl_helper_bootstrap',
         exe=True,
         strip=False,
         cond=C.GnSetTo(_ENABLE_NACL, True)),
    Path('nacl_irt_*.nexe', cond=C.GnSetTo(_ENABLE_NACL, True)),
    Path('nacl_helper',
         exe=True,
         optional=True,
         cond=C.GnSetTo(_ENABLE_NACL, True)),
    Path('nacl_helper_nonsfi',
         exe=True,
         optional=True,
         cond=C.GnSetTo(_ENABLE_NACL, True)),
    Path('natives_blob.bin', optional=True),
    Path('pnacl/', cond=C.GnSetTo(_ENABLE_NACL, True)),
    Path('snapshot_blob.bin', optional=True),
)

_COPY_PATHS_APP_SHELL = (
    Path('app_shell', exe=True),
    Path('extensions_shell_and_test.pak'),
) + _COPY_PATHS_COMMON

_COPY_PATHS_CHROME = (
    Path('chrome', exe=True),
    Path('chrome-wrapper'),
    Path('chrome_100_percent.pak'),
    Path('chrome_200_percent.pak', cond=C.StagingFlagSet(_HIGHDPI_FLAG)),
    Path('dbus/', optional=True),
    Path('keyboard_resources.pak'),
    Path('libassistant.so', exe=True, optional=True),
    Path('libmojo_core.so', exe=True),
    # Widevine CDM is already pre-stripped.  In addition, it doesn't
    # play well with the binutils stripping tools, so skip stripping.
    Path('libwidevinecdm.so',
         exe=True,
         strip=False,
         cond=C.GnSetTo(_IS_CHROME_BRANDED, True)),
    # In component build, copy so files (e.g. libbase.so) except for the
    # blacklist.
    Path('*.so',
         blacklist=(r'libwidevinecdm.so',),
         exe=True,
         cond=C.GnSetTo(_IS_COMPONENT_BUILD, True)),
    Path('locales/*.pak'),
    Path('Packages/chrome_content_browser/manifest.json', optional=True),
    Path('Packages/chrome_content_gpu/manifest.json', optional=True),
    Path('Packages/chrome_content_plugin/manifest.json', optional=True),
    Path('Packages/chrome_content_renderer/manifest.json', optional=True),
    Path('Packages/chrome_content_utility/manifest.json', optional=True),
    Path('Packages/chrome_mash/manifest.json', optional=True),
    Path('Packages/chrome_mash_content_browser/manifest.json', optional=True),
    Path('Packages/content_browser/manifest.json', optional=True),
    Path('resources/'),
    Path('resources.pak'),
    Path('xdg-settings'),
    Path('*.png'),
) + _COPY_PATHS_COMMON

_COPY_PATHS_MAP = {
    'app_shell': _COPY_PATHS_APP_SHELL,
    'chrome': _COPY_PATHS_CHROME,
}


def _FixPermissions(dest_base):
  """Last minute permission fixes."""
  cros_build_lib.DebugRunCommand(['chmod', '-R', 'a+r', dest_base])
  cros_build_lib.DebugRunCommand(
      ['find', dest_base, '-perm', '/110', '-exec', 'chmod', 'a+x', '{}', '+'])


def GetCopyPaths(deployment_type='chrome'):
  """Returns the list of copy paths used as a filter for staging files.

  Args:
    deployment_type: String describing the deployment type. Either "app_shell"
                     or "chrome".

  Returns:
    The list of paths to use as a filter for staging files.
  """
  paths = _COPY_PATHS_MAP.get(deployment_type)
  if paths is None:
    raise RuntimeError('Invalid deployment type "%s"' % deployment_type)
  return paths

def StageChromeFromBuildDir(staging_dir, build_dir, strip_bin, sloppy=False,
                            gn_args=None, staging_flags=None,
                            strip_flags=None, copy_paths=_COPY_PATHS_CHROME):
  """Populates a staging directory with necessary build artifacts.

  If |gn_args| or |staging_flags| are set, then we decide what to stage
  based on the flag values. Otherwise, we stage everything that we know
  about that we can find.

  Args:
    staging_dir: Path to an empty staging directory.
    build_dir: Path to location of Chrome build artifacts.
    strip_bin: Path to executable used for stripping binaries.
    sloppy: Ignore when mandatory artifacts are missing.
    gn_args: A dictionary of args.gn valuses that Chrome was built with.
    staging_flags: A list of extra staging flags.  Valid flags are specified in
      STAGING_FLAGS.
    strip_flags: A list of flags to pass to the tool used to strip binaries.
    copy_paths: The list of paths to use as a filter for staging files.
  """
  os.mkdir(os.path.join(staging_dir, 'plugins'), 0o755)

  if gn_args is None:
    gn_args = {}
  if staging_flags is None:
    staging_flags = []

  copier = Copier(strip_bin=strip_bin, strip_flags=strip_flags)
  copied_paths = []
  for p in copy_paths:
    if p.ShouldProcess(gn_args, staging_flags):
      copied_paths += copier.Copy(build_dir, staging_dir, p, sloppy=sloppy)

  if not copied_paths:
    raise MissingPathError('Couldn\'t find anything to copy!\n'
                           'Are you looking in the right directory?\n'
                           'Aborting copy...')

  _FixPermissions(staging_dir)
