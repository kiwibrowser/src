# -*- python -*-
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import optparse
import os
import re
import socket
import subprocess
import sys
import unittest


if sys.platform == 'win32':
  RETURNCODE_KILL = -9
else:
  RETURNCODE_KILL = -9 & 0xff


def AssertEquals(x, y):
  if x != y:
    raise AssertionError('%r != %r' % (x, y))


def ParseNumber(number):
  if number.startswith('0x'):
    return int(number[2:], 16)
  return int(number)


def FilenameToUnix(str):
  return str.replace('\\', '/')


def MakeOutFileName(output_dir, name, ext):
  # File name should be consistent with .out file name from nacl.scons
  return os.path.join(output_dir, 'gdb_' + name + ext)


def KillProcess(process):
  try:
    process.kill()
  except OSError:
    # If process is already terminated, kill() throws
    # "WindowsError: [Error 5] Access is denied" on Windows.
    pass
  process.wait()


SEL_LDR_RSP_SOCKET_ADDR = ('localhost', 4014)


def EnsurePortIsAvailable(addr=SEL_LDR_RSP_SOCKET_ADDR):
  # As a sanity check, check that the TCP port is available by binding
  # to it ourselves (and then unbinding).  Otherwise, we could end up
  # talking to an old instance of sel_ldr that is still hanging
  # around, or to some unrelated service that uses the same port
  # number.  Of course, there is still a race condition because an
  # unrelated process could bind the port after we unbind.
  sock = socket.socket()
  sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, True)
  sock.bind(addr)
  sock.close()


def DecodeNexeArgsForSubprocess(arg_list):
  if arg_list is None:
    return []
  return arg_list.split(',')


def LaunchSelLdr(sel_ldr_command, options, name):
  args = sel_ldr_command + ['-g', '-a']
  if options.irt is not None:
    args += ['-B', options.irt]
  if options.ld_so is not None:
    args += ['--', options.ld_so,
             '--library-path', options.library_path]
  args += ([FilenameToUnix(options.nexe)] +
           DecodeNexeArgsForSubprocess(options.nexe_args) +
           [name])
  EnsurePortIsAvailable()
  return subprocess.Popen(args)


def GenerateManifest(output_dir, nexe, runnable_ld, name):
  manifest_filename = MakeOutFileName(output_dir, name, '.nmf')
  manifest_dir = os.path.dirname(manifest_filename)
  runnable_ld_url = {'url': os.path.relpath(runnable_ld, manifest_dir)}
  nexe_url = {'url': os.path.relpath(nexe, manifest_dir)}
  manifest = {
      'program': {
          'x86-32': runnable_ld_url,
          'x86-64': runnable_ld_url,
      },
      'files': {
          'main.nexe': {
              'x86-32': nexe_url,
              'x86-64': nexe_url,
          },
      },
  }
  with open(manifest_filename, 'w') as manifest_file:
    json.dump(manifest, manifest_file)
  return manifest_filename


