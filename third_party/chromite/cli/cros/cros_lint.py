# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Run lint checks on the specified files."""

from __future__ import print_function

import functools
import multiprocessing
import os
import re
import sys

from chromite.lib import constants
from chromite.cli import command
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import git
from chromite.lib import parallel


# Extract a script's shebang.
SHEBANG_RE = re.compile(r'^#!\s*([^\s]+)(\s+([^\s]+))?')


PYTHON_EXTENSIONS = frozenset(['.py'])

# Note these are defined to keep in line with cpplint.py. Technically, we could
# include additional ones, but cpplint.py would just filter them out.
CPP_EXTENSIONS = frozenset(['.cc', '.cpp', '.h'])


def _GetProjectPath(path):
  """Find the absolute path of the git checkout that contains |path|."""
  if git.FindRepoCheckoutRoot(path):
    manifest = git.ManifestCheckout.Cached(path)
    return manifest.FindCheckoutFromPath(path).GetPath(absolute=True)
  else:
    # Maybe they're running on a file outside of a checkout.
    # e.g. cros lint ~/foo.py /tmp/test.py
    return os.path.dirname(path)


def _GetPylintrc(path):
  """Locate the pylintrc file that applies to |path|."""
  path = os.path.realpath(path)
  project_path = _GetProjectPath(path)
  parent = os.path.dirname(path)
  while project_path and parent.startswith(project_path):
    pylintrc = os.path.join(parent, 'pylintrc')
    if os.path.isfile(pylintrc):
      break
    parent = os.path.dirname(parent)

  if project_path is None or not os.path.isfile(pylintrc):
    pylintrc = os.path.join(constants.SOURCE_ROOT, 'chromite', 'pylintrc')

  return pylintrc


def _GetPylintGroups(paths):
  """Return a dictionary mapping pylintrc files to lists of paths."""
  groups = {}
  for path in paths:
    pylintrc = _GetPylintrc(path)
    if pylintrc:
      groups.setdefault(pylintrc, []).append(path)
  return groups


def _GetPythonPath(paths):
  """Return the set of Python library paths to use."""
  return sys.path + [
      # Add the Portage installation inside the chroot to the Python path.
      # This ensures that scripts that need to import portage can do so.
      os.path.join(constants.SOURCE_ROOT, 'chroot', 'usr', 'lib', 'portage',
                   'pym'),

      # Scripts outside of chromite expect the scripts in src/scripts/lib to
      # be importable.
      os.path.join(constants.CROSUTILS_DIR, 'lib'),

      # Allow platform projects to be imported by name (e.g. crostestutils).
      os.path.join(constants.SOURCE_ROOT, 'src', 'platform'),

      # Ideally we'd modify meta_path in pylint to handle our virtual chromite
      # module, but that's not possible currently.  We'll have to deal with
      # that at some point if we want `cros lint` to work when the dir is not
      # named 'chromite'.
      constants.SOURCE_ROOT,

      # Also allow scripts to import from their current directory.
  ] + list(set(os.path.dirname(x) for x in paths))


# The mapping between the "cros lint" --output-format flag and cpplint.py
# --output flag.
CPPLINT_OUTPUT_FORMAT_MAP = {
    'colorized': 'emacs',
    'msvs': 'vs7',
    'parseable': 'emacs',
}


def _LinterRunCommand(cmd, debug, **kwargs):
  """Run the linter with common RunCommand args set as higher levels expect."""
  return cros_build_lib.RunCommand(cmd, error_code_ok=True, print_cmd=debug,
                                   debug_level=logging.NOTICE, **kwargs)


def _CpplintFile(path, output_format, debug):
  """Returns result of running cpplint on |path|."""
  cmd = [os.path.join(constants.DEPOT_TOOLS_DIR, 'cpplint.py')]
  if output_format != 'default':
    cmd.append('--output=%s' % CPPLINT_OUTPUT_FORMAT_MAP[output_format])
  cmd.append(path)
  return _LinterRunCommand(cmd, debug)


