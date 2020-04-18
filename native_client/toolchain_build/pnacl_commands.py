#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runnables for toolchain_build_pnacl.py
"""

import base64
import os
import shutil
import stat
import subprocess
import sys
import tarfile

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import pynacl.file_tools
import pynacl.platform
import pynacl.repo_tools


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACL_DIR = os.path.dirname(SCRIPT_DIR)

# User-facing tools
DRIVER_TOOLS = ['pnacl-' + tool + '.py' for tool in
                    ('abicheck', 'ar', 'as', 'clang', 'clang++', 'compress',
                     'dis', 'driver', 'finalize', 'ld', 'nm', 'opt',
                     'ranlib', 'readelf', 'strip', 'translate')]
# Utilities used by the driver
DRIVER_UTILS = [name + '.py' for name in
                    ('artools', 'driver_env', 'driver_log', 'driver_temps',
                     'driver_tools', 'elftools', 'filetype', 'ldtools',
                     'loader', 'nativeld', 'pathtools', 'shelltools')]

# The archive itself contains the 'cmake343' directory.
PREBUILT_CMAKE_DIR = os.path.join(NACL_DIR, 'cmake343')
PREBUILT_CMAKE_ARCHIVE = 'cmake343_%s.tgz'
PREBUILT_CMAKE_URL = ('https://commondatastorage.googleapis.com/' +
                      'chromium-browser-clang/tools/')
PREBUILT_CMAKE_BIN = os.path.join(PREBUILT_CMAKE_DIR, 'bin', 'cmake')


def PrebuiltCmake():
  if (pynacl.platform.IsLinux() and not pynacl.platform.IsLinux64() or
      pynacl.platform.IsWindows()):
    # Prebuilt CMake does not work on linux32, and there is none for Windows.
    return 'cmake'
  return PREBUILT_CMAKE_BIN


def InstallPrebuiltCMake():
  if os.path.isdir(PREBUILT_CMAKE_DIR):
    print 'Prebuilt CMake directory already exists'
    if not os.path.isfile(PREBUILT_CMAKE_BIN):
      raise Exception('Prebuilt CMake dir %s exists but does not contain CMake'%
                      PREBUILT_CMAKE_DIR)
  else:
    assert not pynacl.platform.IsWindows()
    platform = 'Darwin' if pynacl.platform.IsMac() else 'Linux'
    filename = PREBUILT_CMAKE_ARCHIVE % platform
    url = PREBUILT_CMAKE_URL + filename
    os.mkdir(PREBUILT_CMAKE_DIR)
    download_target = os.path.join(PREBUILT_CMAKE_DIR, filename)
    pynacl.http_download.HttpDownload(url, download_target)

    print 'Downloaded %s' % url
    # The tar file itself includes the 'cmake343' directory, so set the
    # extract path to WORK_DIR to get the right path
    with open(download_target) as f:
      tarfile.open(mode='r:gz', fileobj=f).extractall(path=NACL_DIR)
    assert os.path.isfile(PREBUILT_CMAKE_BIN)
    print 'Extracted CMake to %s' % PREBUILT_CMAKE_DIR


def InstallDriverScripts(logger, subst, srcdir, dstdir, host_windows=False,
                         host_64bit=False, extra_config=[]):
  srcdir = subst.SubstituteAbsPaths(srcdir)
  dstdir = subst.SubstituteAbsPaths(dstdir)
  logger.debug('Installing Driver Scripts: %s -> %s', srcdir, dstdir)

  pynacl.file_tools.MakeDirectoryIfAbsent(os.path.join(dstdir, 'pydir'))
  for name in DRIVER_TOOLS + DRIVER_UTILS:
    source = os.path.join(srcdir, name)
    destination = os.path.join(dstdir, 'pydir')
    logger.debug('  Installing: %s -> %s', source, destination)
    shutil.copy(source, destination)
  # Install redirector sh/bat scripts
  for name in DRIVER_TOOLS:
    # Chop the .py off the name
    source = os.path.join(srcdir, 'redirect.sh')
    destination = os.path.join(dstdir, os.path.splitext(name)[0])
    logger.debug('  Installing: %s -> %s', source, destination)
    shutil.copy(source, destination)
    os.chmod(destination,
             stat.S_IRUSR | stat.S_IXUSR | stat.S_IWUSR | stat.S_IRGRP |
             stat.S_IWGRP | stat.S_IXGRP)

    if host_windows:
      # Windows gets both sh and bat extensions so it works w/cygwin and without
      win_source = os.path.join(srcdir, 'redirect.bat')
      win_dest = destination + '.bat'
      logger.debug('  Installing: %s -> %s', win_source, win_dest)
      shutil.copy(win_source, win_dest)
      os.chmod(win_dest,
               stat.S_IRUSR | stat.S_IXUSR | stat.S_IWUSR | stat.S_IRGRP |
               stat.S_IWGRP | stat.S_IXGRP)
  # Install the driver.conf file
  driver_conf = os.path.join(dstdir, 'driver.conf')
  logger.debug('  Installing: %s', driver_conf)
  with open(driver_conf, 'w') as f:
    print >> f, 'HAS_FRONTEND=1'
    print >> f, 'HOST_ARCH=x86_64' if host_64bit else 'HOST_ARCH=x86_32'
    for line in extra_config:
      print >> f, subst.Substitute(line)


def CheckoutGitBundleForTrybot(repo, destination):
  # For testing LLVM, Clang, etc. changes on the trybots, look for a
  # Git bundle file created by llvm_change_try_helper.sh.
  bundle_file = os.path.join(NACL_DIR, 'pnacl', 'not_for_commit',
                             '%s_bundle' % repo)
  base64_file = '%s.b64' % bundle_file
  if os.path.exists(base64_file):
    input_fh = open(base64_file, 'r')
    output_fh = open(bundle_file, 'wb')
    base64.decode(input_fh, output_fh)
    input_fh.close()
    output_fh.close()
    subprocess.check_call(
        pynacl.repo_tools.GitCmd() + ['fetch'],
        cwd=destination
    )
    subprocess.check_call(
        pynacl.repo_tools.GitCmd() + ['bundle', 'unbundle', bundle_file],
        cwd=destination
    )
    commit_id_file = os.path.join(NACL_DIR, 'pnacl', 'not_for_commit',
                                  '%s_commit_id' % repo)
    commit_id = open(commit_id_file, 'r').readline().strip()
    subprocess.check_call(
        pynacl.repo_tools.GitCmd() + ['checkout', commit_id],
        cwd=destination
    )

def CmdCheckoutGitBundleForTrybot(logger, subst, repo, destination):
  destination = subst.SubstituteAbsPaths(destination)
  logger.debug('Checking out Git Bundle for Trybot: %s [%s]', destination, repo)
  return CheckoutGitBundleForTrybot(repo, destination)


def WriteREVFile(logger, subst, dstfile, base_url, repos, revisions):
  # Install the REV file with repo info for all the components
  rev_file = subst.SubstituteAbsPaths(dstfile)
  logger.debug('Installing: %s', rev_file)
  with open(rev_file, 'w') as f:
    url, rev = pynacl.repo_tools.GitRevInfo(NACL_DIR)
    print >> f, '[GIT] %s: %s' % (url, rev)

    for name, revision in revisions.iteritems():
      repo = base_url + repos[name]
      print >> f, '[GIT] %s: %s' % (repo, revision)
