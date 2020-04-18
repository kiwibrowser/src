#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import platform
import os
import random
import re
import shlex
import signal
import subprocess
import sys
import tempfile

import elftools
# filetype needs to be imported here because pnacl-driver injects calls to
# filetype.ForceFileType into argument parse actions.
# TODO(dschuff): That's ugly. Find a better way.
import filetype
import pathtools

from driver_env import env
# TODO: import driver_log and change these references from 'foo' to
# 'driver_log.foo', or split driver_log further
from driver_log import Log, DriverOpen, DriverClose, StringifyCommand, DriverExit, FixArch
from driver_temps import TempFiles
from shelltools import shell

def ParseError(s, leftpos, rightpos, msg):
  Log.Error("Parse Error: %s", msg)
  Log.Error('  ' + s)
  Log.Error('  ' + (' '*leftpos) + ('^'*(rightpos - leftpos + 1)))
  DriverExit(1)


# Run a command with extra environment settings
def RunWithEnv(cmd, **kwargs):
  env.push()
  env.setmany(**kwargs)
  ret = Run(cmd)
  env.pop()
  return ret


def SetExecutableMode(path):
  if os.name == "posix":
    realpath = pathtools.tosys(path)
    # os.umask gets and sets at the same time.
    # There's no way to get it without setting it.
    umask = os.umask(0)
    os.umask(umask)
    os.chmod(realpath, 0755 & ~umask)


def FilterOutArchArgs(args):
  while '-arch' in args:
    i = args.index('-arch')
    args = args[:i] + args[i+2:]
  return args

# Parse and validate the target triple and return the architecture.
# We don't attempt to recognize all possible targets here, just the ones we
# support.
def ParseTriple(triple):
  tokens = triple.split('-')
  arch = tokens[0]
  if arch != 'le32':
    arch = FixArch(arch)
  os = tokens[1]
  # The machine/vendor field could be present or not.
  if os != 'nacl' and len(tokens) >= 3:
    os = tokens[2]
  # Just check that the os is nacl.
  if os == 'nacl':
    return arch

  Log.Fatal('machine/os ' + '-'.join(tokens[1:]) + ' not supported.')


def GetOSName():
  if sys.platform == 'darwin':
    os_name = 'mac'
  elif sys.platform == 'nacl':
    os_name = 'nacl'
  elif sys.platform.startswith('linux'):
    os_name = 'linux'
  elif sys.platform in ('cygwin', 'win32'):
    os_name = 'win'
  else:
    Log.Fatal('Machine: %s not supported.' % sys.platform)

  return os_name

def GetArchNameShort():
  machine = platform.machine().lower()
  if machine.startswith('arm'):
    return 'arm'
  elif machine.startswith('mips'):
    return 'mips'
  elif (machine.startswith('x86')
        or machine in ('amd32', 'i386', 'i686', 'ia32', '32', 'amd64', '64')):
    return 'x86'

  Log.Fatal('Architecture: %s not supported.' % machine)
  return 'unknown'

def RunDriver(module_name, args, suppress_inherited_arch_args=False):
  """
  RunDriver() is used to invoke "driver" tools, e.g.
  those prefixed  with "pnacl-"

  It automatically appends some additional flags to the invocation
  which were inherited from the current invocation.
  Those flags were preserved by ParseArgs
  """

  if isinstance(args, str):
    args = shell.split(env.eval(args))

  script = env.eval('${DRIVER_BIN}/%s' % module_name)
  script = shell.unescape(script)

  inherited_driver_args = env.get('INHERITED_DRIVER_ARGS')
  if suppress_inherited_arch_args:
    inherited_driver_args = FilterOutArchArgs(inherited_driver_args)

  script = pathtools.tosys(script)
  cmd = [script] + args + inherited_driver_args
  Log.Info('Driver invocation: %s', repr(cmd))

  module = __import__(module_name)
  # Save the environment, reset the environment, run
  # the driver module, and then restore the environment.
  env.push()
  env.reset()
  DriverMain(module, cmd)
  env.pop()

def memoize(f):
  """ Memoize a function with no arguments """
  saved = {}
  def newf():
    if len(saved) == 0:
      saved[None] = f()
    return saved[None]
  newf.__name__ = f.__name__
  return newf


