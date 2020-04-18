# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import glob
import heapq
import logging
import os
import os.path
import random
import re
import shutil
import subprocess as subprocess
import sys
import tempfile
import time

import py_utils
from py_utils import cloud_storage  # pylint: disable=import-error
import dependency_manager  # pylint: disable=import-error

from telemetry.internal.util import binary_manager
from telemetry.core import exceptions
from telemetry.internal.backends.chrome import chrome_browser_backend
from telemetry.internal.util import path


DEVTOOLS_ACTIVE_PORT_FILE = 'DevToolsActivePort'


def ParseCrashpadDateTime(date_time_str):
  # Python strptime does not support time zone parsing, strip it.
  date_time_parts = date_time_str.split()
  if len(date_time_parts) >= 3:
    date_time_str = ' '.join(date_time_parts[:2])
  return datetime.datetime.strptime(date_time_str, '%Y-%m-%d %H:%M:%S')


def GetSymbolBinaries(minidump, arch_name, os_name):
  # Returns binary file where symbols are located.
  minidump_dump = binary_manager.FetchPath('minidump_dump', arch_name, os_name)
  assert minidump_dump

  symbol_binaries = []

  minidump_cmd = [minidump_dump, minidump]
  try:
    with open(os.devnull, 'wb') as dev_null:
      minidump_output = subprocess.check_output(minidump_cmd, stderr=dev_null)
  except subprocess.CalledProcessError as e:
    # For some reason minidump_dump always fails despite successful dumping.
    minidump_output = e.output

  minidump_binary_re = re.compile(r'\W+\(code_file\)\W+=\W\"(.*)\"')
  for minidump_line in minidump_output.splitlines():
    line_match = minidump_binary_re.match(minidump_line)
    if line_match:
      binary_path = line_match.group(1)
      if not os.path.isfile(binary_path):
        continue

      # Filter out system binaries.
      if (binary_path.startswith('/usr/lib/') or
          binary_path.startswith('/System/Library/') or
          binary_path.startswith('/lib/')):
        continue

      # Filter out other binary file types which have no symbols.
      if (binary_path.endswith('.pak') or
          binary_path.endswith('.bin') or
          binary_path.endswith('.dat') or
          binary_path.endswith('.ttf')):
        continue

      symbol_binaries.append(binary_path)
  return symbol_binaries


def GenerateBreakpadSymbols(minidump, arch, os_name, symbols_dir, browser_dir):
  logging.info('Dumping breakpad symbols.')
  generate_breakpad_symbols_command = binary_manager.FetchPath(
      'generate_breakpad_symbols', arch, os_name)
  if not generate_breakpad_symbols_command:
    return

  for binary_path in GetSymbolBinaries(minidump, arch, os_name):
    cmd = [
        sys.executable,
        generate_breakpad_symbols_command,
        '--binary=%s' % binary_path,
        '--symbols-dir=%s' % symbols_dir,
        '--build-dir=%s' % browser_dir,
        ]

    try:
      subprocess.check_call(cmd, stderr=open(os.devnull, 'w'))
    except subprocess.CalledProcessError:
      logging.warning('Failed to execute "%s"', ' '.join(cmd))
      return


