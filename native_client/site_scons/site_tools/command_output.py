#!/usr/bin/python2.4
# Copyright 2009 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Command output builder for SCons."""


import os
import signal
import subprocess
import sys
import threading
import time
import SCons.Script


# TODO: Move KillProcessTree() and RunCommand() into their own module, since
# they're used by other tools.


def KillProcessTree(pid):
  """Kills the process and all of its child processes.

  Args:
    pid: process to kill.

  Raises:
    OSError: Unsupported OS.
  """

  if sys.platform in ('win32', 'cygwin'):
    # Use Windows' taskkill utility
    killproc_path = '%s;%s\\system32;%s\\system32\\wbem' % (
        (os.environ['SYSTEMROOT'],) * 3)
    killproc_cmd = 'taskkill /F /T /PID %d' % pid
    killproc_task = subprocess.Popen(killproc_cmd, shell=True,
                                     stdout=subprocess.PIPE,
                                     env={'PATH':killproc_path})
    killproc_task.communicate()

  elif sys.platform in ('linux', 'linux2', 'darwin'):
    # Use ps to get a list of processes
    ps_task = subprocess.Popen(['/bin/ps', 'x', '-o', 'pid,ppid'], stdout=subprocess.PIPE)
    ps_out = ps_task.communicate()[0]

    # Parse out a dict of pid->ppid
    ppid = {}
    for ps_line in ps_out.split('\n'):
      w = ps_line.strip().split()
      if len(w) < 2:
          continue      # Not enough words in this line to be a process list
      try:
        ppid[int(w[0])] = int(w[1])
      except ValueError:
        pass            # Header or footer

    # For each process, kill it if it or any of its parents is our child
    for p in ppid:
      p2 = p
      while p2:
        if p2 == pid:
          os.kill(p, signal.SIGKILL)
          break
        p2 = ppid.get(p2)

  else:
    raise OSError('Unsupported OS for KillProcessTree()')


def RunCommand(cmdargs, cwdir=None, env=None, echo_output=True, timeout=None,
               timeout_errorlevel=14):
  """Runs an external command.

  Args:
    cmdargs: A command string, or a tuple containing the command and its
        arguments.
    cwdir: Working directory for the command, if not None.
    env: Environment variables dict, if not None.
    echo_output: If True, output will be echoed to stdout.
    timeout: If not None, timeout for command in seconds.  If command times
        out, it will be killed and timeout_errorlevel will be returned.
    timeout_errorlevel: The value to return if the command times out.

  Returns:
    The integer errorlevel from the command.
    The combined stdout and stderr as a string.
  """
  # Force unicode string in the environment to strings.
  if env:
    env = dict([(k, str(v)) for k, v in env.items()])
  start_time = time.time()
  child = subprocess.Popen(cmdargs, cwd=cwdir, env=env, shell=True,
                           universal_newlines=True,
                           stdin=subprocess.PIPE,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
  child_out = []
  child_retcode = None

  def _ReadThread():
    """Thread worker function to read output from child process.

    Necessary since there is no cross-platform way of doing non-blocking
    reads of the output pipe.
    """
    read_run = True
    while read_run:
      time.sleep(.1)       # So we don't poll too frequently
      # Need to have a delay of 1 cycle between child completing and
      # thread exit, to pick up the final output from the child.
      if child_retcode is not None:
        read_run = False
      new_out = child.stdout.read()
      if new_out:
        if echo_output:
          print new_out,
        child_out.append(new_out)

  read_thread = threading.Thread(target=_ReadThread)
  read_thread.setDaemon(True)
  read_thread.start()

  # Wait for child to exit or timeout
  while child_retcode is None:
    time.sleep(.1)       # So we don't poll too frequently
    child_retcode = child.poll()
    if timeout and child_retcode is None:
      elapsed = time.time() - start_time
      if elapsed > timeout:
        print '*** RunCommand() timeout:', cmdargs
        KillProcessTree(child.pid)
        child_retcode = timeout_errorlevel

  # Wait a bit for worker thread to pick up final output and die.  No need to
  # worry if it's still alive at the end of this, since it's a daemon thread
  # and won't block python from exiting.  (And since it's blocked, it doesn't
  # chew up CPU.)
  read_thread.join(5)

  if echo_output:
    print   # end last line of output
  return child_retcode, ''.join(child_out)


def CommandOutputBuilder(target, source, env):
  """Command output builder.

  Args:
    self: Environment in which to build
    target: List of target nodes
    source: List of source nodes

  Returns:
    None or 0 if successful; nonzero to indicate failure.

  Runs the command specified in the COMMAND_OUTPUT_CMDLINE environment variable
  and stores its output in the first target file.  Additional target files
  should be specified if the command creates additional output files.

  Runs the command in the COMMAND_OUTPUT_RUN_DIR subdirectory.
  """
  env = env.Clone()

  cmdline = env.subst('$COMMAND_OUTPUT_CMDLINE', target=target, source=source)
  cwdir = env.subst('$COMMAND_OUTPUT_RUN_DIR', target=target, source=source)
  if cwdir:
    cwdir = os.path.normpath(cwdir)
    env.AppendENVPath('PATH', cwdir)
    env.AppendENVPath('LD_LIBRARY_PATH', cwdir)
  else:
    cwdir = None
  cmdecho = env.get('COMMAND_OUTPUT_ECHO', True)
  timeout = env.get('COMMAND_OUTPUT_TIMEOUT')
  timeout_errorlevel = env.get('COMMAND_OUTPUT_TIMEOUT_ERRORLEVEL')

  retcode, output = RunCommand(cmdline, cwdir=cwdir, env=env['ENV'],
                               echo_output=cmdecho, timeout=timeout,
                               timeout_errorlevel=timeout_errorlevel)

  # Save command line output
  output_file = open(str(target[0]), 'w')
  output_file.write(output)
  output_file.close()

  return retcode


def generate(env):
  # NOTE: SCons requires the use of this name, which fails gpylint.
  """SCons entry point for this tool."""

  # Add the builder and tell it which build environment variables we use.
  action = SCons.Script.Action(
      CommandOutputBuilder,
      'Output "$COMMAND_OUTPUT_CMDLINE" to $TARGET',
      varlist=[
          'COMMAND_OUTPUT_CMDLINE',
          'COMMAND_OUTPUT_RUN_DIR',
          'COMMAND_OUTPUT_TIMEOUT',
          'COMMAND_OUTPUT_TIMEOUT_ERRORLEVEL',
          # We use COMMAND_OUTPUT_ECHO also, but that doesn't change the
          # command being run or its output.
      ], )
  builder = SCons.Script.Builder(action = action)
  env.Append(BUILDERS={'CommandOutput': builder})

  # Default command line is to run the first input
  env['COMMAND_OUTPUT_CMDLINE'] = '$SOURCE'

  # TODO: Add a pseudo-builder which takes an additional command line as an
  # argument.
