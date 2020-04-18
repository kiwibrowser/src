#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import posixpath
import subprocess
import sys
import urlparse

import file_tools
import log_tools
import platform

GIT_ALTERNATES_PATH = os.path.join('.git', 'objects', 'info', 'alternates')


class InvalidRepoException(Exception):
  def __init__(self, expected_repo, msg, *args):
    Exception.__init__(self, msg % args)
    self.expected_repo = expected_repo


def GitCmd():
  """Return the git command to execute for the host platform."""
  if platform.IsWindows():
    # On windows, we want to use the depot_tools version of git, which has
    # git.bat as an entry point. When running through the msys command
    # prompt, subprocess does not handle batch files. Explicitly invoking
    # cmd.exe to be sure we run the correct git in this case.
    return ['cmd.exe', '/c', 'git.bat']
  else:
    return ['git']


def CheckGitOutput(args):
  """Run a git subcommand and capture its stdout a la subprocess.check_output.
  Args:
    args: list of arguments to 'git'
  """
  return log_tools.CheckOutput(GitCmd() + args)


def ValidateGitRepo(url, directory, clobber_mismatch=False, logger=None):
  """Validates a git repository tracks a particular URL.

  Given a git directory, this function will validate if the git directory
  actually tracks an expected URL. If the directory does not exist nothing
  will be done.

  Args:
  url: URL to look for.
  directory: Directory to look for.
  clobber_mismatch: If True, will delete invalid directories instead of raising
                    an exception.
  """
  if logger is None:
    logger = log_tools.GetConsoleLogger()
  git_dir = os.path.join(directory, '.git')
  if os.path.exists(git_dir):
    try:
      if IsURLInRemoteRepoList(url, directory, include_fetch=True,
                               include_push=False):
        return

      logger.warn('Local git repo (%s) does not track url (%s)',
                   directory, url)
    except:
      logger.error('Invalid git repo: %s', directory)

    if not clobber_mismatch:
      raise InvalidRepoException(url, 'Invalid local git repo: %s', directory)
    else:
      logger.debug('Clobbering invalid git repo %s' % directory)
      file_tools.RemoveDirectoryIfPresent(directory)
  elif os.path.exists(directory) and len(os.listdir(directory)) != 0:
    if not clobber_mismatch:
      raise InvalidRepoException(url,
                                 'Invalid non-empty repository destination %s',
                                 directory)
    else:
      logger.debug('Clobbering intended repository destination: %s', directory)
      file_tools.RemoveDirectoryIfPresent(directory)


def SyncGitRepo(url, destination, revision, reclone=False, pathspec=None,
                git_cache=None, push_url=None, logger=None):
  """Sync an individual git repo.

  Args:
  url: URL to sync
  destination: Directory to check out into.
  revision: Pinned revision to check out. If None, do not check out a
            pinned revision.
  reclone: If True, delete the destination directory and re-clone the repo.
  pathspec: If not None, add the path to the git checkout command, which
            causes it to just update the working tree without switching
            branches.
  git_cache: If set, assumes URL has been populated within the git cache
             directory specified and sets the fetch URL to be from the
             git_cache.
  """
  if logger is None:
    logger = log_tools.GetConsoleLogger()
  if reclone:
    logger.debug('Clobbering source directory %s' % destination)
    file_tools.RemoveDirectoryIfPresent(destination)

  if git_cache:
    git_cache_url = GetGitCacheURL(git_cache, url)
  else:
    git_cache_url = None

  # If the destination is a git repository, validate the tracked origin.
  git_dir = os.path.join(destination, '.git')
  if os.path.exists(git_dir):
    if not IsURLInRemoteRepoList(url, destination, include_fetch=True,
                                 include_push=False):
      # If the git cache URL is being tracked instead of the fetch URL, we
      # can safely redirect it to the fetch URL instead.
      if git_cache_url and IsURLInRemoteRepoList(git_cache_url, destination,
                                                 include_fetch=True,
                                                 include_push=False):
        GitSetRemoteRepo(url, destination, push_url=push_url,
                         logger=logger)
      else:
        logger.error('Git Repo (%s) does not track URL: %s',
                      destination, url)
        raise InvalidRepoException(url, 'Could not sync git repo: %s',
                                   destination)

      # Make sure the push URL is set correctly as well.
      if not IsURLInRemoteRepoList(push_url, destination, include_fetch=False,
                                   include_push=True):
        GitSetRemoteRepo(url, destination, push_url=push_url)

  git = GitCmd()
  if not os.path.exists(git_dir):
    logger.info('Cloning %s...' % url)

    file_tools.MakeDirectoryIfAbsent(destination)
    clone_args = ['clone', '-n']
    if git_cache_url:
      clone_args.extend(['--reference', git_cache_url])

    log_tools.CheckCall(git + clone_args + [url, '.'],
                        logger=logger, cwd=destination)

    if url != push_url:
      GitSetRemoteRepo(url, destination, push_url=push_url, logger=logger)

  # If a git cache URL is supplied, make sure it is setup as a git alternate.
  if git_cache_url:
    git_alternates = [git_cache_url]
  else:
    git_alternates = []

  GitSetRepoAlternates(destination, git_alternates, append=False, logger=logger)

  if revision is not None:
    logger.info('Checking out pinned revision...')
    log_tools.CheckCall(git + ['fetch', '--all'],
                        logger=logger, cwd=destination)
    path = [pathspec] if pathspec else []
    log_tools.CheckCall(
        git + ['checkout', revision] + path,
        logger=logger, cwd=destination)