@env.register
@memoize
def GetBuildOS():
  name = platform.system().lower()
  if name.startswith('cygwin_nt') or 'windows' in name:
    name = 'windows'
  if name not in ('linux', 'nacl', 'darwin', 'windows'):
    Log.Fatal("Unsupported platform '%s'", name)
  return name

@env.register
@memoize
def GetBuildArch():
  m = platform.machine()

  # Windows is special
  if m == 'x86':
    m = 'i686'

  if m not in ('i386', 'i686', 'x86_64'):
    Log.Fatal("Unsupported architecture '%s'", m)
  return m

# Crawl backwards, starting from the directory containing this script,
# until we find a directory satisfying a filter function.
def FindBaseDir(function):
  Depth = 0
  cur = env.getone('DRIVER_BIN')
  while not function(cur) and Depth < 16:
    cur = pathtools.dirname(cur)
    Depth += 1
  if function(cur):
    return cur
  return None

@env.register
@memoize
def FindBaseNaCl():
  """ Find native_client/ directory """
  dir = FindBaseDir(lambda cur: pathtools.basename(cur) == 'native_client')
  if dir is None:
    Log.Fatal("Unable to find 'native_client' directory")
  return shell.escape(dir)

@env.register
@memoize
def FindBaseToolchain():
  """ Find toolchain/OS_ARCH directory """
  base_dir = FindBaseDir(lambda cur: pathtools.basename(cur) == 'toolchain')
  if base_dir is None:
    Log.Fatal("Unable to find 'toolchain' directory")
  toolchain_dir = os.path.join(
      base_dir,
      '%s_%s' % (GetOSName(), GetArchNameShort())
  )
  return shell.escape(toolchain_dir)

@env.register
@memoize
def FindBasePNaCl():
  """ Find the base directory of the PNaCl toolchain """
  # The <base> directory is one level up from the <base>/bin:
  bindir = env.getone('DRIVER_BIN')
  basedir = pathtools.dirname(bindir)
  return shell.escape(basedir)

def AddHostBinarySearchPath(prefix):
  """ Add a path to the list searched for host binaries. """
  prefix = pathtools.normalize(prefix)
  if pathtools.isdir(prefix) and not prefix.endswith('/'):
    prefix += '/'

  env.append('BPREFIXES', prefix)

@env.register
def FindBaseHost(tool):
  """ Find the base directory for host binaries (i.e. llvm/binutils) """
  if env.has('BPREFIXES'):
    for prefix in env.get('BPREFIXES'):
      if os.path.exists(pathtools.join(prefix, 'bin',
                                       tool + env.getone('EXEC_EXT'))):
        return prefix

  base_pnacl = FindBasePNaCl()
  if not pathtools.exists(pathtools.join(base_pnacl, 'bin',
                          tool + env.getone('EXEC_EXT'))):
    Log.Fatal('Could not find PNaCl host directory for ' + tool)
  return base_pnacl

def ReadConfig():
  # Mock out ReadConfig if running unittests.  Settings are applied directly
  # by DriverTestEnv rather than reading this configuration file.
  if env.has('PNACL_RUNNING_UNITTESTS'):
    return
  driver_bin = env.getone('DRIVER_BIN')
  driver_conf = pathtools.join(driver_bin, 'driver.conf')
  fp = DriverOpen(driver_conf, 'r')
  linecount = 0
  for line in fp:
    linecount += 1
    line = line.strip()
    if line == '' or line.startswith('#'):
      continue
    sep = line.find('=')
    if sep < 0:
      Log.Fatal("%s: Parse error, missing '=' on line %d",
                pathtools.touser(driver_conf), linecount)
    keyname = line[:sep].strip()
    value = line[sep+1:].strip()
    env.setraw(keyname, value)
  DriverClose(fp)

@env.register
def AddPrefix(prefix, varname):
  values = env.get(varname)
  return ' '.join([prefix + shell.escape(v) for v in values ])

######################################################################
#
# Argument Parser
#
######################################################################

