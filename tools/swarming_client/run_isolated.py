#!/usr/bin/env python
# Copyright 2012 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Runs a command with optional isolated input/output.

Despite name "run_isolated", can run a generic non-isolated command specified as
args.

If input isolated hash is provided, fetches it, creates a tree of hard links,
appends args to the command in the fetched isolated and runs it.
To improve performance, keeps a local cache.
The local cache can safely be deleted.

Any ${EXECUTABLE_SUFFIX} on the command line will be replaced with ".exe" string
on Windows and "" on other platforms.

Any ${ISOLATED_OUTDIR} on the command line will be replaced by the location of a
temporary directory upon execution of the command specified in the .isolated
file. All content written to this directory will be uploaded upon termination
and the .isolated file describing this directory will be printed to stdout.

Any ${SWARMING_BOT_FILE} on the command line will be replaced by the value of
the --bot-file parameter. This file is used by a swarming bot to communicate
state of the host to tasks. It is written to by the swarming bot's
on_before_task() hook in the swarming server's custom bot_config.py.
"""

__version__ = '0.10.5'

import argparse
import base64
import collections
import contextlib
import errno
import json
import logging
import optparse
import os
import sys
import tempfile
import time

from third_party.depot_tools import fix_encoding

from utils import file_path
from utils import fs
from utils import large
from utils import logging_utils
from utils import on_error
from utils import subprocess42
from utils import tools
from utils import zip_package

from libs import luci_context

import auth
import cipd
import isolateserver
import named_cache


# Absolute path to this file (can be None if running from zip on Mac).
THIS_FILE_PATH = os.path.abspath(
    __file__.decode(sys.getfilesystemencoding())) if __file__ else None

# Directory that contains this file (might be inside zip package).
BASE_DIR = os.path.dirname(THIS_FILE_PATH) if __file__.decode(
    sys.getfilesystemencoding()) else None

# Directory that contains currently running script file.
if zip_package.get_main_script_path():
  MAIN_DIR = os.path.dirname(
      os.path.abspath(zip_package.get_main_script_path()))
else:
  # This happens when 'import run_isolated' is executed at the python
  # interactive prompt, in that case __file__ is undefined.
  MAIN_DIR = None


# Magic variables that can be found in the isolate task command line.
ISOLATED_OUTDIR_PARAMETER = '${ISOLATED_OUTDIR}'
EXECUTABLE_SUFFIX_PARAMETER = '${EXECUTABLE_SUFFIX}'
SWARMING_BOT_FILE_PARAMETER = '${SWARMING_BOT_FILE}'


# The name of the log file to use.
RUN_ISOLATED_LOG_FILE = 'run_isolated.log'


# The name of the log to use for the run_test_cases.py command
RUN_TEST_CASES_LOG = 'run_test_cases.log'


# Use short names for temporary directories. This is driven by Windows, which
# imposes a relatively short maximum path length of 260 characters, often
# referred to as MAX_PATH. It is relatively easy to create files with longer
# path length. A use case is with recursive depedency treesV like npm packages.
#
# It is recommended to start the script with a `root_dir` as short as
# possible.
# - ir stands for isolated_run
# - io stands for isolated_out
# - it stands for isolated_tmp
ISOLATED_RUN_DIR = u'ir'
ISOLATED_OUT_DIR = u'io'
ISOLATED_TMP_DIR = u'it'


OUTLIVING_ZOMBIE_MSG = """\
*** Swarming tried multiple times to delete the %s directory and failed ***
*** Hard failing the task ***

Swarming detected that your testing script ran an executable, which may have
started a child executable, and the main script returned early, leaving the
children executables playing around unguided.

You don't want to leave children processes outliving the task on the Swarming
bot, do you? The Swarming bot doesn't.

How to fix?
- For any process that starts children processes, make sure all children
  processes terminated properly before each parent process exits. This is
  especially important in very deep process trees.
  - This must be done properly both in normal successful task and in case of
    task failure. Cleanup is very important.
- The Swarming bot sends a SIGTERM in case of timeout.
  - You have %s seconds to comply after the signal was sent to the process
    before the process is forcibly killed.
- To achieve not leaking children processes in case of signals on timeout, you
  MUST handle signals in each executable / python script and propagate them to
  children processes.
  - When your test script (python or binary) receives a signal like SIGTERM or
    CTRL_BREAK_EVENT on Windows), send it to all children processes and wait for
    them to terminate before quitting.

See
https://github.com/luci/luci-py/blob/master/appengine/swarming/doc/Bot.md#graceful-termination-aka-the-sigterm-and-sigkill-dance
for more information.