def CleanGitWorkingDir(directory, reset=False, path=None, logger=None):
  """Clean all or part of an existing git checkout.

     Args:
     directory: Directory where the git repo is currently checked out
     reset: If True, also reset the working directory to HEAD
     path: path to clean, relative to the repo directory. If None,
           clean the whole working directory
  """
  repo_path = [path] if path else []
  log_tools.CheckCall(GitCmd() + ['clean', '-dffx'] + repo_path,
                      logger=logger, cwd=directory)
  if reset:
    log_tools.CheckCall(GitCmd() + ['reset', '--hard', 'HEAD'],
                        logger=logger, cwd=directory)


def PopulateGitCache(cache_dir, url_list, logger=None):
  """Fetches a git repo that combines a list of git repos.

  This is an interface to the "git cache" command found within depot_tools.
  You can populate a cache directory then obtain the local cache url using
  GetGitCacheURL(). It is best to sync with the shared option so that the
  cloned repository shares the same git objects.

  Args:
    cache_dir: Local directory where git cache will be populated.
    url_list: List of URLs which cache_dir should be populated with.
  """
  if url_list:
    file_tools.MakeDirectoryIfAbsent(cache_dir)
    git = GitCmd()
    for url in url_list:
      log_tools.CheckCall(git + ['cache', 'populate', '-c', '.', url],
                          logger=logger, cwd=cache_dir)


def GetGitCacheURL(cache_dir, url, logger=None):
  """Converts a regular git URL to a git cache URL within a cache directory.

  Args:
    url: original Git URL that is already populated within the cache directory.
    cache_dir: Git cache directory that has already populated the URL.

  Returns:
    Git Cache URL where a git repository can clone/fetch from.
  """
  # Make sure we are using absolute paths or else cache exists return relative.
  cache_dir = os.path.abspath(cache_dir)

  # For CygWin, we must first convert the cache_dir name to a non-cygwin path.
  cygwin_path = False
  if platform.IsCygWin() and cache_dir.startswith('/cygdrive/'):
    cygwin_path = True
    drive, file_path = cache_dir[len('/cygdrive/'):].split('/', 1)
    cache_dir = drive + ':\\' + file_path.replace('/', '\\')

  git_url = log_tools.CheckOutput(GitCmd() + ['cache', 'exists',
                                              '-c', cache_dir,
                                              url],
                                  logger=logger).strip()

  # For windows, make sure the git cache URL is a posix path.
  if platform.IsWindows():
    git_url = git_url.replace('\\', '/')
  return git_url


def GitRevInfo(directory):
  """Get the git revision information of a git checkout.

  Args:
    directory: Existing git working directory.
"""
  get_url_command = GitCmd() + ['ls-remote', '--get-url', 'origin']
  url = log_tools.CheckOutput(get_url_command, cwd=directory).strip()
  # If the URL is actually a directory, it might be a git-cache directory.
  # Re-run from that directory to get the actual remote URL.
  if os.path.isdir(url):
    url = log_tools.CheckOutput(get_url_command, cwd=url).strip()
  rev = log_tools.CheckOutput(GitCmd() + ['rev-parse', 'HEAD'],
                              cwd=directory).strip()
  return url, rev


