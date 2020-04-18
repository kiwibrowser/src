# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generic utils."""

import codecs
import collections
import contextlib
import cStringIO
import datetime
import logging
import operator
import os
import pipes
import platform
import Queue
import re
import stat
import subprocess
import sys
import tempfile
import threading
import time
import urlparse

import subprocess2


RETRY_MAX = 3
RETRY_INITIAL_SLEEP = 0.5
START = datetime.datetime.now()


_WARNINGS = []


# These repos are known to cause OOM errors on 32-bit platforms, due the the
# very large objects they contain.  It is not safe to use threaded index-pack
# when cloning/fetching them.
THREADED_INDEX_PACK_BLACKLIST = [
  'https://chromium.googlesource.com/chromium/reference_builds/chrome_win.git'
]


class Error(Exception):
  """gclient exception class."""
  def __init__(self, msg, *args, **kwargs):
    index = getattr(threading.currentThread(), 'index', 0)
    if index:
      msg = '\n'.join('%d> %s' % (index, l) for l in msg.splitlines())
    super(Error, self).__init__(msg, *args, **kwargs)


def Elapsed(until=None):
  if until is None:
    until = datetime.datetime.now()
  return str(until - START).partition('.')[0]


def PrintWarnings():
  """Prints any accumulated warnings."""
  if _WARNINGS:
    print >> sys.stderr, '\n\nWarnings:'
    for warning in _WARNINGS:
      print >> sys.stderr, warning


def AddWarning(msg):
  """Adds the given warning message to the list of accumulated warnings."""
  _WARNINGS.append(msg)


def SplitUrlRevision(url):
  """Splits url and returns a two-tuple: url, rev"""
  if url.startswith('ssh:'):
    # Make sure ssh://user-name@example.com/~/test.git@stable works
    regex = r'(ssh://(?:[-.\w]+@)?[-\w:\.]+/[-~\w\./]+)(?:@(.+))?'
    components = re.search(regex, url).groups()
  else:
    components = url.rsplit('@', 1)
    if re.match(r'^\w+\@', url) and '@' not in components[0]:
      components = [url]

    if len(components) == 1:
      components += [None]
  return tuple(components)


def IsGitSha(revision):
  """Returns true if the given string is a valid hex-encoded sha"""
  return re.match('^[a-fA-F0-9]{6,40}$', revision) is not None


def IsFullGitSha(revision):
  """Returns true if the given string is a valid hex-encoded full sha"""
  return re.match('^[a-fA-F0-9]{40}$', revision) is not None


def IsDateRevision(revision):
  """Returns true if the given revision is of the form "{ ... }"."""
  return bool(revision and re.match(r'^\{.+\}$', str(revision)))


def MakeDateRevision(date):
  """Returns a revision representing the latest revision before the given
  date."""
  return "{" + date + "}"


def SyntaxErrorToError(filename, e):
  """Raises a gclient_utils.Error exception with the human readable message"""
  try:
    # Try to construct a human readable error message
    if filename:
      error_message = 'There is a syntax error in %s\n' % filename
    else:
      error_message = 'There is a syntax error\n'
    error_message += 'Line #%s, character %s: "%s"' % (
        e.lineno, e.offset, re.sub(r'[\r\n]*$', '', e.text))
  except:
    # Something went wrong, re-raise the original exception
    raise e
  else:
    raise Error(error_message)


class PrintableObject(object):
  def __str__(self):
    output = ''
    for i in dir(self):
      if i.startswith('__'):
        continue
      output += '%s = %s\n' % (i, str(getattr(self, i, '')))
    return output


def FileRead(filename, mode='rU'):
  with open(filename, mode=mode) as f:
    # codecs.open() has different behavior than open() on python 2.6 so use
    # open() and decode manually.
    s = f.read()
    try:
      return s.decode('utf-8')
    except UnicodeDecodeError:
      return s


def FileWrite(filename, content, mode='w'):
  with codecs.open(filename, mode=mode, encoding='utf-8') as f:
    f.write(content)


@contextlib.contextmanager
def temporary_directory(**kwargs):
  tdir = tempfile.mkdtemp(**kwargs)
  try:
    yield tdir
  finally:
    if tdir:
      rmtree(tdir)


def safe_rename(old, new):
  """Renames a file reliably.

  Sometimes os.rename does not work because a dying git process keeps a handle
  on it for a few seconds. An exception is then thrown, which make the program
  give up what it was doing and remove what was deleted.
  The only solution is to catch the exception and try again until it works.
  """
  # roughly 10s
  retries = 100
  for i in range(retries):
    try:
      os.rename(old, new)
      break
    except OSError:
      if i == (retries - 1):
        # Give up.
        raise
      # retry
      logging.debug("Renaming failed from %s to %s. Retrying ..." % (old, new))
      time.sleep(0.1)


def rm_file_or_tree(path):
  if os.path.isfile(path):
    os.remove(path)
  else:
    rmtree(path)


