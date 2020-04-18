#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Class capturing a command invocation as data."""

import inspect
import glob
import hashlib
import logging
import os
import shutil
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import pynacl.file_tools
import pynacl.log_tools
import pynacl.repo_tools

import substituter


# MSYS tools do not always work with combinations of Windows and MSYS
# path conventions, e.g. '../foo\\bar' doesn't find '../foo/bar'.
# Since we convert all the directory names to MSYS conventions, we
# should not be using Windows separators with those directory names.
# As this is really an implementation detail of this module, we export
# 'command.path' to use in place of 'os.path', rather than having
# users of the module know which flavor to use.
import posixpath
path = posixpath


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACL_DIR = os.path.dirname(SCRIPT_DIR)

COMMAND_CODE_FILES = [os.path.join(SCRIPT_DIR, f)
                      for f in ('command.py', 'once.py', 'substituter.py',
                                'pnacl_commands.py', 'toolchain_main.py',)]
COMMAND_CODE_FILES += [os.path.join(NACL_DIR, 'pynacl', f)
                       for f in ('platform.py','directory_storage.py',
                                 'file_tools.py', 'gsd_storage.py',
                                 'hashing_tools.py', 'local_storage_cache.py',
                                 'log_tools.py', 'repo_tools.py',)]

def HashBuildSystemSources():
  """Read the build source files to use in hashes for Callbacks."""
  global FILE_CONTENTS_HASH
  h = hashlib.sha1()
  for filename in COMMAND_CODE_FILES:
    with open(filename) as f:
      h.update(f.read())
  FILE_CONTENTS_HASH = h.hexdigest()

HashBuildSystemSources()


def PlatformEnvironment(extra_paths):
  """Select the environment variables to run commands with.

  Args:
    extra_paths: Extra paths to add to the PATH variable.
  Returns:
    A dict to be passed as env to subprocess.
  """
  env = os.environ.copy()
  paths = []
  if sys.platform == 'win32':
    # TODO(bradnelson): switch to something hermetic.
    mingw = os.environ.get('MINGW', r'c:\mingw')
    msys = os.path.join(mingw, 'msys', '1.0')
    if not os.path.exists(msys):
      msys = os.path.join(mingw, 'msys')
    # We need both msys (posix like build environment) and MinGW (windows
    # build of tools like gcc). We add <MINGW>/msys/[1.0/]bin to the path to
    # get sh.exe. We add <MINGW>/bin to allow direct invocation on MinGW
    # tools. We also add an msys style path (/mingw/bin) to get things like
    # gcc from inside msys.
    paths = [
        '/mingw/bin',
        os.path.join(mingw, 'bin'),
        os.path.join(msys, 'bin'),
    ]
  env['PATH'] = os.pathsep.join(
      paths + extra_paths + env.get('PATH', '').split(os.pathsep))
  return env