DriverArgPatterns = [
  ( '--pnacl-driver-verbose',          "env.set('LOG_VERBOSE', '1')"),
  ( ('-arch', '(.+)'),                 "SetArch($0)"),
  ( '--pnacl-sb',                      "env.set('SANDBOXED', '1')"),
  ( '--pnacl-use-emulator',            "env.set('USE_EMULATOR', '1')"),
  ( '--dry-run',                       "env.set('DRY_RUN', '1')"),
  ( '--pnacl-arm-bias',                "env.set('BIAS', 'ARM')"),
  ( '--pnacl-mips-bias',               "env.set('BIAS', 'MIPS32')"),
  ( '--pnacl-i686-bias',               "env.set('BIAS', 'X8632')"),
  ( '--pnacl-x86_64-bias',             "env.set('BIAS', 'X8664')"),
  ( '--pnacl-bias=(.+)',               "env.set('BIAS', FixArch($0))"),
  ( '-save-temps',                     "env.set('SAVE_TEMPS', '1')"),
  ( '-no-save-temps',                  "env.set('SAVE_TEMPS', '0')"),
  ( ('-B', '(.*)'),  AddHostBinarySearchPath),
 ]

DriverArgPatternsNotInherited = [
  ( '--pnacl-driver-set-([^=]+)=(.*)',    "env.set($0, $1)"),
  ( '--pnacl-driver-append-([^=]+)=(.*)', "env.append($0, $1)"),
]


def ShouldExpandCommandFile(arg):
  """ We may be given files with commandline arguments.
  Read in the arguments so that they can be handled as usual. """
  if arg.startswith('@'):
    possible_file = pathtools.normalize(arg[1:])
    return pathtools.isfile(possible_file)
  else:
    return False


def DoExpandCommandFile(argv, i):
  arg = argv[i]
  fd = DriverOpen(pathtools.normalize(arg[1:]), 'r')
  more_args = []

  # Use shlex here to process the response file contents.
  # This ensures that single and double quoted args are
  # handled correctly.  Since this file is very likely
  # to contain paths with windows path seperators we can't
  # use the normal shlex.parse() since we need to disable
  # disable '\' (the default escape char).
  for line in fd:
    lex = shlex.shlex(line, posix=True)
    lex.escape = ''
    lex.whitespace_split = True
    more_args += list(lex)

  fd.close()
  argv = argv[:i] + more_args + argv[i+1:]
  return argv


def ParseArgs(argv,
              patternlist,
              driver_patternlist=DriverArgPatterns,
              driver_patternlist_not_inherited=DriverArgPatternsNotInherited):
  """Parse argv using the patterns in patternlist
     Also apply the built-in DriverArgPatterns unless instructed otherwise.
     This function must be called by all (real) drivers.
  """
  if driver_patternlist:
    driver_args, argv = ParseArgsBase(argv, driver_patternlist)

    # TODO(robertm): think about a less obscure mechanism to
    #                replace the inherited args feature
    assert not env.get('INHERITED_DRIVER_ARGS')
    env.append('INHERITED_DRIVER_ARGS', *driver_args)

  _, argv = ParseArgsBase(argv, driver_patternlist_not_inherited)

  _, unmatched = ParseArgsBase(argv, patternlist)
  if unmatched:
    for u in unmatched:
      Log.Error('Unrecognized argument: ' + u)
    Log.Fatal('unknown arguments')


def ParseArgsBase(argv, patternlist):
  """ Parse argv using the patterns in patternlist
      Returns: (matched, unmatched)
  """
  matched = []
  unmatched = []
  i = 0
  while i < len(argv):
    if ShouldExpandCommandFile(argv[i]):
      argv = DoExpandCommandFile(argv, i)
      if i >= len(argv):
        break
    num_matched, action, groups = MatchOne(argv, i, patternlist)
    if num_matched == 0:
      unmatched.append(argv[i])
      i += 1
      continue
    matched += argv[i:i+num_matched]
    if isinstance(action, str):
      # Perform $N substitution
      for g in xrange(0, len(groups)):
        action = action.replace('$%d' % g, 'groups[%d]' % g)
    try:
      if isinstance(action, str):
        # NOTE: this is essentially an eval for python expressions
        # which does rely on the current environment for unbound vars
        # Log.Info('about to exec [%s]', str(action))
        exec(action)
      else:
        action(*groups)
    except Exception, err:
      Log.Fatal('ParseArgs action [%s] failed with: %s', action, err)
    i += num_matched
  return (matched, unmatched)