def rmtree(path):
  """shutil.rmtree() on steroids.

  Recursively removes a directory, even if it's marked read-only.

  shutil.rmtree() doesn't work on Windows if any of the files or directories
  are read-only. We need to be able to force the files to be writable (i.e.,
  deletable) as we traverse the tree.

  Even with all this, Windows still sometimes fails to delete a file, citing
  a permission error (maybe something to do with antivirus scans or disk
  indexing).  The best suggestion any of the user forums had was to wait a
  bit and try again, so we do that too.  It's hand-waving, but sometimes it
  works. :/

  On POSIX systems, things are a little bit simpler.  The modes of the files
  to be deleted doesn't matter, only the modes of the directories containing
  them are significant.  As the directory tree is traversed, each directory
  has its mode set appropriately before descending into it.  This should
  result in the entire tree being removed, with the possible exception of
  *path itself, because nothing attempts to change the mode of its parent.
  Doing so would be hazardous, as it's not a directory slated for removal.
  In the ordinary case, this is not a problem: for our purposes, the user
  will never lack write permission on *path's parent.
  """
  if not os.path.exists(path):
    return

  if os.path.islink(path) or not os.path.isdir(path):
    raise Error('Called rmtree(%s) in non-directory' % path)

  if sys.platform == 'win32':
    # Give up and use cmd.exe's rd command.
    path = os.path.normcase(path)
    for _ in xrange(3):
      exitcode = subprocess.call(['cmd.exe', '/c', 'rd', '/q', '/s', path])
      if exitcode == 0:
        return
      else:
        print >> sys.stderr, 'rd exited with code %d' % exitcode
      time.sleep(3)
    raise Exception('Failed to remove path %s' % path)

  # On POSIX systems, we need the x-bit set on the directory to access it,
  # the r-bit to see its contents, and the w-bit to remove files from it.
  # The actual modes of the files within the directory is irrelevant.
  os.chmod(path, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR)

  def remove(func, subpath):
    func(subpath)

  for fn in os.listdir(path):
    # If fullpath is a symbolic link that points to a directory, isdir will
    # be True, but we don't want to descend into that as a directory, we just
    # want to remove the link.  Check islink and treat links as ordinary files
    # would be treated regardless of what they reference.
    fullpath = os.path.join(path, fn)
    if os.path.islink(fullpath) or not os.path.isdir(fullpath):
      remove(os.remove, fullpath)
    else:
      # Recurse.
      rmtree(fullpath)

  remove(os.rmdir, path)


def safe_makedirs(tree):
  """Creates the directory in a safe manner.

  Because multiple threads can create these directories concurently, trap the
  exception and pass on.
  """
  count = 0
  while not os.path.exists(tree):
    count += 1
    try:
      os.makedirs(tree)
    except OSError, e:
      # 17 POSIX, 183 Windows
      if e.errno not in (17, 183):
        raise
      if count > 40:
        # Give up.
        raise


def CommandToStr(args):
  """Converts an arg list into a shell escaped string."""
  return ' '.join(pipes.quote(arg) for arg in args)


def CheckCallAndFilterAndHeader(args, always=False, header=None, **kwargs):
  """Adds 'header' support to CheckCallAndFilter.

  If |always| is True, a message indicating what is being done
  is printed to stdout all the time even if not output is generated. Otherwise
  the message header is printed only if the call generated any ouput.
  """
  stdout = kwargs.setdefault('stdout', sys.stdout)
  if header is None:
    # The automatically generated header only prepends newline if always is
    # false: always is usually set to false if there's an external progress
    # display, and it's better not to clobber it in that case.
    header = "%s________ running '%s' in '%s'\n" % (
                 '' if always else '\n',
                 ' '.join(args), kwargs.get('cwd', '.'))

  if always:
    stdout.write(header)
  else:
    filter_fn = kwargs.get('filter_fn')
    def filter_msg(line):
      if line is None:
        stdout.write(header)
      elif filter_fn:
        filter_fn(line)
    kwargs['filter_fn'] = filter_msg
    kwargs['call_filter_on_first_line'] = True
  # Obviously.
  kwargs.setdefault('print_stdout', True)
  return CheckCallAndFilter(args, **kwargs)


class Wrapper(object):
  """Wraps an object, acting as a transparent proxy for all properties by
  default.
  """
  def __init__(self, wrapped):
    self._wrapped = wrapped

  def __getattr__(self, name):
    return getattr(self._wrapped, name)


class WriteToStdout(Wrapper):
  """Creates a file object clone to also print to sys.stdout."""
  def __init__(self, wrapped):
    super(WriteToStdout, self).__init__(wrapped)
    if not hasattr(self, 'lock'):
      self.lock = threading.Lock()

  def write(self, out, *args, **kwargs):
    self._wrapped.write(out, *args, **kwargs)
    self.lock.acquire()
    try:
      sys.stdout.write(out, *args, **kwargs)
    finally:
      self.lock.release()


class AutoFlush(Wrapper):
  """Creates a file object clone to automatically flush after N seconds."""
  def __init__(self, wrapped, delay):
    super(AutoFlush, self).__init__(wrapped)
    if not hasattr(self, 'lock'):
      self.lock = threading.Lock()
    self.__last_flushed_at = time.time()
    self.delay = delay

  @property
  def autoflush(self):
    return self

  def write(self, out, *args, **kwargs):
    self._wrapped.write(out, *args, **kwargs)
    should_flush = False
    self.lock.acquire()
    try:
      if self.delay and (time.time() - self.__last_flushed_at) > self.delay:
        should_flush = True
        self.__last_flushed_at = time.time()
    finally:
      self.lock.release()
    if should_flush:
      self.flush()