class Runnable(object):
  """An object representing a single command."""
  def __init__(self, run_cond, func, *args, **kwargs):
    """Construct a runnable which will call 'func' with 'args' and 'kwargs'.

    Args:
      run_cond: If not None, expects a function which takes a CommandOptions
                object and returns whether or not to run the command.
      func: Function which will be called by Invoke
      args: Positional arguments to be passed to func
      kwargs: Keyword arguments to be passed to func

      RUNNABLES SHOULD ONLY BE IMPLEMENTED IN THIS FILE, because their
      string representation (which is used to calculate whether targets should
      be rebuilt) is based on this file's hash and does not attempt to capture
      the code or bound variables of the function itself (the one exception is
      once_test.py which injects its own callbacks to verify its expectations).

      When 'func' is called, its first argument will be a substitution object
      which it can use to substitute %-templates in its arguments.
    """
    self._run_cond = run_cond
    self._func = func
    self._args = args or []
    self._kwargs = kwargs or {}

  def __str__(self):
    values = []

    sourcefile = inspect.getsourcefile(self._func)
    # Check that the code for the runnable is implemented in one of the known
    # source files of the build system (which are included in its hash). This
    # isn't a perfect test because it could still import code from an outside
    # module, so we should be sure to add any new build system files to the list
    found_match = (os.path.basename(sourcefile) in
                   [os.path.basename(f) for f in
                    COMMAND_CODE_FILES + ['once_test.py']])
    if not found_match:
      print 'Function', self._func.func_name, 'in', sourcefile
      raise Exception('Python Runnable objects must be implemented in one of' +
                      ' the following files: ' + str(COMMAND_CODE_FILES))

    # Like repr(datum), but do something stable for dictionaries.
    # This only properly handles dictionaries that use simple types
    # as keys.
    def ReprForHash(datum):
      if isinstance(datum, dict):
        # The order of a dictionary's items is unpredictable.
        # Manually make a string in dict syntax, but sorted on keys.
        return ('{' +
                ', '.join(repr(key) + ': ' + ReprForHash(value)
                          for key, value in sorted(datum.iteritems(),
                                                   key=lambda t: t[0])) +
                '}')
      elif isinstance(datum, list):
        # A list is already ordered, but its items might be dictionaries.
        return ('[' +
                ', '.join(ReprForHash(value) for value in datum) +
                ']')
      else:
        return repr(datum)

    for v in self._args:
      values += [ReprForHash(v)]
    # The order of a dictionary's items is unpredictable.
    # Sort by key for hashing purposes.
    for k, v in sorted(self._kwargs.iteritems(), key=lambda t: t[0]):
      values += [repr(k), ReprForHash(v)]
    values += [FILE_CONTENTS_HASH]

    return '\n'.join(values)

  def CheckRunCond(self, cmd_options):
    if self._run_cond and not self._run_cond(cmd_options):
      return False
    return True

  def Invoke(self, logger, subst):
    return self._func(logger, subst, *self._args, **self._kwargs)


def Command(command, stdout=None, run_cond=None, **kwargs):
  """Return a Runnable which invokes 'command' with check_call.

  Args:
    command: List or string with a command suitable for check_call
    stdout (optional): File name to redirect command's stdout to
    kwargs: Keyword arguments suitable for check_call (or 'cwd' or 'path_dirs')

  The command will be %-substituted and paths will be assumed to be relative to
  the cwd given by Invoke. If kwargs contains 'cwd' it will be appended to the
  cwd given by Invoke and used as the cwd for the call. If kwargs contains
  'path_dirs', the directories therein will be added to the paths searched for
  the command. Any other kwargs will be passed to check_call.
  """
  def runcmd(logger, subst, command, stdout, **kwargs):
    check_call_kwargs = kwargs.copy()
    command = command[:]

    cwd = subst.SubstituteAbsPaths(check_call_kwargs.get('cwd', '.'))
    subst.SetCwd(cwd)
    check_call_kwargs['cwd'] = cwd

    # Extract paths from kwargs and add to the command environment.
    path_dirs = []
    if 'path_dirs' in check_call_kwargs:
      path_dirs = [subst.Substitute(dirname) for dirname
                   in check_call_kwargs['path_dirs']]
      del check_call_kwargs['path_dirs']
    # Perform substitution on any env overrides.
    if 'env' in check_call_kwargs:
      check_call_kwargs['env'] = { k: subst.Substitute(v)
            for (k, v) in check_call_kwargs['env'].iteritems() }
      check_call_kwargs['env'].update(PlatformEnvironment(path_dirs))
    else:
      check_call_kwargs['env'] = PlatformEnvironment(path_dirs)

    if isinstance(command, str):
      command = subst.Substitute(command)
    else:
      command = [subst.Substitute(arg) for arg in command]
      paths = check_call_kwargs['env']['PATH'].split(os.pathsep)
      command[0] = pynacl.file_tools.Which(command[0], paths=paths)

    if stdout is not None:
      stdout = subst.SubstituteAbsPaths(stdout)

    pynacl.log_tools.CheckCall(command, stdout=stdout, logger=logger,
                               **check_call_kwargs)

  return Runnable(run_cond, runcmd, command, stdout, **kwargs)


