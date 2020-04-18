# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Common python commands used by various build scripts."""

from __future__ import print_function

import collections
import contextlib
from datetime import datetime
import email.utils
import errno
import functools
import getpass
import hashlib
import inspect
import operator
import os
import pprint
import re
import signal
import socket
import subprocess
import sys
import tempfile
import time
import traceback
import types

from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import signals


STRICT_SUDO = False

# For use by ShellQuote.  Match all characters that the shell might treat
# specially.  This means a number of things:
#  - Reserved characters.
#  - Characters used in expansions (brace, variable, path, globs, etc...).
#  - Characters that an interactive shell might use (like !).
#  - Whitespace so that one arg turns into multiple.
# See the bash man page as well as the POSIX shell documentation for more info:
#   http://www.gnu.org/software/bash/manual/bashref.html
#   http://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html
_SHELL_QUOTABLE_CHARS = frozenset('[|&;()<> \t!{}[]=*?~$"\'\\#^')
# The chars that, when used inside of double quotes, need escaping.
# Order here matters as we need to escape backslashes first.
_SHELL_ESCAPE_CHARS = r'\"`$'


def ShellQuote(s):
  """Quote |s| in a way that is safe for use in a shell.

  We aim to be safe, but also to produce "nice" output.  That means we don't
  use quotes when we don't need to, and we prefer to use less quotes (like
  putting it all in single quotes) than more (using double quotes and escaping
  a bunch of stuff, or mixing the quotes).

  While python does provide a number of alternatives like:
   - pipes.quote
   - shlex.quote
  They suffer from various problems like:
   - Not widely available in different python versions.
   - Do not produce pretty output in many cases.
   - Are in modules that rarely otherwise get used.

  Note: We don't handle reserved shell words like "for" or "case".  This is
  because those only matter when they're the first element in a command, and
  there is no use case for that.  When we want to run commands, we tend to
  run real programs and not shell ones.

  Args:
    s: The string to quote.

  Returns:
    A safely (possibly quoted) string.
  """
  s = s.encode('utf-8')

  # See if no quoting is needed so we can return the string as-is.
  for c in s:
    if c in _SHELL_QUOTABLE_CHARS:
      break
  else:
    if not s:
      return "''"
    else:
      return s

  # See if we can use single quotes first.  Output is nicer.
  if "'" not in s:
    return "'%s'" % s

  # Have to use double quotes.  Escape the few chars that still expand when
  # used inside of double quotes.
  for c in _SHELL_ESCAPE_CHARS:
    if c in s:
      s = s.replace(c, r'\%s' % c)
  return '"%s"' % s


def TruncateStringToLine(s, maxlen=80):
  """Truncate |s| to a maximum length of |maxlen| including elipsis (...)

  Args:
    s: A string.
    maxlen: Maximum length of desired returned string. Must be at least 3.

  Returns:
    s if len(s) <= maxlen already and s has no newline in it.
    Otherwise, a single line truncation that ends with '...' and is of
    length |maxlen|.
  """
  assert maxlen >= 3
  line = s.splitlines()[0]
  if len(line) <= maxlen:
    return line
  else:
    return line[:maxlen-3] + '...'


def ShellUnquote(s):
  """Do the opposite of ShellQuote.

  This function assumes that the input is a valid escaped string. The behaviour
  is undefined on malformed strings.

  Args:
    s: An escaped string.

  Returns:
    The unescaped version of the string.
  """
  if not s:
    return ''

  if s[0] == "'":
    return s[1:-1]

  if s[0] != '"':
    return s

  s = s[1:-1]
  output = ''
  i = 0
  while i < len(s) - 1:
    # Skip the backslash when it makes sense.
    if s[i] == '\\' and s[i + 1] in _SHELL_ESCAPE_CHARS:
      i += 1
    output += s[i]
    i += 1
  return output + s[i] if i < len(s) else output


def CmdToStr(cmd):
  """Translate a command list into a space-separated string.

  The resulting string should be suitable for logging messages and for
  pasting into a terminal to run.  Command arguments are surrounded by
  quotes to keep them grouped, even if an argument has spaces in it.

  Examples:
    ['a', 'b'] ==> "'a' 'b'"
    ['a b', 'c'] ==> "'a b' 'c'"
    ['a', 'b\'c'] ==> '\'a\' "b\'c"'
    [u'a', "/'$b"] ==> '\'a\' "/\'$b"'
    [] ==> ''
    See unittest for additional (tested) examples.

  Args:
    cmd: List of command arguments.

  Returns:
    String representing full command.
  """
  # Use str before repr to translate unicode strings to regular strings.
  return ' '.join(ShellQuote(arg) for arg in cmd)


class CommandResult(object):
  """An object to store various attributes of a child process."""

  def __init__(self, cmd=None, error=None, output=None, returncode=None):
    self.cmd = cmd
    self.error = error
    self.output = output
    self.returncode = returncode

  @property
  def cmdstr(self):
    """Return self.cmd as a space-separated string, useful for log messages."""
    return CmdToStr(self.cmd or '')


class RunCommandError(Exception):
  """Error caught in RunCommand() method."""

  def __init__(self, msg, result, exception=None):
    self.msg, self.result, self.exception = msg, result, exception
    if exception is not None and not isinstance(exception, Exception):
      raise ValueError('exception must be an exception instance; got %r'
                       % (exception,))
    Exception.__init__(self, msg)
    self.args = (msg, result, exception)

  def Stringify(self, error=True, output=True):
    """Custom method for controlling what is included in stringifying this.

    Each individual argument is the literal name of an attribute
    on the result object; if False, that value is ignored for adding
    to this string content.  If true, it'll be incorporated.

    Args:
      error: See comment about individual arguments above.
      output: See comment about individual arguments above.
    """
    items = [
        'return code: %s; command: %s' % (
            self.result.returncode, self.result.cmdstr),
    ]
    if error and self.result.error:
      items.append(self.result.error)
    if output and self.result.output:
      items.append(self.result.output)
    if self.msg:
      items.append(self.msg)
    return '\n'.join(items)

  def __str__(self):
    # __str__ needs to return ascii, thus force a conversion to be safe.
    return self.Stringify().decode('utf-8', 'replace').encode(
        'ascii', 'xmlcharrefreplace')

  def __eq__(self, other):
    return (type(self) == type(other) and
            self.args == other.args)

  def __ne__(self, other):
    return not self.__eq__(other)


class TerminateRunCommandError(RunCommandError):
  """We were signaled to shutdown while running a command.

  Client code shouldn't generally know, nor care about this class.  It's
  used internally to suppress retry attempts when we're signaled to die.
  """


def SudoRunCommand(cmd, user='root', **kwargs):
  """Run a command via sudo.

  Client code must use this rather than coming up with their own RunCommand
  invocation that jams sudo in- this function is used to enforce certain
  rules in our code about sudo usage, and as a potential auditing point.

  Args:
    cmd: The command to run.  See RunCommand for rules of this argument-
         SudoRunCommand purely prefixes it with sudo.
    user: The user to run the command as.
    kwargs: See RunCommand options, it's a direct pass thru to it.
          Note that this supports a 'strict' keyword that defaults to True.
          If set to False, it'll suppress strict sudo behavior.

  Returns:
    See RunCommand documentation.

  Raises:
    This function may immediately raise RunCommandError if we're operating
    in a strict sudo context and the API is being misused.
    Barring that, see RunCommand's documentation- it can raise the same things
    RunCommand does.
  """
  sudo_cmd = ['sudo']

  strict = kwargs.pop('strict', True)

  if user == 'root' and os.geteuid() == 0:
    return RunCommand(cmd, **kwargs)

  if strict and STRICT_SUDO:
    if 'CROS_SUDO_KEEP_ALIVE' not in os.environ:
      raise RunCommandError(
          'We were invoked in a strict sudo non - interactive context, but no '
          'sudo keep alive daemon is running.  This is a bug in the code.',
          CommandResult(cmd=cmd, returncode=126))
    sudo_cmd += ['-n']

  if user != 'root':
    sudo_cmd += ['-u', user]

  # Pass these values down into the sudo environment, since sudo will
  # just strip them normally.
  extra_env = kwargs.pop('extra_env', None)
  extra_env = {} if extra_env is None else extra_env.copy()

  for var in constants.ENV_PASSTHRU:
    if var not in extra_env and var in os.environ:
      extra_env[var] = os.environ[var]

  sudo_cmd.extend('%s=%s' % (k, v) for k, v in extra_env.iteritems())

  # Finally, block people from passing options to sudo.
  sudo_cmd.append('--')

  if isinstance(cmd, basestring):
    # We need to handle shell ourselves so the order is correct:
    #  $ sudo [sudo args] -- bash -c '[shell command]'
    # If we let RunCommand take care of it, we'd end up with:
    #  $ bash -c 'sudo [sudo args] -- [shell command]'
    shell = kwargs.pop('shell', False)
    if not shell:
      raise Exception('Cannot run a string command without a shell')
    sudo_cmd.extend(['/bin/bash', '-c', cmd])
  else:
    sudo_cmd.extend(cmd)

  return RunCommand(sudo_cmd, **kwargs)