class Annotated(Wrapper):
  """Creates a file object clone to automatically prepends every line in worker
  threads with a NN> prefix.
  """
  def __init__(self, wrapped, include_zero=False):
    super(Annotated, self).__init__(wrapped)
    if not hasattr(self, 'lock'):
      self.lock = threading.Lock()
    self.__output_buffers = {}
    self.__include_zero = include_zero

  @property
  def annotated(self):
    return self

  def write(self, out):
    index = getattr(threading.currentThread(), 'index', 0)
    if not index and not self.__include_zero:
      # Unindexed threads aren't buffered.
      return self._wrapped.write(out)

    self.lock.acquire()
    try:
      # Use a dummy array to hold the string so the code can be lockless.
      # Strings are immutable, requiring to keep a lock for the whole dictionary
      # otherwise. Using an array is faster than using a dummy object.
      if not index in self.__output_buffers:
        obj = self.__output_buffers[index] = ['']
      else:
        obj = self.__output_buffers[index]
    finally:
      self.lock.release()

    # Continue lockless.
    obj[0] += out
    while '\n' in obj[0]:
      line, remaining = obj[0].split('\n', 1)
      if line:
        self._wrapped.write('%d>%s\n' % (index, line))
      obj[0] = remaining

  def flush(self):
    """Flush buffered output."""
    orphans = []
    self.lock.acquire()
    try:
      # Detect threads no longer existing.
      indexes = (getattr(t, 'index', None) for t in threading.enumerate())
      indexes = filter(None, indexes)
      for index in self.__output_buffers:
        if not index in indexes:
          orphans.append((index, self.__output_buffers[index][0]))
      for orphan in orphans:
        del self.__output_buffers[orphan[0]]
    finally:
      self.lock.release()

    # Don't keep the lock while writting. Will append \n when it shouldn't.
    for orphan in orphans:
      if orphan[1]:
        self._wrapped.write('%d>%s\n' % (orphan[0], orphan[1]))
    return self._wrapped.flush()


def MakeFileAutoFlush(fileobj, delay=10):
  autoflush = getattr(fileobj, 'autoflush', None)
  if autoflush:
    autoflush.delay = delay
    return fileobj
  return AutoFlush(fileobj, delay)


def MakeFileAnnotated(fileobj, include_zero=False):
  if getattr(fileobj, 'annotated', None):
    return fileobj
  return Annotated(fileobj)


GCLIENT_CHILDREN = []
GCLIENT_CHILDREN_LOCK = threading.Lock()


class GClientChildren(object):
  @staticmethod
  def add(popen_obj):
    with GCLIENT_CHILDREN_LOCK:
      GCLIENT_CHILDREN.append(popen_obj)

  @staticmethod
  def remove(popen_obj):
    with GCLIENT_CHILDREN_LOCK:
      GCLIENT_CHILDREN.remove(popen_obj)

  @staticmethod
  def _attemptToKillChildren():
    global GCLIENT_CHILDREN
    with GCLIENT_CHILDREN_LOCK:
      zombies = [c for c in GCLIENT_CHILDREN if c.poll() is None]

    for zombie in zombies:
      try:
        zombie.kill()
      except OSError:
        pass

    with GCLIENT_CHILDREN_LOCK:
      GCLIENT_CHILDREN = [k for k in GCLIENT_CHILDREN if k.poll() is not None]

  @staticmethod
  def _areZombies():
    with GCLIENT_CHILDREN_LOCK:
      return bool(GCLIENT_CHILDREN)

  @staticmethod
  def KillAllRemainingChildren():
    GClientChildren._attemptToKillChildren()

    if GClientChildren._areZombies():
      time.sleep(0.5)
      GClientChildren._attemptToKillChildren()

    with GCLIENT_CHILDREN_LOCK:
      if GCLIENT_CHILDREN:
        print >> sys.stderr, 'Could not kill the following subprocesses:'
        for zombie in GCLIENT_CHILDREN:
          print >> sys.stderr, '  ', zombie.pid


