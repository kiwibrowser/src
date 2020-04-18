# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""SCM-specific utility classes."""

import cStringIO
import glob
import logging
import os
import platform
import re
import sys
import tempfile
import time
from xml.etree import ElementTree

import gclient_utils
import subprocess2


def ValidateEmail(email):
  return (re.match(r"^[a-zA-Z0-9._%-+]+@[a-zA-Z0-9._%-]+.[a-zA-Z]{2,6}$", email)
          is not None)


def GetCasedPath(path):
  """Elcheapos way to get the real path case on Windows."""
  if sys.platform.startswith('win') and os.path.exists(path):
    # Reconstruct the path.
    path = os.path.abspath(path)
    paths = path.split('\\')
    for i in range(len(paths)):
      if i == 0:
        # Skip drive letter.
        continue
      subpath = '\\'.join(paths[:i+1])
      prev = len('\\'.join(paths[:i]))
      # glob.glob will return the cased path for the last item only. This is why
      # we are calling it in a loop. Extract the data we want and put it back
      # into the list.
      paths[i] = glob.glob(subpath + '*')[0][prev+1:len(subpath)]
    path = '\\'.join(paths)
  return path


def GenFakeDiff(filename):
  """Generates a fake diff from a file."""
  file_content = gclient_utils.FileRead(filename, 'rb').splitlines(True)
  filename = filename.replace(os.sep, '/')
  nb_lines = len(file_content)
  # We need to use / since patch on unix will fail otherwise.
  data = cStringIO.StringIO()
  data.write("Index: %s\n" % filename)
  data.write('=' * 67 + '\n')
  # Note: Should we use /dev/null instead?
  data.write("--- %s\n" % filename)
  data.write("+++ %s\n" % filename)
  data.write("@@ -0,0 +1,%d @@\n" % nb_lines)
  # Prepend '+' to every lines.
  for line in file_content:
    data.write('+')
    data.write(line)
  result = data.getvalue()
  data.close()
  return result


def determine_scm(root):
  """Similar to upload.py's version but much simpler.

  Returns 'git' or None.
  """
  if os.path.isdir(os.path.join(root, '.git')):
    return 'git'
  else:
    try:
      subprocess2.check_call(
          ['git', 'rev-parse', '--show-cdup'],
          stdout=subprocess2.VOID,
          stderr=subprocess2.VOID,
          cwd=root)
      return 'git'
    except (OSError, subprocess2.CalledProcessError):
      return None


def only_int(val):
  if val.isdigit():
    return int(val)
  else:
    return 0


