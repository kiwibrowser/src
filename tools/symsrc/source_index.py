#!/usr/bin/env python
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Usage: <win-path-to-pdb.pdb>
This tool will take a PDB on the command line, extract the source files that
were used in building the PDB, query the source server for which repository
and revision these files are at, and then finally write this information back
into the PDB in a format that the debugging tools understand.  This allows for
automatic source debugging, as all of the information is contained in the PDB,
and the debugger can go out and fetch the source files.

You most likely want to run these immediately after a build, since the source
input files need to match the generated PDB, and we want the correct
revision information for the exact files that were used for the build.

The following files from a windbg + source server installation are expected
to reside in the same directory as this python script:
  dbghelp.dll
  pdbstr.exe
  srctool.exe

NOTE: Expected to run under a native win32 python, NOT cygwin.  All paths are
dealt with as win32 paths, since we have to interact with the Microsoft tools.
"""

import os
import optparse
import sys
import tempfile
import time
import subprocess
import win32api

from collections import namedtuple

# This serves two purposes.  First, it acts as a whitelist, and only files
# from repositories listed here will be source indexed.  Second, it allows us
# to map from one URL to another, so we can map to external source servers.  It
# also indicates if the source for this project will be retrieved in a base64
# encoded format.
# TODO(sebmarchand): Initialize this variable in the main function and pass it
#     to the sub functions instead of having a global variable.
REPO_MAP = {
    'http://src.chromium.org/svn': {
        'url': 'https://src.chromium.org/chrome/'
            '{file_path}?revision={revision}',
        'base64': False
    },
    'https://src.chromium.org/svn': {
        'url': 'https://src.chromium.org/chrome/'
            '{file_path}?revision={revision}',
        'base64': False
    }
}


PROJECT_GROUPS = [
  # Googlecode SVN projects
  {
    'projects': [
      'angleproject',
      'google-breakpad',
      'google-cache-invalidation-api',
      'google-url',
      'googletest',
      'leveldb',
      'libphonenumber',
      'libyuv',
      'open-vcdiff',
      'ots',
      'sawbuck',
      'sfntly',
      'smhasher',
      'v8',
      'v8-i18n',
      'webrtc',
    ],
    'public_url': 'https://%s.googlecode.com/svn-history/' \
        'r{revision}/{file_path}',
    'svn_urls': [
        'svn://svn-mirror.golo.chromium.org/%s',
        'http://src.chromium.org/%s',
        'https://src.chromium.org/%s',
        'http://%s.googlecode.com/svn',
        'https://%s.googlecode.com/svn',
    ],
  },
  # Googlecode Git projects
  {
    'projects': [
      'syzygy',
    ],
    'public_url': 'https://%s.googlecode.com/git-history/' \
        '{revision}/{file_path}',
    'svn_urls': [
        'https://code.google.com/p/%s/',
    ],
  },
  # Chrome projects
  {
    'projects': [
        'blink',
        'chrome',
        'multivm',
        'native_client',
    ],
    'public_url': 'https://src.chromium.org/%s/' \
        '{file_path}?revision={revision}',
    'svn_urls': [
        'svn://chrome-svn/%s',
        'svn://chrome-svn.corp.google.com/%s',
        'svn://svn-mirror.golo.chromium.org/%s',
        'svn://svn.chromium.org/%s',
    ],
  },
]

# A named tuple used to store the information about a repository.
#
# It contains the following members:
#     - repo: The URL of the repository;
#     - rev: The revision (or hash) of the current checkout.
#     - file_list: The list of files coming from this repository.
#     - root_path: The root path of this checkout.
#     - path_prefix: A prefix to apply to the filename of the files coming from
#         this repository.
RevisionInfo = namedtuple('RevisionInfo',
                          ['repo', 'rev', 'files', 'root_path', 'path_prefix'])


def GetCasedFilePath(filename):
  """Return the correctly cased path for a given filename"""
  return win32api.GetLongPathName(win32api.GetShortPathName(unicode(filename)))


def FillRepositoriesMap():
  """ Fill the repositories map with the whitelisted projects. """
  for project_group in PROJECT_GROUPS:
    for project in project_group['projects']:
      for svn_url in project_group['svn_urls']:
        REPO_MAP[svn_url % project] = {
            'url': project_group['public_url'] % project,
            'base64': False
        }
      REPO_MAP[project_group['public_url'] % project] = None

FillRepositoriesMap()


def FindFile(filename):
  """Return the full windows path to a file in the same dir as this code."""
  thisdir = os.path.dirname(os.path.join(os.path.curdir, __file__))
  return os.path.abspath(os.path.join(thisdir, filename))


def RunCommand(*cmd, **kwargs):
  """Runs a command.

  Returns what have been printed to stdout by this command.

  kwargs:
    raise_on_failure: Indicates if an exception should be raised on failure, if
        set to false then the function will return None.
  """
  kwargs.setdefault('stdin', subprocess.PIPE)
  kwargs.setdefault('stdout', subprocess.PIPE)
  kwargs.setdefault('stderr', subprocess.PIPE)
  kwargs.setdefault('universal_newlines', True)
  raise_on_failure = kwargs.pop('raise_on_failure', True)

  proc = subprocess.Popen(cmd, **kwargs)
  ret, err = proc.communicate()
  if proc.returncode != 0:
    if raise_on_failure:
      print 'Error: %s' % err
      raise subprocess.CalledProcessError(proc.returncode, cmd)
    return

  ret = (ret or '').rstrip('\n')
  return ret


def ExtractSourceFiles(pdb_filename):
  """Extract a list of local paths of the source files from a PDB."""
  src_files = RunCommand(FindFile('srctool.exe'), '-r', pdb_filename)
  if not src_files or src_files.startswith("srctool: "):
    raise Exception("srctool failed: " + src_files)
  return set(x.lower() for x in src_files.split('\n') if len(x) != 0)


def ReadSourceStream(pdb_filename):
  """Read the contents of the source information stream from a PDB."""
  srctool = subprocess.Popen([FindFile('pdbstr.exe'),
                              '-r', '-s:srcsrv',
                              '-p:%s' % pdb_filename],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  data, _ = srctool.communicate()

  if ((srctool.returncode != 0 and srctool.returncode != -1) or
      data.startswith("pdbstr: ")):
    raise Exception("pdbstr failed: " + data)
  return data


def WriteSourceStream(pdb_filename, data):
  """Write the contents of the source information stream to a PDB."""
  # Write out the data to a temporary filename that we can pass to pdbstr.
  (f, fname) = tempfile.mkstemp()
  f = os.fdopen(f, "wb")
  f.write(data)
  f.close()

  srctool = subprocess.Popen([FindFile('pdbstr.exe'),
                              '-w', '-s:srcsrv',
                              '-i:%s' % fname,
                              '-p:%s' % pdb_filename],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  data, _ = srctool.communicate()

  if ((srctool.returncode != 0 and srctool.returncode != -1) or
      data.startswith("pdbstr: ")):
    raise Exception("pdbstr failed: " + data)

  os.unlink(fname)


def GetSVNRepoInfo(local_path):
  """Calls svn info to extract the SVN information about a path."""
  # We call svn.bat to make sure and get the depot tools SVN and not cygwin.
  info = RunCommand('svn.bat', 'info', local_path, raise_on_failure=False)
  if not info:
    return
  # Hack up into a dictionary of the fields printed by svn info.
  vals = dict((y.split(': ', 2) for y in info.split('\n') if y))
  return vals


def ExtractSVNInfo(local_filename):
  """Checks if a file is coming from a svn repository and if so returns some
  information about it.

  Args:
    local_filename: The name of the file that we want to check.

  Returns:
    None if the file doesn't come from a svn repository, otherwise it returns a
    RevisionInfo tuple.
  """
  # Try to get the svn information about this file.
  vals = GetSVNRepoInfo(local_filename)
  if not vals:
    return

  repo = vals['Repository Root']
  if not vals['URL'].startswith(repo):
    raise Exception("URL is not inside of the repository root?!?")
  rev  = vals['Revision']

  svn_local_root = os.path.split(local_filename)[0]

  # We need to look at the SVN URL of the current path to handle the case when
  # we do a partial SVN checkout inside another checkout of the same repository.
  # This happens in Chromium where we do some checkout of
  # '/trunk/deps/third_party' in 'src/third_party'.
  svn_root_url = os.path.dirname(vals['URL'])

  # Don't try to list all the files from this repository as this seem to slow
  # down the indexing, instead index one file at a time.
  file_list = [local_filename.replace(svn_local_root, '').lstrip(os.path.sep)]

  return RevisionInfo(repo=repo, rev=rev, files=file_list,
      root_path=svn_local_root, path_prefix=svn_root_url.replace(repo, ''))


def ExtractGitInfo(local_filename):
  """Checks if a file is coming from a git repository and if so returns some
  information about it.

  Args:
    local_filename: The name of the file that we want to check.

  Returns:
    None if the file doesn't come from a git repository, otherwise it returns a
    RevisionInfo tuple.
  """
  # Starts by checking if this file is coming from a git repository. For that
  # we'll start by calling 'git info' on this file; for this to work we need to
  # make sure that the current working directory is correctly cased. It turns
  # out that even on Windows the casing of the path passed in the |cwd| argument
  # of subprocess.Popen matters and if it's not correctly cased then 'git info'
  # will return None even if the file is coming from a git repository. This
  # is not the case if we're just interested in checking if the path containing
  # |local_filename| is coming from a git repository, in this case the casing
  # doesn't matter.
  local_filename = GetCasedFilePath(local_filename)
  local_file_basename = os.path.basename(local_filename)
  local_file_dir = os.path.dirname(local_filename)
  file_info = RunCommand('git.bat', 'log', '-n', '1', local_file_basename,
                          cwd=local_file_dir, raise_on_failure=False)

  if not file_info:
    return

  # Get the revision of the master branch.
  rev = RunCommand('git.bat', 'rev-parse', 'HEAD', cwd=local_file_dir)

  # Get the url of the remote repository.
  repo = RunCommand('git.bat', 'config', '--get', 'remote.origin.url',
      cwd=local_file_dir)
  # If the repository point to a local directory then we need to run this
  # command one more time from this directory to get the repository url.
  if os.path.isdir(repo):
    repo = RunCommand('git.bat', 'config', '--get', 'remote.origin.url',
        cwd=repo)

  # Don't use the authenticated path.
  repo = repo.replace('googlesource.com/a/', 'googlesource.com/')

  # Get the relative file path for this file in the git repository.
  git_path = RunCommand('git.bat', 'ls-tree', '--full-name', '--name-only',
      'HEAD', local_file_basename, cwd=local_file_dir).replace('/','\\')

  if not git_path:
    return

  git_root_path = local_filename.replace(git_path, '')

  if repo not in REPO_MAP:
    # Automatically adds the project coming from a git GoogleCode repository to
    # the repository map. The files from these repositories are accessible via
    # gitiles in a base64 encoded format.
    if 'chromium.googlesource.com' in repo:
      REPO_MAP[repo] = {
          'url': '%s/+/{revision}/{file_path}?format=TEXT' % repo,
          'base64': True
      }

  # Get the list of files coming from this repository.
  git_file_list = RunCommand('git.bat', 'ls-tree', '--full-name', '--name-only',
      'HEAD', '-r', cwd=git_root_path)

  file_list = [x for x in git_file_list.splitlines() if len(x) != 0]

  return RevisionInfo(repo=repo, rev=rev, files=file_list,
      root_path=git_root_path, path_prefix=None)


def IndexFilesFromRepo(local_filename, file_list, output_lines):
  """Checks if a given file is a part of a revision control repository (svn or
  git) and index all the files from this repository if it's the case.

  Args:
    local_filename: The filename of the current file.
    file_list: The list of files that should be indexed.
    output_lines: The source indexing lines that will be appended to the PDB.

  Returns the number of indexed files.
  """
  indexed_files = 0

  # Try to extract the revision info for the current file.
  info = ExtractGitInfo(local_filename)
  if not info:
    info = ExtractSVNInfo(local_filename)

  repo = info.repo
  rev = info.rev
  files = info.files
  root_path = info.root_path.lower()

  # Checks if we should index this file and if the source that we'll retrieve
  # will be base64 encoded.
  should_index = False
  base_64 = False
  if repo in REPO_MAP:
    should_index = True
    base_64 = REPO_MAP[repo].get('base64')
  else:
    repo = None

  # Iterates over the files from this repo and index them if needed.
  for file_iter in files:
    current_filename = file_iter.lower()
    full_file_path = os.path.normpath(os.path.join(root_path, current_filename))
    # Checks if the file is in the list of files to be indexed.
    if full_file_path in file_list:
      if should_index:
        source_url = ''
        current_file = file_iter
        # Prefix the filename with the prefix for this repository if needed.
        if info.path_prefix:
          current_file = os.path.join(info.path_prefix, current_file)
        source_url = REPO_MAP[repo].get('url').format(revision=rev,
            file_path=os.path.normpath(current_file).replace('\\', '/'))
        output_lines.append('%s*%s*%s*%s*%s' % (full_file_path, current_file,
            rev, source_url, 'base64.b64decode' if base_64 else ''))
        indexed_files += 1
        file_list.remove(full_file_path)

  # The input file should have been removed from the list of files to index.
  if indexed_files and local_filename in file_list:
    print '%s shouldn\'t be in the list of files to index anymore.' % \
        local_filename
    # TODO(sebmarchand): Turn this into an exception once I've confirmed that
    #     this doesn't happen on the official builder.
    file_list.remove(local_filename)

  return indexed_files


def DirectoryIsUnderPublicVersionControl(local_dir):
  # Checks if this directory is from a Git checkout.
  info = RunCommand('git.bat', 'config', '--get', 'remote.origin.url',
      cwd=local_dir, raise_on_failure=False)
  if info:
    return True

  # If not checks if it's from a SVN checkout.
  info = GetSVNRepoInfo(local_dir)
  if info:
    return True

  return False


def UpdatePDB(pdb_filename, verbose=True, build_dir=None, toolchain_dir=None):
  """Update a pdb file with source information."""
  dir_blacklist = { }

  if build_dir:
    # Blacklisting the build directory allows skipping the generated files, for
    # Chromium this makes the indexing ~10x faster.
    build_dir = (os.path.normpath(build_dir)).lower()
    for directory, _, _ in os.walk(build_dir):
      dir_blacklist[directory.lower()] = True
    dir_blacklist[build_dir.lower()] = True

  if toolchain_dir:
    # Blacklisting the directories from the toolchain as we don't have revision
    # info for them.
    toolchain_dir = (os.path.normpath(toolchain_dir)).lower()
    for directory, _, _ in os.walk(build_dir):
      dir_blacklist[directory.lower()] = True
    dir_blacklist[toolchain_dir.lower()] = True

  # Writes the header of the source index stream.
  #
  # Here's the description of the variables used in the SRC_* macros (those
  # variables have to be defined for every source file that we want to index):
  #   var1: The file path.
  #   var2: The name of the file without its path.
  #   var3: The revision or the hash of this file's repository.
  #   var4: The URL to this file.
  #   var5: (optional) The python method to call to decode this file, e.g. for
  #       a base64 encoded file this value should be 'base64.b64decode'.
  lines = [
    'SRCSRV: ini ------------------------------------------------',
    'VERSION=1',
    'INDEXVERSION=2',
    'VERCTRL=Subversion',
    'DATETIME=%s' % time.asctime(),
    'SRCSRV: variables ------------------------------------------',
    'SRC_EXTRACT_TARGET_DIR=%targ%\%fnbksl%(%var2%)\%var3%',
    'SRC_EXTRACT_TARGET=%SRC_EXTRACT_TARGET_DIR%\%fnfile%(%var1%)',
    'SRC_EXTRACT_CMD=cmd /c "mkdir "%SRC_EXTRACT_TARGET_DIR%" & python -c '
        '"import urllib2, base64;'
        'url = \\\"%var4%\\\";'
        'u = urllib2.urlopen(url);'
        'print %var5%(u.read());" > "%SRC_EXTRACT_TARGET%""',
    'SRCSRVTRG=%SRC_EXTRACT_TARGET%',
    'SRCSRVCMD=%SRC_EXTRACT_CMD%',
    'SRCSRV: source files ---------------------------------------',
  ]

  if ReadSourceStream(pdb_filename):
    raise Exception("PDB already has source indexing information!")

  filelist = ExtractSourceFiles(pdb_filename)
  number_of_files = len(filelist)
  indexed_files_total = 0
  while filelist:
    filename = next(iter(filelist))
    filedir = os.path.dirname(filename)
    if verbose:
      print "[%d / %d] Processing: %s" % (number_of_files - len(filelist),
          number_of_files, filename)

    # This directory is blacklisted, either because it's not part of a
    # repository, or from one we're not interested in indexing.
    if dir_blacklist.get(filedir, False):
      if verbose:
        print "  skipping, directory is blacklisted."
      filelist.remove(filename)
      continue

    # Skip the files that don't exist on the current machine.
    if not os.path.exists(filename):
      filelist.remove(filename)
      continue

    # Try to index the current file and all the ones coming from the same
    # repository.
    indexed_files = IndexFilesFromRepo(filename, filelist, lines)
    if not indexed_files:
      if not DirectoryIsUnderPublicVersionControl(filedir):
        dir_blacklist[filedir] = True
        if verbose:
          print "Adding %s to the blacklist." % filedir
      filelist.remove(filename)
      continue

    indexed_files_total += indexed_files

    if verbose:
      print "  %d files have been indexed." % indexed_files

  lines.append('SRCSRV: end ------------------------------------------------')

  WriteSourceStream(pdb_filename, '\r\n'.join(lines))

  if verbose:
    print "%d / %d files have been indexed." % (indexed_files_total,
                                                number_of_files)


def main():
  parser = optparse.OptionParser()
  parser.add_option('-v', '--verbose', action='store_true', default=False)
  parser.add_option('--build-dir', help='The original build directory, if set '
      'all the files present in this directory (or one of its subdirectories) '
      'will be skipped.')
  parser.add_option('--toolchain-dir', help='The directory containing the '
      'toolchain that has been used for this build. If set all the files '
      'present in this directory (or one of its subdirectories) will be '
      'skipped.')
  options, args = parser.parse_args()

  if not args:
    parser.error('Specify a pdb')

  for pdb in args:
    UpdatePDB(pdb, options.verbose, options.build_dir)

  return 0


if __name__ == '__main__':
  sys.exit(main())