def _KillChildProcess(proc, int_timeout, kill_timeout, cmd, original_handler,
                      signum, frame):
  """Used as a signal handler by RunCommand.

  This is internal to Runcommand.  No other code should use this.
  """
  if signum:
    # If we've been invoked because of a signal, ignore delivery of that signal
    # from this point forward.  The invoking context of _KillChildProcess
    # restores signal delivery to what it was prior; we suppress future delivery
    # till then since this code handles SIGINT/SIGTERM fully including
    # delivering the signal to the original handler on the way out.
    signal.signal(signum, signal.SIG_IGN)

  # Do not trust Popen's returncode alone; we can be invoked from contexts where
  # the Popen instance was created, but no process was generated.
  if proc.returncode is None and proc.pid is not None:
    try:
      while proc.poll() is None and int_timeout >= 0:
        time.sleep(0.1)
        int_timeout -= 0.1

      proc.terminate()
      while proc.poll() is None and kill_timeout >= 0:
        time.sleep(0.1)
        kill_timeout -= 0.1

      if proc.poll() is None:
        # Still doesn't want to die.  Too bad, so sad, time to die.
        proc.kill()
    except EnvironmentError as e:
      logging.warning('Ignoring unhandled exception in _KillChildProcess: %s',
                      e)

    # Ensure our child process has been reaped.
    proc.wait()

  if not signals.RelaySignal(original_handler, signum, frame):
    # Mock up our own, matching exit code for signaling.
    cmd_result = CommandResult(cmd=cmd, returncode=signum << 8)
    raise TerminateRunCommandError('Received signal %i' % signum, cmd_result)


class _Popen(subprocess.Popen):
  """subprocess.Popen derivative customized for our usage.

  Specifically, we fix terminate/send_signal/kill to work if the child process
  was a setuid binary; on vanilla kernels, the parent can wax the child
  regardless, on goobuntu this apparently isn't allowed, thus we fall back
  to the sudo machinery we have.

  While we're overriding send_signal, we also suppress ESRCH being raised
  if the process has exited, and suppress signaling all together if the process
  has knowingly been waitpid'd already.
  """

  def send_signal(self, signum):
    if self.returncode is not None:
      # The original implementation in Popen would allow signaling whatever
      # process now occupies this pid, even if the Popen object had waitpid'd.
      # Since we can escalate to sudo kill, we do not want to allow that.
      # Fixing this addresses that angle, and makes the API less sucky in the
      # process.
      return

    try:
      os.kill(self.pid, signum)
    except EnvironmentError as e:
      if e.errno == errno.EPERM:
        # Kill returns either 0 (signal delivered), or 1 (signal wasn't
        # delivered).  This isn't particularly informative, but we still
        # need that info to decide what to do, thus the error_code_ok=True.
        ret = SudoRunCommand(['kill', '-%i' % signum, str(self.pid)],
                             print_cmd=False, redirect_stdout=True,
                             redirect_stderr=True, error_code_ok=True)
        if ret.returncode == 1:
          # The kill binary doesn't distinguish between permission denied,
          # and the pid is missing.  Denied can only occur under weird
          # grsec/selinux policies.  We ignore that potential and just
          # assume the pid was already dead and try to reap it.
          self.poll()
      elif e.errno == errno.ESRCH:
        # Since we know the process is dead, reap it now.
        # Normally Popen would throw this error- we suppress it since frankly
        # that's a misfeature and we're already overriding this method.
        self.poll()
      else:
        raise


# pylint: disable=redefined-builtin
def RunCommand(cmd, print_cmd=True, error_message=None, redirect_stdout=False,
               redirect_stderr=False, cwd=None, input=None, enter_chroot=False,
               shell=False, env=None, extra_env=None, ignore_sigint=False,
               combine_stdout_stderr=False, log_stdout_to_file=None,
               append_to_file=False, chroot_args=None, debug_level=logging.INFO,
               error_code_ok=False, int_timeout=1, kill_timeout=1,
               log_output=False, stdout_to_pipe=False, capture_output=False,
               quiet=False, mute_output=None, stream_log=False):
  """Runs a command.

  Args:
    cmd: cmd to run.  Should be input to subprocess.Popen. If a string, shell
      must be true. Otherwise the command must be an array of arguments, and
      shell must be false.
    print_cmd: prints the command before running it.
    error_message: prints out this message when an error occurs.
    redirect_stdout: returns the stdout.
    redirect_stderr: holds stderr output until input is communicated.
    cwd: the working directory to run this cmd.
    input: The data to pipe into this command through stdin.  If a file object
      or file descriptor, stdin will be connected directly to that.
    enter_chroot: this command should be run from within the chroot.  If set,
      cwd must point to the scripts directory. If we are already inside the
      chroot, this command will be run as if |enter_chroot| is False.
    shell: Controls whether we add a shell as a command interpreter.  See cmd
      since it has to agree as to the type.
    env: If non-None, this is the environment for the new process.  If
      enter_chroot is true then this is the environment of the enter_chroot,
      most of which gets removed from the cmd run.
    extra_env: If set, this is added to the environment for the new process.
      In enter_chroot=True case, these are specified on the post-entry
      side, and so are often more useful.  This dictionary is not used to
      clear any entries though.
    ignore_sigint: If True, we'll ignore signal.SIGINT before calling the
      child.  This is the desired behavior if we know our child will handle
      Ctrl-C.  If we don't do this, I think we and the child will both get
      Ctrl-C at the same time, which means we'll forcefully kill the child.
    combine_stdout_stderr: Combines stdout and stderr streams into stdout.
    log_stdout_to_file: If set, redirects stdout to file specified by this path.
      If |combine_stdout_stderr| is set to True, then stderr will also be logged
      to the specified file.
    append_to_file: If True, the stdout streams are appended to the end of log
      stdout_to_file.
    chroot_args: An array of arguments for the chroot environment wrapper.
    debug_level: The debug level of RunCommand's output.
    error_code_ok: Does not raise an exception when command returns a non-zero
      exit code. Instead, returns the CommandResult object containing the exit
      code. Note: will still raise an exception if the cmd file does not exist.
    int_timeout: If we're interrupted, how long (in seconds) should we give the
      invoked process to clean up before we send a SIGTERM.
    kill_timeout: If we're interrupted, how long (in seconds) should we give the
      invoked process to shutdown from a SIGTERM before we SIGKILL it.
    log_output: Log the command and its output automatically.
    stdout_to_pipe: Redirect stdout to pipe.
    capture_output: Set |redirect_stdout| and |redirect_stderr| to True.
    quiet: Set |print_cmd| to False, |stdout_to_pipe| and
      |combine_stdout_stderr| to True.
    mute_output: Mute subprocess printing to parent stdout/stderr. Defaults to
      None, which bases muting on |debug_level|.
    stream_log: Stream output to the logs as the command runs. Implies
      log_output, stdout_to_pipe, combine_stdout_stderr.

  Returns:
    A CommandResult object.

  Raises:
    RunCommandError:  Raises exception on error with optional error_message.
  """
  if capture_output:
    redirect_stdout, redirect_stderr = True, True

  if quiet:
    debug_level = logging.DEBUG
    stdout_to_pipe, combine_stdout_stderr = True, True

  if stream_log:
    log_output, stdout_to_pipe, combine_stdout_stderr = True, True, True

  # Set default for variables.
  stdout = None
  stderr = None
  stdin = None
  cmd_result = CommandResult()

  if mute_output is None:
    mute_output = logging.getLogger().getEffectiveLevel() > debug_level

  # Force the timeout to float; in the process, if it's not convertible,
  # a self-explanatory exception will be thrown.
  kill_timeout = float(kill_timeout)

  def _get_tempfile():
    try:
      return tempfile.TemporaryFile(bufsize=0)
    except EnvironmentError as e:
      if e.errno != errno.ENOENT:
        raise
      # This can occur if we were pointed at a specific location for our
      # TMP, but that location has since been deleted.  Suppress that issue
      # in this particular case since our usage gurantees deletion,
      # and since this is primarily triggered during hard cgroups shutdown.
      return tempfile.TemporaryFile(bufsize=0, dir='/tmp')

  # Modify defaults based on parameters.
  # Note that tempfiles must be unbuffered else attempts to read
  # what a separate process did to that file can result in a bad
  # view of the file.
  if log_stdout_to_file:
    if append_to_file:
      stdout = open(log_stdout_to_file, 'a+')
    else:
      stdout = open(log_stdout_to_file, 'w+')
  elif stdout_to_pipe:
    stdout = subprocess.PIPE
  elif redirect_stdout or mute_output or log_output:
    stdout = _get_tempfile()

  if combine_stdout_stderr:
    stderr = subprocess.STDOUT
  elif redirect_stderr or mute_output or log_output:
    stderr = _get_tempfile()

  # If subprocesses have direct access to stdout or stderr, they can bypass
  # our buffers, so we need to flush to ensure that output is not interleaved.
  if stdout is None or stderr is None:
    sys.stdout.flush()
    sys.stderr.flush()

  # If input is a string, we'll create a pipe and send it through that.
  # Otherwise we assume it's a file object that can be read from directly.
  if isinstance(input, basestring):
    stdin = subprocess.PIPE
  elif input is not None:
    stdin = input
    input = None

  # stream_log needs PIPE.
  assert not stream_log or stdout == subprocess.PIPE

  if isinstance(cmd, basestring):
    if not shell:
      raise Exception('Cannot run a string command without a shell')
    cmd = ['/bin/bash', '-c', cmd]
    shell = False
  elif shell:
    raise Exception('Cannot run an array command with a shell')

  # If we are using enter_chroot we need to use enterchroot pass env through
  # to the final command.
  env = env.copy() if env is not None else os.environ.copy()
  env.update(extra_env if extra_env else {})
  if enter_chroot and not IsInsideChroot():
    wrapper = ['cros_sdk']
    if cwd:
      # If the current working directory is set, try to find cros_sdk relative
      # to cwd. Generally cwd will be the buildroot therefore we want to use
      # {cwd}/chromite/bin/cros_sdk. For more info PTAL at crbug.com/432620
      path = os.path.join(cwd, constants.CHROMITE_BIN_SUBDIR, 'cros_sdk')
      if os.path.exists(path):
        wrapper = [path]

    if chroot_args:
      wrapper += chroot_args

    if extra_env:
      wrapper.extend('%s=%s' % (k, v) for k, v in extra_env.iteritems())

    cmd = wrapper + ['--'] + cmd

  for var in constants.ENV_PASSTHRU:
    if var not in env and var in os.environ:
      env[var] = os.environ[var]

  # Print out the command before running.
  if print_cmd or log_output:
    if cwd:
      logging.log(debug_level, 'RunCommand: %s in %s', CmdToStr(cmd), cwd)
    else:
      logging.log(debug_level, 'RunCommand: %s', CmdToStr(cmd))

  cmd_result.cmd = cmd

  proc = None
  # Verify that the signals modules is actually usable, and won't segfault
  # upon invocation of getsignal.  See signals.SignalModuleUsable for the
  # details and upstream python bug.
  use_signals = signals.SignalModuleUsable()
  try:
    proc = _Popen(cmd, cwd=cwd, stdin=stdin, stdout=stdout,
                  stderr=stderr, shell=False, env=env,
                  close_fds=True)

    if use_signals:
      if ignore_sigint:
        old_sigint = signal.signal(signal.SIGINT, signal.SIG_IGN)
      else:
        old_sigint = signal.getsignal(signal.SIGINT)
        signal.signal(signal.SIGINT,
                      functools.partial(_KillChildProcess, proc, int_timeout,
                                        kill_timeout, cmd, old_sigint))

      old_sigterm = signal.getsignal(signal.SIGTERM)
      signal.signal(signal.SIGTERM,
                    functools.partial(_KillChildProcess, proc, int_timeout,
                                      kill_timeout, cmd, old_sigterm))

    try:
      if stream_log:
        logging.log(debug_level, '(stdout/stderr):\n')
        cmd_result.output = ''
        while True:
          output = proc.stdout.readline()
          if not len(output):
            break
          logging.log(debug_level, output.strip())
          cmd_result.output += output
        proc.wait()
      else:
        (cmd_result.output, cmd_result.error) = proc.communicate(input)
    finally:
      if use_signals:
        signal.signal(signal.SIGINT, old_sigint)
        signal.signal(signal.SIGTERM, old_sigterm)

      if stdout and not log_stdout_to_file and not stdout_to_pipe:
        stdout.seek(0)
        cmd_result.output = stdout.read()
        stdout.close()

      if stderr and stderr != subprocess.STDOUT:
        stderr.seek(0)
        cmd_result.error = stderr.read()
        stderr.close()

    cmd_result.returncode = proc.returncode

    if log_output and not stream_log:
      if cmd_result.output:
        logging.log(debug_level, '(stdout):\n%s', cmd_result.output)
      if cmd_result.error:
        logging.log(debug_level, '(stderr):\n%s', cmd_result.error)

    if not error_code_ok and proc.returncode:
      msg = 'cmd=%s' % cmd
      if cwd:
        msg += ', cwd=%s' % cwd
      if extra_env:
        msg += ', extra env=%s' % extra_env
      if error_message:
        msg += '\n%s' % error_message
      raise RunCommandError(msg, cmd_result)
  except OSError as e:
    estr = str(e)
    if e.errno == errno.EACCES:
      estr += '; does the program need `chmod a+x`?'
    raise RunCommandError(estr, CommandResult(cmd=cmd), exception=e)
  finally:
    if proc is not None:
      # Ensure the process is dead.
      _KillChildProcess(proc, int_timeout, kill_timeout, cmd, None, None, None)

  return cmd_result