*** May the SIGKILL force be with you ***
"""


TaskData = collections.namedtuple(
    'TaskData', [
      # List of strings; the command line to use, independent of what was
      # specified in the isolated file.
      'command',
      # Relative directory to start command into.
      'relative_cwd',
      # List of strings; the arguments to add to the command specified in the
      # isolated file.
      'extra_args',
      # Hash of the .isolated file that must be retrieved to recreate the tree
      # of files to run the target executable. The command specified in the
      # .isolated is executed.  Mutually exclusive with command argument.
      'isolated_hash',
      # isolateserver.Storage instance to retrieve remote objects. This object
      # has a reference to an isolateserver.StorageApi, which does the actual
      # I/O.
      'storage',
      # isolateserver.LocalCache instance to keep from retrieving the same
      # objects constantly by caching the objects retrieved. Can be on-disk or
      # in-memory.
      'isolate_cache',
      # List of paths relative to root_dir to put into the output isolated
      # bundle upon task completion (see link_outputs_to_outdir).
      'outputs',
      # Function (run_dir) => context manager that installs named caches into
      # |run_dir|.
      'install_named_caches',
      # If True, the temporary directory will be deliberately leaked for later
      # examination.
      'leak_temp_dir',
      # Path to the directory to use to create the temporary directory. If not
      # specified, a random temporary directory is created.
      'root_dir',
      # Kills the process if it lasts more than this amount of seconds.
      'hard_timeout',
      # Number of seconds to wait between SIGTERM and SIGKILL.
      'grace_period',
      # Path to a file with bot state, used in place of ${SWARMING_BOT_FILE}
      # task command line argument.
      'bot_file',
      # Logical account to switch LUCI_CONTEXT into.
      'switch_to_account',
      # Context manager dir => CipdInfo, see install_client_and_packages.
      'install_packages_fn',
      # Create tree with symlinks instead of hardlinks.
      'use_symlinks',
      # Environment variables to set.
      'env',
      # Environment variables to mutate with relative directories.
      # Example: {"ENV_KEY": ['relative', 'paths', 'to', 'prepend']}
      'env_prefix'])


def get_as_zip_package(executable=True):
  """Returns ZipPackage with this module and all its dependencies.

  If |executable| is True will store run_isolated.py as __main__.py so that
  zip package is directly executable be python.
  """
  # Building a zip package when running from another zip package is
  # unsupported and probably unneeded.
  assert not zip_package.is_zipped_module(sys.modules[__name__])
  assert THIS_FILE_PATH
  assert BASE_DIR
  package = zip_package.ZipPackage(root=BASE_DIR)
  package.add_python_file(THIS_FILE_PATH, '__main__.py' if executable else None)
  package.add_python_file(os.path.join(BASE_DIR, 'isolate_storage.py'))
  package.add_python_file(os.path.join(BASE_DIR, 'isolated_format.py'))
  package.add_python_file(os.path.join(BASE_DIR, 'isolateserver.py'))
  package.add_python_file(os.path.join(BASE_DIR, 'auth.py'))
  package.add_python_file(os.path.join(BASE_DIR, 'cipd.py'))
  package.add_python_file(os.path.join(BASE_DIR, 'local_caching.py'))
  package.add_python_file(os.path.join(BASE_DIR, 'named_cache.py'))
  package.add_directory(os.path.join(BASE_DIR, 'libs'))
  package.add_directory(os.path.join(BASE_DIR, 'third_party'))
  package.add_directory(os.path.join(BASE_DIR, 'utils'))
  return package


def _to_str(s):
  """Downgrades a unicode instance to str. Pass str through as-is."""
  if isinstance(s, str):
    return s
  # This is technically incorrect, especially on Windows. In theory
  # sys.getfilesystemencoding() should be used to use the right 'ANSI code
  # page' on Windows, but that causes other problems, as the character set
  # is very limited.
  return s.encode('utf-8')


def _to_unicode(s):
  """Upgrades a str instance to unicode. Pass unicode through as-is."""
  if isinstance(s, unicode) or s is None:
    return s
  return s.decode('utf-8')


def make_temp_dir(prefix, root_dir):
  """Returns a new unique temporary directory."""
  return unicode(tempfile.mkdtemp(prefix=prefix, dir=root_dir))


def change_tree_read_only(rootdir, read_only):
  """Changes the tree read-only bits according to the read_only specification.

  The flag can be 0, 1 or 2, which will affect the possibility to modify files
  and create or delete files.
  """
  if read_only == 2:
    # Files and directories (except on Windows) are marked read only. This
    # inhibits modifying, creating or deleting files in the test directory,
    # except on Windows where creating and deleting files is still possible.
    file_path.make_tree_read_only(rootdir)
  elif read_only == 1:
    # Files are marked read only but not the directories. This inhibits
    # modifying files but creating or deleting files is still possible.
    file_path.make_tree_files_read_only(rootdir)
  elif read_only in (0, None):
    # Anything can be modified.
    # TODO(maruel): This is currently dangerous as long as DiskCache.touch()
    # is not yet changed to verify the hash of the content of the files it is
    # looking at, so that if a test modifies an input file, the file must be
    # deleted.
    file_path.make_tree_writeable(rootdir)
  else:
    raise ValueError(
        'change_tree_read_only(%s, %s): Unknown flag %s' %
        (rootdir, read_only, read_only))


@contextlib.contextmanager
def set_luci_context_account(account, tmp_dir):
  """Sets LUCI_CONTEXT account to be used by the task.

  If 'account' is None or '', does nothing at all. This happens when
  run_isolated.py is called without '--switch-to-account' flag. In this case,
  if run_isolated.py is running in some LUCI_CONTEXT environment, the task will
  just inherit whatever account is already set. This may happen is users invoke
  run_isolated.py explicitly from their code.

  If the requested account is not defined in the context, switches to
  non-authenticated access. This happens for Swarming tasks that don't use
  'task' service accounts.

  If not using LUCI_CONTEXT-based auth, does nothing.
  If already running as requested account, does nothing.
  """
  if not account:
    # Not actually switching.
    yield
    return

  local_auth = luci_context.read('local_auth')
  if not local_auth:
    # Not using LUCI_CONTEXT auth at all.
    yield
    return

  # See LUCI_CONTEXT.md for the format of 'local_auth'.
  if local_auth.get('default_account_id') == account:
    # Already set, no need to switch.
    yield
    return

  available = {a['id'] for a in local_auth.get('accounts') or []}
  if account in available:
    logging.info('Switching default LUCI_CONTEXT account to %r', account)
    local_auth['default_account_id'] = account
  else:
    logging.warning(
        'Requested LUCI_CONTEXT account %r is not available (have only %r), '
        'disabling authentication', account, sorted(available))
    local_auth.pop('default_account_id', None)

  with luci_context.write(_tmpdir=tmp_dir, local_auth=local_auth):
    yield


def process_command(command, out_dir, bot_file):
  """Replaces variables in a command line.

  Raises:
    ValueError if a parameter is requested in |command| but its value is not
      provided.
  """
  def fix(arg):
    arg = arg.replace(EXECUTABLE_SUFFIX_PARAMETER, cipd.EXECUTABLE_SUFFIX)
    replace_slash = False
    if ISOLATED_OUTDIR_PARAMETER in arg:
      if not out_dir:
        raise ValueError(
            'output directory is requested in command, but not provided; '
            'please specify one')
      arg = arg.replace(ISOLATED_OUTDIR_PARAMETER, out_dir)
      replace_slash = True
    if SWARMING_BOT_FILE_PARAMETER in arg:
      if bot_file:
        arg = arg.replace(SWARMING_BOT_FILE_PARAMETER, bot_file)
        replace_slash = True
      else:
        logging.warning('SWARMING_BOT_FILE_PARAMETER found in command, but no '
                        'bot_file specified. Leaving parameter unchanged.')
    if replace_slash:
      # Replace slashes only if parameters are present
      # because of arguments like '${ISOLATED_OUTDIR}/foo/bar'
      arg = arg.replace('/', os.sep)
    return arg

  return [fix(arg) for arg in command]


def get_command_env(tmp_dir, cipd_info, run_dir, env, env_prefixes):
  """Returns full OS environment to run a command in.

  Sets up TEMP, puts directory with cipd binary in front of PATH, exposes
  CIPD_CACHE_DIR env var, and installs all env_prefixes.

  Args:
    tmp_dir: temp directory.
    cipd_info: CipdInfo object is cipd client is used, None if not.
    run_dir: The root directory the isolated tree is mapped in.
    env: environment variables to use
    env_prefixes: {"ENV_KEY": ['cwd', 'relative', 'paths', 'to', 'prepend']}
  """
  out = os.environ.copy()
  for k, v in env.iteritems():
    if not v:
      out.pop(k, None)
    else:
      out[k] = v

  if cipd_info:
    bin_dir = os.path.dirname(cipd_info.client.binary_path)
    out['PATH'] = '%s%s%s' % (_to_str(bin_dir), os.pathsep, out['PATH'])
    out['CIPD_CACHE_DIR'] = _to_str(cipd_info.cache_dir)

  for key, paths in env_prefixes.iteritems():
    assert isinstance(paths, list), paths
    paths = [os.path.normpath(os.path.join(run_dir, p)) for p in paths]
    cur = out.get(key)
    if cur:
      paths.append(cur)
    out[key] = _to_str(os.path.pathsep.join(paths))

  # TMPDIR is specified as the POSIX standard envvar for the temp directory.
  #   * mktemp on linux respects $TMPDIR, not $TMP
  #   * mktemp on OS X SOMETIMES respects $TMPDIR
  #   * chromium's base utils respects $TMPDIR on linux, $TEMP on windows.
  #     Unfortunately at the time of writing it completely ignores all envvars
  #     on OS X.
  #   * python respects TMPDIR, TEMP, and TMP (regardless of platform)
  #   * golang respects TMPDIR on linux+mac, TEMP on windows.
  key = {'win32': 'TEMP'}.get(sys.platform, 'TMPDIR')
  out[key] = _to_str(tmp_dir)

  return out


def run_command(command, cwd, env, hard_timeout, grace_period):
  """Runs the command.

  Returns:
    tuple(process exit code, bool if had a hard timeout)
  """
  logging.info('run_command(%s, %s)' % (command, cwd))

  exit_code = None
  had_hard_timeout = False
  with tools.Profiler('RunTest'):
    proc = None
    had_signal = []
    try:
      # TODO(maruel): This code is imperfect. It doesn't handle well signals
      # during the download phase and there's short windows were things can go
      # wrong.
      def handler(signum, _frame):
        if proc and not had_signal:
          logging.info('Received signal %d', signum)
          had_signal.append(True)
          raise subprocess42.TimeoutExpired(command, None)

      proc = subprocess42.Popen(command, cwd=cwd, env=env, detached=True)
      with subprocess42.set_signal_handler(subprocess42.STOP_SIGNALS, handler):
        try:
          exit_code = proc.wait(hard_timeout or None)
        except subprocess42.TimeoutExpired:
          if not had_signal:
            logging.warning('Hard timeout')
            had_hard_timeout = True
          logging.warning('Sending SIGTERM')
          proc.terminate()

      # Ignore signals in grace period. Forcibly give the grace period to the
      # child process.
      if exit_code is None:
        ignore = lambda *_: None
        with subprocess42.set_signal_handler(subprocess42.STOP_SIGNALS, ignore):
          try:
            exit_code = proc.wait(grace_period or None)
          except subprocess42.TimeoutExpired:
            # Now kill for real. The user can distinguish between the
            # following states:
            # - signal but process exited within grace period,
            #   hard_timed_out will be set but the process exit code will be
            #   script provided.
            # - processed exited late, exit code will be -9 on posix.
            logging.warning('Grace exhausted; sending SIGKILL')
            proc.kill()
      logging.info('Waiting for process exit')
      exit_code = proc.wait()
    except OSError:
      # This is not considered to be an internal error. The executable simply
      # does not exit.
      sys.stderr.write(
          '<The executable does not exist or a dependent library is missing>\n'
          '<Check for missing .so/.dll in the .isolate or GN file>\n'
          '<Command: %s>\n' % command)
      if os.environ.get('SWARMING_TASK_ID'):
        # Give an additional hint when running as a swarming task.
        sys.stderr.write(
            '<See the task\'s page for commands to help diagnose this issue '
            'by reproducing the task locally>\n')
      exit_code = 1
  logging.info(
      'Command finished with exit code %d (%s)',
      exit_code, hex(0xffffffff & exit_code))
  return exit_code, had_hard_timeout


def fetch_and_map(isolated_hash, storage, cache, outdir, use_symlinks):
  """Fetches an isolated tree, create the tree and returns (bundle, stats)."""
  start = time.time()
  bundle = isolateserver.fetch_isolated(
      isolated_hash=isolated_hash,
      storage=storage,
      cache=cache,
      outdir=outdir,
      use_symlinks=use_symlinks)
  return bundle, {
    'duration': time.time() - start,
    'initial_number_items': cache.initial_number_items,
    'initial_size': cache.initial_size,
    'items_cold': base64.b64encode(large.pack(sorted(cache.added))),
    'items_hot': base64.b64encode(
        large.pack(sorted(set(cache.used) - set(cache.added)))),
  }


def link_outputs_to_outdir(run_dir, out_dir, outputs):
  """Links any named outputs to out_dir so they can be uploaded.

  Raises an error if the file already exists in that directory.
  """
  if not outputs:
    return
  isolateserver.create_directories(out_dir, outputs)
  for o in outputs:
    copy_recursively(os.path.join(run_dir, o), os.path.join(out_dir, o))


def copy_recursively(src, dst):
  """Efficiently copies a file or directory from src_dir to dst_dir.

  `item` may be a file, directory, or a symlink to a file or directory.
  All symlinks are replaced with their targets, so the resulting
  directory structure in dst_dir will never have any symlinks.

  To increase speed, copy_recursively hardlinks individual files into the
  (newly created) directory structure if possible, unlike Python's
  shutil.copytree().
  """
  orig_src = src
  try:
    # Replace symlinks with their final target.
    while fs.islink(src):
      res = fs.readlink(src)
      src = os.path.join(os.path.dirname(src), res)
    # TODO(sadafm): Explicitly handle cyclic symlinks.

    # Note that fs.isfile (which is a wrapper around os.path.isfile) throws
    # an exception if src does not exist. A warning will be logged in that case.
    if fs.isfile(src):
      file_path.link_file(dst, src, file_path.HARDLINK_WITH_FALLBACK)
      return

    if not fs.exists(dst):
      os.makedirs(dst)

    for child in fs.listdir(src):
      copy_recursively(os.path.join(src, child), os.path.join(dst, child))

  except OSError as e:
    if e.errno == errno.ENOENT:
      logging.warning('Path %s does not exist or %s is a broken symlink',
                      src, orig_src)
    else:
      logging.info("Couldn't collect output file %s: %s", src, e)


def delete_and_upload(storage, out_dir, leak_temp_dir):
  """Deletes the temporary run directory and uploads results back.

  Returns:
    tuple(outputs_ref, success, stats)
    - outputs_ref: a dict referring to the results archived back to the isolated
          server, if applicable.
    - success: False if something occurred that means that the task must
          forcibly be considered a failure, e.g. zombie processes were left
          behind.
    - stats: uploading stats.
  """
  # Upload out_dir and generate a .isolated file out of this directory. It is
  # only done if files were written in the directory.
  outputs_ref = None
  cold = []
  hot = []
  start = time.time()

  if fs.isdir(out_dir) and fs.listdir(out_dir):
    with tools.Profiler('ArchiveOutput'):
      try:
        results, f_cold, f_hot = isolateserver.archive_files_to_storage(
            storage, [out_dir], None)
        outputs_ref = {
          'isolated': results[0][0],
          'isolatedserver': storage.location,
          'namespace': storage.namespace,
        }
        cold = sorted(i.size for i in f_cold)
        hot = sorted(i.size for i in f_hot)
      except isolateserver.Aborted:
        # This happens when a signal SIGTERM was received while uploading data.
        # There is 2 causes:
        # - The task was too slow and was about to be killed anyway due to
        #   exceeding the hard timeout.
        # - The amount of data uploaded back is very large and took too much
        #   time to archive.
        sys.stderr.write('Received SIGTERM while uploading')
        # Re-raise, so it will be treated as an internal failure.
        raise

  success = False
  try:
    if (not leak_temp_dir and fs.isdir(out_dir) and
        not file_path.rmtree(out_dir)):
      logging.error('Had difficulties removing out_dir %s', out_dir)
    else:
      success = True
  except OSError as e:
    # When this happens, it means there's a process error.
    logging.exception('Had difficulties removing out_dir %s: %s', out_dir, e)
  stats = {
    'duration': time.time() - start,
    'items_cold': base64.b64encode(large.pack(cold)),
    'items_hot': base64.b64encode(large.pack(hot)),
  }
  return outputs_ref, success, stats


def map_and_run(data, constant_run_path):
  """Runs a command with optional isolated input/output.

  Arguments:
  - data: TaskData instance.
  - constant_run_path: TODO

  Returns metadata about the result.
  """
  result = {
    'duration': None,
    'exit_code': None,
    'had_hard_timeout': False,
    'internal_failure': 'run_isolated did not complete properly',
    'stats': {
    # 'isolated': {
    #    'cipd': {
    #      'duration': 0.,
    #      'get_client_duration': 0.,
    #    },
    #    'download': {
    #      'duration': 0.,
    #      'initial_number_items': 0,
    #      'initial_size': 0,
    #      'items_cold': '<large.pack()>',
    #      'items_hot': '<large.pack()>',
    #    },
    #    'upload': {
    #      'duration': 0.,
    #      'items_cold': '<large.pack()>',
    #      'items_hot': '<large.pack()>',
    #    },
    #  },
    },
    # 'cipd_pins': {
    #   'packages': [
    #     {'package_name': ..., 'version': ..., 'path': ...},
    #     ...
    #   ],
    #  'client_package': {'package_name': ..., 'version': ...},
    # },
    'outputs_ref': None,
    'version': 5,
  }

  if data.root_dir:
    file_path.ensure_tree(data.root_dir, 0700)
  elif data.isolate_cache.cache_dir:
    data = data._replace(
        root_dir=os.path.dirname(data.isolate_cache.cache_dir))
  # See comment for these constants.
  # If root_dir is not specified, it is not constant.
  # TODO(maruel): This is not obvious. Change this to become an error once we
  # make the constant_run_path an exposed flag.
  if constant_run_path and data.root_dir:
    run_dir = os.path.join(data.root_dir, ISOLATED_RUN_DIR)
    if os.path.isdir(run_dir):
      file_path.rmtree(run_dir)
    os.mkdir(run_dir, 0700)
  else:
    run_dir = make_temp_dir(ISOLATED_RUN_DIR, data.root_dir)
  # storage should be normally set but don't crash if it is not. This can happen
  # as Swarming task can run without an isolate server.
  out_dir = make_temp_dir(
      ISOLATED_OUT_DIR, data.root_dir) if data.storage else None
  tmp_dir = make_temp_dir(ISOLATED_TMP_DIR, data.root_dir)
  cwd = run_dir
  if data.relative_cwd:
    cwd = os.path.normpath(os.path.join(cwd, data.relative_cwd))
  command = data.command
  try:
    with data.install_packages_fn(run_dir) as cipd_info:
      if cipd_info:
        result['stats']['cipd'] = cipd_info.stats
        result['cipd_pins'] = cipd_info.pins

      if data.isolated_hash:
        isolated_stats = result['stats'].setdefault('isolated', {})
        bundle, isolated_stats['download'] = fetch_and_map(
            isolated_hash=data.isolated_hash,
            storage=data.storage,
            cache=data.isolate_cache,
            outdir=run_dir,
            use_symlinks=data.use_symlinks)
        change_tree_read_only(run_dir, bundle.read_only)
        # Inject the command
        if not command and bundle.command:
          command = bundle.command + data.extra_args
          # Only set the relative directory if the isolated file specified a
          # command, and no raw command was specified.
          if bundle.relative_cwd:
            cwd = os.path.normpath(os.path.join(cwd, bundle.relative_cwd))

      if not command:
        # Handle this as a task failure, not an internal failure.
        sys.stderr.write(
            '<No command was specified!>\n'
            '<Please secify a command when triggering your Swarming task>\n')
        result['exit_code'] = 1
        return result

      if not cwd.startswith(run_dir):
        # Handle this as a task failure, not an internal failure. This is a
        # 'last chance' way to gate against directory escape.
        sys.stderr.write('<Relative CWD is outside of run directory!>\n')
        result['exit_code'] = 1
        return result

      if not os.path.isdir(cwd):
        # Accepts relative_cwd that does not exist.
        os.makedirs(cwd, 0700)

      # If we have an explicit list of files to return, make sure their
      # directories exist now.
      if data.storage and data.outputs:
        isolateserver.create_directories(run_dir, data.outputs)

      with data.install_named_caches(run_dir):
        sys.stdout.flush()
        start = time.time()
        try:
          # Need to switch the default account before 'get_command_env' call,
          # so it can grab correct value of LUCI_CONTEXT env var.
          with set_luci_context_account(data.switch_to_account, tmp_dir):
            env = get_command_env(
                tmp_dir, cipd_info, run_dir, data.env, data.env_prefix)
            command = tools.fix_python_cmd(command, env)
            command = process_command(command, out_dir, data.bot_file)
            file_path.ensure_command_has_abs_path(command, cwd)

            result['exit_code'], result['had_hard_timeout'] = run_command(
                command, cwd, env, data.hard_timeout, data.grace_period)
        finally:
          result['duration'] = max(time.time() - start, 0)

    # We successfully ran the command, set internal_failure back to
    # None (even if the command failed, it's not an internal error).
    result['internal_failure'] = None
  except Exception as e:
    # An internal error occurred. Report accordingly so the swarming task will
    # be retried automatically.
    logging.exception('internal failure: %s', e)
    result['internal_failure'] = str(e)
    on_error.report(None)

  # Clean up
  finally:
    try:
      # Try to link files to the output directory, if specified.
      if out_dir:
        link_outputs_to_outdir(run_dir, out_dir, data.outputs)

      success = False
      if data.leak_temp_dir:
        success = True
        logging.warning(
            'Deliberately leaking %s for later examination', run_dir)
      else:
        # On Windows rmtree(run_dir) call above has a synchronization effect: it
        # finishes only when all task child processes terminate (since a running
        # process locks *.exe file). Examine out_dir only after that call
        # completes (since child processes may write to out_dir too and we need
        # to wait for them to finish).
        if fs.isdir(run_dir):
          try:
            success = file_path.rmtree(run_dir)
          except OSError as e:
            logging.error('Failure with %s', e)
            success = False
          if not success:
            sys.stderr.write(OUTLIVING_ZOMBIE_MSG % ('run', data.grace_period))
            if result['exit_code'] == 0:
              result['exit_code'] = 1
        if fs.isdir(tmp_dir):
          try:
            success = file_path.rmtree(tmp_dir)
          except OSError as e:
            logging.error('Failure with %s', e)
            success = False
          if not success:
            sys.stderr.write(OUTLIVING_ZOMBIE_MSG % ('temp', data.grace_period))
            if result['exit_code'] == 0:
              result['exit_code'] = 1

      # This deletes out_dir if leak_temp_dir is not set.
      if out_dir:
        isolated_stats = result['stats'].setdefault('isolated', {})
        result['outputs_ref'], success, isolated_stats['upload'] = (
            delete_and_upload(data.storage, out_dir, data.leak_temp_dir))
      if not success and result['exit_code'] == 0:
        result['exit_code'] = 1
    except Exception as e:
      # Swallow any exception in the main finally clause.
      if out_dir:
        logging.exception('Leaking out_dir %s: %s', out_dir, e)
      result['internal_failure'] = str(e)
  return result


def run_tha_test(data, result_json):
  """Runs an executable and records execution metadata.

  If isolated_hash is specified, downloads the dependencies in the cache,
  hardlinks them into a temporary directory and runs the command specified in
  the .isolated.

  A temporary directory is created to hold the output files. The content inside
  this directory will be uploaded back to |storage| packaged as a .isolated
  file.

  Arguments:
  - data: TaskData instance.
  - result_json: File path to dump result metadata into. If set, the process
    exit code is always 0 unless an internal error occurred.

  Returns:
    Process exit code that should be used.
  """
  if result_json:
    # Write a json output file right away in case we get killed.
    result = {
      'exit_code': None,
      'had_hard_timeout': False,
      'internal_failure': 'Was terminated before completion',
      'outputs_ref': None,
      'version': 5,
    }
    tools.write_json(result_json, result, dense=True)

  # run_isolated exit code. Depends on if result_json is used or not.
  result = map_and_run(data, True)
  logging.info('Result:\n%s', tools.format_json(result, dense=True))

  if result_json:
    # We've found tests to delete 'work' when quitting, causing an exception
    # here. Try to recreate the directory if necessary.
    file_path.ensure_tree(os.path.dirname(result_json))
    tools.write_json(result_json, result, dense=True)
    # Only return 1 if there was an internal error.
    return int(bool(result['internal_failure']))

  # Marshall into old-style inline output.
  if result['outputs_ref']:
    data = {
      'hash': result['outputs_ref']['isolated'],
      'namespace': result['outputs_ref']['namespace'],
      'storage': result['outputs_ref']['isolatedserver'],
    }
    sys.stdout.flush()
    print(
        '[run_isolated_out_hack]%s[/run_isolated_out_hack]' %
        tools.format_json(data, dense=True))
    sys.stdout.flush()
  return result['exit_code'] or int(bool(result['internal_failure']))


# Yielded by 'install_client_and_packages'.
CipdInfo = collections.namedtuple('CipdInfo', [
  'client',     # cipd.CipdClient object
  'cache_dir',  # absolute path to bot-global cipd tag and instance cache
  'stats',      # dict with stats to return to the server
  'pins',       # dict with installed cipd pins to return to the server
])


@contextlib.contextmanager
def noop_install_packages(_run_dir):
  """Placeholder for 'install_client_and_packages' if cipd is disabled."""
  yield None


def _install_packages(run_dir, cipd_cache_dir, client, packages, timeout):
  """Calls 'cipd ensure' for packages.

  Args:
    run_dir (str): root of installation.
    cipd_cache_dir (str): the directory to use for the cipd package cache.
    client (CipdClient): the cipd client to use
    packages: packages to install, list [(path, package_name, version), ...].
    timeout: max duration in seconds that this function can take.

  Returns: list of pinned packages.  Looks like [
    {
      'path': 'subdirectory',
      'package_name': 'resolved/package/name',
      'version': 'deadbeef...',
    },
    ...
  ]
  """
  package_pins = [None]*len(packages)
  def insert_pin(path, name, version, idx):
    package_pins[idx] = {
      'package_name': name,
      # swarming deals with 'root' as '.'
      'path': path or '.',
      'version': version,
    }

  by_path = collections.defaultdict(list)
  for i, (path, name, version) in enumerate(packages):
    # cipd deals with 'root' as ''
    if path == '.':
      path = ''
    by_path[path].append((name, version, i))

  pins = client.ensure(
    run_dir,
    {
      subdir: [(name, vers) for name, vers, _ in pkgs]
      for subdir, pkgs in by_path.iteritems()
    },
    cache_dir=cipd_cache_dir,
    timeout=timeout,
  )

  for subdir, pin_list in sorted(pins.iteritems()):
    this_subdir = by_path[subdir]
    for i, (name, version) in enumerate(pin_list):
      insert_pin(subdir, name, version, this_subdir[i][2])

  assert None not in package_pins, (packages, pins, package_pins)

  return package_pins


@contextlib.contextmanager
def install_client_and_packages(
    run_dir, packages, service_url, client_package_name,
    client_version, cache_dir, timeout=None):
  """Bootstraps CIPD client and installs CIPD packages.

  Yields CipdClient, stats, client info and pins (as single CipdInfo object).

  Pins and the CIPD client info are in the form of:
    [
      {
        "path": path, "package_name": package_name, "version": version,
      },
      ...
    ]
  (the CIPD client info is a single dictionary instead of a list)

  such that they correspond 1:1 to all input package arguments from the command
  line. These dictionaries make their all the way back to swarming, where they
  become the arguments of CipdPackage.

  If 'packages' list is empty, will bootstrap CIPD client, but won't install
  any packages.

  The bootstrapped client (regardless whether 'packages' list is empty or not),
  will be made available to the task via $PATH.

  Args:
    run_dir (str): root of installation.
    packages: packages to install, list [(path, package_name, version), ...].
    service_url (str): CIPD server url, e.g.
      "https://chrome-infra-packages.appspot.com."
    client_package_name (str): CIPD package name of CIPD client.
    client_version (str): Version of CIPD client.
    cache_dir (str): where to keep cache of cipd clients, packages and tags.
    timeout: max duration in seconds that this function can take.
  """
  assert cache_dir

  timeoutfn = tools.sliding_timeout(timeout)
  start = time.time()

  cache_dir = os.path.abspath(cache_dir)
  cipd_cache_dir = os.path.join(cache_dir, 'cache')  # tag and instance caches
  run_dir = os.path.abspath(run_dir)
  packages = packages or []

  get_client_start = time.time()
  client_manager = cipd.get_client(
      service_url, client_package_name, client_version, cache_dir,
      timeout=timeoutfn())

  with client_manager as client:
    get_client_duration = time.time() - get_client_start

    package_pins = []
    if packages:
      package_pins = _install_packages(
        run_dir, cipd_cache_dir, client, packages, timeoutfn())

    file_path.make_tree_files_read_only(run_dir)

    total_duration = time.time() - start
    logging.info(
        'Installing CIPD client and packages took %d seconds', total_duration)

    yield CipdInfo(
      client=client,
      cache_dir=cipd_cache_dir,
      stats={
        'duration': total_duration,
        'get_client_duration': get_client_duration,
      },
      pins={
        'client_package': {
          'package_name': client.package_name,
          'version': client.instance_id,
        },
        'packages': package_pins,
      })


def clean_caches(isolate_cache, named_cache_manager):
  """Trims isolated and named caches.

  The goal here is to coherently trim both caches, deleting older items
  independent of which container they belong to.
  """
  # TODO(maruel): Trim CIPD cache the same way.
  total = 0
  with named_cache_manager.open():
    oldest_isolated = isolate_cache.get_oldest()
    oldest_named = named_cache_manager.get_oldest()
    trimmers = [
      (
        isolate_cache.trim,
        isolate_cache.get_timestamp(oldest_isolated) if oldest_isolated else 0,
      ),
      (
        named_cache_manager.trim,
        named_cache_manager.get_timestamp(oldest_named) if oldest_named else 0,
      ),
    ]
    trimmers.sort(key=lambda (_, ts): ts)
    # TODO(maruel): This is incorrect, we want to trim 'items' that are strictly
    # the oldest independent of in which cache they live in. Right now, the
    # cache with the oldest item pays the price.
    for trim, _ in trimmers:
      total += trim()
  isolate_cache.cleanup()
  return total


def create_option_parser():
  parser = logging_utils.OptionParserWithLogging(
      usage='%prog <options> [command to run or extra args]',
      version=__version__,
      log_file=RUN_ISOLATED_LOG_FILE)
  parser.add_option(
      '--clean', action='store_true',
      help='Cleans the cache, trimming it necessary and remove corrupted items '
           'and returns without executing anything; use with -v to know what '
           'was done')
  parser.add_option(
      '--no-clean', action='store_true',
      help='Do not clean the cache automatically on startup. This is meant for '
           'bots where a separate execution with --clean was done earlier so '
           'doing it again is redundant')
  parser.add_option(
      '--use-symlinks', action='store_true',
      help='Use symlinks instead of hardlinks')
  parser.add_option(
      '--json',
      help='dump output metadata to json file. When used, run_isolated returns '
           'non-zero only on internal failure')
  parser.add_option(
      '--hard-timeout', type='float', help='Enforce hard timeout in execution')
  parser.add_option(
      '--grace-period', type='float',
      help='Grace period between SIGTERM and SIGKILL')
  parser.add_option(
      '--raw-cmd', action='store_true',
      help='Ignore the isolated command, use the one supplied at the command '
           'line')
  parser.add_option(
      '--relative-cwd',
      help='Ignore the isolated \'relative_cwd\' and use this one instead; '
           'requires --raw-cmd')
  parser.add_option(
      '--env', default=[], action='append',
      help='Environment variables to set for the child process')
  parser.add_option(
      '--env-prefix', default=[], action='append',
      help='Specify a VAR=./path/fragment to put in the environment variable '
           'before executing the command. The path fragment must be relative '
           'to the isolated run directory, and must not contain a `..` token. '
           'The path will be made absolute and prepended to the indicated '
           '$VAR using the OS\'s path separator. Multiple items for the same '
           '$VAR will be prepended in order.')
  parser.add_option(
      '--bot-file',
      help='Path to a file describing the state of the host. The content is '
           'defined by on_before_task() in bot_config.')
  parser.add_option(
      '--switch-to-account',
      help='If given, switches LUCI_CONTEXT to given logical service account '
           '(e.g. "task" or "system") before launching the isolated process.')
  parser.add_option(
      '--output', action='append',
      help='Specifies an output to return. If no outputs are specified, all '
           'files located in $(ISOLATED_OUTDIR) will be returned; '
           'otherwise, outputs in both $(ISOLATED_OUTDIR) and those '
           'specified by --output option (there can be multiple) will be '
           'returned. Note that if a file in OUT_DIR has the same path '
           'as an --output option, the --output version will be returned.')
  parser.add_option(
      '-a', '--argsfile',
      # This is actually handled in parse_args; it's included here purely so it
      # can make it into the help text.
      help='Specify a file containing a JSON array of arguments to this '
           'script. If --argsfile is provided, no other argument may be '
           'provided on the command line.')
  data_group = optparse.OptionGroup(parser, 'Data source')
  data_group.add_option(
      '-s', '--isolated',
      help='Hash of the .isolated to grab from the isolate server.')
  isolateserver.add_isolate_server_options(data_group)
  parser.add_option_group(data_group)

  isolateserver.add_cache_options(parser)

  cipd.add_cipd_options(parser)
  named_cache.add_named_cache_options(parser)

  debug_group = optparse.OptionGroup(parser, 'Debugging')
  debug_group.add_option(
      '--leak-temp-dir',
      action='store_true',
      help='Deliberately leak isolate\'s temp dir for later examination. '
           'Default: %default')
  debug_group.add_option(
      '--root-dir', help='Use a directory instead of a random one')
  parser.add_option_group(debug_group)

  auth.add_auth_options(parser)

  parser.set_defaults(
      cache='cache',
      cipd_cache='cipd_cache',
      named_cache_root='named_caches')
  return parser


def parse_args(args):
  # Create a fake mini-parser just to get out the "-a" command. Note that
  # it's not documented here; instead, it's documented in create_option_parser
  # even though that parser will never actually get to parse it. This is
  # because --argsfile is exclusive with all other options and arguments.
  file_argparse = argparse.ArgumentParser(add_help=False)
  file_argparse.add_argument('-a', '--argsfile')
  (file_args, nonfile_args) = file_argparse.parse_known_args(args)
  if file_args.argsfile:
    if nonfile_args:
      file_argparse.error('Can\'t specify --argsfile with'
                          'any other arguments (%s)' % nonfile_args)
    try:
      with open(file_args.argsfile, 'r') as f:
        args = json.load(f)
    except (IOError, OSError, ValueError) as e:
      # We don't need to error out here - "args" is now empty,
      # so the call below to parser.parse_args(args) will fail
      # and print the full help text.
      print >> sys.stderr, 'Couldn\'t read arguments: %s' % e

  # Even if we failed to read the args, just call the normal parser now since it
  # will print the correct help message.
  parser = create_option_parser()
  options, args = parser.parse_args(args)
  return (parser, options, args)


def main(args):
  # Warning: when --argsfile is used, the strings are unicode instances, when
  # parsed normally, the strings are str instances.
  (parser, options, args) = parse_args(args)

  if not file_path.enable_symlink():
    logging.error('Symlink support is not enabled')

  isolate_cache = isolateserver.process_cache_options(options, trim=False)
  named_cache_manager = named_cache.process_named_cache_options(parser, options)
  if options.clean:
    if options.isolated:
      parser.error('Can\'t use --isolated with --clean.')
    if options.isolate_server:
      parser.error('Can\'t use --isolate-server with --clean.')
    if options.json:
      parser.error('Can\'t use --json with --clean.')
    if options.named_caches:
      parser.error('Can\t use --named-cache with --clean.')
    clean_caches(isolate_cache, named_cache_manager)
    return 0

  if not options.no_clean:
    clean_caches(isolate_cache, named_cache_manager)

  if not options.isolated and not args:
    parser.error('--isolated or command to run is required.')

  auth.process_auth_options(parser, options)

  isolateserver.process_isolate_server_options(
      parser, options, True, False)
  if not options.isolate_server:
    if options.isolated:
      parser.error('--isolated requires --isolate-server')
    if ISOLATED_OUTDIR_PARAMETER in args:
      parser.error(
        '%s in args requires --isolate-server' % ISOLATED_OUTDIR_PARAMETER)

  if options.root_dir:
    options.root_dir = unicode(os.path.abspath(options.root_dir))
  if options.json:
    options.json = unicode(os.path.abspath(options.json))

  if any('=' not in i for i in options.env):
    parser.error(
        '--env required key=value form. value can be skipped to delete '
        'the variable')
  options.env = dict(i.split('=', 1) for i in options.env)

  prefixes = {}
  cwd = os.path.realpath(os.getcwd())
  for item in options.env_prefix:
    if '=' not in item:
      parser.error(
        '--env-prefix %r is malformed, must be in the form `VAR=./path`'
        % item)
    key, opath = item.split('=', 1)
    if os.path.isabs(opath):
      parser.error('--env-prefix %r path is bad, must be relative.' % opath)
    opath = os.path.normpath(opath)
    if not os.path.realpath(os.path.join(cwd, opath)).startswith(cwd):
      parser.error(
        '--env-prefix %r path is bad, must be relative and not contain `..`.'
        % opath)
    prefixes.setdefault(key, []).append(opath)
  options.env_prefix = prefixes

  cipd.validate_cipd_options(parser, options)

  install_packages_fn = noop_install_packages
  if options.cipd_enabled:
    install_packages_fn = lambda run_dir: install_client_and_packages(
        run_dir, cipd.parse_package_args(options.cipd_packages),
        options.cipd_server, options.cipd_client_package,
        options.cipd_client_version, cache_dir=options.cipd_cache)

  @contextlib.contextmanager
  def install_named_caches(run_dir):
    # WARNING: this function depends on "options" variable defined in the outer
    # function.
    caches = [
      (os.path.join(run_dir, unicode(relpath)), name)
      for name, relpath in options.named_caches
    ]
    with named_cache_manager.open():
      for path, name in caches:
        named_cache_manager.install(path, name)
    try:
      yield
    finally:
      # Uninstall each named cache, returning it to the cache pool. If an
      # uninstall fails for a given cache, it will remain in the task's
      # temporary space, get cleaned up by the Swarming bot, and be lost.
      #
      # If the Swarming bot cannot clean up the cache, it will handle it like
      # any other bot file that could not be removed.
      with named_cache_manager.open():
        for path, name in caches:
          try:
            named_cache_manager.uninstall(path, name)
          except named_cache.Error:
            logging.exception('Error while removing named cache %r at %r. '
                              'The cache will be lost.', path, name)

  extra_args = []
  command = []
  if options.raw_cmd:
    command = args
    if options.relative_cwd:
      a = os.path.normpath(os.path.abspath(options.relative_cwd))
      if not a.startswith(os.getcwd()):
        parser.error(
            '--relative-cwd must not try to escape the working directory')
  else:
    if options.relative_cwd:
      parser.error('--relative-cwd requires --raw-cmd')
    extra_args = args

  data = TaskData(
      command=command,
      relative_cwd=options.relative_cwd,
      extra_args=extra_args,
      isolated_hash=options.isolated,
      storage=None,
      isolate_cache=isolate_cache,
      outputs=options.output,
      install_named_caches=install_named_caches,
      leak_temp_dir=options.leak_temp_dir,
      root_dir=_to_unicode(options.root_dir),
      hard_timeout=options.hard_timeout,
      grace_period=options.grace_period,
      bot_file=options.bot_file,
      switch_to_account=options.switch_to_account,
      install_packages_fn=install_packages_fn,
      use_symlinks=options.use_symlinks,
      env=options.env,
      env_prefix=options.env_prefix)
  try:
    if options.isolate_server:
      storage = isolateserver.get_storage(
          options.isolate_server, options.namespace)
      with storage:
        data = data._replace(storage=storage)
        # Hashing schemes used by |storage| and |isolate_cache| MUST match.
        assert storage.hash_algo == isolate_cache.hash_algo
        return run_tha_test(data, options.json)
    return run_tha_test(data, options.json)
  except (cipd.Error, named_cache.Error) as ex:
    print >> sys.stderr, ex.message
    return 1


if __name__ == '__main__':
  subprocess42.inhibit_os_error_reporting()
  # Ensure that we are always running with the correct encoding.
  fix_encoding.fix_encoding()
  sys.exit(main(sys.argv[1:]))