def SkipForIncrementalCommand(command, run_cond=None, **kwargs):
  """Return a command which gets skipped for incremental builds.

  Incremental builds are defined to be when the clobber flag is not on and
  the working directory is not empty.
  """
  def SkipForIncrementalCondition(cmd_opts):
    # Check if caller passed their own run_cond.
    if run_cond and not run_cond(cmd_opts):
      return False

    dir_list = os.listdir(cmd_opts.GetWorkDir())
    # Only run when clobbering working directory or working directory is empty.
    return (cmd_opts.IsClobberWorking() or
            not os.path.isdir(cmd_opts.GetWorkDir()) or
            len(dir_list) == 0 or
            (len(dir_list) == 1 and dir_list[0].endswith('.log')))

  return Command(command, run_cond=SkipForIncrementalCondition, **kwargs)


def Mkdir(path, parents=False, run_cond=None):
  """Convenience method for generating mkdir commands."""
  def mkdir(logger, subst, path):
    path = subst.SubstituteAbsPaths(path)
    if os.path.isdir(path):
      return
    logger.debug('Making Directory: %s', path)
    if parents:
      os.makedirs(path)
    else:
      os.mkdir(path)
  return Runnable(run_cond, mkdir, path)


def Copy(src, dst, permissions=False, run_cond=None):
  """Convenience method for generating cp commands."""
  def copy(logger, subst, src, dst, permissions):
    src = subst.SubstituteAbsPaths(src)
    dst = subst.SubstituteAbsPaths(dst)
    logger.debug('Copying: %s %s-> %s',
                 src,
                 '(with permissions) ' if permissions else '',
                 dst)
    shutil.copyfile(src, dst)
    if permissions:
      shutil.copymode(src, dst)

  return Runnable(run_cond, copy, src, dst, permissions)


def CopyRecursive(src, dst, run_cond=None):
  """Recursively copy items in a directory tree.

     If src is a file, the semantics are like shutil.copyfile+copymode.
     If src is a directory, the semantics are like shutil.copytree, except
     that the destination may exist (it must be a directory) and will not be
     clobbered. There must be no files in dst which have names/subpaths which
     match files in src.
  """
  def rcopy(logger, subst, src, dst):
    src = subst.SubstituteAbsPaths(src)
    dst = subst.SubstituteAbsPaths(dst)
    if os.path.isfile(src):
      shutil.copyfile(src, dst)
      shutil.copymode(src, dst)
    elif os.path.isdir(src):
      logger.debug('Copying directory: %s -> %s', src, dst)
      pynacl.file_tools.MakeDirectoryIfAbsent(dst)
      for item in os.listdir(src):
        rcopy(logger, subst, os.path.join(src, item), os.path.join(dst, item))
  return Runnable(run_cond, rcopy, src, dst)

def CopyTree(src, dst, exclude=[], run_cond=None):
  """Copy a directory tree, excluding a list of top-level entries.

     The destination directory will be clobbered if it exists.
  """
  def copyTree(logger, subst, src, dst, exclude):
    src = subst.SubstituteAbsPaths(src)
    dst = subst.SubstituteAbsPaths(dst)
    def ignoreExcludes(dir, files):
      if dir == src:
        return exclude
      else:
        return []
    logger.debug('Copying Tree: %s -> %s', src, dst)
    pynacl.file_tools.RemoveDirectoryIfPresent(dst)
    shutil.copytree(src, dst, symlinks=True, ignore=ignoreExcludes)
  return Runnable(run_cond, copyTree, src, dst, exclude)


def RemoveDirectory(path, run_cond=None):
  """Convenience method for generating a command to remove a directory tree."""
  def remove(logger, subst, path):
    path = subst.SubstituteAbsPaths(path)
    logger.debug('Removing Directory: %s', path)
    pynacl.file_tools.RemoveDirectoryIfPresent(path)
  return Runnable(run_cond, remove, path)