class RecordParser(object):

  STATUS_RE = re.compile('[^,]+')
  KEY_RE = re.compile('([^"{\[=]+)=')
  VALUE_PREFIX_RE = re.compile('"|{|\[')
  STRING_VALUE_RE = re.compile('([^"]*)"')

  def __init__(self, line):
    self.line = line
    self.pos = 0

  def Skip(self, c):
    if self.line.startswith(c, self.pos):
      self.pos += len(c)
      return True
    return False

  def Match(self, r):
    match = r.match(self.line, self.pos)
    if match is not None:
      self.pos = match.end()
    return match

  def ParseString(self):
    string_value_match = self.Match(self.STRING_VALUE_RE)
    assert string_value_match is not None
    return string_value_match.group(1)

  def ParseValue(self):
    value_prefix_match = self.Match(self.VALUE_PREFIX_RE)
    assert value_prefix_match is not None
    if value_prefix_match.group(0) == '"':
      return self.ParseString()
    elif value_prefix_match.group(0) == '{':
      return self.ParseDict()
    else:
      return self.ParseList()

  def ParseListMembers(self):
    result = []
    while True:
      # List syntax:
      #   [foo, bar]
      #   [foo=x, bar=y] - we parse this as [{foo=x}, {bar=y}]
      key_match = self.Match(self.KEY_RE)
      value = self.ParseValue()
      if key_match is not None:
        result.append({key_match.group(1): value})
      else:
        result.append(value)
      if not self.Skip(','):
        break
    return result

  def ParseList(self):
    if self.Skip(']'):
      return []
    result = self.ParseListMembers()
    assert self.Skip(']')
    return result

  def ParseDictMembers(self):
    result = {}
    while True:
      key_match = self.Match(self.KEY_RE)
      assert key_match is not None
      result[key_match.group(1)] = self.ParseValue()
      if not self.Skip(','):
        break
    return result

  def ParseDict(self):
    if self.Skip('}'):
      return {}
    result = self.ParseDictMembers()
    assert self.Skip('}')
    return result

  def Parse(self):
    status_match = self.Match(self.STATUS_RE)
    assert status_match is not None
    result = {}
    if self.Skip(','):
      result = self.ParseDictMembers()
    AssertEquals(self.pos, len(self.line))
    return (status_match.group(0), result)


class Gdb(object):

  def __init__(self, options, name):
    self._options = options
    self._name = name
    args = [options.gdb, '--interpreter=mi']
    self._log = sys.stderr
    self._gdb = subprocess.Popen(args,
                                 stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE)
    self._expected_success = True

  def Wait(self):
    # Require a graceful exit from gdb.
    self._gdb.communicate()
    AssertEquals(self._gdb.returncode == 0, self._expected_success)

  def _SendRequest(self, request):
    self._log.write('To GDB: %s\n' % request)
    self._gdb.stdin.write(request)
    self._gdb.stdin.write('\n')
    return self._GetResponse()

  def _GetResponse(self):
    results = []
    while True:
      line = self._gdb.stdout.readline().rstrip()
      if line == '':
        return results
      self._log.write('From GDB: %s\n' % line)
      if line == '(gdb)':
        return results
      results.append(line)

  def _GetResultRecord(self, result):
    for line in result:
      if line.startswith('^'):
        return RecordParser(line).Parse()
    raise AssertionError('No result record found in %r' % result)

  def _GetLastExecAsyncRecord(self, result):
    for line in reversed(result):
      if line.startswith('*'):
        return RecordParser(line).Parse()
    raise AssertionError('No asynchronous execute status record found in %r'
                           % result)

  def Command(self, command):
    status, items = self._GetResultRecord(self._SendRequest(command))
    AssertEquals(status, '^done')
    return items

  def ExpectToFailCommand(self, command):
    status, items = self._GetResultRecord(self._SendRequest(command))
    AssertEquals(status, '^error')

  def ResumeCommand(self, command):
    status, items = self._GetResultRecord(self._SendRequest(command))
    AssertEquals(status, '^running')
    status, items = self._GetLastExecAsyncRecord(self._GetResponse())
    AssertEquals(status, '*stopped')
    return items

  def ResumeAndExpectStop(self, resume_command, expected_stop_reason):
    stop_info = self.ResumeCommand(resume_command)
    if 'reason' not in stop_info or stop_info['reason'] != expected_stop_reason:
      raise AssertionError(
          'GDB reported stop reason %r but we expected %r (full info is %r)'
          % (stop_info.get('reason'), expected_stop_reason, stop_info))

  def Quit(self):
    status, items = self._GetResultRecord(self._SendRequest('-gdb-exit'))
    AssertEquals(status, '^exit')

  def Disconnect(self):
    status, items = self._GetResultRecord(self._SendRequest('disconnect'))
    AssertEquals(status, '^done')

  def Detach(self):
    status, items = self._GetResultRecord(self._SendRequest('detach'))
    AssertEquals(status, '^done')

  def Kill(self):
    status, items = self._GetResultRecord(self._SendRequest('kill'))
    AssertEquals(status, '^done')

  def KillProcess(self):
    self._expected_success = False
    KillProcess(self._gdb)

  def Eval(self, expression):
    return self.Command('-data-evaluate-expression ' + expression)['value']

  def GetPC(self):
    return ParseNumber(self.Eval('$pc')) & ((1 << 32) - 1)

  def LoadManifestFile(self):
    assert self._manifest_file is not None
    # gdb uses bash-like escaping which removes slashes from Windows paths.
    self.Command('nacl-manifest ' + FilenameToUnix(self._manifest_file))

  def Connect(self):
    self._GetResponse()
    self.Reconnect()

  def Reconnect(self):
    if self._options.irt is not None:
      self.Command('nacl-irt ' + FilenameToUnix(self._options.irt))
    if self._options.ld_so is not None:
      self._manifest_file = GenerateManifest(self._options.output_dir,
                                             self._options.nexe,
                                             self._options.ld_so,
                                             self._name)
      self.LoadManifestFile()
      self.Command('set breakpoint pending on')
    else:
      self.Command('file ' + FilenameToUnix(self._options.nexe))
    self.Command('target remote :4014')

  def FetchMainNexe(self):
    nexe_filename = MakeOutFileName(
        self._options.output_dir, 'remote', '.nexe')
    self.Command('remote get nexe ' + FilenameToUnix(nexe_filename))
    return nexe_filename

  def FetchIrtNexe(self):
    nexe_filename = MakeOutFileName(
        self._options.output_dir, 'remote', '.nexe')
    self.Command('remote get irt ' + FilenameToUnix(nexe_filename))
    return nexe_filename

  def GetMainNexe(self):
    if self._options.ld_so is not None:
      return self._options.ld_so
    return self._options.nexe

  def GetIrtNexe(self):
    return self._options.irt