class DesktopBrowserBackend(chrome_browser_backend.ChromeBrowserBackend):
  """The backend for controlling a locally-executed browser instance, on Linux,
  Mac or Windows.
  """
  def __init__(self, desktop_platform_backend, browser_options,
               browser_directory, profile_directory,
               executable, flash_path, is_content_shell):
    super(DesktopBrowserBackend, self).__init__(
        desktop_platform_backend,
        browser_options=browser_options,
        browser_directory=browser_directory,
        profile_directory=profile_directory,
        supports_extensions=not is_content_shell,
        supports_tab_control=not is_content_shell)
    self._executable = executable
    self._flash_path = flash_path
    self._is_content_shell = is_content_shell

    # Initialize fields so that an explosion during init doesn't break in Close.
    self._proc = None
    self._tmp_output_file = None
    # pylint: disable=invalid-name
    self._most_recent_symbolized_minidump_paths = set([])
    self._minidump_path_crashpad_retrieval = {}
    # pylint: enable=invalid-name

    if not self._executable:
      raise Exception('Cannot create browser, no executable found!')

    if self._flash_path and not os.path.exists(self._flash_path):
      raise RuntimeError('Flash path does not exist: %s' % self._flash_path)

    self._tmp_minidump_dir = tempfile.mkdtemp()
    if self.is_logging_enabled:
      self._log_file_path = os.path.join(tempfile.mkdtemp(), 'chrome.log')
    else:
      self._log_file_path = None

  @property
  def is_logging_enabled(self):
    return self.browser_options.logging_verbosity in [
        self.browser_options.NON_VERBOSE_LOGGING,
        self.browser_options.VERBOSE_LOGGING,
        self.browser_options.SUPER_VERBOSE_LOGGING]

  @property
  def log_file_path(self):
    return self._log_file_path

  @property
  def supports_uploading_logs(self):
    return (self.browser_options.logs_cloud_bucket and self.log_file_path and
            os.path.isfile(self.log_file_path))

  def _GetDevToolsActivePortPath(self):
    return os.path.join(self.profile_directory, DEVTOOLS_ACTIVE_PORT_FILE)

  def _GetCdbPath(self):
    # cdb.exe might have been co-located with the browser's executable
    # during the build, but that's not a certainty. (This is only done
    # in Chromium builds on the bots, which is why it's not a hard
    # requirement.) See if it's available.
    colocated_cdb = os.path.join(self.browser_directory, 'cdb', 'cdb.exe')
    if path.IsExecutable(colocated_cdb):
      return colocated_cdb
    possible_paths = (
        # Installed copies of the Windows SDK.
        os.path.join('Windows Kits', '*', 'Debuggers', 'x86'),
        os.path.join('Windows Kits', '*', 'Debuggers', 'x64'),
        # Old copies of the Debugging Tools for Windows.
        'Debugging Tools For Windows',
        'Debugging Tools For Windows (x86)',
        'Debugging Tools For Windows (x64)',
        # The hermetic copy of the Windows toolchain in depot_tools.
        os.path.join('win_toolchain', 'vs_files', '*', 'win_sdk',
                     'Debuggers', 'x86'),
        os.path.join('win_toolchain', 'vs_files', '*', 'win_sdk',
                     'Debuggers', 'x64'),
    )
    for possible_path in possible_paths:
      app_path = os.path.join(possible_path, 'cdb.exe')
      app_path = path.FindInstalledWindowsApplication(app_path)
      if app_path:
        return app_path
    return None

  def _FindDevToolsPortAndTarget(self):
    devtools_file_path = self._GetDevToolsActivePortPath()
    if not os.path.isfile(devtools_file_path):
      raise EnvironmentError('DevTools file doest not exist yet')
    # Attempt to avoid reading the file until it's populated.
    # Both stat and open may raise IOError if not ready, the caller will retry.
    lines = None
    if os.stat(devtools_file_path).st_size > 0:
      with open(devtools_file_path) as f:
        lines = [line.rstrip() for line in f]
    if not lines:
      raise EnvironmentError('DevTools file empty')

    devtools_port = int(lines[0])
    browser_target = lines[1] if len(lines) >= 2 else None
    return devtools_port, browser_target

  def GetBrowserStartupUrl(self):
    # TODO(crbug.com/787834): Move to the corresponding possible-browser class.
    return self.browser_options.startup_url

  def Start(self, startup_args, startup_url=None):
    assert not self._proc, 'Must call Close() before Start()'

    # macOS displays a blocking crash resume dialog that we need to suppress.
    if self.browser.platform.GetOSName() == 'mac':
      # Default write expects either the application name or the
      # path to the application. self._executable has the path to the app
      # with a few other bits tagged on after .app. Thus, we shorten the path
      # to end with .app. If this is ineffective on your mac, please delete
      # the saved state of the browser you are testing on here:
      # /Users/.../Library/Saved\ Application State/...
      # http://stackoverflow.com/questions/20226802
      dialog_path = re.sub(r'\.app\/.*', '.app', self._executable)
      subprocess.check_call([
          'defaults', 'write', '-app', dialog_path, 'NSQuitAlwaysKeepsWindows',
          '-bool', 'false'
      ])

    cmd = [self._executable]
    cmd.extend(startup_args)
    if startup_url:
      cmd.append(startup_url)
    env = os.environ.copy()
    env['CHROME_HEADLESS'] = '1'  # Don't upload minidumps.
    env['BREAKPAD_DUMP_LOCATION'] = self._tmp_minidump_dir
    if self.is_logging_enabled:
      sys.stderr.write(
          'Chrome log file will be saved in %s\n' % self.log_file_path)
      env['CHROME_LOG_FILE'] = self.log_file_path
    logging.info('Starting Chrome %s', cmd)

    if not self.browser_options.show_stdout:
      self._tmp_output_file = tempfile.NamedTemporaryFile('w', 0)
      self._proc = subprocess.Popen(
          cmd, stdout=self._tmp_output_file, stderr=subprocess.STDOUT, env=env)
    else:
      self._proc = subprocess.Popen(cmd, env=env)

    try:
      self.BindDevToolsClient()
      # browser is foregrounded by default on Windows and Linux, but not Mac.
      if self.browser.platform.GetOSName() == 'mac':
        subprocess.Popen([
            'osascript', '-e',
            ('tell application "%s" to activate' % self._executable)
        ])
      if self._supports_extensions:
        self._WaitForExtensionsToLoad()
    except:
      self.Close()
      raise

  def BindDevToolsClient(self):
    # In addition to the work performed by the base class, quickly check if
    # the browser process is still alive.
    if not self.IsBrowserRunning():
      raise exceptions.ProcessGoneException(
          'Return code: %d' % self._proc.returncode)
    super(DesktopBrowserBackend, self).BindDevToolsClient()

  @property
  def pid(self):
    if self._proc:
      return self._proc.pid
    return None

  def IsBrowserRunning(self):
    return self._proc and self._proc.poll() == None

  def GetStandardOutput(self):
    if not self._tmp_output_file:
      if self.browser_options.show_stdout:
        # This can happen in the case that loading the Chrome binary fails.
        # We print rather than using logging here, because that makes a
        # recursive call to this function.
        print >> sys.stderr, "Can't get standard output with --show-stdout"
      return ''
    self._tmp_output_file.flush()
    try:
      with open(self._tmp_output_file.name) as f:
        return f.read()
    except IOError:
      return ''

  def _MinidumpObtainedFromCrashpad(self, minidump):
    if minidump in self._minidump_path_crashpad_retrieval:
      return self._minidump_path_crashpad_retrieval[minidump]
    # Default to crashpad where we hope to be eventually
    return True

  def _GetAllCrashpadMinidumps(self):
    if not self._tmp_minidump_dir:
      logging.warning('No _tmp_minidump_dir; browser already closed?')
      return None
    os_name = self.browser.platform.GetOSName()
    arch_name = self.browser.platform.GetArchName()
    try:
      crashpad_database_util = binary_manager.FetchPath(
          'crashpad_database_util', arch_name, os_name)
      if not crashpad_database_util:
        logging.warning('No crashpad_database_util found')
        return None
    except dependency_manager.NoPathFoundError:
      logging.warning('No path to crashpad_database_util found')
      return None

    logging.info('Found crashpad_database_util')

    report_output = subprocess.check_output([
        crashpad_database_util, '--database=' + self._tmp_minidump_dir,
        '--show-pending-reports', '--show-completed-reports',
        '--show-all-report-info'])

    last_indentation = -1
    reports_list = []
    report_dict = {}
    for report_line in report_output.splitlines():
      # Report values are grouped together by the same indentation level.
      current_indentation = 0
      for report_char in report_line:
        if not report_char.isspace():
          break
        current_indentation += 1

      # Decrease in indentation level indicates a new report is being printed.
      if current_indentation >= last_indentation:
        report_key, report_value = report_line.split(':', 1)
        if report_value:
          report_dict[report_key.strip()] = report_value.strip()
      elif report_dict:
        try:
          report_time = ParseCrashpadDateTime(report_dict['Creation time'])
          report_path = report_dict['Path'].strip()
          reports_list.append((report_time, report_path))
        except (ValueError, KeyError) as e:
          logging.warning('Crashpad report expected valid keys'
                          ' "Path" and "Creation time": %s', e)
        finally:
          report_dict = {}

      last_indentation = current_indentation

    # Include the last report.
    if report_dict:
      try:
        report_time = ParseCrashpadDateTime(report_dict['Creation time'])
        report_path = report_dict['Path'].strip()
        reports_list.append((report_time, report_path))
      except (ValueError, KeyError) as e:
        logging.warning(
            'Crashpad report expected valid keys'
            ' "Path" and "Creation time": %s', e)

    return reports_list

  def _GetMostRecentCrashpadMinidump(self):
    reports_list = self._GetAllCrashpadMinidumps()
    if reports_list:
      _, most_recent_report_path = max(reports_list)
      return most_recent_report_path

    return None

  def _GetBreakPadMinidumpPaths(self):
    if not self._tmp_minidump_dir:
      logging.warning('No _tmp_minidump_dir; browser already closed?')
      return None
    return glob.glob(os.path.join(self._tmp_minidump_dir, '*.dmp'))

  def _GetMostRecentMinidump(self):
    # Crashpad dump layout will be the standard eventually, check it first.
    crashpad_dump = True
    most_recent_dump = self._GetMostRecentCrashpadMinidump()

    # Typical breakpad format is simply dump files in a folder.
    if not most_recent_dump:
      crashpad_dump = False
      logging.info('No minidump found via crashpad_database_util')
      dumps = self._GetBreakPadMinidumpPaths()
      if dumps:
        most_recent_dump = heapq.nlargest(1, dumps, os.path.getmtime)[0]
        if most_recent_dump:
          logging.info('Found minidump via globbing in minidump dir')

    # As a sanity check, make sure the crash dump is recent.
    if (most_recent_dump and
        os.path.getmtime(most_recent_dump) < (time.time() - (5 * 60))):
      logging.warning('Crash dump is older than 5 minutes. May not be correct.')

    self._minidump_path_crashpad_retrieval[most_recent_dump] = crashpad_dump
    return most_recent_dump

  def _IsExecutableStripped(self):
    if self.browser.platform.GetOSName() == 'mac':
      try:
        symbols = subprocess.check_output(['/usr/bin/nm', self._executable])
      except subprocess.CalledProcessError as err:
        logging.warning(
            'Error when checking whether executable is stripped: %s',
            err.output)
        # Just assume that binary is stripped to skip breakpad symbol generation
        # if this check failed.
        return True
      num_symbols = len(symbols.splitlines())
      # We assume that if there are more than 10 symbols the executable is not
      # stripped.
      return num_symbols < 10
    else:
      return False

  def _GetStackFromMinidump(self, minidump):
    os_name = self.browser.platform.GetOSName()
    if os_name == 'win':
      cdb = self._GetCdbPath()
      if not cdb:
        logging.warning('cdb.exe not found.')
        return None
      # Move to the thread which triggered the exception (".ecxr"). Then include
      # a description of the exception (".lastevent"). Also include all the
      # threads' stacks ("~*kb30") as well as the ostensibly crashed stack
      # associated with the exception context record ("kb30"). Note that stack
      # dumps, including that for the crashed thread, may not be as precise as
      # the one starting from the exception context record.
      # Specify kb instead of k in order to get four arguments listed, for
      # easier diagnosis from stacks.
      output = subprocess.check_output([cdb, '-y', self.browser_directory,
                                        '-c', '.ecxr;.lastevent;kb30;~*kb30;q',
                                        '-z', minidump])
      # The output we care about starts with "Last event:" or possibly
      # other things we haven't seen yet. If we can't find the start of the
      # last event entry, include output from the beginning.
      info_start = 0
      info_start_match = re.search("Last event:", output, re.MULTILINE)
      if info_start_match:
        info_start = info_start_match.start()
      info_end = output.find('quit:')
      return output[info_start:info_end]

    arch_name = self.browser.platform.GetArchName()
    stackwalk = binary_manager.FetchPath(
        'minidump_stackwalk', arch_name, os_name)
    if not stackwalk:
      logging.warning('minidump_stackwalk binary not found.')
      return None
    # We only want this logic on linux platforms that are still using breakpad.
    # See crbug.com/667475
    if not self._MinidumpObtainedFromCrashpad(minidump):
      with open(minidump, 'rb') as infile:
        minidump += '.stripped'
        with open(minidump, 'wb') as outfile:
          outfile.write(''.join(infile.read().partition('MDMP')[1:]))

    symbols_path = os.path.join(self._tmp_minidump_dir, 'symbols')
    GenerateBreakpadSymbols(minidump, arch_name, os_name,
                            symbols_path, self.browser_directory)

    return subprocess.check_output([stackwalk, minidump, symbols_path],
                                   stderr=open(os.devnull, 'w'))

  def _UploadMinidumpToCloudStorage(self, minidump_path):
    """ Upload minidump_path to cloud storage and return the cloud storage url.
    """
    remote_path = ('minidump-%s-%i.dmp' %
                   (datetime.datetime.now().strftime('%Y-%m-%d_%H-%M-%S'),
                    random.randint(0, 1000000)))
    try:
      return cloud_storage.Insert(cloud_storage.TELEMETRY_OUTPUT, remote_path,
                                  minidump_path)
    except cloud_storage.CloudStorageError as err:
      logging.error('Cloud storage error while trying to upload dump: %s',
                    repr(err))
      return '<Missing link>'

  def GetStackTrace(self):
    """Returns a stack trace if a valid minidump is found, will return a tuple
       (valid, output) where valid will be True if a valid minidump was found
       and output will contain either an error message or the attempt to
       symbolize the minidump if one was found.
    """
    most_recent_dump = self._GetMostRecentMinidump()
    if not most_recent_dump:
      return (False, 'No crash dump found.')
    logging.info('Minidump found: %s', most_recent_dump)
    return self._InternalSymbolizeMinidump(most_recent_dump)

  def GetMostRecentMinidumpPath(self):
    return self._GetMostRecentMinidump()

  def GetAllMinidumpPaths(self):
    reports_list = self._GetAllCrashpadMinidumps()
    if reports_list:
      for report in reports_list:
        self._minidump_path_crashpad_retrieval[report[1]] = True
      return [report[1] for report in reports_list]
    else:
      logging.info('No minidump found via crashpad_database_util')
      dumps = self._GetBreakPadMinidumpPaths()
      if dumps:
        logging.info('Found minidump via globbing in minidump dir')
        for dump in dumps:
          self._minidump_path_crashpad_retrieval[dump] = False
        return dumps
      return []

  def GetAllUnsymbolizedMinidumpPaths(self):
    minidump_paths = set(self.GetAllMinidumpPaths())
    # If we have already symbolized paths remove them from the list
    unsymbolized_paths = (
        minidump_paths - self._most_recent_symbolized_minidump_paths)
    return list(unsymbolized_paths)

  def SymbolizeMinidump(self, minidump_path):
    return self._InternalSymbolizeMinidump(minidump_path)

  def _InternalSymbolizeMinidump(self, minidump_path):
    cloud_storage_link = self._UploadMinidumpToCloudStorage(minidump_path)

    stack = self._GetStackFromMinidump(minidump_path)
    if not stack:
      error_message = ('Failed to symbolize minidump. Raw stack is uploaded to'
                       ' cloud storage: %s.' % cloud_storage_link)
      return (False, error_message)

    self._most_recent_symbolized_minidump_paths.add(minidump_path)
    return (True, stack)

  def __del__(self):
    self.Close()

  def _TryCooperativeShutdown(self):
    if self.browser.platform.IsCooperativeShutdownSupported():
      # Ideally there would be a portable, cooperative shutdown
      # mechanism for the browser. This seems difficult to do
      # correctly for all embedders of the content API. The only known
      # problem with unclean shutdown of the browser process is on
      # Windows, where suspended child processes frequently leak. For
      # now, just solve this particular problem. See Issue 424024.
      if self.browser.platform.CooperativelyShutdown(self._proc, "chrome"):
        try:
          # Use a long timeout to handle slow Windows debug
          # (see crbug.com/815004)
          py_utils.WaitFor(lambda: not self.IsBrowserRunning(), timeout=15)
          logging.info('Successfully shut down browser cooperatively')
        except py_utils.TimeoutException as e:
          logging.warning('Failed to cooperatively shutdown. ' +
                          'Proceeding to terminate: ' + str(e))

  def Background(self):
    raise NotImplementedError

  def Close(self):
    super(DesktopBrowserBackend, self).Close()

    # First, try to cooperatively shutdown.
    if self.IsBrowserRunning():
      self._TryCooperativeShutdown()

    # Second, try to politely shutdown with SIGTERM.
    if self.IsBrowserRunning():
      self._proc.terminate()
      try:
        py_utils.WaitFor(lambda: not self.IsBrowserRunning(), timeout=5)
        self._proc = None
      except py_utils.TimeoutException:
        logging.warning('Failed to gracefully shutdown.')

    # Shutdown aggressively if all above failed.
    if self.IsBrowserRunning():
      logging.warning('Proceed to kill the browser.')
      self._proc.kill()
    self._proc = None

    if self._tmp_output_file:
      self._tmp_output_file.close()
      self._tmp_output_file = None

    if self._tmp_minidump_dir:
      shutil.rmtree(self._tmp_minidump_dir, ignore_errors=True)
      self._tmp_minidump_dir = None