def Remove(*args):
  """Convenience method for generating a command to remove files."""
  def remove(logger, subst, *args):
    for arg in args:
      path = subst.SubstituteAbsPaths(arg)
      logger.debug('Removing Pattern: %s', path)
      expanded = glob.glob(path)
      if len(expanded) == 0:
        logger.debug('command.Remove: argument %s (substituted from %s) '
                     'does not match any file' %
                      (path, arg))
      for f in expanded:
        logger.debug('Removing File: %s', f)
        os.remove(f)
  return Runnable(None, remove, *args)


def Rename(src, dst, run_cond=None):
  """Convenience method for generating a command to rename a file."""
  def rename(logger, subst, src, dst):
    src = subst.SubstituteAbsPaths(src)
    dst = subst.SubstituteAbsPaths(dst)
    logger.debug('Renaming: %s -> %s', src, dst)
    os.rename(src, dst)
  return Runnable(run_cond, rename, src, dst)


def WriteData(data, dst, run_cond=None):
  """Convenience method to write a file with fixed contents."""
  def writedata(logger, subst, dst, data):
    dst = subst.SubstituteAbsPaths(dst)
    logger.debug('Writing Data to File: %s', dst)
    with open(subst.SubstituteAbsPaths(dst), 'wb') as f:
      f.write(data)
  return Runnable(run_cond, writedata, dst, data)