# pylint: enable=redefined-builtin


# Convenience RunCommand methods.
#
# We don't use functools.partial because it binds the methods at import time,
# which doesn't work well with unit tests, since it bypasses the mock that may
# be set up for RunCommand.

def DebugRunCommand(*args, **kwargs):
  kwargs.setdefault('debug_level', logging.DEBUG)
  return RunCommand(*args, **kwargs)


class DieSystemExit(SystemExit):
  """Custom Exception used so we can intercept this if necessary."""


def Die(message, *args, **kwargs):
  """Emits an error message with a stack trace and halts execution.

  Args:
    message: The message to be emitted before exiting.
  """
  logging.error(message, *args, **kwargs)
  raise DieSystemExit(1)


def GetSysrootToolPath(sysroot, tool_name):
  """Returns the path to the sysroot specific version of a tool.

  Does not check that the tool actually exists.

  Args:
    sysroot: build root of the system in question.
    tool_name: string name of tool desired (e.g. 'equery').

  Returns:
    string path to tool inside the sysroot.
  """
  if sysroot == '/':
    return os.path.join(sysroot, 'usr', 'bin', tool_name)

  return os.path.join(sysroot, 'build', 'bin', tool_name)


def ListFiles(base_dir):
  """Recursively list files in a directory.

  Args:
    base_dir: directory to start recursively listing in.

  Returns:
    A list of files relative to the base_dir path or
    An empty list of there are no files in the directories.
  """
  directories = [base_dir]
  files_list = []
  while directories:
    directory = directories.pop()
    for name in os.listdir(directory):
      fullpath = os.path.join(directory, name)
      if os.path.isfile(fullpath):
        files_list.append(fullpath)
      elif os.path.isdir(fullpath):
        directories.append(fullpath)

  return files_list


def IsInsideChroot():
  """Returns True if we are inside chroot."""
  return os.path.exists('/etc/cros_chroot_version')


def AssertInsideChroot():
  """Die if we are outside the chroot"""
  if not IsInsideChroot():
    Die('%s: please run inside the chroot', os.path.basename(sys.argv[0]))


def AssertOutsideChroot():
  """Die if we are inside the chroot"""
  if IsInsideChroot():
    Die('%s: please run outside the chroot', os.path.basename(sys.argv[0]))


def GetChromeosVersion(str_obj):
  """Helper method to parse output for CHROMEOS_VERSION_STRING.

  Args:
    str_obj: a string, which may contain Chrome OS version info.

  Returns:
    A string, value of CHROMEOS_VERSION_STRING environment variable set by
      chromeos_version.sh. Or None if not found.
  """
  if str_obj is not None:
    match = re.search(r'CHROMEOS_VERSION_STRING=([0-9_.]+)', str_obj)
    if match and match.group(1):
      logging.info('CHROMEOS_VERSION_STRING = %s' % match.group(1))
      return match.group(1)

  logging.info('CHROMEOS_VERSION_STRING NOT found')
  return None


def GetHostName(fully_qualified=False):
  """Return hostname of current machine, with domain if |fully_qualified|."""
  hostname = socket.gethostname()
  try:
    hostname = socket.gethostbyaddr(hostname)[0]
  except socket.gaierror as e:
    logging.warning('please check your /etc/hosts file; resolving your hostname'
                    ' (%s) failed: %s', hostname, e)

  if fully_qualified:
    return hostname
  else:
    return hostname.partition('.')[0]


def GetHostDomain():
  """Return domain of current machine.

  If there is no domain, return 'localdomain'.
  """

  hostname = GetHostName(fully_qualified=True)
  domain = hostname.partition('.')[2]
  return domain if domain else 'localdomain'


def HostIsCIBuilder(fq_hostname=None, golo_only=False, gce_only=False):
  """Return True iff a host is a continuous-integration builder.

  Args:
    fq_hostname: The fully qualified hostname. By default, we fetch it for you.
    golo_only: Only return True if the host is in the Chrome Golo. Defaults to
      False.
    gce_only: Only return True if the host is in the Chrome GCE block. Defaults
      to False.
  """
  if not fq_hostname:
    fq_hostname = GetHostName(fully_qualified=True)
  in_golo = fq_hostname.endswith('.' + constants.GOLO_DOMAIN)
  in_gce = (fq_hostname.endswith('.' + constants.CHROME_DOMAIN) or
            fq_hostname.endswith('.' + constants.CHROMEOS_BOT_INTERNAL))
  if golo_only:
    return in_golo
  elif gce_only:
    return in_gce
  else:
    return in_golo or in_gce


COMP_NONE = 0
COMP_GZIP = 1
COMP_BZIP2 = 2
COMP_XZ = 3


def FindCompressor(compression, chroot=None):
  """Locate a compressor utility program (possibly in a chroot).

  Since we compress/decompress a lot, make it easy to locate a
  suitable utility program in a variety of locations.  We favor
  the one in the chroot over /, and the parallel implementation
  over the single threaded one.

  Args:
    compression: The type of compression desired.
    chroot: Optional path to a chroot to search.

  Returns:
    Path to a compressor.

  Raises:
    ValueError: If compression is unknown.
  """
  if compression == COMP_GZIP:
    std = 'gzip'
    para = 'pigz'
  elif compression == COMP_BZIP2:
    std = 'bzip2'
    para = 'pbzip2'
  elif compression == COMP_XZ:
    std = 'xz'
    para = 'pixz'
  elif compression == COMP_NONE:
    return 'cat'
  else:
    raise ValueError('unknown compression')

  roots = []
  if chroot:
    roots.append(chroot)
  roots.append('/')

  for prog in [para, std]:
    for root in roots:
      for subdir in ['', 'usr']:
        path = os.path.join(root, subdir, 'bin', prog)
        if os.path.exists(path):
          return path

  return std