def CheckCallAndFilter(args, stdout=None, filter_fn=None,
                       print_stdout=None, call_filter_on_first_line=False,
                       retry=False, **kwargs):
  """Runs a command and calls back a filter function if needed.

  Accepts all subprocess2.Popen() parameters plus:
    print_stdout: If True, the command's stdout is forwarded to stdout.
    filter_fn: A function taking a single string argument called with each line
               of the subprocess2's output. Each line has the trailing newline
               character trimmed.
    stdout: Can be any bufferable output.
    retry: If the process exits non-zero, sleep for a brief interval and try
           again, up to RETRY_MAX times.

  stderr is always redirected to stdout.
  """
  assert print_stdout or filter_fn
  stdout = stdout or sys.stdout
  output = cStringIO.StringIO()
  filter_fn = filter_fn or (lambda x: None)

  sleep_interval = RETRY_INITIAL_SLEEP
  run_cwd = kwargs.get('cwd', os.getcwd())
  for _ in xrange(RETRY_MAX + 1):
    kid = subprocess2.Popen(
        args, bufsize=0, stdout=subprocess2.PIPE, stderr=subprocess2.STDOUT,
        **kwargs)

    GClientChildren.add(kid)

    # Do a flush of stdout before we begin reading from the subprocess2's stdout
    stdout.flush()

    # Also, we need to forward stdout to prevent weird re-ordering of output.
    # This has to be done on a per byte basis to make sure it is not buffered:
    # normally buffering is done for each line, but if the process requests
    # input, no end-of-line character is output after the prompt and it would
    # not show up.
    try:
      in_byte = kid.stdout.read(1)
      if in_byte:
        if call_filter_on_first_line:
          filter_fn(None)
        in_line = ''
        while in_byte:
          output.write(in_byte)
          if print_stdout:
            stdout.write(in_byte)
          if in_byte not in ['\r', '\n']:
            in_line += in_byte
          else:
            filter_fn(in_line)
            in_line = ''
          in_byte = kid.stdout.read(1)
        # Flush the rest of buffered output. This is only an issue with
        # stdout/stderr not ending with a \n.
        if len(in_line):
          filter_fn(in_line)
      rv = kid.wait()

      # Don't put this in a 'finally,' since the child may still run if we get
      # an exception.
      GClientChildren.remove(kid)

    except KeyboardInterrupt:
      print >> sys.stderr, 'Failed while running "%s"' % ' '.join(args)
      raise

    if rv == 0:
      return output.getvalue()
    if not retry:
      break
    print ("WARNING: subprocess '%s' in %s failed; will retry after a short "
           'nap...' % (' '.join('"%s"' % x for x in args), run_cwd))
    time.sleep(sleep_interval)
    sleep_interval *= 2
  raise subprocess2.CalledProcessError(
      rv, args, kwargs.get('cwd', None), None, None)


class GitFilter(object):
  """A filter_fn implementation for quieting down git output messages.

  Allows a custom function to skip certain lines (predicate), and will throttle
  the output of percentage completed lines to only output every X seconds.
  """
  PERCENT_RE = re.compile('(.*) ([0-9]{1,3})% .*')

  def __init__(self, time_throttle=0, predicate=None, out_fh=None):
    """
    Args:
      time_throttle (int): GitFilter will throttle 'noisy' output (such as the
        XX% complete messages) to only be printed at least |time_throttle|
        seconds apart.
      predicate (f(line)): An optional function which is invoked for every line.
        The line will be skipped if predicate(line) returns False.
      out_fh: File handle to write output to.
    """
    self.last_time = 0
    self.time_throttle = time_throttle
    self.predicate = predicate
    self.out_fh = out_fh or sys.stdout
    self.progress_prefix = None

  def __call__(self, line):
    # git uses an escape sequence to clear the line; elide it.
    esc = line.find(unichr(033))
    if esc > -1:
      line = line[:esc]
    if self.predicate and not self.predicate(line):
      return
    now = time.time()
    match = self.PERCENT_RE.match(line)
    if match:
      if match.group(1) != self.progress_prefix:
        self.progress_prefix = match.group(1)
      elif now - self.last_time < self.time_throttle:
        return
    self.last_time = now
    self.out_fh.write('[%s] ' % Elapsed())
    print >> self.out_fh, line


def FindGclientRoot(from_dir, filename='.gclient'):
  """Tries to find the gclient root."""
  real_from_dir = os.path.realpath(from_dir)
  path = real_from_dir
  while not os.path.exists(os.path.join(path, filename)):
    split_path = os.path.split(path)
    if not split_path[1]:
      return None
    path = split_path[0]

  # If we did not find the file in the current directory, make sure we are in a
  # sub directory that is controlled by this configuration.
  if path != real_from_dir:
    entries_filename = os.path.join(path, filename + '_entries')
    if not os.path.exists(entries_filename):
      # If .gclient_entries does not exist, a previous call to gclient sync
      # might have failed. In that case, we cannot verify that the .gclient
      # is the one we want to use. In order to not to cause too much trouble,
      # just issue a warning and return the path anyway.
      print >> sys.stderr, ("%s missing, %s file in parent directory %s might "
          "not be the file you want to use." %
          (entries_filename, filename, path))
      return path
    scope = {}
    try:
      exec(FileRead(entries_filename), scope)
    except SyntaxError, e:
      SyntaxErrorToError(filename, e)
    all_directories = scope['entries'].keys()
    path_to_check = real_from_dir[len(path)+1:]
    while path_to_check:
      if path_to_check in all_directories:
        return path
      path_to_check = os.path.dirname(path_to_check)
    return None

  logging.info('Found gclient root at ' + path)
  return path


def PathDifference(root, subpath):
  """Returns the difference subpath minus root."""
  root = os.path.realpath(root)
  subpath = os.path.realpath(subpath)
  if not subpath.startswith(root):
    return None
  # If the root does not have a trailing \ or /, we add it so the returned
  # path starts immediately after the seperator regardless of whether it is
  # provided.
  root = os.path.join(root, '')
  return subpath[len(root):]


def FindFileUpwards(filename, path=None):
  """Search upwards from the a directory (default: current) to find a file.

  Returns nearest upper-level directory with the passed in file.
  """
  if not path:
    path = os.getcwd()
  path = os.path.realpath(path)
  while True:
    file_path = os.path.join(path, filename)
    if os.path.exists(file_path):
      return path
    (new_path, _) = os.path.split(path)
    if new_path == path:
      return None
    path = new_path