class GIT(object):
  current_version = None

  @staticmethod
  def ApplyEnvVars(kwargs):
    env = kwargs.pop('env', None) or os.environ.copy()
    # Don't prompt for passwords; just fail quickly and noisily.
    # By default, git will use an interactive terminal prompt when a username/
    # password is needed.  That shouldn't happen in the chromium workflow,
    # and if it does, then gclient may hide the prompt in the midst of a flood
    # of terminal spew.  The only indication that something has gone wrong
    # will be when gclient hangs unresponsively.  Instead, we disable the
    # password prompt and simply allow git to fail noisily.  The error
    # message produced by git will be copied to gclient's output.
    env.setdefault('GIT_ASKPASS', 'true')
    env.setdefault('SSH_ASKPASS', 'true')
    # 'cat' is a magical git string that disables pagers on all platforms.
    env.setdefault('GIT_PAGER', 'cat')
    return env

  @staticmethod
  def Capture(args, cwd, strip_out=True, **kwargs):
    env = GIT.ApplyEnvVars(kwargs)
    output = subprocess2.check_output(
        ['git'] + args,
        cwd=cwd, stderr=subprocess2.PIPE, env=env, **kwargs)
    return output.strip() if strip_out else output

  @staticmethod
  def CaptureStatus(files, cwd, upstream_branch):
    """Returns git status.

    @files can be a string (one file) or a list of files.

    Returns an array of (status, file) tuples."""
    if upstream_branch is None:
      upstream_branch = GIT.GetUpstreamBranch(cwd)
      if upstream_branch is None:
        raise gclient_utils.Error('Cannot determine upstream branch')
    command = ['-c', 'core.quotePath=false', 'diff',
               '--name-status', '--no-renames', '-r', '%s...' % upstream_branch]
    if not files:
      pass
    elif isinstance(files, basestring):
      command.append(files)
    else:
      command.extend(files)
    status = GIT.Capture(command, cwd)
    results = []
    if status:
      for statusline in status.splitlines():
        # 3-way merges can cause the status can be 'MMM' instead of 'M'. This
        # can happen when the user has 2 local branches and he diffs between
        # these 2 branches instead diffing to upstream.
        m = re.match('^(\w)+\t(.+)$', statusline)
        if not m:
          raise gclient_utils.Error(
              'status currently unsupported: %s' % statusline)
        # Only grab the first letter.
        results.append(('%s      ' % m.group(1)[0], m.group(2)))
    return results

  @staticmethod
  def IsWorkTreeDirty(cwd):
    return GIT.Capture(['status', '-s'], cwd=cwd) != ''

  @staticmethod
  def GetEmail(cwd):
    """Retrieves the user email address if known."""
    try:
      return GIT.Capture(['config', 'user.email'], cwd=cwd)
    except subprocess2.CalledProcessError:
      return ''

  @staticmethod
  def ShortBranchName(branch):
    """Converts a name like 'refs/heads/foo' to just 'foo'."""
    return branch.replace('refs/heads/', '')

  @staticmethod
  def GetBranchRef(cwd):
    """Returns the full branch reference, e.g. 'refs/heads/master'."""
    return GIT.Capture(['symbolic-ref', 'HEAD'], cwd=cwd)

  @staticmethod
  def GetBranch(cwd):
    """Returns the short branch name, e.g. 'master'."""
    return GIT.ShortBranchName(GIT.GetBranchRef(cwd))

  @staticmethod
  def FetchUpstreamTuple(cwd):
    """Returns a tuple containg remote and remote ref,
       e.g. 'origin', 'refs/heads/master'
    """
    remote = '.'
    branch = GIT.GetBranch(cwd)
    try:
      upstream_branch = GIT.Capture(
          ['config', '--local', 'branch.%s.merge' % branch], cwd=cwd)
    except subprocess2.CalledProcessError:
      upstream_branch = None
    if upstream_branch:
      try:
        remote = GIT.Capture(
            ['config', '--local', 'branch.%s.remote' % branch], cwd=cwd)
      except subprocess2.CalledProcessError:
        pass
    else:
      try:
        upstream_branch = GIT.Capture(
            ['config', '--local', 'rietveld.upstream-branch'], cwd=cwd)
      except subprocess2.CalledProcessError:
        upstream_branch = None
      if upstream_branch:
        try:
          remote = GIT.Capture(
              ['config', '--local', 'rietveld.upstream-remote'], cwd=cwd)
        except subprocess2.CalledProcessError:
          pass
      else:
        # Else, try to guess the origin remote.
        remote_branches = GIT.Capture(['branch', '-r'], cwd=cwd).split()
        if 'origin/master' in remote_branches:
          # Fall back on origin/master if it exits.
          remote = 'origin'
          upstream_branch = 'refs/heads/master'
        else:
          # Give up.
          remote = None
          upstream_branch = None
    return remote, upstream_branch

  @staticmethod
  def RefToRemoteRef(ref, remote=None):
    """Convert a checkout ref to the equivalent remote ref.

    Returns:
      A tuple of the remote ref's (common prefix, unique suffix), or None if it
      doesn't appear to refer to a remote ref (e.g. it's a commit hash).
    """
    # TODO(mmoss): This is just a brute-force mapping based of the expected git
    # config. It's a bit better than the even more brute-force replace('heads',
    # ...), but could still be smarter (like maybe actually using values gleaned
    # from the git config).
    m = re.match('^(refs/(remotes/)?)?branch-heads/', ref or '')
    if m:
      return ('refs/remotes/branch-heads/', ref.replace(m.group(0), ''))
    if remote:
      m = re.match('^((refs/)?remotes/)?%s/|(refs/)?heads/' % remote, ref or '')
      if m:
        return ('refs/remotes/%s/' % remote, ref.replace(m.group(0), ''))
    return None

  @staticmethod
  def GetUpstreamBranch(cwd):
    """Gets the current branch's upstream branch."""
    remote, upstream_branch = GIT.FetchUpstreamTuple(cwd)
    if remote != '.' and upstream_branch:
      remote_ref = GIT.RefToRemoteRef(upstream_branch, remote)
      if remote_ref:
        upstream_branch = ''.join(remote_ref)
    return upstream_branch

  @staticmethod
  def GetOldContents(cwd, filename, branch=None):
    if not branch:
      branch = GIT.GetUpstreamBranch(cwd)
    if platform.system() == 'Windows':
      # git show <sha>:<path> wants a posix path.
      filename = filename.replace('\\', '/')
    command = ['show', '%s:%s' % (branch, filename)]
    try:
      return GIT.Capture(command, cwd=cwd, strip_out=False)
    except subprocess2.CalledProcessError:
      return ''

  @staticmethod
  def GenerateDiff(cwd, branch=None, branch_head='HEAD', full_move=False,
                   files=None):
    """Diffs against the upstream branch or optionally another branch.

    full_move means that move or copy operations should completely recreate the
    files, usually in the prospect to apply the patch for a try job."""
    if not branch:
      branch = GIT.GetUpstreamBranch(cwd)
    command = ['-c', 'core.quotePath=false', 'diff',
               '-p', '--no-color', '--no-prefix', '--no-ext-diff',
               branch + "..." + branch_head]
    if full_move:
      command.append('--no-renames')
    else:
      command.append('-C')
    # TODO(maruel): --binary support.
    if files:
      command.append('--')
      command.extend(files)
    diff = GIT.Capture(command, cwd=cwd, strip_out=False).splitlines(True)
    for i in range(len(diff)):
      # In the case of added files, replace /dev/null with the path to the
      # file being added.
      if diff[i].startswith('--- /dev/null'):
        diff[i] = '--- %s' % diff[i+1][4:]
    return ''.join(diff)

  @staticmethod
  def GetDifferentFiles(cwd, branch=None, branch_head='HEAD'):
    """Returns the list of modified files between two branches."""
    if not branch:
      branch = GIT.GetUpstreamBranch(cwd)
    command = ['-c', 'core.quotePath=false', 'diff',
               '--name-only', branch + "..." + branch_head]
    return GIT.Capture(command, cwd=cwd).splitlines(False)

  @staticmethod
  def GetPatchName(cwd):
    """Constructs a name for this patch."""
    short_sha = GIT.Capture(['rev-parse', '--short=4', 'HEAD'], cwd=cwd)
    return "%s#%s" % (GIT.GetBranch(cwd), short_sha)

  @staticmethod
  def GetCheckoutRoot(cwd):
    """Returns the top level directory of a git checkout as an absolute path.
    """
    root = GIT.Capture(['rev-parse', '--show-cdup'], cwd=cwd)
    return os.path.abspath(os.path.join(cwd, root))

  @staticmethod
  def GetGitDir(cwd):
    return os.path.abspath(GIT.Capture(['rev-parse', '--git-dir'], cwd=cwd))

  @staticmethod
  def IsInsideWorkTree(cwd):
    try:
      return GIT.Capture(['rev-parse', '--is-inside-work-tree'], cwd=cwd)
    except (OSError, subprocess2.CalledProcessError):
      return False

  @staticmethod
  def IsDirectoryVersioned(cwd, relative_dir):
    """Checks whether the given |relative_dir| is part of cwd's repo."""
    return bool(GIT.Capture(['ls-tree', 'HEAD', relative_dir], cwd=cwd))

  @staticmethod
  def CleanupDir(cwd, relative_dir):
    """Cleans up untracked file inside |relative_dir|."""
    return bool(GIT.Capture(['clean', '-df', relative_dir], cwd=cwd))

  @staticmethod
  def IsValidRevision(cwd, rev, sha_only=False):
    """Verifies the revision is a proper git revision.

    sha_only: Fail unless rev is a sha hash.
    """
    # 'git rev-parse foo' where foo is *any* 40 character hex string will return
    # the string and return code 0. So strip one character to force 'git
    # rev-parse' to do a hash table look-up and returns 128 if the hash is not
    # present.
    lookup_rev = rev
    if re.match(r'^[0-9a-fA-F]{40}$', rev):
      lookup_rev = rev[:-1]
    try:
      sha = GIT.Capture(['rev-parse', lookup_rev], cwd=cwd).lower()
      if lookup_rev != rev:
        # Make sure we get the original 40 chars back.
        return rev.lower() == sha
      if sha_only:
        return sha.startswith(rev.lower())
      return True
    except subprocess2.CalledProcessError:
      return False

  @classmethod
  def AssertVersion(cls, min_version):
    """Asserts git's version is at least min_version."""
    if cls.current_version is None:
      current_version = cls.Capture(['--version'], '.')
      matched = re.search(r'version ([0-9\.]+)', current_version)
      cls.current_version = matched.group(1)
    current_version_list = map(only_int, cls.current_version.split('.'))
    for min_ver in map(int, min_version.split('.')):
      ver = current_version_list.pop(0)
      if ver < min_ver:
        return (False, cls.current_version)
      elif ver > min_ver:
        return (True, cls.current_version)
    return (True, cls.current_version)