def CompressionStrToType(s):
  """Convert a compression string type to a constant.

  Args:
    s: string to check

  Returns:
    A constant, or None if the compression type is unknown.
  """
  _COMP_STR = {
      'gz': COMP_GZIP,
      'bz2': COMP_BZIP2,
      'xz': COMP_XZ,
  }
  if s:
    return _COMP_STR.get(s)
  else:
    return COMP_NONE


def CompressionExtToType(file_name):
  """Retrieve a compression type constant from a compression file's name.

  Args:
    file_name: Name of a compression file.

  Returns:
    A constant, return COMP_NONE if the extension is unknown.
  """
  ext = os.path.splitext(file_name)[-1]
  _COMP_EXT = {
      '.tgz': COMP_GZIP,
      '.gz': COMP_GZIP,
      '.tbz2': COMP_BZIP2,
      '.bz2': COMP_BZIP2,
      '.txz': COMP_XZ,
      '.xz': COMP_XZ,
  }
  return _COMP_EXT.get(ext, COMP_NONE)


def CompressFile(infile, outfile):
  """Compress a file using compressor specified by |outfile| suffix.

  Args:
    infile: File to compress.
    outfile: Name of output file. Compression used is based on the
             type of suffix of the name specified (e.g.: .bz2).
  """
  comp_type = CompressionExtToType(outfile)
  assert comp_type and comp_type != COMP_NONE
  comp = FindCompressor(comp_type)
  if os.path.basename(comp) == 'pixz':
    # pixz does not accept '-c'; instead an explicit '-i' indicates input file
    # should not be deleted, and '-o' specifies output file.
    cmd = [comp, '-i', infile, '-o', outfile]
    RunCommand(cmd)
  else:
    cmd = [comp, '-c', infile]
    RunCommand(cmd, log_stdout_to_file=outfile)


def UncompressFile(infile, outfile):
  """Uncompress a file using compressor specified by |infile| suffix.

  Args:
    infile: File to uncompress. Compression used is based on the
            type of suffix of the name specified (e.g.: .bz2).
    outfile: Name of output file.
  """
  comp_type = CompressionExtToType(infile)
  assert comp_type and comp_type != COMP_NONE
  comp = FindCompressor(comp_type)
  if os.path.basename(comp) == 'pixz':
    # pixz does not accept '-c'; instead an explicit '-i' indicates input file
    # should not be deleted, and '-o' specifies output file.
    cmd = [comp, '-d', '-i', infile, '-o', outfile]
    RunCommand(cmd)
  else:
    cmd = [comp, '-dc', infile]
    RunCommand(cmd, log_stdout_to_file=outfile)


class CreateTarballError(RunCommandError):
  """Error while running tar.

  We may run tar multiple times because of "soft" errors.  The result is from
  the last RunCommand instance.
  """


def CreateTarball(target, cwd, sudo=False, compression=COMP_XZ, chroot=None,
                  inputs=None, extra_args=None, **kwargs):
  """Create a tarball.  Executes 'tar' on the commandline.

  Args:
    target: The path of the tar file to generate.
    cwd: The directory to run the tar command.
    sudo: Whether to run with "sudo".
    compression: The type of compression desired.  See the FindCompressor
      function for details.
    chroot: See FindCompressor().
    inputs: A list of files or directories to add to the tarball.  If unset,
      defaults to ".".
    extra_args: A list of extra args to pass to "tar".
    kwargs: Any RunCommand options/overrides to use.

  Returns:
    The cmd_result object returned by the RunCommand invocation.

  Raises:
    CreateTarballError: if the tar command failed, possibly after retry.
  """
  if inputs is None:
    inputs = ['.']

  if extra_args is None:
    extra_args = []
  kwargs.setdefault('debug_level', logging.INFO)

  comp = FindCompressor(compression, chroot=chroot)
  cmd = (['tar'] +
         extra_args +
         ['--sparse', '-I', comp, '-cf', target] +
         list(inputs))
  rc_func = SudoRunCommand if sudo else RunCommand

  # If tar fails with status 1, retry, but only once.  We think this is
  # acceptable because we see directories being modified, but not files.  Our
  # theory is that temporary files are created in those directories, but we
  # haven't been able to prove it yet.
  for try_count in range(2):
    result = rc_func(cmd, cwd=cwd, **dict(kwargs, error_code_ok=True))
    if result.returncode == 0:
      return result
    if result.returncode != 1 or try_count > 0:
      raise CreateTarballError('CreateTarball', result)
    assert result.returncode == 1 and try_count == 0
    logging.warning('CreateTarball: tar: source modification time changed ' +
                    '(see crbug.com/547055), retrying once')
    logging.PrintBuildbotStepWarnings()


def GroupByKey(input_iter, key):
  """Split an iterable of dicts, based on value of a key.

  GroupByKey([{'a': 1}, {'a': 2}, {'a': 1, 'b': 2}], 'a') =>
    {1: [{'a': 1}, {'a': 1, 'b': 2}], 2: [{'a': 2}]}

  Args:
    input_iter: An iterable of dicts.
    key: A string specifying the key name to split by.

  Returns:
    A dictionary, mapping from each unique value for |key| that
    was encountered in |input_iter| to a list of entries that had
    that value.
  """
  split_dict = dict()
  for entry in input_iter:
    split_dict.setdefault(entry.get(key), []).append(entry)
  return split_dict


def GroupNamedtuplesByKey(input_iter, key):
  """Split an iterable of namedtuples, based on value of a key.

  Args:
    input_iter: An iterable of namedtuples.
    key: A string specifying the key name to split by.

  Returns:
    A dictionary, mapping from each unique value for |key| that
    was encountered in |input_iter| to a list of entries that had
    that value.
  """
  split_dict = {}
  for entry in input_iter:
    split_dict.setdefault(getattr(entry, key, None), []).append(entry)
  return split_dict


def InvertDictionary(origin_dict):
  """Invert the key value mapping in the origin_dict.

  Given an origin_dict {'key1': {'val1', 'val2'}, 'key2': {'val1', 'val3'},
  'key3': {'val3'}}, the returned inverted dict will be
  {'val1': {'key1', 'key2'}, 'val2': {'key1'}, 'val3': {'key2', 'key3'}}

  Args:
    origin_dict: A dict mapping each key to a group (collection) of values.

  Returns:
    An inverted dict mapping each key to a set of its values.
  """
  new_dict = {}
  for origin_key, origin_values in origin_dict.iteritems():
    for origin_value in origin_values:
      new_dict.setdefault(origin_value, set()).add(origin_key)

  return new_dict


def GetInput(prompt):
  """Helper function to grab input from a user.   Makes testing easier."""
  return raw_input(prompt)


def GetChoice(title, options, group_size=0):
  """Ask user to choose an option from the list.

  When |group_size| is 0, then all items in |options| will be extracted and
  shown at the same time.  Otherwise, the items will be extracted |group_size|
  at a time, and then shown to the user.  This makes it easier to support
  generators that are slow, extremely large, or people usually want to pick
  from the first few choices.

  Args:
    title: The text to display before listing options.
    options: Iterable which provides options to display.
    group_size: How many options to show before asking the user to choose.

  Returns:
    An integer of the index in |options| the user picked.
  """
  def PromptForChoice(max_choice, more):
    prompt = 'Please choose an option [0-%d]' % max_choice
    if more:
      prompt += ' (Enter for more options)'
    prompt += ': '

    while True:
      choice = GetInput(prompt)
      if more and not choice.strip():
        return None
      try:
        choice = int(choice)
      except ValueError:
        print('Input is not an integer')
        continue
      if choice < 0 or choice > max_choice:
        print('Choice %d out of range (0-%d)' % (choice, max_choice))
        continue
      return choice

  print(title)
  max_choice = 0
  for i, opt in enumerate(options):
    if i and group_size and not i % group_size:
      choice = PromptForChoice(i - 1, True)
      if choice is not None:
        return choice
    print('  [%d]: %s' % (i, opt))
    max_choice = i

  return PromptForChoice(max_choice, False)


def BooleanPrompt(prompt='Do you want to continue?', default=True,
                  true_value='yes', false_value='no', prolog=None):
  """Helper function for processing boolean choice prompts.

  Args:
    prompt: The question to present to the user.
    default: Boolean to return if the user just presses enter.
    true_value: The text to display that represents a True returned.
    false_value: The text to display that represents a False returned.
    prolog: The text to display before prompt.

  Returns:
    True or False.
  """
  true_value, false_value = true_value.lower(), false_value.lower()
  true_text, false_text = true_value, false_value
  if true_value == false_value:
    raise ValueError('true_value and false_value must differ: got %r'
                     % true_value)

  if default:
    true_text = true_text[0].upper() + true_text[1:]
  else:
    false_text = false_text[0].upper() + false_text[1:]

  prompt = ('\n%s (%s/%s)? ' % (prompt, true_text, false_text))

  if prolog:
    prompt = ('\n%s\n%s' % (prolog, prompt))

  while True:
    try:
      response = GetInput(prompt).lower()
    except EOFError:
      # If the user hits CTRL+D, or stdin is disabled, use the default.
      print()
      response = None
    except KeyboardInterrupt:
      # If the user hits CTRL+C, just exit the process.
      print()
      Die('CTRL+C detected; exiting')

    if not response:
      return default
    if true_value.startswith(response):
      if not false_value.startswith(response):
        return True
      # common prefix between the two...
    elif false_value.startswith(response):
      return False