def _PylintFile(path, output_format, debug):
  """Returns result of running pylint on |path|."""
  pylint = os.path.join(constants.DEPOT_TOOLS_DIR, 'pylint')
  pylintrc = _GetPylintrc(path)
  cmd = [pylint, '--rcfile=%s' % pylintrc]
  if output_format != 'default':
    cmd.append('--output-format=%s' % output_format)
  cmd.append(path)
  extra_env = {'PYTHONPATH': ':'.join(_GetPythonPath([path]))}
  return _LinterRunCommand(cmd, debug, extra_env=extra_env)


def _ShellFile(path, _output_format, debug):
  """Returns result of running lint checks on |path|."""
  # TODO: Try using `checkbashisms`.
  return _LinterRunCommand(['bash', '-n', path], debug)


def _BreakoutDataByLinter(map_to_return, path):
  """Maps a linter method to the content of the |path|."""
  # Detect by content of the file itself.
  try:
    with open(path) as fp:
      # We read 128 bytes because that's the Linux kernel's current limit.
      # Look for BINPRM_BUF_SIZE in fs/binfmt_script.c.
      data = fp.read(128)

      if not data.startswith('#!'):
        # If the file doesn't have a shebang, nothing to do.
        return

      m = SHEBANG_RE.match(data)
      if m:
        prog = m.group(1)
        if prog == '/usr/bin/env':
          prog = m.group(3)
        basename = os.path.basename(prog)
        if basename.startswith('python'):
          pylint_list = map_to_return.setdefault(_PylintFile, [])
          pylint_list.append(path)
        elif basename in ('sh', 'dash', 'bash'):
          shlint_list = map_to_return.setdefault(_ShellFile, [])
          shlint_list.append(path)
  except IOError as e:
    logging.debug('%s: reading initial data failed: %s', path, e)


def _BreakoutFilesByLinter(files):
  """Maps a linter method to the list of files to lint."""
  map_to_return = {}
  for f in files:
    extension = os.path.splitext(f)[1]
    if extension in PYTHON_EXTENSIONS:
      pylint_list = map_to_return.setdefault(_PylintFile, [])
      pylint_list.append(f)
    elif extension in CPP_EXTENSIONS:
      cpplint_list = map_to_return.setdefault(_CpplintFile, [])
      cpplint_list.append(f)
    elif os.path.isfile(f):
      _BreakoutDataByLinter(map_to_return, f)

  return map_to_return


def _Dispatcher(errors, output_format, debug, linter, path):
  """Call |linter| on |path| and take care of coalescing exit codes/output."""
  result = linter(path, output_format, debug)
  if result.returncode:
    with errors.get_lock():
      errors.value += 1


@command.CommandDecorator('lint')
class LintCommand(command.CliCommand):
  """Run lint checks on the specified files."""

  EPILOG = """
Right now, only supports cpplint and pylint. We may also in the future
run other checks (e.g. pyflakes, etc.)
"""

  # The output formats supported by cros lint.
  OUTPUT_FORMATS = ('default', 'colorized', 'msvs', 'parseable')

  @classmethod
  def AddParser(cls, parser):
    super(LintCommand, cls).AddParser(parser)
    parser.add_argument('files', help='Files to lint', nargs='*')
    parser.add_argument('--output', default='default',
                        choices=LintCommand.OUTPUT_FORMATS,
                        help='Output format to pass to the linters. Supported '
                        'formats are: default (no option is passed to the '
                        'linter), colorized, msvs (Visual Studio) and '
                        'parseable.')

  def Run(self):
    files = self.options.files
    if not files:
      # Running with no arguments is allowed to make the repo upload hook
      # simple, but print a warning so that if someone runs this manually
      # they are aware that nothing was linted.
      logging.warning('No files provided to lint.  Doing nothing.')

    errors = multiprocessing.Value('i')
    linter_map = _BreakoutFilesByLinter(files)
    dispatcher = functools.partial(_Dispatcher, errors,
                                   self.options.output, self.options.debug)

    # Special case one file as it's common -- faster to avoid parallel startup.
    if sum([len(x) for _, x in linter_map.iteritems()]) == 1:
      linter, files = linter_map.items()[0]
      dispatcher(linter, files[0])
    else:
      # Run the linter in parallel on the files.
      with parallel.BackgroundTaskRunner(dispatcher) as q:
        for linter, files in linter_map.iteritems():
          for path in files:
            q.put([linter, path])

    if errors.value:
      logging.error('linter found errors in %i files', errors.value)
      sys.exit(1)