def MatchOne(argv, i, patternlist):
  """Find a pattern which matches argv starting at position i"""
  for (regex, action) in patternlist:
    if isinstance(regex, str):
      regex = [regex]
    j = 0
    matches = []
    for r in regex:
      if i+j < len(argv):
        match = re.compile('^'+r+'$').match(argv[i+j])
      else:
        match = None
      matches.append(match)
      j += 1
    if None in matches:
      continue
    groups = [ list(m.groups()) for m in matches ]
    groups = reduce(lambda x,y: x+y, groups, [])
    return (len(regex), action, groups)
  return (0, '', [])

def UnrecognizedOption(*args):
  Log.Fatal("Unrecognized option: " + ' '.join(args) + "\n" +
            "Use '--help' for more information.")


######################################################################
#
# File Naming System (Temp files & Output files)
#
######################################################################

def DefaultOutputName(filename, outtype):
  # For pre-processor mode, just print to stdout.
  if outtype in ('pp'): return '-'

  base = pathtools.basename(filename)
  base = RemoveExtension(base)
  if outtype in ('po'): return base + '.o'

  assert(outtype in filetype.ExtensionMap.values())
  assert(not filetype.IsSourceType(outtype))

  return base + '.' + outtype

def DefaultPCHOutputName(filename):
  # Clang currently uses the GCC '.gch' by default for precompiled headers,
  # though their documentation example uses '-o foo.h.pch' as the example.
  return filename + '.gch'

def RemoveExtension(filename):
  if filename.endswith('.opt.bc'):
    return filename[0:-len('.opt.bc')]

  name, ext = pathtools.splitext(filename)
  return name

def PathSplit(f):
  paths = []
  cur = f
  while True:
    cur, piece = pathtools.split(cur)
    if piece == '':
      break
    paths.append(piece)
  paths.reverse()
  return paths


def CheckPathLength(filename, exit_on_failure=True):
  '''Check that the length of the path is short enough for Windows.

  On Windows, MAX_PATH is ~260 and applies to absolute paths, and to relative
  paths and the absolute paths they expand to (except for specific uses of
  some APIs; see link below). Most applications don't bother to support long
  paths properly (including LLVM, GNU binutils, and ninja). If a path is too
  long, ERROR_PATH_NOT_FOUND is returned, which isn't very useful or clear for
  users. In addition the Chrome build has deep directory hierarchies with long
  names.
  This function checks that the path is valid, so we can throw meaningful
  errors.

  http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
  '''
  if not IsWindowsPython() and not env.has('PNACL_RUNNING_UNITTESTS'):
    return True

  # First check the name as-is (it's usually a relative path)
  if len(filename) > 255:
    if exit_on_failure:
      Log.Fatal('Path name %s is too long (%d characters)' %
                (filename, len(filename)))
    return False
  if os.path.isabs(filename):
    return True

  # Don't assume that the underlying tools or windows APIs will normalize
  # the path before using it. Conservatively count the length of CWD + filename
  appended_name = os.path.join(os.getcwd(), filename)
  if len(appended_name) > 255:
    if exit_on_failure:
      Log.Fatal('Path name %s (expanded from %s) is too long (%d characters)' %
                (appended_name, filename, len(appended_name)))
    return False
  return True