def BooleanShellValue(sval, default, msg=None):
  """See if the string value is a value users typically consider as boolean

  Often times people set shell variables to different values to mean "true"
  or "false".  For example, they can do:
    export FOO=yes
    export BLAH=1
    export MOO=true
  Handle all that user ugliness here.

  If the user picks an invalid value, you can use |msg| to display a non-fatal
  warning rather than raising an exception.

  Args:
    sval: The string value we got from the user.
    default: If we can't figure out if the value is true or false, use this.
    msg: If |sval| is an unknown value, use |msg| to warn the user that we
         could not decode the input.  Otherwise, raise ValueError().

  Returns:
    The interpreted boolean value of |sval|.

  Raises:
    ValueError() if |sval| is an unknown value and |msg| is not set.
  """
  if sval is None:
    return default

  if isinstance(sval, basestring):
    s = sval.lower()
    if s in ('yes', 'y', '1', 'true'):
      return True
    elif s in ('no', 'n', '0', 'false'):
      return False

  if msg is not None:
    logging.warning('%s: %r', msg, sval)
    return default
  else:
    raise ValueError('Could not decode as a boolean value: %r' % sval)


# Suppress whacked complaints about abstract class being unused.
class MasterPidContextManager(object):
  """Allow context managers to restrict their exit to within the same PID."""

  # In certain cases we actually want this ran outside
  # of the main pid- specifically in backup processes
  # doing cleanup.
  ALTERNATE_MASTER_PID = None

  def __init__(self):
    self._invoking_pid = None

  def __enter__(self):
    self._invoking_pid = os.getpid()
    return self._enter()

  def __exit__(self, exc_type, exc, exc_tb):
    curpid = os.getpid()
    if curpid == self.ALTERNATE_MASTER_PID:
      self._invoking_pid = curpid
    if curpid == self._invoking_pid:
      return self._exit(exc_type, exc, exc_tb)

  def _enter(self):
    raise NotImplementedError(self, '_enter')

  def _exit(self, exc_type, exc, exc_tb):
    raise NotImplementedError(self, '_exit')


@contextlib.contextmanager
def NoOpContextManager():
  yield


def AllowDisabling(enabled, functor, *args, **kwargs):
  """Context Manager wrapper that can be used to enable/disable usage.

  This is mainly useful to control whether or not a given Context Manager
  is used.

  For example:

  with AllowDisabling(options.timeout <= 0, Timeout, options.timeout):
    ... do code w/in a timeout context..

  If options.timeout is a positive integer, then the_Timeout context manager is
  created and ran.  If it's zero or negative, then the timeout code is disabled.

  While Timeout *could* handle this itself, it's redundant having each
  implementation do this, thus the generic wrapper.
  """
  if enabled:
    return functor(*args, **kwargs)
  return NoOpContextManager()


class ContextManagerStack(object):
  """Context manager that is designed to safely allow nesting and stacking.

  Python2.7 directly supports a with syntax generally removing the need for
  this, although this form avoids indentation hell if there is a lot of context
  managers.  It also permits more programmatic control and allowing conditional
  usage.

  For Python2.6, see http://docs.python.org/library/contextlib.html; the short
  version is that there is a race in the available stdlib/language rules under
  2.6 when dealing w/ multiple context managers, thus this safe version was
  added.

  For each context manager added to this instance, it will unwind them,
  invoking them as if it had been constructed as a set of manually nested
  with statements.
  """

  def __init__(self):
    self._stack = []

  def Add(self, functor, *args, **kwargs):
    """Add a context manager onto the stack.

    Usage of this is essentially the following:
    >>> stack.add(Timeout, 60)

    It must be done in this fashion, else there is a mild race that exists
    between context manager instantiation and initial __enter__.

    Invoking it in the form specified eliminates that race.

    Args:
      functor: A callable to instantiate a context manager.
      args and kwargs: positional and optional args to functor.

    Returns:
      The newly created (and __enter__'d) context manager.
      Note: This is not the same value as the "with" statement -- that returns
      the value from the __enter__ function while this is the manager itself.
    """
    obj = None
    try:
      obj = functor(*args, **kwargs)
      return obj
    finally:
      if obj is not None:
        obj.__enter__()
        self._stack.append(obj)

  def __enter__(self):
    # Nothing to do in this case.  The individual __enter__'s are done
    # when the context managers are added, which will likely be after
    # the __enter__ method of this stack is called.
    return self

  def __exit__(self, exc_type, exc, exc_tb):
    # Exit each context manager in stack in reverse order, tracking the results
    # to know whether or not to suppress the exception raised (or to switch that
    # exception to a new one triggered by an individual handler's __exit__).
    for handler in reversed(self._stack):
      # pylint: disable=bare-except
      try:
        if handler.__exit__(exc_type, exc, exc_tb):
          exc_type = exc = exc_tb = None
      except:
        exc_type, exc, exc_tb = sys.exc_info()

    self._stack = []

    # Return True if any exception was handled.
    if all(x is None for x in (exc_type, exc, exc_tb)):
      return True

    # Raise any exception that is left over from exiting all context managers.
    # Normally a single context manager would return False to allow caller to
    # re-raise the exception itself, but here the exception might have been
    # raised during the exiting of one of the individual context managers.
    raise exc_type, exc, exc_tb


class ApiMismatchError(Exception):
  """Raised by GetTargetChromiteApiVersion."""


class NoChromiteError(Exception):
  """Raised when an expected chromite installation was missing."""


def GetTargetChromiteApiVersion(buildroot, validate_version=True):
  """Get the re-exec API version of the target chromite.

  Args:
    buildroot: The directory containing the chromite to check.
    validate_version: If set to true, checks the target chromite for
      compatibility, and raises an ApiMismatchError when there is an
      incompatibility.

  Returns:
    The version number in (major, minor) tuple.

  Raises:
    May raise an ApiMismatchError if validate_version is set.
  """
  try:
    api = RunCommand(
        [constants.PATH_TO_CBUILDBOT, '--reexec-api-version'],
        cwd=buildroot, error_code_ok=True, capture_output=True)
  except RunCommandError:
    # Although error_code_ok=True was used, this exception will still be raised
    # if the executible did not exist.
    full_cbuildbot_path = os.path.join(buildroot, constants.PATH_TO_CBUILDBOT)
    if not os.path.exists(full_cbuildbot_path):
      raise NoChromiteError('No cbuildbot found in buildroot %s, expected to '
                            'find %s. ' % (buildroot, full_cbuildbot_path))
    raise

  # If the command failed, then we're targeting a cbuildbot that lacks the
  # option; assume 0:0 (ie, initial state).
  major = minor = 0
  if api.returncode == 0:
    major, minor = map(int, api.output.strip().split('.', 1))

  if validate_version and major != constants.REEXEC_API_MAJOR:
    raise ApiMismatchError(
        'The targeted version of chromite in buildroot %s requires '
        'api version %i, but we are api version %i.  We cannot proceed.'
        % (buildroot, major, constants.REEXEC_API_MAJOR))

  return major, minor


def iflatten_instance(iterable, terminate_on_kls=(basestring,)):
  """Derivative of snakeoil.lists.iflatten_instance; flatten an object.

  Given an object, flatten it into a single depth iterable-
  stopping descent on objects that either aren't iterable, or match
  isinstance(obj, terminate_on_kls).

  Example:
  >>> print list(iflatten_instance([1, 2, "as", ["4", 5]))
  [1, 2, "as", "4", 5]
  """
  def descend_into(item):
    if isinstance(item, terminate_on_kls):
      return False
    try:
      iter(item)
    except TypeError:
      return False
    # Note strings can be infinitely descended through- thus this
    # recursion limiter.
    return not isinstance(item, basestring) or len(item) > 1

  if not descend_into(iterable):
    yield iterable
    return
  for item in iterable:
    if not descend_into(item):
      yield item
    else:
      for subitem in iflatten_instance(item, terminate_on_kls):
        yield subitem


# TODO: Remove this once we move to snakeoil.
def load_module(name):
  """load a module

  Args:
    name: python dotted namespace path of the module to import

  Returns:
    imported module

  Raises:
    FailedImport if importing fails
  """
  m = __import__(name)
  # __import__('foo.bar') returns foo, so...
  for bit in name.split('.')[1:]:
    m = getattr(m, bit)
  return m


def PredicateSplit(func, iterable):
  """Splits an iterable into two groups based on a predicate return value.

  Args:
    func: A functor that takes an item as its argument and returns a boolean
      value indicating which group the item belongs.
    iterable: The collection to split.

  Returns:
    A tuple containing two lists, the first containing items that func()
    returned True for, and the second containing items that func() returned
    False for.
  """
  trues, falses = [], []
  for x in iterable:
    (trues if func(x) else falses).append(x)
  return trues, falses


@contextlib.contextmanager
def Open(obj, mode='r'):
  """Convenience ctx that accepts a file path or an already open file object."""
  if isinstance(obj, basestring):
    with open(obj, mode=mode) as f:
      yield f
  else:
    yield obj