def DecodeOptions():
  parser = optparse.OptionParser()
  parser.add_option('--output_dir', help='Output directory for log files')
  parser.add_option('--gdb', help='Filename of GDB')
  parser.add_option('--irt', help='Filename of irt.nexe (optional)')
  parser.add_option('--ld_so', help='Filename of dynamic linker (optional)')
  parser.add_option('--library_path',
                    help='Directory containing dynamic libraries, '
                    'if using dynamic linking (optional)')
  parser.add_option('--nexe', help='Filename of main NaCl executable')
  parser.add_option('--nexe_args', help='Comma-separated list of arguments')
  return parser.parse_args()


def Main():
  global g_options
  global g_sel_ldr_command
  g_options, g_sel_ldr_command = DecodeOptions()
  sys.argv = [sys.argv[0]]
  unittest.main()


class GdbTest(unittest.TestCase):
  """Base class for tests of gdb, assumes a single sel_ldr + gdb."""

  def GetTestName(self):
    parts = self.id().split('.')
    return parts[-1][len('test_'):]

  def AssertSelLdrExits(self, expected_returncode=RETURNCODE_KILL):
    self.sel_ldr.wait()
    self.assertEqual(expected_returncode, self.sel_ldr.returncode)

  def LaunchSelLdr(self):
    self.sel_ldr = LaunchSelLdr(
        g_sel_ldr_command, g_options, self.GetTestName())

  def LaunchGdb(self):
    try:
      self.gdb = Gdb(g_options, self.GetTestName())
      self.gdb.Connect()
    except:
      KillProcess(self.sel_ldr)
      raise

  def setUp(self):
    self.LaunchSelLdr()
    self.LaunchGdb()

  def tearDown(self):
    try:
      if self.gdb:
        self.gdb.Quit()
        self.gdb.Wait()
      self.AssertSelLdrExits()
    finally:
      if self.gdb:
        self.gdb.KillProcess()
      KillProcess(self.sel_ldr)