# Generate a unique identifier for each input file.
# Start with the basename, and if that is not unique enough,
# add parent directories. Rinse, repeat.
class TempNameGen(object):
  def __init__(self, inputs, output):
    inputs = [ pathtools.abspath(i) for i in inputs ]
    output = pathtools.abspath(output)

    self.TempBase = output + '---linked'
    self.OutputDir = pathtools.dirname(output)

    # TODO(pdox): Figure out if there's a less confusing way
    #             to simplify the intermediate filename in this case.
    #if len(inputs) == 1:
    #  # There's only one input file, don't bother adding the source name.
    #  TempMap[inputs[0]] = output + '---'
    #  return

    # Build the initial mapping
    self.TempMap = dict()
    for f in inputs:
      if f.startswith('-'):
        continue
      path = PathSplit(f)
      self.TempMap[f] = [1, path]

    while True:
      # Find conflicts
      ConflictMap = dict()
      Conflicts = set()
      for (f, [n, path]) in self.TempMap.iteritems():
        candidate = output + '---' + '_'.join(path[-n:]) + '---'
        if candidate in ConflictMap:
          Conflicts.add(ConflictMap[candidate])
          Conflicts.add(f)
        else:
          ConflictMap[candidate] = f

      if len(Conflicts) == 0:
        break

      # Resolve conflicts
      for f in Conflicts:
        n = self.TempMap[f][0]
        if n+1 > len(self.TempMap[f][1]):
          Log.Fatal('Unable to resolve naming conflicts')
        self.TempMap[f][0] = n+1

    # Clean up the map
    NewMap = dict()
    for (f, [n, path]) in self.TempMap.iteritems():
      candidate = output + '---' + '_'.join(path[-n:]) + '---'
      NewMap[f] = candidate
    self.TempMap = NewMap
    return

  def ValidatePathLength(self, temp, imtype):
    temp = pathtools.normpath(temp) if temp else temp
    # If the temp name is too long, just pick a random one instead.
    if not CheckPathLength(temp, exit_on_failure=False):
      # imtype is sometimes just an extension, and sometimes a compound
      # extension (e.g. pre_opt.pexe). To keep name length shorter,
      # only take the last extension
      if '.' in imtype:
        imtype = imtype[imtype.rfind('.') + 1:]
      temp = pathtools.join(
          self.OutputDir,
          str(random.randrange(100000, 1000000)) + '.' + imtype)
      CheckPathLength(temp)
    return temp

  def TempNameForOutput(self, imtype):
    temp = self.ValidatePathLength(self.TempBase + '.' + imtype, imtype)
    TempFiles.add(temp)
    return temp

  def TempNameForInput(self, input, imtype):
    fullpath = pathtools.abspath(input)
    # If input is already a temporary name, just change the extension
    if fullpath.startswith(self.TempBase):
      temp = self.TempBase + '.' + imtype
    else:
      # Source file
      temp = self.TempMap[fullpath] + '.' + imtype

    temp = self.ValidatePathLength(temp, imtype)
    TempFiles.add(temp)
    return temp

# (Invoked from loader.py)
# If the driver is waiting on a background process in RunWithLog()
# and the user Ctrl-C's or kill's the driver, it may leave
# the child process (such as llc) running. To prevent this,
# the code below sets up a signal handler which issues a kill to
# the currently running child processes.
CleanupProcesses = []
def SetupSignalHandlers():
  global CleanupProcesses
  def signal_handler(unused_signum, unused_frame):
    for p in CleanupProcesses:
      try:
        p.kill()
      except BaseException:
        pass
    os.kill(os.getpid(), signal.SIGKILL)
    return 0
  if os.name == 'posix' and sys.platform != 'nacl':
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGHUP, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)


def ArgsTooLongForWindows(args):
  """ Detect when a command line might be too long for Windows.  """
  if not IsWindowsPython():
    return False
  else:
    return len(' '.join(args)) > 8191


def ConvertArgsToFile(args):
  fd, outfile = tempfile.mkstemp()
  # Remember to delete this file afterwards.
  TempFiles.add(outfile)
  cmd = args[0]
  other_args = args[1:]
  os.write(fd, ' '.join(other_args))
  os.close(fd)
  return [cmd, '@' + outfile]