def LoadKeyValueFile(obj, ignore_missing=False, multiline=False):
  """Turn a key=value file into a dict

  Note: If you're designing a new data store, please use json rather than
  this format.  This func is designed to work with legacy/external files
  where json isn't an option.

  Args:
    obj: The file to read.  Can be a path or an open file object.
    ignore_missing: If the file does not exist, return an empty dict.
    multiline: Allow a value enclosed by quotes to span multiple lines.

  Returns:
    a dict of all the key=value pairs found in the file.
  """
  d = {}

  try:
    with Open(obj) as f:
      key = None
      in_quotes = None
      for raw_line in f:
        line = raw_line.split('#')[0]
        if not line.strip():
          continue

        # Continue processing a multiline value.
        if multiline and in_quotes and key:
          if line.rstrip()[-1] == in_quotes:
            # Wrap up the multiline value if the line ends with a quote.
            d[key] += line.rstrip()[:-1]
            in_quotes = None
          else:
            d[key] += line
          continue

        chunks = line.split('=', 1)
        if len(chunks) != 2:
          raise ValueError('Malformed key=value file %r; line %r'
                           % (obj, raw_line))
        key = chunks[0].strip()
        val = chunks[1].strip()
        if len(val) >= 2 and val[0] in "\"'" and val[0] == val[-1]:
          # Strip matching quotes on the same line.
          val = val[1:-1]
        elif val and multiline and val[0] in "\"'":
          # Unmatched quote here indicates a multiline value. Do not
          # strip the '\n' at the end of the line.
          in_quotes = val[0]
          val = chunks[1].lstrip()[1:]
        d[key] = val
  except EnvironmentError as e:
    if not (ignore_missing and e.errno == errno.ENOENT):
      raise

  return d


def MemoizedSingleCall(functor):
  """Decorator for simple functor targets, caching the results

  The functor must accept no arguments beyond either a class or self (depending
  on if this is used in a classmethod/instancemethod context).  Results of the
  wrapped method will be written to the class/instance namespace in a specially
  named cached value.  All future invocations will just reuse that value.

  Note that this cache is per-process, so sibling and parent processes won't
  notice updates to the cache.
  """
  # TODO(build): Should we rebase to snakeoil.klass.cached* functionality?
  # pylint: disable=protected-access
  @functools.wraps(functor)
  def wrapper(obj):
    key = wrapper._cache_key
    val = getattr(obj, key, None)
    if val is None:
      val = functor(obj)
      setattr(obj, key, val)
    return val

  # Use name mangling to store the cached value in a (hopefully) unique place.
  wrapper._cache_key = '_%s_cached' % (functor.__name__.lstrip('_'),)
  return wrapper


def Memoize(f):
  """Decorator for memoizing a function.

  Caches all calls to the function using a ._memo_cache dict mapping (args,
  kwargs) to the results of the first function call with those args and kwargs.

  If any of args or kwargs are not hashable, trying to store them in a dict will
  cause a ValueError.

  Note that this cache is per-process, so sibling and parent processes won't
  notice updates to the cache.
  """
  # pylint: disable=protected-access
  f._memo_cache = {}

  @functools.wraps(f)
  def wrapper(*args, **kwargs):
    # Make sure that the key is hashable... as long as the contents of args and
    # kwargs are hashable.
    # TODO(phobbs) we could add an option to use the id(...) of an object if
    # it's not hashable.  Then "MemoizedSingleCall" would be obsolete.
    key = (tuple(args), tuple(sorted(kwargs.items())))
    if key in f._memo_cache:
      return f._memo_cache[key]

    result = f(*args, **kwargs)
    f._memo_cache[key] = result
    return result

  return wrapper


def SafeRun(functors, combine_exceptions=False):
  """Executes a list of functors, continuing on exceptions.

  Args:
    functors: An iterable of functors to call.
    combine_exceptions: If set, and multiple exceptions are encountered,
      SafeRun will raise a RuntimeError containing a list of all the exceptions.
      If only one exception is encountered, then the default behavior of
      re-raising the original exception with unmodified stack trace will be
      kept.

  Raises:
    The first exception encountered, with corresponding backtrace, unless
    |combine_exceptions| is specified and there is more than one exception
    encountered, in which case a RuntimeError containing a list of all the
    exceptions that were encountered is raised.
  """
  errors = []

  for f in functors:
    try:
      f()
    except Exception as e:
      # Append the exception object and the traceback.
      errors.append((e, sys.exc_info()[2]))

  if errors:
    if len(errors) == 1 or not combine_exceptions:
      # To preserve the traceback.
      inst, tb = errors[0]
      raise inst, None, tb
    else:
      raise RuntimeError([e[0] for e in errors])


def ParseDurationToSeconds(duration):
  """Parses a string duration of the form HH:MM:SS into seconds.

  Args:
    duration: A string such as '12:43:12' (representing in this case
              12 hours, 43 minutes, 12 seconds).

  Returns:
    An integer number of seconds.
  """
  h, m, s = [int(t) for t in duration.split(':')]
  return s + 60 * m + 3600 * h


def UserDateTimeFormat(timeval=None):
  """Format a date meant to be viewed by a user

  The focus here is to have a format that is easily readable by humans,
  but still easy (and unambiguous) for a machine to parse.  Hence, we
  use the RFC 2822 date format (with timezone name appended).

  Args:
    timeval: Either a datetime object or a floating point time value as accepted
             by gmtime()/localtime().  If None, the current time is used.

  Returns:
    A string format such as 'Wed, 20 Feb 2013 15:25:15 -0500 (EST)'
  """
  if isinstance(timeval, datetime):
    timeval = time.mktime(timeval.timetuple())
  return '%s (%s)' % (email.utils.formatdate(timeval=timeval, localtime=True),
                      time.strftime('%Z', time.localtime(timeval)))


def GetCommonPathPrefix(paths):
  """Get the longest common directory of |paths|.

  Args:
    paths: A list of absolute directory or file paths.

  Returns:
    Absolute path to the longest directory common to |paths|, with no
    trailing '/'.
  """
  return os.path.dirname(os.path.commonprefix(paths))


def ParseUserDateTimeFormat(time_string):
  """Parse a time string into a floating point time value.

  This function is essentially the inverse of UserDateTimeFormat.

  Args:
    time_string: A string datetime represetation in RFC 2822 format, such as
                 'Wed, 20 Feb 2013 15:25:15 -0500 (EST)'.

  Returns:
    Floating point Unix timestamp (seconds since epoch).
  """
  return email.utils.mktime_tz(email.utils.parsedate_tz(time_string))


def GetDefaultBoard():
  """Gets the default board.

  Returns:
    The default board (as a string), or None if either the default board
    file was missing or malformed.
  """
  default_board_file_name = os.path.join(constants.SOURCE_ROOT, 'src',
                                         'scripts', '.default_board')
  try:
    with open(default_board_file_name) as default_board_file:
      default_board = default_board_file.read().strip()
      # Check for user typos like whitespace
      if not re.match('[a-zA-Z0-9-_]*$', default_board):
        logging.warning('Noticed invalid default board: |%s|. Ignoring this '
                        'default.', default_board)
        default_board = None
  except IOError:
    return None

  return default_board


def GetBoard(device_board, override_board=None, force=False):
  """Gets the board name to use.

  Ask user to confirm when |override_board| and |device_board| are
  both None.

  Args:
    device_board: The board detected on the device.
    override_board: Overrides the board.
    force: Force using the default board if |device_board| is None.

  Returns:
    Returns the first non-None board in the following order:
    |override_board|, |device_board|, and GetDefaultBoard().

  Raises:
    DieSystemExit: If user enters no.
  """
  if override_board:
    return override_board

  board = device_board or GetDefaultBoard()
  if not device_board:
    msg = 'Cannot detect board name; using default board %s.' % board
    if not force and not BooleanPrompt(default=False, prolog=msg):
      Die('Exiting...')

    logging.warning(msg)

  return board


class AttributeFrozenError(Exception):
  """Raised when frozen attribute value is modified."""


class FrozenAttributesClass(type):
  """Metaclass for any class to support freezing attribute values.

  This metaclass can be used by any class to add the ability to
  freeze attribute values with the Freeze method.

  Use by adding this line in a class:
    __metaclass__ = FrozenAttributesClass
  """
  _FROZEN_ERR_MSG = 'Attribute values are frozen, cannot alter %s.'

  def __new__(mcs, clsname, bases, scope):
    # Create Freeze method that freezes current attributes.
    if 'Freeze' in scope:
      raise TypeError('Class %s has its own Freeze method, cannot use with'
                      ' the FrozenAttributesClass metaclass.' % clsname)

    # Make sure cls will have _FROZEN_ERR_MSG set.
    scope.setdefault('_FROZEN_ERR_MSG', mcs._FROZEN_ERR_MSG)

    # Create the class.
    # pylint: disable=bad-super-call
    cls = super(FrozenAttributesClass, mcs).__new__(mcs, clsname, bases, scope)

    # Replace cls.__setattr__ with the one that honors freezing.
    orig_setattr = cls.__setattr__

    def SetAttr(obj, name, value):
      """If the object is frozen then abort."""
      # pylint: disable=protected-access
      if getattr(obj, '_frozen', False):
        raise AttributeFrozenError(obj._FROZEN_ERR_MSG % name)
      if isinstance(orig_setattr, types.MethodType):
        orig_setattr(obj, name, value)
      else:
        super(cls, obj).__setattr__(name, value)
    cls.__setattr__ = SetAttr

    # Add new cls.Freeze method.
    def Freeze(obj):
      # pylint: disable=protected-access
      obj._frozen = True
    cls.Freeze = Freeze

    return cls