def SyncGitRepoCmds(url, destination, revision, clobber_invalid_repo=False,
                    reclone=False, pathspec=None, git_cache=None, push_url=None,
                    known_mirrors=[], push_mirrors=[],
                    run_cond=None):
  """Returns a list of commands to sync and validate a git repo.

  Args:
    url: Git repo URL to sync from.
    destination: Local git repo directory to sync to.
    revision: If not None, will sync the git repository to this revision.
    clobber_invalid_repo: Always True for bots, but can be forced for users.
    reclone: If True, delete the destination directory and re-clone the repo.
    pathspec: If not None, add the path to the git checkout command, which
              causes it to just update the working tree without switching
              branches.
    known_mirrors: List of tuples specifying known mirrors for a subset of the
                   git URL. IE: [('http://mirror.com/mirror', 'http://git.com')]
    push_mirrors: List of tuples specifying known push mirrors, see
                  known_mirrors argument for the format.
    git_cache: If not None, will use git_cache directory as a cache for the git
               repository and share the objects with any other destination with
               the same URL.
    push_url: If not None, specifies what the push URL should be set to.
    run_cond: Run condition for when to sync the git repo.

  Returns:
    List of commands, this is a little different from the other command funcs.
  """
  def update_valid_mirrors(logger, subst, url, push_url, directory,
                           known_mirrors, push_mirrors):
    if push_url is None:
      push_url = url

    abs_dir = subst.SubstituteAbsPaths(directory)
    git_dir = os.path.join(abs_dir, '.git')
    if os.path.exists(git_dir):
      fetch_list = pynacl.repo_tools.GitRemoteRepoList(abs_dir,
                                                       include_fetch=True,
                                                       include_push=False,
                                                       logger=logger)
      tracked_fetch_url = dict(fetch_list).get('origin', 'None')

      push_list = pynacl.repo_tools.GitRemoteRepoList(abs_dir,
                                                      include_fetch=False,
                                                      include_push=True,
                                                      logger=logger)
      tracked_push_url = dict(push_list).get('origin', 'None')

      if ((known_mirrors and tracked_fetch_url != url) or
          (push_mirrors and tracked_push_url != push_url)):
        updated_fetch_url = tracked_fetch_url
        for mirror, url_subset in known_mirrors:
          if mirror in updated_fetch_url:
            updated_fetch_url = updated_fetch_url.replace(mirror, url_subset)

        updated_push_url = tracked_push_url
        for mirror, url_subset in push_mirrors:
          if mirror in updated_push_url:
            updated_push_url = updated_push_url.replace(mirror, url_subset)

        if ((updated_fetch_url != tracked_fetch_url) or
            (updated_push_url != tracked_push_url)):
          logger.warn('Your git repo is using an old mirror: %s', abs_dir)
          logger.warn('Updating git repo using known mirror:')
          logger.warn('  [FETCH] %s -> %s',
                      tracked_fetch_url, updated_fetch_url)
          logger.warn('  [PUSH] %s -> %s',
                      tracked_push_url, updated_push_url)
          pynacl.repo_tools.GitSetRemoteRepo(updated_fetch_url, abs_dir,
                                             push_url=updated_push_url,
                                             logger=logger)

  def populate_cache(logger, subst, git_cache, url):
    if git_cache:
      abs_git_cache = subst.SubstituteAbsPaths(git_cache)
      logger.debug('Populating Cache: %s [%s]', abs_git_cache, url)
      if abs_git_cache:
        pynacl.repo_tools.PopulateGitCache(abs_git_cache, [url],
                                           logger=logger)

  def validate(logger, subst, url, directory):
    abs_dir = subst.SubstituteAbsPaths(directory)
    logger.debug('Validating Repo: %s [%s]', abs_dir, url)
    pynacl.repo_tools.ValidateGitRepo(url,
                                      subst.SubstituteAbsPaths(directory),
                                      clobber_mismatch=True,
                                      logger=logger)

  def sync(logger, subst, url, dest, revision, reclone, pathspec, git_cache,
           push_url):
    abs_dest = subst.SubstituteAbsPaths(dest)
    if git_cache:
      git_cache = subst.SubstituteAbsPaths(git_cache)

    logger.debug('Syncing Git Repo: %s [%s]', abs_dest, url)
    try:
      pynacl.repo_tools.SyncGitRepo(url, abs_dest, revision,
                                    reclone=reclone,
                                    pathspec=pathspec, git_cache=git_cache,
                                    push_url=push_url, logger=logger)
    except pynacl.repo_tools.InvalidRepoException, e:
      remote_repos = dict(pynacl.repo_tools.GitRemoteRepoList(abs_dest,
                                                              logger=logger))
      tracked_url = remote_repos.get('origin', 'None')
      logger.error('Invalid Git Repo: %s' % e)
      logger.error('Destination Directory: %s', abs_dest)
      logger.error('Currently Tracked Repo: %s', tracked_url)
      logger.error('Expected Repo: %s', e.expected_repo)
      logger.warn('Possible solutions:')
      logger.warn('  1. The simplest way if you have no local changes is to'
                  ' simply delete the directory and let the tool resync.')
      logger.warn('  2. If the tracked repo is merely a mirror, simply go to'
                  ' the directory and run "git remote set-url origin %s"',
                  e.expected_repo)
      raise Exception('Could not validate local git repository.')

  def ClobberInvalidRepoCondition(cmd_opts):
    # Check if caller passed their own run_cond
    if run_cond and not run_cond(cmd_opts):
      return False
    elif clobber_invalid_repo:
      return True
    return cmd_opts.IsBot()

  def CleanOnBotCondition(cmd_opts):
    # Check if caller passed their own run_cond
    if run_cond and not run_cond(cmd_opts):
      return False
    return cmd_opts.IsBot()

  commands = [CleanGitWorkingDir(destination, reset=True, path=None,
                                 run_cond=CleanOnBotCondition)]
  if git_cache:
    commands.append(Runnable(run_cond, populate_cache, git_cache, url))

  commands.extend([Runnable(run_cond, update_valid_mirrors, url, push_url,
                            destination, known_mirrors, push_mirrors),
                   Runnable(ClobberInvalidRepoCondition, validate, url,
                            destination),
                   Runnable(run_cond, sync, url, destination, revision, reclone,
                            pathspec, git_cache, push_url)])
  return commands


def CleanGitWorkingDir(directory, reset=False, path=None, run_cond=None):
  """Clean a path in a git checkout, if the checkout directory exists."""
  def clean(logger, subst, directory, reset, path):
    directory = subst.SubstituteAbsPaths(directory)
    logger.debug('Cleaning Git Working Directory: %s', directory)
    if os.path.exists(directory) and len(os.listdir(directory)) > 0:
      pynacl.repo_tools.CleanGitWorkingDir(directory, reset, path,logger=logger)
  return Runnable(run_cond, clean, directory, reset, path)