def GetMacWinOrLinux():
  """Returns 'mac', 'win', or 'linux', matching the current platform."""
  if sys.platform.startswith(('cygwin', 'win')):
    return 'win'
  elif sys.platform.startswith('linux'):
    return 'linux'
  elif sys.platform == 'darwin':
    return 'mac'
  raise Error('Unknown platform: ' + sys.platform)


def GetPrimarySolutionPath():
  """Returns the full path to the primary solution. (gclient_root + src)"""

  gclient_root = FindGclientRoot(os.getcwd())
  if not gclient_root:
    # Some projects might not use .gclient. Try to see whether we're in a git
    # checkout.
    top_dir = [os.getcwd()]
    def filter_fn(line):
      repo_root_path = os.path.normpath(line.rstrip('\n'))
      if os.path.exists(repo_root_path):
        top_dir[0] = repo_root_path
    try:
      CheckCallAndFilter(["git", "rev-parse", "--show-toplevel"],
                         print_stdout=False, filter_fn=filter_fn)
    except Exception:
      pass
    top_dir = top_dir[0]
    if os.path.exists(os.path.join(top_dir, 'buildtools')):
      return top_dir
    return None

  # Some projects' top directory is not named 'src'.
  source_dir_name = GetGClientPrimarySolutionName(gclient_root) or 'src'
  return os.path.join(gclient_root, source_dir_name)


def GetBuildtoolsPath():
  """Returns the full path to the buildtools directory.
  This is based on the root of the checkout containing the current directory."""

  # Overriding the build tools path by environment is highly unsupported and may
  # break without warning.  Do not rely on this for anything important.
  override = os.environ.get('CHROMIUM_BUILDTOOLS_PATH')
  if override is not None:
    return override

  primary_solution = GetPrimarySolutionPath()
  if not primary_solution:
    return None
  buildtools_path = os.path.join(primary_solution, 'buildtools')
  if not os.path.exists(buildtools_path):
    # Buildtools may be in the gclient root.
    gclient_root = FindGclientRoot(os.getcwd())
    buildtools_path = os.path.join(gclient_root, 'buildtools')
  return buildtools_path


def GetBuildtoolsPlatformBinaryPath():
  """Returns the full path to the binary directory for the current platform."""
  buildtools_path = GetBuildtoolsPath()
  if not buildtools_path:
    return None

  if sys.platform.startswith(('cygwin', 'win')):
    subdir = 'win'
  elif sys.platform == 'darwin':
    subdir = 'mac'
  elif sys.platform.startswith('linux'):
      subdir = 'linux64'
  else:
    raise Error('Unknown platform: ' + sys.platform)
  return os.path.join(buildtools_path, subdir)


def GetExeSuffix():
  """Returns '' or '.exe' depending on how executables work on this platform."""
  if sys.platform.startswith(('cygwin', 'win')):
    return '.exe'
  return ''


def GetGClientPrimarySolutionName(gclient_root_dir_path):
  """Returns the name of the primary solution in the .gclient file specified."""
  gclient_config_file = os.path.join(gclient_root_dir_path, '.gclient')
  env = {}
  execfile(gclient_config_file, env)
  solutions = env.get('solutions', [])
  if solutions:
    return solutions[0].get('name')
  return None


def GetGClientRootAndEntries(path=None):
  """Returns the gclient root and the dict of entries."""
  config_file = '.gclient_entries'
  root = FindFileUpwards(config_file, path)
  if not root:
    print "Can't find %s" % config_file
    return None
  config_path = os.path.join(root, config_file)
  env = {}
  execfile(config_path, env)
  config_dir = os.path.dirname(config_path)
  return config_dir, env['entries']


def lockedmethod(method):
  """Method decorator that holds self.lock for the duration of the call."""
  def inner(self, *args, **kwargs):
    try:
      try:
        self.lock.acquire()
      except KeyboardInterrupt:
        print >> sys.stderr, 'Was deadlocked'
        raise
      return method(self, *args, **kwargs)
    finally:
      self.lock.release()
  return inner


class WorkItem(object):
  """One work item."""
  # On cygwin, creating a lock throwing randomly when nearing ~100 locks.
  # As a workaround, use a single lock. Yep you read it right. Single lock for
  # all the 100 objects.
  lock = threading.Lock()

  def __init__(self, name):
    # A unique string representing this work item.
    self._name = name
    self.outbuf = cStringIO.StringIO()
    self.start = self.finish = None
    self.resources = []  # List of resources this work item requires.

  def run(self, work_queue):
    """work_queue is passed as keyword argument so it should be
    the last parameters of the function when you override it."""
    pass

  @property
  def name(self):
    return self._name