class FrozenAttributesMixin(object):
  """Alternate mechanism for freezing attributes in a class.

  If an existing class is not a new-style class then it will be unable to
  use the FrozenAttributesClass metaclass directly.  Simply use this class
  as a mixin instead to accomplish the same thing.
  """
  __metaclass__ = FrozenAttributesClass


def GetIPv4Address(dev=None, global_ip=True):
  """Returns any global/host IP address or the IP address of the given device.

  socket.gethostname() is insufficient for machines where the host files are
  not set up "correctly."  Since some of our builders may have this issue,
  this method gives you a generic way to get the address so you are reachable
  either via a VM or remote machine on the same network.

  Args:
    dev: Get the IP address of the device (e.g. 'eth0').
    global_ip: If set True, returns a globally valid IP address. Otherwise,
      returns a local IP address (default: True).
  """
  cmd = ['ip', 'addr', 'show']
  cmd += ['scope', 'global' if global_ip else 'host']
  cmd += [] if dev is None else ['dev', dev]

  result = RunCommand(cmd, print_cmd=False, capture_output=True)
  matches = re.findall(r'\binet (\d+\.\d+\.\d+\.\d+).*', result.output)
  if matches:
    return matches[0]
  logging.warning('Failed to find ip address in %r', result.output)
  return None


def GetSysroot(board=None):
  """Returns the sysroot for |board| or '/' if |board| is None."""
  return '/' if board is None else os.path.join('/build', board)


def Collection(classname, **kwargs):
  """Create a new class with mutable named members.

  This is like collections.namedtuple, but mutable.  Also similar to the
  python 3.3 types.SimpleNamespace.

  Example:
    # Declare default values for this new class.
    Foo = cros_build_lib.Collection('Foo', a=0, b=10)
    # Create a new class but set b to 4.
    foo = Foo(b=4)
    # Print out a (will be the default 0) and b (will be 4).
    print('a = %i, b = %i' % (foo.a, foo.b))
  """

  def sn_init(self, **kwargs):
    """The new class's __init__ function."""
    # First verify the kwargs don't have excess settings.
    valid_keys = set(self.__slots__[1:])
    these_keys = set(kwargs.keys())
    invalid_keys = these_keys - valid_keys
    if invalid_keys:
      raise TypeError('invalid keyword arguments for this object: %r' %
                      invalid_keys)

    # Now initialize this object.
    for k in valid_keys:
      setattr(self, k, kwargs.get(k, self.__defaults__[k]))

  def sn_repr(self):
    """The new class's __repr__ function."""
    return '%s(%s)' % (classname, ', '.join(
        '%s=%r' % (k, getattr(self, k)) for k in self.__slots__[1:]))

  # Give the new class a unique name and then generate the code for it.
  classname = 'Collection_%s' % classname
  expr = '\n'.join((
      'class %(classname)s(object):',
      '  __slots__ = ["__defaults__", "%(slots)s"]',
      '  __defaults__ = {}',
  )) % {
      'classname': classname,
      'slots': '", "'.join(sorted(str(k) for k in kwargs)),
  }

  # Create the class in a local namespace as exec requires.
  namespace = {}
  exec expr in namespace
  new_class = namespace[classname]

  # Bind the helpers.
  new_class.__defaults__ = kwargs.copy()
  new_class.__init__ = sn_init
  new_class.__repr__ = sn_repr

  return new_class


# Structure to hold the values produced by TimedSection.
#
#  Attributes:
#    start: The absolute start time as a datetime.
#    finish: The absolute finish time as a datetime, or None if in progress.
#    delta: The runtime as a timedelta, or None if in progress.
TimedResults = Collection('TimedResults', start=None, finish=None, delta=None)


@contextlib.contextmanager
def TimedSection():
  """Context manager to time how long a code block takes.

  Example usage:
    with cros_build_lib.TimedSection() as timer:
      DoWork()
    logging.info('DoWork took %s', timer.delta)

  Context manager value will be a TimedResults instance.
  """
  # Create our context manager value.
  times = TimedResults(start=datetime.now())
  try:
    yield times
  finally:
    times.finish = datetime.now()
    times.delta = times.finish - times.start


PartitionInfo = collections.namedtuple(
    'PartitionInfo',
    ['number', 'start', 'end', 'size', 'file_system', 'name', 'flags']
)


def _ParseParted(lines, unit='MB'):
  """Returns partition information from `parted print` output."""
  ret = []
  # Sample output (partition #, start, end, size, file system, name, flags):
  #   /foo/chromiumos_qemu_image.bin:3360MB:file:512:512:gpt:;
  #   11:0.03MB:8.42MB:8.39MB::RWFW:;
  #   6:8.42MB:8.42MB:0.00MB::KERN-C:;
  #   7:8.42MB:8.42MB:0.00MB::ROOT-C:;
  #   9:8.42MB:8.42MB:0.00MB::reserved:;
  #   10:8.42MB:8.42MB:0.00MB::reserved:;
  #   2:10.5MB:27.3MB:16.8MB::KERN-A:;
  #   4:27.3MB:44.0MB:16.8MB::KERN-B:;
  #   8:44.0MB:60.8MB:16.8MB:ext4:OEM:;
  #   12:128MB:145MB:16.8MB:fat16:EFI-SYSTEM:boot;
  #   5:145MB:2292MB:2147MB::ROOT-B:;
  #   3:2292MB:4440MB:2147MB:ext2:ROOT-A:;
  #   1:4440MB:7661MB:3221MB:ext4:STATE:;
  pattern = re.compile(r'(([^:]*:){6}[^:]*);')
  for line in lines:
    match = pattern.match(line)
    if match:
      d = dict(zip(PartitionInfo._fields, match.group(1).split(':')))
      # Disregard any non-numeric partition number (e.g. the file path).
      if d['number'].isdigit():
        d['number'] = int(d['number'])
        for key in ['start', 'end', 'size']:
          d[key] = float(d[key][:-len(unit)])
        ret.append(PartitionInfo(**d))
  return ret


def _ParseCgpt(lines, unit='MB'):
  """Returns partition information from `cgpt show` output."""
  #   start        size    part  contents
  # 1921024     2097152       1  Label: "STATE"
  #                              Type: Linux data
  #                              UUID: EEBD83BE-397E-BD44-878B-0DDDD5A5C510
  #   20480       32768       2  Label: "KERN-A"
  #                              Type: ChromeOS kernel
  #                              UUID: 7007C2F3-08E5-AB40-A4BC-FF5B01F5460D
  #                              Attr: priority=15 tries=15 successful=1
  start_pattern = re.compile(r'''\s+(\d+)\s+(\d+)\s+(\d+)\s+Label: "(.+)"''')
  ret = []
  line_no = 0
  while line_no < len(lines):
    line = lines[line_no]
    line_no += 1
    m = start_pattern.match(line)
    if not m:
      continue

    start, size, number, label = m.groups()
    number = int(number)
    start = int(start) * 512
    size = int(size) * 512
    end = start + size
    # Parted uses 1000, not 1024.
    divisors = {
        'B': 1.0,
        'KB': 1000.0,
        'MB': 1000000.0,
        'GB': 1000000000.0,
    }
    divisor = divisors[unit]
    start = start / divisor
    end = end / divisor
    size = size / divisor

    ret.append(PartitionInfo(number=number, start=start, end=end, size=size,
                             name=label, file_system='', flags=''))

  return ret


def GetImageDiskPartitionInfo(image_path, unit='MB', key_selector='name'):
  """Returns the disk partition table of an image.

  Args:
    image_path: Path to the image file.
    unit: The unit to display (e.g., 'B', 'KB', 'MB', 'GB').
      See `parted` documentation for more info.
    key_selector: The value of the partition that will be used as the key for
      that partition in this function's returned dictionary.

  Returns:
    A dictionary of ParitionInfo items keyed by |key_selector|.
  """

  if IsInsideChroot():
    # Inside chroot, use `cgpt`.
    cmd = ['cgpt', 'show', image_path]
    func = _ParseCgpt
  else:
    # Outside chroot, use `parted`.
    cmd = ['parted', '-m', image_path, 'unit', unit, 'print']
    func = _ParseParted

  lines = RunCommand(
      cmd,
      extra_env={'PATH': '/sbin:%s' % os.environ['PATH'], 'LC_ALL': 'C'},
      capture_output=True).output.splitlines()
  infos = func(lines, unit)
  selector = operator.attrgetter(key_selector)
  return dict((selector(x), x) for x in infos)


def GetRandomString(length=20):
  """Returns a random string of |length|."""
  md5 = hashlib.md5(os.urandom(length))
  md5.update(UserDateTimeFormat())
  return md5.hexdigest()


def MachineDetails():
  """Returns a string to help identify the source of a job.

  This is not meant for machines to parse; instead, we want content that is easy
  for humans to read when trying to figure out where "something" is coming from.
  For example, when a service has grabbed a lock in Google Storage, and we want
  to see what process actually triggered that (in case it is a test gone rogue),
  the content in here should help triage.

  Note: none of the details included may be secret so they can be freely pasted
  into bug reports/chats/logs/etc...

  Note: this content should not be large

  Returns:
    A string with content that helps identify this system/process/etc...
  """
  return '\n'.join((
      'PROG=%s' % inspect.stack()[-1][1],
      'USER=%s' % getpass.getuser(),
      'HOSTNAME=%s' % GetHostName(fully_qualified=True),
      'PID=%s' % os.getpid(),
      'TIMESTAMP=%s' % UserDateTimeFormat(),
      'RANDOM_JUNK=%s' % GetRandomString(),
  )) + '\n'