# Note:
# The redirect_stdout and redirect_stderr is only used a handful of times
def Run(args,
        errexit=True,
        redirect_stdout=None,
        redirect_stderr=None):
  """ Run: Run a command.
      Returns: return_code, stdout, stderr

      Run() is used to invoke "other" tools, e.g.
      those NOT prefixed with "pnacl-"

      stdout and stderr only contain meaningful data if
          redirect_{stdout,stderr} == subprocess.PIPE

      Run will terminate the program upon failure unless errexit == False
      TODO(robertm): errexit == True has not been tested and needs more work

      redirect_stdout and redirect_stderr are passed straight
      to subprocess.Popen
  """

  result_stdout = None
  result_stderr = None
  if isinstance(args, str):
    args = shell.split(env.eval(args))

  args = [pathtools.tosys(args[0])] + args[1:]

  Log.Info('Running: ' + StringifyCommand(args))

  if env.getbool('DRY_RUN'):
    if redirect_stderr or redirect_stdout:
      # TODO(pdox): Prevent this from happening, so that
      # dry-run is more useful.
      Log.Fatal("Unhandled dry-run case.")
    return 0, None, None

  try:
    # If we have too long of a cmdline on windows, running it would fail.
    # Attempt to use a file with the command line options instead in that case.
    if ArgsTooLongForWindows(args):
      actual_args = ConvertArgsToFile(args)
      Log.Info('Wrote long commandline to file for Windows: ' +
               StringifyCommand(actual_args))

    else:
      actual_args = args

    p = subprocess.Popen(actual_args,
                         stdout=redirect_stdout,
                         stderr=redirect_stderr)
    result_stdout, result_stderr = p.communicate()
  except Exception, e:
    msg =  '%s\nCommand was: %s' % (str(e), StringifyCommand(args))
    print msg
    DriverExit(1)

  Log.Info('Return Code: ' + str(p.returncode))

  if errexit and p.returncode != 0:
    if redirect_stdout == subprocess.PIPE:
      Log.Error('--------------stdout: begin')
      Log.Error(result_stdout)
      Log.Error('--------------stdout: end')

    if redirect_stderr == subprocess.PIPE:
      Log.Error('--------------stderr: begin')
      Log.Error(result_stderr)
      Log.Error('--------------stderr: end')
    DriverExit(p.returncode)

  return p.returncode, result_stdout, result_stderr


def IsWindowsPython():
  return 'windows' in platform.system().lower()

def SetupCygwinLibs():
  bindir = env.getone('DRIVER_BIN')
  # Prepend the directory containing cygwin1.dll etc. to the PATH to ensure we
  # get the right one.
  os.environ['PATH'] = os.pathsep.join(
      [pathtools.tosys(bindir)] + os.environ['PATH'].split(os.pathsep))


def HelpNotAvailable():
  return 'Help text not available'

def DriverMain(module, argv):
  # TODO(robertm): this is ugly - try to get rid of this
  if '--pnacl-driver-verbose' in argv:
    Log.IncreaseVerbosity()
    env.set('LOG_VERBOSE', '1')

  # driver_path has the form: /foo/bar/pnacl_root/newlib/bin/pnacl-clang
  driver_path = pathtools.abspath(pathtools.normalize(argv[0]))
  driver_bin = pathtools.dirname(driver_path)
  script_name = pathtools.basename(driver_path)
  env.set('SCRIPT_NAME', script_name)
  env.set('DRIVER_PATH', driver_path)
  env.set('DRIVER_BIN', driver_bin)

  Log.SetScriptName(script_name)

  ReadConfig()

  if IsWindowsPython():
    SetupCygwinLibs()

  # skip tool name
  argv = argv[1:]

  # Handle help info
  if ('--help' in argv or
      '-h' in argv or
      '-help' in argv or
      '--help-full' in argv):
    help_func = getattr(module, 'get_help', None)
    if not help_func:
      Log.Fatal(HelpNotAvailable())
    helpstr = help_func(argv)
    print helpstr
    return 0

  return module.main(argv)


def MaybeStripNonSFISuffix(s):
  """Removes _NONSFI suffix if possible, otherwise |s| as is."""
  return s[:-len('_NONSFI')] if s.endswith('_NONSFI') else s


def SetArch(arch):
  arch = FixArch(arch)
  env.set('ARCH', arch)
  base_arch = MaybeStripNonSFISuffix(arch)
  env.set('BASE_ARCH', base_arch)
  env.setbool('NONSFI_NACL', arch != base_arch)


def GetArch(required = False):
  arch = env.getone('ARCH')
  if arch == '':
    arch = None

  if required and not arch:
    Log.Fatal('Missing -arch!')

  return arch