class ExecutionQueue(object):
  """Runs a set of WorkItem that have interdependencies and were WorkItem are
  added as they are processed.

  This class manages that all the required dependencies are run
  before running each one.

  Methods of this class are thread safe.
  """
  def __init__(self, jobs, progress, ignore_requirements, verbose=False):
    """jobs specifies the number of concurrent tasks to allow. progress is a
    Progress instance."""
    # Set when a thread is done or a new item is enqueued.
    self.ready_cond = threading.Condition()
    # Maximum number of concurrent tasks.
    self.jobs = jobs
    # List of WorkItem, for gclient, these are Dependency instances.
    self.queued = []
    # List of strings representing each Dependency.name that was run.
    self.ran = []
    # List of items currently running.
    self.running = []
    # Exceptions thrown if any.
    self.exceptions = Queue.Queue()
    # Progress status
    self.progress = progress
    if self.progress:
      self.progress.update(0)

    self.ignore_requirements = ignore_requirements
    self.verbose = verbose
    self.last_join = None
    self.last_subproc_output = None

  def enqueue(self, d):
    """Enqueue one Dependency to be executed later once its requirements are
    satisfied.
    """
    assert isinstance(d, WorkItem)
    self.ready_cond.acquire()
    try:
      self.queued.append(d)
      total = len(self.queued) + len(self.ran) + len(self.running)
      if self.jobs == 1:
        total += 1
      logging.debug('enqueued(%s)' % d.name)
      if self.progress:
        self.progress._total = total
        self.progress.update(0)
      self.ready_cond.notifyAll()
    finally:
      self.ready_cond.release()

  def out_cb(self, _):
    self.last_subproc_output = datetime.datetime.now()
    return True

  @staticmethod
  def format_task_output(task, comment=''):
    if comment:
      comment = ' (%s)' % comment
    if task.start and task.finish:
      elapsed = ' (Elapsed: %s)' % (
          str(task.finish - task.start).partition('.')[0])
    else:
      elapsed = ''
    return """
%s%s%s
----------------------------------------
%s
----------------------------------------""" % (
    task.name, comment, elapsed, task.outbuf.getvalue().strip())

  def _is_conflict(self, job):
    """Checks to see if a job will conflict with another running job."""
    for running_job in self.running:
      for used_resource in running_job.item.resources:
        logging.debug('Checking resource %s' % used_resource)
        if used_resource in job.resources:
          return True
    return False

  def flush(self, *args, **kwargs):
    """Runs all enqueued items until all are executed."""
    kwargs['work_queue'] = self
    self.last_subproc_output = self.last_join = datetime.datetime.now()
    self.ready_cond.acquire()
    try:
      while True:
        # Check for task to run first, then wait.
        while True:
          if not self.exceptions.empty():
            # Systematically flush the queue when an exception logged.
            self.queued = []
          self._flush_terminated_threads()
          if (not self.queued and not self.running or
              self.jobs == len(self.running)):
            logging.debug('No more worker threads or can\'t queue anything.')
            break

          # Check for new tasks to start.
          for i in xrange(len(self.queued)):
            # Verify its requirements.
            if (self.ignore_requirements or
                not (set(self.queued[i].requirements) - set(self.ran))):
              if not self._is_conflict(self.queued[i]):
                # Start one work item: all its requirements are satisfied.
                self._run_one_task(self.queued.pop(i), args, kwargs)
                break
          else:
            # Couldn't find an item that could run. Break out the outher loop.
            break

        if not self.queued and not self.running:
          # We're done.
          break
        # We need to poll here otherwise Ctrl-C isn't processed.
        try:
          self.ready_cond.wait(10)
          # If we haven't printed to terminal for a while, but we have received
          # spew from a suprocess, let the user know we're still progressing.
          now = datetime.datetime.now()
          if (now - self.last_join > datetime.timedelta(seconds=60) and
              self.last_subproc_output > self.last_join):
            if self.progress:
              print >> sys.stdout, ''
              sys.stdout.flush()
            elapsed = Elapsed()
            print >> sys.stdout, '[%s] Still working on:' % elapsed
            sys.stdout.flush()
            for task in self.running:
              print >> sys.stdout, '[%s]   %s' % (elapsed, task.item.name)
              sys.stdout.flush()
        except KeyboardInterrupt:
          # Help debugging by printing some information:
          print >> sys.stderr, (
              ('\nAllowed parallel jobs: %d\n# queued: %d\nRan: %s\n'
                'Running: %d') % (
              self.jobs,
              len(self.queued),
              ', '.join(self.ran),
              len(self.running)))
          for i in self.queued:
            print >> sys.stderr, '%s (not started): %s' % (
                i.name, ', '.join(i.requirements))
          for i in self.running:
            print >> sys.stderr, self.format_task_output(i.item, 'interrupted')
          raise
        # Something happened: self.enqueue() or a thread terminated. Loop again.
    finally:
      self.ready_cond.release()

    assert not self.running, 'Now guaranteed to be single-threaded'
    if not self.exceptions.empty():
      if self.progress:
        print >> sys.stdout, ''
      # To get back the stack location correctly, the raise a, b, c form must be
      # used, passing a tuple as the first argument doesn't work.
      e, task = self.exceptions.get()
      print >> sys.stderr, self.format_task_output(task.item, 'ERROR')
      raise e[0], e[1], e[2]
    elif self.progress:
      self.progress.end()

  def _flush_terminated_threads(self):
    """Flush threads that have terminated."""
    running = self.running
    self.running = []
    for t in running:
      if t.isAlive():
        self.running.append(t)
      else:
        t.join()
        self.last_join = datetime.datetime.now()
        sys.stdout.flush()
        if self.verbose:
          print >> sys.stdout, self.format_task_output(t.item)
        if self.progress:
          self.progress.update(1, t.item.name)
        if t.item.name in self.ran:
          raise Error(
              'gclient is confused, "%s" is already in "%s"' % (
                t.item.name, ', '.join(self.ran)))
        if not t.item.name in self.ran:
          self.ran.append(t.item.name)

  def _run_one_task(self, task_item, args, kwargs):
    if self.jobs > 1:
      # Start the thread.
      index = len(self.ran) + len(self.running) + 1
      new_thread = self._Worker(task_item, index, args, kwargs)
      self.running.append(new_thread)
      new_thread.start()
    else:
      # Run the 'thread' inside the main thread. Don't try to catch any
      # exception.
      try:
        task_item.start = datetime.datetime.now()
        print >> task_item.outbuf, '[%s] Started.' % Elapsed(task_item.start)
        task_item.run(*args, **kwargs)
        task_item.finish = datetime.datetime.now()
        print >> task_item.outbuf, '[%s] Finished.' % Elapsed(task_item.finish)
        self.ran.append(task_item.name)
        if self.verbose:
          if self.progress:
            print >> sys.stdout, ''
          print >> sys.stdout, self.format_task_output(task_item)
        if self.progress:
          self.progress.update(1, ', '.join(t.item.name for t in self.running))
      except KeyboardInterrupt:
        print >> sys.stderr, self.format_task_output(task_item, 'interrupted')
        raise
      except Exception:
        print >> sys.stderr, self.format_task_output(task_item, 'ERROR')
        raise


  class _Worker(threading.Thread):
    """One thread to execute one WorkItem."""
    def __init__(self, item, index, args, kwargs):
      threading.Thread.__init__(self, name=item.name or 'Worker')
      logging.info('_Worker(%s) reqs:%s' % (item.name, item.requirements))
      self.item = item
      self.index = index
      self.args = args
      self.kwargs = kwargs
      self.daemon = True

    def run(self):
      """Runs in its own thread."""
      logging.debug('_Worker.run(%s)' % self.item.name)
      work_queue = self.kwargs['work_queue']
      try:
        self.item.start = datetime.datetime.now()
        print >> self.item.outbuf, '[%s] Started.' % Elapsed(self.item.start)
        self.item.run(*self.args, **self.kwargs)
        self.item.finish = datetime.datetime.now()
        print >> self.item.outbuf, '[%s] Finished.' % Elapsed(self.item.finish)
      except KeyboardInterrupt:
        logging.info('Caught KeyboardInterrupt in thread %s', self.item.name)
        logging.info(str(sys.exc_info()))
        work_queue.exceptions.put((sys.exc_info(), self))
        raise
      except Exception:
        # Catch exception location.
        logging.info('Caught exception in thread %s', self.item.name)
        logging.info(str(sys.exc_info()))
        work_queue.exceptions.put((sys.exc_info(), self))
      finally:
        logging.info('_Worker.run(%s) done', self.item.name)
        work_queue.ready_cond.acquire()
        try:
          work_queue.ready_cond.notifyAll()
        finally:
          work_queue.ready_cond.release()