def FormatDetailedTraceback(exc_info=None):
  """Generate a traceback including details like local variables.

  Args:
    exc_info: The exception tuple to format; defaults to sys.exc_info().
      See the help on that function for details on the type.

  Returns:
    A string of the formatted |exc_info| details.
  """
  if exc_info is None:
    exc_info = sys.exc_info()

  ret = []
  try:
    # pylint: disable=unpacking-non-sequence
    exc_type, exc_value, exc_tb = exc_info

    if exc_type:
      ret += [
          'Traceback (most recent call last):\n',
          'Note: Call args reflect *current* state, not *entry* state\n',
      ]

    while exc_tb:
      frame = exc_tb.tb_frame

      ret += traceback.format_tb(exc_tb, 1)
      args = inspect.getargvalues(frame)
      _, _, fname, _ = traceback.extract_tb(exc_tb, 1)[0]
      ret += [
          '    Call: %s%s\n' % (fname, inspect.formatargvalues(*args)),
          '    Locals:\n',
      ]
      if frame.f_locals:
        keys = sorted(frame.f_locals.keys(), key=str.lower)
        keylen = max(len(x) for x in keys)
        typelen = max(len(str(type(x))) for x in frame.f_locals.values())
        for key in keys:
          val = frame.f_locals[key]
          ret += ['      %-*s: %-*s %s\n' %
                  (keylen, key, typelen, type(val), pprint.saferepr(val))]
      exc_tb = exc_tb.tb_next

    if exc_type:
      ret += traceback.format_exception_only(exc_type, exc_value)
  finally:
    # Help python with its circular references.
    del exc_tb

  return ''.join(ret)


def PrintDetailedTraceback(exc_info=None, file=None):
  """Print a traceback including details like local variables.

  Args:
    exc_info: The exception tuple to format; defaults to sys.exc_info().
      See the help on that function for details on the type.
    file: The file object to write the details to; defaults to sys.stderr.
  """
  # We use |file| to match the existing traceback API.
  # pylint: disable=redefined-builtin
  if exc_info is None:
    exc_info = sys.exc_info()
  if file is None:
    file = sys.stderr

  # Try to print out extended details on the current exception.
  # If that fails, still fallback to the normal exception path.
  curr_exc_info = exc_info
  try:
    output = FormatDetailedTraceback()
    if output:
      print(output, file=file)
  except Exception:
    print('Could not decode extended exception details:', file=file)
    traceback.print_exc(file=file)
    print(file=file)
    traceback.print_exception(*curr_exc_info, file=sys.stdout)
  finally:
    # Help python with its circular references.
    del exc_info
    del curr_exc_info


class _FdCapturer(object):
  """Helper class to capture output at the file descriptor level.

  This is meant to be used with sys.stdout or sys.stderr. By capturing
  file descriptors, this will also intercept subprocess output, which
  reassigning sys.stdout or sys.stderr will not do.

  Output will only be captured, it will no longer be printed while
  the capturer is active.
  """

  def __init__(self, source, output=None):
    """Construct the _FdCapturer object.

    Does not start capturing until Start() is called.

    Args:
      source: A file object to capture. Typically sys.stdout or
        sys.stderr, but will work with anything that implements flush()
        and fileno().
      output: A file name where the captured output is to be stored. If None,
        then the output will be stored to a temporary file.
    """
    self._source = source
    self._captured = ''
    self._saved_fd = None
    self._tempfile = None
    self._capturefile = None
    self._capturefile_reader = None
    self._capturefile_name = output

  def _SafeCreateTempfile(self, tempfile_obj):
    """Ensure that the tempfile is created safely.

    (1) Stash away a reference to the tempfile.
    (2) Unlink the file from the filesystem.

    (2) ensures that if we crash, the file gets deleted. (1) ensures that while
    we are running, we hold a reference to the file so the system does not close
    the file.

    Args:
      tempfile_obj: A tempfile object.
    """
    self._tempfile = tempfile_obj
    os.unlink(tempfile_obj.name)

  def Start(self):
    """Begin capturing output."""
    if self._capturefile_name is None:
      tempfile_obj = tempfile.NamedTemporaryFile(delete=False)
      self._capturefile = tempfile_obj.file
      self._capturefile_name = tempfile_obj.name
      self._capturefile_reader = open(self._capturefile_name)
      self._SafeCreateTempfile(tempfile_obj)
    else:
      # Open file passed in for writing. Set buffering=1 for line level
      # buffering.
      self._capturefile = open(self._capturefile_name, 'w', buffering=1)
      self._capturefile_reader = open(self._capturefile_name)
    # Save the original fd so we can revert in Stop().
    self._saved_fd = os.dup(self._source.fileno())
    os.dup2(self._capturefile.fileno(), self._source.fileno())

  def Stop(self):
    """Stop capturing output."""
    self.GetCaptured()
    if self._saved_fd is not None:
      os.dup2(self._saved_fd, self._source.fileno())
      os.close(self._saved_fd)
      self._saved_fd = None
    # If capturefile and capturefile_reader exist, close them as they were
    # opened in self.Start().
    if self._capturefile_reader is not None:
      self._capturefile_reader.close()
      self._capturefile_reader = None
    if self._capturefile is not None:
      self._capturefile.close()
      self._capturefile = None

  def GetCaptured(self):
    """Return all output captured up to this point.

    Can be used while capturing or after Stop() has been called.
    """
    self._source.flush()
    if self._capturefile_reader is not None:
      self._captured += self._capturefile_reader.read()
    return self._captured

  def ClearCaptured(self):
    """Erase all captured output."""
    self.GetCaptured()
    self._captured = ''


class OutputCapturer(object):
  """Class for capturing stdout/stderr output.

  Class is designed as a 'ContextManager'.  Example usage:

  with cros_build_lib.OutputCapturer() as output:
    # Capturing of stdout/stderr automatically starts now.
    # Do stuff that sends output to stdout/stderr.
    # Capturing automatically stops at end of 'with' block.

  # stdout/stderr can be retrieved from the OutputCapturer object:
  stdout = output.GetStdoutLines() # Or other access methods

  # Some Assert methods are only valid if capturing was used in test.
  self.AssertOutputContainsError() # Or other related methods

  # OutputCapturer can also be used to capture output to specified files.
  with self.OutputCapturer(stdout_path='/tmp/stdout.txt') as output:
    # Do stuff.
    # stdout will be captured to /tmp/stdout.txt.
  """

  OPER_MSG_SPLIT_RE = re.compile(r'^\033\[1;.*?\033\[0m$|^[^\n]*$',
                                 re.DOTALL | re.MULTILINE)

  __slots__ = ['_stdout_capturer', '_stderr_capturer', '_quiet_fail']

  def __init__(self, stdout_path=None, stderr_path=None, quiet_fail=False):
    """Initalize OutputCapturer with capture files.

    If OutputCapturer is initialized with filenames to capture stdout and stderr
    to, then those files are used. Otherwise, temporary files are created.

    Args:
      stdout_path: File to capture stdout to. If None, a temporary file is used.
      stderr_path: File to capture stderr to. If None, a temporary file is used.
      quiet_fail: If True fail quietly without printing the captured stdout and
        stderr.
    """
    self._stdout_capturer = _FdCapturer(sys.stdout, output=stdout_path)
    self._stderr_capturer = _FdCapturer(sys.stderr, output=stderr_path)
    self._quiet_fail = quiet_fail

  def __enter__(self):
    # This method is called with entering 'with' block.
    self.StartCapturing()
    return self

  def __exit__(self, exc_type, exc_val, exc_tb):
    # This method is called when exiting 'with' block.
    self.StopCapturing()

    if exc_type and not self._quiet_fail:
      print('Exception during output capturing: %r' % (exc_val,))
      stdout = self.GetStdout()
      if stdout:
        print('Captured stdout was:\n%s' % stdout)
      else:
        print('No captured stdout')
      stderr = self.GetStderr()
      if stderr:
        print('Captured stderr was:\n%s' % stderr)
      else:
        print('No captured stderr')

  def StartCapturing(self):
    """Begin capturing stdout and stderr."""
    self._stdout_capturer.Start()
    self._stderr_capturer.Start()

  def StopCapturing(self):
    """Stop capturing stdout and stderr."""
    self._stdout_capturer.Stop()
    self._stderr_capturer.Stop()

  def ClearCaptured(self):
    """Clear any captured stdout/stderr content."""
    self._stdout_capturer.ClearCaptured()
    self._stderr_capturer.ClearCaptured()

  def GetStdout(self):
    """Return captured stdout so far."""
    return self._stdout_capturer.GetCaptured()

  def GetStderr(self):
    """Return captured stderr so far."""
    return self._stderr_capturer.GetCaptured()

  def _GetOutputLines(self, output, include_empties):
    """Split |output| into lines, optionally |include_empties|.

    Return array of lines.
    """

    lines = self.OPER_MSG_SPLIT_RE.findall(output)
    if not include_empties:
      lines = [ln for ln in lines if ln]

    return lines

  def GetStdoutLines(self, include_empties=True):
    """Return captured stdout so far as array of lines.

    If |include_empties| is false filter out all empty lines.
    """
    return self._GetOutputLines(self.GetStdout(), include_empties)

  def GetStderrLines(self, include_empties=True):
    """Return captured stderr so far as array of lines.

    If |include_empties| is false filter out all empty lines.
    """
    return self._GetOutputLines(self.GetStderr(), include_empties)