# Read an ELF file or an archive file to determine the machine type. If ARCH is
# already set, make sure the file has the same architecture. If ARCH is not
# set, set the ARCH to the file's architecture.
# Note that the SFI and NONSFI shares the same file format, so they will be
# treated as same.
#
# Returns True if the file matches ARCH.
#
# Returns False if the file doesn't match ARCH. This only happens when
# must_match is False. If must_match is True, then a fatal error is generated
# instead.
def ArchMerge(filename, must_match):
  file_type = filetype.FileType(filename)
  if file_type in ('o','so'):
    elfheader = elftools.GetELFHeader(filename)
    if not elfheader:
      Log.Fatal("%s: Cannot read ELF header", filename)
    new_arch = elfheader.arch
  elif filetype.IsNativeArchive(filename):
    new_arch = file_type[len('archive-'):]
  else:
    Log.Fatal('%s: Unexpected file type in ArchMerge', filename)

  existing_arch = GetArch()
  if not existing_arch:
    SetArch(new_arch)
    return True

  # The _NONSFI binary format is as same as the SFI's.
  existing_arch = MaybeStripNonSFISuffix(existing_arch)

  if new_arch != existing_arch:
    if must_match:
      msg = "%s: Incompatible object file (%s != %s)"
      logfunc = Log.Fatal
    else:
      msg = "%s: Skipping incompatible object file (%s != %s)"
      logfunc = Log.Warning
    logfunc(msg, filename, new_arch, existing_arch)
    return False

  # existing_arch and new_arch == existing_arch
  return True

def CheckTranslatorPrerequisites():
  """ Assert that the scons artifacts for running the sandboxed translator
      exist: sel_ldr, and the IRT blob. """
  if env.getbool('DRY_RUN'):
    return
  reqs = ['SEL_LDR', 'IRT_BLOB']
  # Linux also requires the nacl bootstrap helper.
  if GetBuildOS() == 'linux':
    reqs.append('BOOTSTRAP_LDR')
  for var in reqs:
    needed_file = env.getone(var)
    if not pathtools.exists(needed_file):
      Log.Fatal('Could not find %s [%s]', var, needed_file)

def SelLdrCommand():
  if GetBuildOS() == 'linux':
    cmd = '${BOOTSTRAP_LDR} ${SEL_LDR} --reserved_at_zero=0x%s' % ('X' * 16)
  else:
    cmd = '${SEL_LDR}'
  return '${SEL_LDR_PREFIX} %s ${SEL_LDR_FLAGS}' % cmd

def AddListToEnv(command, env_var_prefix, string_list):
  for index, string in enumerate(string_list):
    command.append('-E')
    command.append('%s_%d=%s' % (env_var_prefix, index, string))

class DriverChain(object):
  """ The DriverChain class takes one or more input files,
      an output file, and a sequence of steps. It executes
      those steps, using intermediate files in between,
      to generate the final outpu.
  """

  def __init__(self, input, output, namegen):
    self.input = input
    self.output = pathtools.normpath(output) if output else output
    self.steps = []
    self.namegen = namegen

    # "input" can be a list of files or a single file.
    # If we're compiling for a single file, then we use
    # TempNameForInput. If there are multiple files
    # (e.g. linking), then we use TempNameForOutput.
    if isinstance(self.input, str):
      self.use_names_for_input = True
      self.input = pathtools.normpath(self.input) if self.input else self.input
      CheckPathLength(self.input)
    else:
      self.use_names_for_input = False
      self.input = [pathtools.normpath(p) if p else p for p in self.input]
      for path in self.input:
        CheckPathLength(path)
    CheckPathLength(output)

  def add(self, callback, output_type, **extra):
    step = (callback, output_type, extra)
    self.steps.append(step)

  def run(self):
    step_input = self.input
    for (i, (callback, output_type, extra)) in enumerate(self.steps):
      if i == len(self.steps)-1:
        # Last step
        step_output = self.output
      else:
        # Intermediate step
        if self.use_names_for_input:
          step_output = self.namegen.TempNameForInput(self.input, output_type)
        else:
          step_output = self.namegen.TempNameForOutput(output_type)
      callback(step_input, step_output, **extra)
      step_input = step_output