def GetEditor(git_editor=None):
  """Returns the most plausible editor to use.

  In order of preference:
  - GIT_EDITOR environment variable
  - core.editor git configuration variable (if supplied by git-cl)
  - VISUAL environment variable
  - EDITOR environment variable
  - vi (non-Windows) or notepad (Windows)

  In the case of git-cl, this matches git's behaviour, except that it does not
  include dumb terminal detection.
  """
  editor = os.environ.get('GIT_EDITOR') or git_editor
  if not editor:
    editor = os.environ.get('VISUAL')
  if not editor:
    editor = os.environ.get('EDITOR')
  if not editor:
    if sys.platform.startswith('win'):
      editor = 'notepad'
    else:
      editor = 'vi'
  return editor


def RunEditor(content, git, git_editor=None):
  """Opens up the default editor in the system to get the CL description."""
  file_handle, filename = tempfile.mkstemp(text=True, prefix='cl_description')
  # Make sure CRLF is handled properly by requiring none.
  if '\r' in content:
    print >> sys.stderr, (
        '!! Please remove \\r from your change description !!')
  fileobj = os.fdopen(file_handle, 'w')
  # Still remove \r if present.
  content = re.sub('\r?\n', '\n', content)
  # Some editors complain when the file doesn't end in \n.
  if not content.endswith('\n'):
    content += '\n'
  fileobj.write(content)
  fileobj.close()

  try:
    editor = GetEditor(git_editor=git_editor)
    if not editor:
      return None
    cmd = '%s %s' % (editor, filename)
    if sys.platform == 'win32' and os.environ.get('TERM') == 'msys':
      # Msysgit requires the usage of 'env' to be present.
      cmd = 'env ' + cmd
    try:
      # shell=True to allow the shell to handle all forms of quotes in
      # $EDITOR.
      subprocess2.check_call(cmd, shell=True)
    except subprocess2.CalledProcessError:
      return None
    return FileRead(filename)
  finally:
    os.remove(filename)


def UpgradeToHttps(url):
  """Upgrades random urls to https://.

  Do not touch unknown urls like ssh:// or git://.
  Do not touch http:// urls with a port number,
  Fixes invalid GAE url.
  """
  if not url:
    return url
  if not re.match(r'[a-z\-]+\://.*', url):
    # Make sure it is a valid uri. Otherwise, urlparse() will consider it a
    # relative url and will use http:///foo. Note that it defaults to http://
    # for compatibility with naked url like "localhost:8080".
    url = 'http://%s' % url
  parsed = list(urlparse.urlparse(url))
  # Do not automatically upgrade http to https if a port number is provided.
  if parsed[0] == 'http' and not re.match(r'^.+?\:\d+$', parsed[1]):
    parsed[0] = 'https'
  return urlparse.urlunparse(parsed)


