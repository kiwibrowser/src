# Copyright 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import glob
import imp
import logging
import os
import socket
import sys

from telemetry import decorators

import py_utils as catapult_util  # pylint: disable=import-error


IsRunningOnCrosDevice = ( # pylint: disable=invalid-name
    catapult_util.IsRunningOnCrosDevice)
GetCatapultDir = catapult_util.GetCatapultDir # pylint: disable=invalid-name


def GetBaseDir():
  main_module = sys.modules['__main__']
  if hasattr(main_module, '__file__'):
    return os.path.dirname(os.path.abspath(main_module.__file__))
  else:
    return os.getcwd()


def GetCatapultThirdPartyDir():
  return os.path.normpath(os.path.join(GetCatapultDir(), 'third_party'))


def GetTelemetryDir():
  return os.path.normpath(os.path.join(
      os.path.abspath(__file__), '..', '..', '..'))


def GetTelemetryThirdPartyDir():
  return os.path.join(GetTelemetryDir(), 'third_party')


def GetUnittestDataDir():
  return os.path.join(GetTelemetryDir(), 'telemetry', 'internal', 'testing')


def GetChromiumSrcDir():
  return os.path.normpath(os.path.join(GetTelemetryDir(), '..', '..', '..'))


_COUNTER = [0]


def _GetUniqueModuleName():
  _COUNTER[0] += 1
  return "page_set_module_" + str(_COUNTER[0])


def GetPythonPageSetModule(file_path):
  return imp.load_source(_GetUniqueModuleName(), file_path)


@decorators.Deprecated(
    2017, 6, 1,
    'telemetry.core.utils.WaitFor() is being deprecated. Please use '
    'catapult.common.py_utils.WaitFor() instead.')
def WaitFor(condition, timeout):
  return catapult_util.WaitFor(condition, timeout)


def GetUnreservedAvailableLocalPort():
  """Returns an available port on the system.

  WARNING: This method does not reserve the port it returns, so it may be used
  by something else before you get to use it. This can lead to flake.
  """
  # AF_INET restricts port to IPv4 addresses.
  # SOCK_STREAM means that it is a TCP socket.
  tmp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  # Setting SOL_SOCKET + SO_REUSEADDR to 1 allows the reuse of local addresses,
  # tihs is so sockets do not fail to bind for being in the CLOSE_WAIT state.
  tmp.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
  tmp.bind(('', 0))
  port = tmp.getsockname()[1]
  tmp.close()

  return port


def GetBuildDirectories(chrome_root=None):
  """Yields all combination of Chromium build output directories."""
  # chrome_root can be set to something else via --chrome-root.
  if not chrome_root:
    chrome_root = GetChromiumSrcDir()

  # CHROMIUM_OUTPUT_DIR can be set by --chromium-output-directory.
  output_dir = os.environ.get('CHROMIUM_OUTPUT_DIR')
  if output_dir:
    yield os.path.join(chrome_root, output_dir)
  elif os.path.exists('build.ninja'):
    yield os.getcwd()
  else:
    out_dir = os.environ.get('CHROMIUM_OUT_DIR')
    if out_dir:
      build_dirs = [out_dir]
    else:
      build_dirs = ['build',
                    'out',
                    'xcodebuild']

    build_types = ['Debug', 'Debug_x64', 'Release', 'Release_x64', 'Default']

    for build_dir in build_dirs:
      for build_type in build_types:
        yield os.path.join(chrome_root, build_dir, build_type)


def GetSequentialFileName(base_name):
  """Returns the next sequential file name based on |base_name| and the
  existing files. base_name should not contain extension.
  e.g: if base_name is /tmp/test, and /tmp/test_000.json,
  /tmp/test_001.mp3 exist, this returns /tmp/test_002. In case no
  other sequential file name exist, this will return /tmp/test_000
  """
  name, ext = os.path.splitext(base_name)
  assert ext == '', 'base_name cannot contain file extension.'
  index = 0
  while True:
    output_name = '%s_%03d' % (name, index)
    if not glob.glob(output_name + '.*'):
      break
    index = index + 1
  return output_name


def LogExtraDebugInformation(*args):
  """Call methods to obtain and log additional debug information.

  Example usage:

      def RunCommandWhichOutputsUsefulInfo():
        '''Output of some/useful_command'''
        return subprocess.check_output(
            ['bin/some/useful_command', '--with', 'args']).splitlines()

      def ReadFileWithUsefulInfo():
        '''Contents of that debug.info file'''
        with open('/path/to/that/debug.info') as f:
          for line in f:
            yield line

      LogExtraDebugInformation(
          RunCommandWhichOutputsUsefulInfo,
          ReadFileWithUsefulInfo
      )

  Args:
    Each arg is expected to be a function (or method) to be called with no
    arguments and returning a sequence of lines with debug info to be logged.
    The docstring of the function is also logged to provide further context.
    Exceptions raised during the call are ignored (but also logged), so it's
    OK to make calls which may fail.
  """
  # For local testing you may switch this to e.g. logging.WARNING
  level = logging.DEBUG
  if logging.getLogger().getEffectiveLevel() > level:
    logging.warning('Increase verbosity to see more debug information.')
    return

  logging.log(level, '== Dumping possibly useful debug information ==')
  for get_debug_lines in args:
    logging.log(level, '- %s:', get_debug_lines.__doc__)
    try:
      for line in get_debug_lines():
        logging.log(level, '  - %s', line)
    except Exception:  # pylint: disable=broad-except
      logging.log(level, 'Ignoring exception raised during %s.',
                  get_debug_lines.__name__, exc_info=True)
  logging.log(level, '===============================================')