def GetAuthenticatedGitURL(url):
  """Returns the authenticated version of a git URL.

  In chromium, there is a special URL that is the "authenticated" version. The
  URLs are identical but the authenticated one has special privileges.
  """
  urlsplit = urlparse.urlsplit(url)
  if urlsplit.scheme in ('https', 'http'):
    urldict = urlsplit._asdict()
    urldict['scheme'] = 'https'
    urldict['path'] = '/a' + urlsplit.path
    urlsplit = urlparse.SplitResult(**urldict)

  return urlsplit.geturl()


def GitRemoteRepoList(directory, include_fetch=True, include_push=True,
                      logger=None):
  """Returns a list of remote git repos associated with a directory.

  Args:
      directory: Existing git working directory.
  Returns:
      List of (repo_name, repo_url) for tracked remote repos.
  """
  remote_repos = log_tools.CheckOutput(GitCmd() + ['remote', '-v'],
                                       logger=logger, cwd=directory)

  repo_set = set()
  for remote_repo_line in remote_repos.splitlines():
    repo_name, repo_url, repo_type = remote_repo_line.split()
    if include_fetch and repo_type == '(fetch)':
      repo_set.add((repo_name, repo_url))
    elif include_push and repo_type == '(push)':
      repo_set.add((repo_name, repo_url))

  return sorted(repo_set)


def GitSetRemoteRepo(url, directory, push_url=None,
                     repo_name='origin', logger=None):
  """Sets the remotely tracked URL for a git repository.

  Args:
      url: Remote git URL to set.
      directory: Local git repository to set tracked repo for.
      push_url: If specified, uses a different URL for pushing.
      repo_name: set the URL for a particular remote repo name.
  """
  git = GitCmd()
  try:
    log_tools.CheckCall(git + ['remote', 'set-url', repo_name, url],
                        logger=logger, cwd=directory)
  except subprocess.CalledProcessError:
    # If setting the URL failed, repo_name may be new. Try adding the URL.
    log_tools.CheckCall(git + ['remote', 'add', repo_name, url],
                        logger=logger, cwd=directory)

  if push_url:
    log_tools.CheckCall(git + ['remote', 'set-url', '--push',
                               repo_name, push_url],
                        logger=logger, cwd=directory)


def IsURLInRemoteRepoList(url, directory, include_fetch=True, include_push=True,
                          try_authenticated_url=True, logger=None):
  """Returns whether or not a url is a remote repo in a local git directory.

  Args:
      url: URL to look for in remote repo list.
      directory: Existing git working directory.
  """
  if try_authenticated_url:
    valid_urls = (url, GetAuthenticatedGitURL(url))
  else:
    valid_urls = (url,)

  remote_repo_list = GitRemoteRepoList(directory,
                                       include_fetch=include_fetch,
                                       include_push=include_push,
                                       logger=logger)
  return len([repo_name for
              repo_name, repo_url in remote_repo_list
              if repo_url in valid_urls]) > 0


def GitGetRepoAlternates(directory):
  """Gets the list of git alternates for a local git repo.

  Args:
      directory: Local git repository to get the git alternate for.

  Returns:
      List of git alternates set for the local git repository.
  """
  git_alternates_file = os.path.join(directory, GIT_ALTERNATES_PATH)
  if os.path.isfile(git_alternates_file):
    with open(git_alternates_file, 'rt') as f:
      alternates_list = []
      for line in f.readlines():
        line = line.strip()
        if line:
          if posixpath.basename(line) == 'objects':
            line = posixpath.dirname(line)
          alternates_list.append(line)

      return alternates_list

  return []


def GitSetRepoAlternates(directory, alternates_list, append=True, logger=None):
  """Sets the list of git alternates for a local git repo.

  Args:
      directory: Local git repository.
      alternates_list: List of local git repositories for the git alternates.
      append: If True, will append the list to currently set list of alternates.
  """
  if logger is None:
    logger = log_tools.GetConsoleLogger()
  git_alternates_file = os.path.join(directory, GIT_ALTERNATES_PATH)
  git_alternates_dir = os.path.dirname(git_alternates_file)
  if not os.path.isdir(git_alternates_dir):
    raise InvalidRepoException(directory,
                               'Invalid local git repo: %s', directory)

  original_alternates_list = GitGetRepoAlternates(directory)
  if append:
    alternates_list.extend(original_alternates_list)
    alternates_list = sorted(set(alternates_list))

  if set(original_alternates_list) != set(alternates_list):
    lines = [posixpath.join(line, 'objects') + '\n' for line in alternates_list]
    logger.info('Setting git alternates:\n\t%s', '\t'.join(lines))

    with open(git_alternates_file, 'wb') as f:
      f.writelines(lines)