def ParseCodereviewSettingsContent(content):
  """Process a codereview.settings file properly."""
  lines = (l for l in content.splitlines() if not l.strip().startswith("#"))
  try:
    keyvals = dict([x.strip() for x in l.split(':', 1)] for l in lines if l)
  except ValueError:
    raise Error(
        'Failed to process settings, please fix. Content:\n\n%s' % content)
  def fix_url(key):
    if keyvals.get(key):
      keyvals[key] = UpgradeToHttps(keyvals[key])
  fix_url('CODE_REVIEW_SERVER')
  fix_url('VIEW_VC')
  return keyvals


def NumLocalCpus():
  """Returns the number of processors.

  multiprocessing.cpu_count() is permitted to raise NotImplementedError, and
  is known to do this on some Windows systems and OSX 10.6. If we can't get the
  CPU count, we will fall back to '1'.
  """
  # Surround the entire thing in try/except; no failure here should stop gclient
  # from working.
  try:
    # Use multiprocessing to get CPU count. This may raise
    # NotImplementedError.
    try:
      import multiprocessing
      return multiprocessing.cpu_count()
    except NotImplementedError:  # pylint: disable=bare-except
      # (UNIX) Query 'os.sysconf'.
      # pylint: disable=no-member
      if hasattr(os, 'sysconf') and 'SC_NPROCESSORS_ONLN' in os.sysconf_names:
        return int(os.sysconf('SC_NPROCESSORS_ONLN'))

      # (Windows) Query 'NUMBER_OF_PROCESSORS' environment variable.
      if 'NUMBER_OF_PROCESSORS' in os.environ:
        return int(os.environ['NUMBER_OF_PROCESSORS'])
  except Exception as e:
    logging.exception("Exception raised while probing CPU count: %s", e)

  logging.debug('Failed to get CPU count. Defaulting to 1.')
  return 1


def DefaultDeltaBaseCacheLimit():
  """Return a reasonable default for the git config core.deltaBaseCacheLimit.

  The primary constraint is the address space of virtual memory.  The cache
  size limit is per-thread, and 32-bit systems can hit OOM errors if this
  parameter is set too high.
  """
  if platform.architecture()[0].startswith('64'):
    return '2g'
  else:
    return '512m'


def DefaultIndexPackConfig(url=''):
  """Return reasonable default values for configuring git-index-pack.

  Experiments suggest that higher values for pack.threads don't improve
  performance."""
  cache_limit = DefaultDeltaBaseCacheLimit()
  result = ['-c', 'core.deltaBaseCacheLimit=%s' % cache_limit]
  if url in THREADED_INDEX_PACK_BLACKLIST:
    result.extend(['-c', 'pack.threads=1'])
  return result


def FindExecutable(executable):
  """This mimics the "which" utility."""
  path_folders = os.environ.get('PATH').split(os.pathsep)

  for path_folder in path_folders:
    target = os.path.join(path_folder, executable)
    # Just incase we have some ~/blah paths.
    target = os.path.abspath(os.path.expanduser(target))
    if os.path.isfile(target) and os.access(target, os.X_OK):
      return target
    if sys.platform.startswith('win'):
      for suffix in ('.bat', '.cmd', '.exe'):
        alt_target = target + suffix
        if os.path.isfile(alt_target) and os.access(alt_target, os.X_OK):
          return alt_target
  return None


def freeze(obj):
  """Takes a generic object ``obj``, and returns an immutable version of it.

  Supported types:
    * dict / OrderedDict -> FrozenDict
    * list -> tuple
    * set -> frozenset
    * any object with a working __hash__ implementation (assumes that hashable
      means immutable)

  Will raise TypeError if you pass an object which is not hashable.
  """
  if isinstance(obj, collections.Mapping):
    return FrozenDict((freeze(k), freeze(v)) for k, v in obj.iteritems())
  elif isinstance(obj, (list, tuple)):
    return tuple(freeze(i) for i in obj)
  elif isinstance(obj, set):
    return frozenset(freeze(i) for i in obj)
  else:
    hash(obj)
    return obj


class FrozenDict(collections.Mapping):
  """An immutable OrderedDict.

  Modified From: http://stackoverflow.com/a/2704866
  """
  def __init__(self, *args, **kwargs):
    self._d = collections.OrderedDict(*args, **kwargs)

    # Calculate the hash immediately so that we know all the items are
    # hashable too.
    self._hash = reduce(operator.xor,
                        (hash(i) for i in enumerate(self._d.iteritems())), 0)

  def __eq__(self, other):
    if not isinstance(other, collections.Mapping):
      return NotImplemented
    if self is other:
      return True
    if len(self) != len(other):
      return False
    for k, v in self.iteritems():
      if k not in other or other[k] != v:
        return False
    return True

  def __iter__(self):
    return iter(self._d)

  def __len__(self):
    return len(self._d)

  def __getitem__(self, key):
    return self._d[key]

  def __hash__(self):
    return self._hash

  def __repr__(self):
    return 'FrozenDict(%r)' % (self._d.items(),)
