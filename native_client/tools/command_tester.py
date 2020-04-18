#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""Simple testing harness for running commands and checking expected output.

This harness is used instead of shell scripts to ensure windows compatibility

"""


# python imports
import getopt
import os
import pipes
import re
import sys

# local imports
import test_lib

GlobalPlatform=None  # for pychecker, initialized in ProcessOptions
GlobalSettings = {}


def Print(message):
  print message


def Banner(message):
  Print('=' * 70)
  print(message)
  print('=' * 70)


def DifferentFromGolden(actual, golden, output_type):
  """Compares actual output against golden output.

  If there are any differences, output an error message (to stdout) with
  appropriate banners.

  Args:
    actual: actual output from the program under test, as a single
      string.

    golden: expected output from the program under test, as a single
      string.

    output_type: the name / title for the output type being compared.
      Used in banner output.

  Returns:
    True when there is a difference, False otherwise.
  """

  diff = list(test_lib.DiffStringsIgnoringWhiteSpace(golden, actual))
  diff = '\n'.join(diff)
  if diff:
    Banner('Error %s diff found' % output_type)
    Print(diff)
    Banner('Potential New Golden Output')
    Print(actual)
    return True
  return False


def ResetGlobalSettings():
  global GlobalSettings
  GlobalSettings = {
      'exit_status': 0,
      'using_nacl_signal_handler': False,

      # When declares_exit_status is set, we read the expected exit
      # status from stderr.  We look for a line of the form
      # "** intended_exit_status=X".  This allows crash tests to
      # declare their expected exit status close to where the crash is
      # generated, rather than in a Scons file.  It reduces the risk
      # that the test passes accidentally by crashing during setup.
      'declares_exit_status': False,

      # List of environment variables to set.
      'osenv': '',

      'arch': None,
      'subarch': None,

      # An environment description that should include all factors that may
      # affect tracked performance. Used to compare different environments.
      'perf_env_description': None,

      'name': None,
      'output_stamp': None,

      'stdin': None,
      'log_file': None,

      'stdout_golden': None,
      'stderr_golden': None,
      'log_golden': None,

      # This option must be '1' for the output to be captured, for checking
      # against golden files, special exit_status signals, etc.
      # When this option is '0', stdout and stderr will be streamed out.
      'capture_output': '1',
      # This option must be '1' for the stderr to be captured with stdout
      # (it's ignored if capture_output == 0).  If this option is '0' only
      # stdout will be captured and stderr will be streamed out.
      'capture_stderr': '1',

      'filter_regex': None,
      'filter_inverse': False,
      'filter_group_only': False,

      # Number of times a test is run.
      # This is useful for getting multiple samples for time perf tests.
      'num_runs': 1,

      # Scripts for processing output along with its arguments.
      # This script is given the output of a single run.
      'process_output_single': None,
      # This script is given the concatenated output of all |num_runs|, after
      # having been filtered by |process_output_single| for individual runs.
      'process_output_combined': None,

      'time_warning': 0,
      'time_error': 0,

      'run_under': None,
  }

def StringifyList(lst):
  return ','.join(lst)

def DestringifyList(lst):
  # BUG(robertm): , is a legitimate character for an environment variable
  # value.
  return lst.split(',')

# The following messages match gtest's formatting.  This improves log
# greppability for people who primarily work on Chrome.  It also allows
# gtest-specific hooks on the buildbots to fire.
# The buildbots expect test names in the format "suite_name.test_name", so we
# prefix the test name with a bogus suite name (nacl).
def RunMessage():
  return '[ RUN      ] %s' % (GlobalSettings['name'],)

def FailureMessage(total_time):
  return '[  FAILED  ] %s (%d ms)' % (GlobalSettings['name'],
                                      total_time * 1000.0)

def SuccessMessage(total_time):
  return '[       OK ] %s (%d ms)' % (GlobalSettings['name'],
                                      total_time * 1000.0)

def LogPerfResult(graph_name, trace_name, value, units):
  # NOTE: This RESULT message is parsed by Chrome's perf graph generator.
  Print('RESULT %s: %s= %s %s' %
        (graph_name, trace_name, value, units))


# On POSIX systems, exit() codes are 8-bit.  You cannot use exit() to
# make it look like the process was killed by a signal.  Instead,
# NaCl's signal handler encodes the signal number into the exit() code
# by returning with exit(-signum) or equivalently, exit((-signum) & 0xff).
def IndirectSignal(signum):
  return (-signum) & 0xff

# Windows exit codes that indicate unhandled exceptions.
STATUS_ACCESS_VIOLATION = 0xc0000005
STATUS_ILLEGAL_INSTRUCTION = 0xc000001d
STATUS_PRIVILEGED_INSTRUCTION = 0xc0000096
STATUS_FLOAT_DIVIDE_BY_ZERO = 0xc000008e
STATUS_INTEGER_DIVIDE_BY_ZERO = 0xc0000094

# Python's wrapper for GetExitCodeProcess() treats the STATUS_* values
# as negative, although the unsigned values are used in headers and
# are more widely recognised.
def MungeWindowsErrorExit(num):
  return num - 0x100000000

# If a crash occurs in x86-32 untrusted code on Windows, the kernel
# apparently gets confused about the cause.  It fails to take %cs into
# account when examining the faulting instruction, so it looks at the
# wrong instruction, so we could get either of the errors below.
# See http://code.google.com/p/nativeclient/issues/detail?id=1689
win32_untrusted_crash_exit = [
    MungeWindowsErrorExit(STATUS_ACCESS_VIOLATION),
    MungeWindowsErrorExit(STATUS_PRIVILEGED_INSTRUCTION)]

win32_sigfpe = [
    MungeWindowsErrorExit(STATUS_FLOAT_DIVIDE_BY_ZERO),
    MungeWindowsErrorExit(STATUS_INTEGER_DIVIDE_BY_ZERO),
    ]

# We patch Windows' KiUserExceptionDispatcher on x86-64 to terminate
# the process safely when untrusted code crashes.  We get the exit
# code associated with the HLT instruction.
win64_exit_via_ntdll_patch = [
    MungeWindowsErrorExit(STATUS_PRIVILEGED_INSTRUCTION)]


# Mach exception code for Mac OS X.
EXC_BAD_ACCESS = 1


# 32-bit processes on Mac OS X return SIGBUS in most of the cases where Linux
# returns SIGSEGV, except for actual x86 segmentation violations. 64-bit
# processes on Mac OS X behave differently.
status_map = {
    'sigtrap' : {
        'linux2': [-5], # SIGTRAP
        'darwin': [-5], # SIGTRAP
        },
    'trusted_sigabrt' : {
        'linux2': [-6], # SIGABRT
        'mac32': [-6], # SIGABRT
        'mac64': [-6], # SIGABRT
        # On Windows, NaClAbort() exits using the HLT instruction.
        'win32': [MungeWindowsErrorExit(STATUS_PRIVILEGED_INSTRUCTION)],
        'win64': [MungeWindowsErrorExit(STATUS_PRIVILEGED_INSTRUCTION)],
        },
    'naclabort_coverage' : {
        # This case is here because NaClAbort() behaves differently when
        # code coverage is enabled.
        # This is not used on Windows.
        'linux2': [IndirectSignal(6)], # SIGABRT
        'mac32': [IndirectSignal(6)], # SIGABRT
        'mac64': [IndirectSignal(6)], # SIGABRT
        },
    'sigpipe': {
        # This is not used on Windows because Windows does not have an
        # equivalent of SIGPIPE.
        'linux2': [-13], # SIGPIPE
        'mac32': [-13], # SIGPIPE
        'mac64': [-13], # SIGPIPE
        },
    'untrusted_sigsegv': {
        'linux2': [-11], # SIGSEGV
        'mac32': [-11], # SIGSEGV
        'mac64': [-11], # SIGSEGV
        'win32':  win32_untrusted_crash_exit,
        'win64':  win64_exit_via_ntdll_patch,
        },
    'untrusted_sigill' : {
        'linux2': [-4], # SIGILL
        'mac32': [-4], # SIGILL
        'mac64': [-4], # SIGILL
        'win32':  win32_untrusted_crash_exit,
        'win64':  win64_exit_via_ntdll_patch,
        },
    'untrusted_sigfpe' : {
        'linux2': [-8], # SIGFPE
        'mac32': [-8], # SIGFPE
        'mac64': [-8], # SIGFPE
        'win32':  win32_sigfpe,
        'win64':  win64_exit_via_ntdll_patch,
        },
    'untrusted_segfault': {
        'linux2': [-11], # SIGSEGV
        'mac32': [-10], # SIGBUS
        'mac64': [-10], # SIGBUS
        'mach_exception': EXC_BAD_ACCESS,
        'win32':  win32_untrusted_crash_exit,
        'win64':  win64_exit_via_ntdll_patch,
        },
    'untrusted_sigsegv_or_equivalent': {
        'linux2': [-11], # SIGSEGV
        'mac32': [-11], # SIGSEGV
        'mac64': [-10], # SIGBUS
        'win32':  win32_untrusted_crash_exit,
        'win64':  win64_exit_via_ntdll_patch,
        },
    'trusted_segfault': {
        'linux2': [-11], # SIGSEGV
        'mac32': [-10], # SIGBUS
        'mac64': [-11], # SIGSEGV
        'mach_exception': EXC_BAD_ACCESS,
        'win32':  [MungeWindowsErrorExit(STATUS_ACCESS_VIOLATION)],
        'win64':  [MungeWindowsErrorExit(STATUS_ACCESS_VIOLATION)],
        },
    'trusted_sigsegv_or_equivalent': {
        'linux2': [-11], # SIGSEGV
        'mac32': [-11], # SIGSEGV
        'mac64': [-11], # SIGSEGV
        'win32':  [],
        'win64':  [],
        },
    # This is like 'untrusted_segfault', but without the 'untrusted_'
    # prefix which marks the status type as expecting a
    # gracefully-printed exit message from nacl_signal_common.c.  This
    # is a special case because we use different methods for writing
    # the exception stack frame on different platforms.  On Mac and
    # Windows, NaCl uses a system call which will detect unwritable
    # pages, so the exit status appears as an unhandled fault from
    # untrusted code.  On Linux, NaCl's signal handler writes the
    # frame directly, so the exit status comes from getting a SIGSEGV
    # inside the SIGSEGV handler.
    'unwritable_exception_stack': {
        'linux2': [-11], # SIGSEGV
        'mac32': [-10], # SIGBUS
        'mac64': [-10], # SIGBUS
        'win32':  win32_untrusted_crash_exit,
        'win64':  win64_exit_via_ntdll_patch,
        },
    # Expectations for __builtin_trap(), which is compiled to different
    # instructions by the GCC and LLVM toolchains.  The exact exit status
    # does not matter too much, as long as it's a crash and not a graceful
    # exit via exit() or _exit().  We want __builtin_trap() to trigger the
    # debugger or a crash reporter.
    'untrusted_builtin_trap': {
        'linux2': [-4, -5, -11],
        'mac32': [-4, -10, -11],
        'mac64': [-4, -10, -11],
        'win32': win32_untrusted_crash_exit +
                 [MungeWindowsErrorExit(STATUS_ILLEGAL_INSTRUCTION)],
        'win64': win64_exit_via_ntdll_patch,
        },
    }


def ProcessOptions(argv):
  global GlobalPlatform

  """Process command line options and return the unprocessed left overs."""
  ResetGlobalSettings()
  try:
    opts, args = getopt.getopt(argv, '', [x + '='  for x in GlobalSettings])
  except getopt.GetoptError, err:
    Print(str(err))  # will print something like 'option -a not recognized'
    sys.exit(1)

  for o, a in opts:
    # strip the leading '--'
    option = o[2:]
    assert option in GlobalSettings
    if option == 'exit_status':
      GlobalSettings[option] = a
    elif type(GlobalSettings[option]) == int:
      GlobalSettings[option] = int(a)
    else:
      GlobalSettings[option] = a

  if (sys.platform == 'win32') and (GlobalSettings['subarch'] == '64'):
    GlobalPlatform = 'win64'
  elif (sys.platform == 'darwin'):
    # mac32, mac64
    GlobalPlatform = 'mac' + GlobalSettings['subarch']
  else:
    GlobalPlatform = sys.platform

  # return the unprocessed options, i.e. the command
  return args


# Parse output for signal type and number
#
# The '** Signal' output is from the nacl signal handler code.
#
# Since it is possible for there to be an output race with another
# thread, or additional output due to atexit functions, we scan the
# output in reverse order for the signal signature.
def GetNaClSignalInfoFromStderr(stderr):
  lines = stderr.splitlines()

  # Scan for signal msg in reverse order
  for curline in reversed(lines):
    match = re.match('\*\* (Signal|Mach exception) (\d+) from '
                     '(trusted|untrusted) code', curline)
    if match is not None:
      return match.group(0)
  return None

def GetQemuSignalFromStderr(stderr, default):
  for line in reversed(stderr.splitlines()):
    # Look for 'qemu: uncaught target signal XXX'.
    words = line.split()
    if (len(words) > 4 and
        words[0] == 'qemu:' and words[1] == 'uncaught' and
        words[2] == 'target' and words[3] == 'signal'):
      return -int(words[4])
  return default

def FormatExitStatus(number):
  # Include the hex version because it makes the Windows error exit
  # statuses (STATUS_*) more recognisable.
  return '%i (0x%x)' % (number, number & 0xffffffff)

def FormatResult((exit_status, printed_status)):
  return 'exit status %s and signal info %r' % (
      FormatExitStatus(exit_status), printed_status)

def PrintStdStreams(stdout, stderr):
  if stderr is not None:
    Banner('Stdout for %s:' % os.path.basename(GlobalSettings['name']))
    Print(stdout)
    Banner('Stderr for %s:' % os.path.basename(GlobalSettings['name']))
    Print(stderr)

def GetIntendedExitStatuses(stderr):
  statuses = []
  for line in stderr.splitlines():
    match = re.match(r'\*\* intended_exit_status=(.*)$', line)
    if match is not None:
      statuses.append(match.group(1))
  return statuses

def CheckExitStatus(failed, req_status, using_nacl_signal_handler,
                    exit_status, stdout, stderr):
  if GlobalSettings['declares_exit_status']:
    assert req_status == 0
    intended_statuses = GetIntendedExitStatuses(stderr)
    if len(intended_statuses) == 0:
      Print('\nERROR: Command returned exit status %s but did not output an '
            'intended_exit_status line to stderr - did it exit too early?'
            % FormatExitStatus(exit_status))
      return False
    elif len(intended_statuses) != 1:
      Print('\nERROR: Command returned exit status %s but produced '
            'multiple intended_exit_status lines (%s)'
            % (FormatExitStatus(exit_status), ', '.join(intended_statuses)))
      return False
    else:
      req_status = intended_statuses[0]

  expected_sigtype = 'normal'
  if req_status in status_map:
    expected_statuses = status_map[req_status][GlobalPlatform]
    if using_nacl_signal_handler:
      if req_status.startswith('trusted_'):
        expected_sigtype = 'trusted'
      elif req_status.startswith('untrusted_'):
        expected_sigtype = 'untrusted'
  else:
    expected_statuses = [int(req_status)]

  expected_printed_status = None
  if expected_sigtype == 'normal':
    expected_results = [(status, None) for status in expected_statuses]
  else:
    if sys.platform == 'darwin':
      # Mac OS X
      default = '<mach_exception field missing for %r>' % req_status
      expected_printed_status = '** Mach exception %s from %s code' % (
          status_map.get(req_status, {}).get('mach_exception', default),
          expected_sigtype)
      expected_results = [(status, expected_printed_status)
                          for status in expected_statuses]
    else:
      # Linux
      assert sys.platform != 'win32'

      def MapStatus(status):
        # Expected value should be a signal number, negated.
        assert status < 0, status
        expected_printed_signum = -status
        expected_printed_status = '** Signal %d from %s code' % (
            expected_printed_signum,
            expected_sigtype)
        return (IndirectSignal(expected_printed_signum),
                expected_printed_status)

      expected_results = [MapStatus(status) for status in expected_statuses]

  # If an uncaught signal occurs under QEMU (on ARM), the exit status
  # contains the signal number, mangled as per IndirectSignal().  We
  # extract the unadulterated signal number from QEMU's log message in
  # stderr instead.  If we are not using QEMU, or no signal is raised
  # under QEMU, this is a no-op.
  if stderr is not None:
    exit_status = GetQemuSignalFromStderr(stderr, exit_status)

  if using_nacl_signal_handler and stderr is not None:
    actual_printed_status = GetNaClSignalInfoFromStderr(stderr)
  else:
    actual_printed_status = None

  actual_result = (exit_status, actual_printed_status)
  msg = '\nERROR: Command returned: %s\n' % FormatResult(actual_result)
  msg += 'but we expected: %s' % '\n  or: '.join(FormatResult(r)
                                                 for r in expected_results)
  if actual_result not in expected_results:
    Print(msg)
    failed = True
  return not failed

def CheckTimeBounds(total_time):
  if GlobalSettings['time_error']:
    if total_time > GlobalSettings['time_error']:
      Print('ERROR: should have taken less than %f secs' %
            (GlobalSettings['time_error']))
      return False

  if GlobalSettings['time_warning']:
    if total_time > GlobalSettings['time_warning']:
      Print('WARNING: should have taken less than %f secs' %
            (GlobalSettings['time_warning']))
  return True

def CheckGoldenOutput(stdout, stderr):
  for (stream, getter) in [
      ('stdout', lambda: stdout),
      ('stderr', lambda: stderr),
      ('log', lambda: open(GlobalSettings['log_file']).read()),
      ]:
    golden = stream + '_golden'
    if GlobalSettings[golden]:
      golden_data = open(GlobalSettings[golden]).read()
      actual = getter()
      if GlobalSettings['filter_regex']:
        actual = test_lib.RegexpFilterLines(GlobalSettings['filter_regex'],
                                            GlobalSettings['filter_inverse'],
                                            GlobalSettings['filter_group_only'],
                                            actual)
      if DifferentFromGolden(actual, golden_data, stream):
        return False
  return True

def ProcessLogOutputSingle(stdout, stderr):
  output_processor = GlobalSettings['process_output_single']
  if output_processor is None:
    return (True, stdout, stderr)
  else:
    output_processor_cmd = DestringifyList(output_processor)
    # Also, get the output from log_file to get NaClLog output in Windows.
    log_output = open(GlobalSettings['log_file']).read()
    # Assume the log processor does not care about the order of the lines.
    all_output = log_output + stdout + stderr
    _, retcode, failed, new_stdout, new_stderr = \
        test_lib.RunTestWithInputOutput(
            output_processor_cmd, all_output,
            timeout=GlobalSettings['time_error'])
    # Print the result, since we have done some processing and we need
    # to have the processed data. However, if we intend to process it some
    # more later via process_output_combined, do not duplicate the data here.
    # Only print out the final result!
    if not GlobalSettings['process_output_combined']:
      PrintStdStreams(new_stdout, new_stderr)
    if retcode != 0 or failed:
      return (False, new_stdout, new_stderr)
    else:
      return (True, new_stdout, new_stderr)

def ProcessLogOutputCombined(stdout, stderr):
  output_processor = GlobalSettings['process_output_combined']
  if output_processor is None:
    return True
  else:
    output_processor_cmd = DestringifyList(output_processor)
    all_output = stdout + stderr
    _, retcode, failed, new_stdout, new_stderr = \
        test_lib.RunTestWithInputOutput(
            output_processor_cmd, all_output,
            timeout=GlobalSettings['time_error'])
    # Print the result, since we have done some processing.
    PrintStdStreams(new_stdout, new_stderr)
    if retcode != 0 or failed:
      return False
    else:
      return True

def DoRun(command, stdin_data):
  """
  Run the command, given stdin_data. Returns a return code (0 is good)
  and optionally a captured version of stdout, stderr from the run
  (if the global setting capture_output is true).
  """
  # Initialize stdout, stderr to indicate we have not captured
  # any of stdout or stderr.
  stdout = ''
  stderr = ''
  if not int(GlobalSettings['capture_output']):
    # We are only blurting out the stdout and stderr, not capturing it
    # for comparison, etc.
    assert (not GlobalSettings['stdout_golden']
            and not GlobalSettings['stderr_golden']
            and not GlobalSettings['log_golden']
            and not GlobalSettings['filter_regex']
            and not GlobalSettings['filter_inverse']
            and not GlobalSettings['filter_group_only']
            and not GlobalSettings['process_output_single']
            and not GlobalSettings['process_output_combined']
            )
    # If python ever changes popen.stdout.read() to not risk deadlock,
    # we could stream and capture, and use RunTestWithInputOutput instead.
    (total_time, exit_status, failed) = test_lib.RunTestWithInput(
        command, stdin_data,
        timeout=GlobalSettings['time_error'])
    if not CheckExitStatus(failed,
                           GlobalSettings['exit_status'],
                           GlobalSettings['using_nacl_signal_handler'],
                           exit_status, None, None):
      Print(FailureMessage(total_time))
      return (1, stdout, stderr)
  else:
    (total_time, exit_status,
     failed, stdout, stderr) = test_lib.RunTestWithInputOutput(
         command, stdin_data, int(GlobalSettings['capture_stderr']),
         timeout=GlobalSettings['time_error'])
    # CheckExitStatus may spew stdout/stderr when there is an error.
    # Otherwise, we do not spew stdout/stderr in this case (capture_output).
    if not CheckExitStatus(failed,
                           GlobalSettings['exit_status'],
                           GlobalSettings['using_nacl_signal_handler'],
                           exit_status, stdout, stderr):
      PrintStdStreams(stdout, stderr)
      Print(FailureMessage(total_time))
      return (1, stdout, stderr)
    if not CheckGoldenOutput(stdout, stderr):
      Print(FailureMessage(total_time))
      return (1, stdout, stderr)
    success, stdout, stderr = ProcessLogOutputSingle(stdout, stderr)
    if not success:
      Print(FailureMessage(total_time) + ' ProcessLogOutputSingle failed!')
      return (1, stdout, stderr)

  if not CheckTimeBounds(total_time):
    Print(FailureMessage(total_time))
    return (1, stdout, stderr)

  Print(SuccessMessage(total_time))
  return (0, stdout, stderr)


def DisableCrashDialog():
  """
  Disable Windows' crash dialog box, which pops up when a process exits with
  an unhandled fault. This causes the process to hang on the Buildbots. We
  duplicate this function from SConstruct because ErrorMode flags are
  overwritten in scons due to race conditions. See bug
  https://code.google.com/p/nativeclient/issues/detail?id=2968
  """
  if sys.platform == 'win32':
    import win32api
    import win32con
    # The double call is to preserve existing flags, as discussed at
    # http://blogs.msdn.com/oldnewthing/archive/2004/07/27/198410.aspx
    new_flags = win32con.SEM_NOGPFAULTERRORBOX
    existing_flags = win32api.SetErrorMode(new_flags)
    win32api.SetErrorMode(existing_flags | new_flags)


def Main(argv):
  DisableCrashDialog()
  command = ProcessOptions(argv)

  if not GlobalSettings['name']:
    GlobalSettings['name'] = command[0]
  GlobalSettings['name'] = os.path.basename(GlobalSettings['name'])

  Print(RunMessage())
  num_runs = GlobalSettings['num_runs']
  if num_runs > 1:
    Print(' (running %d times)' % num_runs)

  if GlobalSettings['osenv']:
    Banner('setting environment')
    env_vars = DestringifyList(GlobalSettings['osenv'])
  else:
    env_vars = []
  for env_var in env_vars:
    key, val = env_var.split('=', 1)
    Print('[%s] = [%s]' % (key, val))
    os.environ[key] = val

  stdin_data = ''
  if GlobalSettings['stdin']:
    stdin_data = open(GlobalSettings['stdin'])

  run_under = GlobalSettings['run_under']
  if run_under:
    command = run_under.split(',') + command

  # print the command in copy-and-pastable fashion
  print ' '.join(pipes.quote(arg) for arg in env_vars + command)

  # Concatenate output when running multiple times (e.g., for timing).
  combined_stdout = ''
  combined_stderr = ''
  cur_runs = 0
  num_runs = GlobalSettings['num_runs']
  while cur_runs < num_runs:
    cur_runs += 1
    # Clear out previous log_file.
    if GlobalSettings['log_file']:
      try:
        os.unlink(GlobalSettings['log_file'])  # might not pre-exist
      except OSError:
        pass
    ret_code, stdout, stderr = DoRun(command, stdin_data)
    if ret_code != 0:
      return ret_code
    combined_stdout += stdout
    combined_stderr += stderr
  # Process the log output after all the runs.
  success = ProcessLogOutputCombined(combined_stdout, combined_stderr)
  if not success:
    # Bogus time, since only ProcessLogOutputCombined failed.
    Print(FailureMessage(0.0) + ' ProcessLogOutputCombined failed!')
    return 1
  if GlobalSettings['output_stamp'] is not None:
    # Create an empty stamp file to indicate success.
    fh = open(GlobalSettings['output_stamp'], 'w')
    fh.close()
  return 0

if __name__ == '__main__':
  retval = Main(sys.argv[1:])
  # Add some whitepsace to make the logs easier to read.
  sys.stdout.write('\n\n')
  sys.exit(retval)
