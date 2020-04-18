# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generic utilities for all python scripts."""

import atexit
import httplib
import os
import platform
import signal
import stat
import subprocess
import sys
import tempfile
import urlparse
import zipfile

import chrome_paths


def GetPlatformName():
  """Return a string to be used in paths for the platform."""
  if IsWindows():
    return 'win'
  if IsMac():
    return 'mac'
  if IsLinux():
    return 'linux'
  raise NotImplementedError('Unknown platform "%s".' % sys.platform)


def IsWindows():
  return sys.platform == 'cygwin' or sys.platform.startswith('win')


def IsLinux():
  return sys.platform.startswith('linux')


def IsMac():
  return sys.platform.startswith('darwin')


def Is64Bit():
  # This function checks whether the target (not host) word size is 64-bits.
  # Since 64-bit hosts can cross-compile 32-bit binaries, check the GN args to
  # see what CPU we're targetting.
  try:
    args_gn = os.path.join(chrome_paths.GetBuildDir(['args.gn']), 'args.gn')
    with open(args_gn) as build_args:
      for build_arg in build_args:
        decommented = build_arg.split('#')[0]
        key_and_value = decommented.split('=')
        if len(key_and_value) != 2:
          continue
        key = key_and_value[0].strip()
        value = key_and_value[1].strip()
        if key == 'target_cpu':
          return value.endswith('64')
  except:
    pass
  # If we don't find anything, or if there is no GN args file, default to the
  # host architecture.
  return platform.architecture()[0] == '64bit'


def GetAbsolutePathOfUserPath(user_path):
  """Expand the given |user_path| (like "~/file") and return its absolute path.
  """
  if user_path is None:
    return None
  return os.path.abspath(os.path.expanduser(user_path))


def _DeleteDir(path):
  """Deletes a directory recursively, which must exist."""
  # Don't use shutil.rmtree because it can't delete read-only files on Win.
  for root, dirs, files in os.walk(path, topdown=False):
    for name in files:
      filename = os.path.join(root, name)
      os.chmod(filename, stat.S_IWRITE)
      os.remove(filename)
    for name in dirs:
      os.rmdir(os.path.join(root, name))
  os.rmdir(path)


def Delete(path):
  """Deletes the given file or directory (recursively), which must exist."""
  if os.path.isdir(path):
    _DeleteDir(path)
  else:
    os.remove(path)


def MaybeDelete(path):
  """Deletes the given file or directory (recurisvely), if it exists."""
  if os.path.exists(path):
    Delete(path)


def MakeTempDir(parent_dir=None):
  """Creates a temporary directory and returns an absolute path to it.

  The temporary directory is automatically deleted when the python interpreter
  exits normally.

  Args:
    parent_dir: the directory to create the temp dir in. If None, the system
                temp dir is used.

  Returns:
    The absolute path to the temporary directory.
  """
  path = tempfile.mkdtemp(prefix='chromedriver_', dir=parent_dir)
  atexit.register(MaybeDelete, path)
  return path


def Zip(path):
  """Zips the given path and returns the zipped file."""
  zip_path = os.path.join(MakeTempDir(), 'build.zip')
  f = zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED)
  f.write(path, os.path.basename(path))
  f.close()
  return zip_path


def Unzip(zip_path, output_dir):
  """Unzips the given zip file using a system installed unzip tool.

  Args:
    zip_path: zip file to unzip.
    output_dir: directory to unzip the contents of the zip file. The directory
                must exist.

  Raises:
    RuntimeError if the unzip operation fails.
  """
  if IsWindows():
    unzip_cmd = ['C:\\Program Files\\7-Zip\\7z.exe', 'x', '-y']
  else:
    unzip_cmd = ['unzip', '-o']
  unzip_cmd += [zip_path]
  if RunCommand(unzip_cmd, output_dir) != 0:
    raise RuntimeError('Unable to unzip %s to %s' % (zip_path, output_dir))


def Kill(pid):
  """Terminate the given pid."""
  if IsWindows():
    subprocess.call(['taskkill.exe', '/T', '/F', '/PID', str(pid)])
  else:
    os.kill(pid, signal.SIGTERM)


def RunCommand(cmd, cwd=None):
  """Runs the given command and returns the exit code.

  Args:
    cmd: list of command arguments.
    cwd: working directory to execute the command, or None if the current
         working directory should be used.

  Returns:
    The exit code of the command.
  """
  sys.stdout.flush()
  process = subprocess.Popen(cmd, cwd=cwd)
  process.wait()
  sys.stdout.flush()
  return process.returncode


def DoesUrlExist(url):
  """Determines whether a resource exists at the given URL.

  Args:
    url: URL to be verified.

  Returns:
    True if url exists, otherwise False.
  """
  parsed = urlparse.urlparse(url)
  try:
    conn = httplib.HTTPConnection(parsed.netloc)
    conn.request('HEAD', parsed.path)
    response = conn.getresponse()
  except httplib.HTTPException:
    return False
  finally:
    conn.close()
  # Follow both permanent (301) and temporary (302) redirects.
  if response.status == 302 or response.status == 301:
    return DoesUrlExist(response.getheader('location'))
  return response.status == 200


def MarkBuildStepStart(name):
  print '@@@BUILD_STEP %s@@@' % name
  sys.stdout.flush()


def MarkBuildStepError():
  print '@@@STEP_FAILURE@@@'
  sys.stdout.flush()


def AddBuildStepText(text):
  print '@@@STEP_TEXT@%s@@@' % text
  sys.stdout.flush()


def PrintAndFlush(text):
  print text
  sys.stdout.flush()


def AddLink(label, url):
  """Adds a link with name |label| linking to |url| to current buildbot step.

  Args:
    label: A string with the name of the label.
    url: A string of the URL.
  """
  print '@@@STEP_LINK@%s@%s@@@' % (label, url)