def GenerateGitPatches(git_dir, info, run_cond=None):
  """Generate patches from a Git repository.

  Args:
    git_dir: bare git repository directory to examine (.../.git)
    info: dictionary containing:
      'rev': commit that we build
      'upstream-name': basename of the upstream baseline release
        (i.e. what the release tarball would be called before ".tar")
      'upstream-base': commit corresponding to upstream-name
      'upstream-branch': tracking branch used for upstream merges

  This will produce between zero and two patch files (in %(output)s/):
    <upstream-name>-g<commit-abbrev>.patch: From 'upstream-base' to the common
      ancestor (merge base) of 'rev' and 'upstream-branch'.  Omitted if none.
    <upstream-name>[-g<commit-abbrev>]-nacl.patch: From the result of that
      (or from 'upstream-base' if none above) to 'rev'.
  """
  def generatePatches(logger, subst, git_dir, info, run_cond=None):
    git_dir = subst.SubstituteAbsPaths(git_dir)
    git_dir_flag = '--git-dir=' + git_dir
    basename = info['upstream-name']
    logger.debug('Generating Git Patches: %s', git_dir)

    patch_files = []

    def generatePatch(description, src_rev, dst_rev, suffix):
      src_prefix = '--src-prefix=' + basename + '/'
      dst_prefix = '--dst-prefix=' + basename + suffix + '/'
      patch_name = basename + suffix + '.patch'
      patch_file = subst.SubstituteAbsPaths(path.join('%(output)s', patch_name))
      git_args = [git_dir_flag, 'diff',
                  '--patch-with-stat', '--ignore-space-at-eol', '--full-index',
                  '--no-ext-diff', '--no-color', '--no-renames',
                  '--no-textconv', '--text', src_prefix, dst_prefix,
                  src_rev, dst_rev]
      pynacl.log_tools.CheckCall(
          pynacl.repo_tools.GitCmd() + git_args,
          stdout=patch_file,
          logger=logger,
      )
      patch_files.append((description, patch_name))

    def revParse(args):
      output = pynacl.repo_tools.CheckGitOutput([git_dir_flag] + args)
      lines = output.splitlines()
      if len(lines) != 1:
        raise Exception('"git %s" did not yield a single commit' %
                        ' '.join(args))
      return lines[0]

    # This turns a rev-spec into a fully-expanded SHA1 of a commit object.
    # If the specified rev is a tag, this will get the tagged commit.
    def getCommit(rev):
      return revParse(['rev-parse', rev + '^{commit}'])

    rev = getCommit(info['rev'])
    upstream_base = getCommit(info['upstream-base'])
    upstream_branch = getCommit('refs/remotes/origin/' +
                                info['upstream-branch'])
    upstream_snapshot = revParse(['merge-base', rev, upstream_branch])

    if rev == upstream_base:
      # We're building a stock upstream release.  Nothing to do!
      return

    if upstream_snapshot == upstream_base:
      # We've forked directly from the upstream baseline release.
      suffix = ''
    else:
      # We're using an upstream baseline snapshot past the baseline
      # release, so generate a snapshot patch.  The leading seven
      # hex digits of the commit ID is what Git usually produces
      # for --abbrev-commit behavior, 'git describe', etc.
      suffix = '-g' + upstream_snapshot[:7]
      generatePatch('Patch the release up to the upstream snapshot version.',
                    upstream_base, upstream_snapshot, suffix)

    if rev != upstream_snapshot:
      # We're using local changes, so generate a patch of those.
      generatePatch('Apply NaCl-specific changes.',
                    upstream_snapshot, rev, suffix + '-nacl')

    with open(subst.SubstituteAbsPaths(path.join('%(output)s',
                                                 info['name'] + '.series')),
              'w') as f:
      f.write("""\
# This is a "series file" in the style used by the "quilt" tool.
# It describes how to unpack and apply patches to produce the source
# tree of the %(name)s component of a toolchain targetting Native Client.

# Source: %(upstream-name)s.tar
"""
              % info)
      for patch in patch_files:
        f.write('\n# %s\n%s\n' % patch)

  return Runnable(run_cond, generatePatches, git_dir, info)
