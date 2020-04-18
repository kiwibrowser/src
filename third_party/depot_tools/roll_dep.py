#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Rolls DEPS controlled dependency.

Works only with git checkout and git dependencies.  Currently this
script will always roll to the tip of to origin/master.
"""

import argparse
import collections
import gclient_eval
import os
import re
import subprocess
import sys

NEED_SHELL = sys.platform.startswith('win')


class Error(Exception):
  pass


class AlreadyRolledError(Error):
  pass


def check_output(*args, **kwargs):
  """subprocess.check_output() passing shell=True on Windows for git."""
  kwargs.setdefault('shell', NEED_SHELL)
  return subprocess.check_output(*args, **kwargs)


def check_call(*args, **kwargs):
  """subprocess.check_call() passing shell=True on Windows for git."""
  kwargs.setdefault('shell', NEED_SHELL)
  subprocess.check_call(*args, **kwargs)


def is_pristine(root, merge_base='origin/master'):
  """Returns True if a git checkout is pristine."""
  cmd = ['git', 'diff', '--ignore-submodules', merge_base]
  return not (check_output(cmd, cwd=root).strip() or
              check_output(cmd + ['--cached'], cwd=root).strip())


def get_log_url(upstream_url, head, master):
  """Returns an URL to read logs via a Web UI if applicable."""
  if re.match(r'https://[^/]*\.googlesource\.com/', upstream_url):
    # gitiles
    return '%s/+log/%s..%s' % (upstream_url, head[:12], master[:12])
  if upstream_url.startswith('https://github.com/'):
    upstream_url = upstream_url.rstrip('/')
    if upstream_url.endswith('.git'):
      upstream_url = upstream_url[:-len('.git')]
    return '%s/compare/%s...%s' % (upstream_url, head[:12], master[:12])
  return None


def should_show_log(upstream_url):
  """Returns True if a short log should be included in the tree."""
  # Skip logs for very active projects.
  if upstream_url.endswith('/v8/v8.git'):
    return False
  if 'webrtc' in upstream_url:
    return False
  return True


def get_gclient_root():
  gclient = os.path.join(
      os.path.dirname(os.path.abspath(__file__)), 'gclient.py')
  return check_output([sys.executable, gclient, 'root']).strip()


def get_deps(root):
  """Returns the path and the content of the DEPS file."""
  deps_path = os.path.join(root, 'DEPS')
  try:
    with open(deps_path, 'rb') as f:
      deps_content = f.read()
  except (IOError, OSError):
    raise Error('Ensure the script is run in the directory '
                'containing DEPS file.')
  return deps_path, deps_content


def generate_commit_message(
    full_dir, dependency, head, roll_to, no_log, log_limit):
  """Creates the commit message for this specific roll."""
  commit_range = '%s..%s' % (head[:9], roll_to[:9])
  upstream_url = check_output(
      ['git', 'config', 'remote.origin.url'], cwd=full_dir).strip()
  log_url = get_log_url(upstream_url, head, roll_to)
  cmd = ['git', 'log', commit_range, '--date=short', '--no-merges']
  logs = check_output(
      cmd + ['--format=%ad %ae %s'], # Args with '=' are automatically quoted.
      cwd=full_dir).rstrip()
  logs = re.sub(r'(?m)^(\d\d\d\d-\d\d-\d\d [^@]+)@[^ ]+( .*)$', r'\1\2', logs)
  lines = logs.splitlines()
  cleaned_lines = [
      l for l in lines
      if not l.endswith('recipe-roller Roll recipe dependencies (trivial).')
  ]
  logs = '\n'.join(cleaned_lines) + '\n'

  nb_commits = len(lines)
  rolls = nb_commits - len(cleaned_lines)
  header = 'Roll %s/ %s (%d commit%s%s)\n\n' % (
      dependency,
      commit_range,
      nb_commits,
      's' if nb_commits > 1 else '',
      ('; %s trivial rolls' % rolls) if rolls else '')
  log_section = ''
  if log_url:
    log_section = log_url + '\n\n'
  log_section += '$ %s ' % ' '.join(cmd)
  log_section += '--format=\'%ad %ae %s\'\n'
  # It is important that --no-log continues to work, as it is used by
  # internal -> external rollers. Please do not remove or break it.
  if not no_log and should_show_log(upstream_url):
    if len(cleaned_lines) > log_limit:
      # Keep the first N log entries.
      logs = ''.join(logs.splitlines(True)[:log_limit]) + '(...)\n'
    log_section += logs
  return header + log_section


def calculate_roll(full_dir, dependency, gclient_dict, roll_to):
  """Calculates the roll for a dependency by processing gclient_dict, and
  fetching the dependency via git.
  """
  head = gclient_eval.GetRevision(gclient_dict, dependency)
  if not head:
    raise Error('%s is unpinned.' % dependency)
  check_call(['git', 'fetch', 'origin', '--quiet'], cwd=full_dir)
  roll_to = check_output(['git', 'rev-parse', roll_to], cwd=full_dir).strip()
  return head, roll_to


def gen_commit_msg(logs, cmdline, rolls, reviewers, bug):
  """Returns the final commit message."""
  commit_msg = ''
  if len(logs) > 1:
    commit_msg = 'Rolling %d dependencies\n\n' % len(logs)
  commit_msg += '\n\n'.join(logs)
  commit_msg += '\nCreated with:\n  ' + cmdline + '\n'
  commit_msg += 'R=%s\n' % ','.join(reviewers) if reviewers else ''
  commit_msg += 'BUG=%s\n' % bug if bug else ''
  return commit_msg


def finalize(commit_msg, deps_path, deps_content, rolls, is_relative, root_dir):
  """Edits the DEPS file, commits it, then uploads a CL."""
  print('Commit message:')
  print('\n'.join('    ' + i for i in commit_msg.splitlines()))

  with open(deps_path, 'wb') as f:
    f.write(deps_content)
  current_dir = os.path.dirname(deps_path)
  check_call(['git', 'add', 'DEPS'], cwd=current_dir)
  check_call(['git', 'commit', '--quiet', '-m', commit_msg], cwd=current_dir)

  # Pull the dependency to the right revision. This is surprising to users
  # otherwise.
  for dependency, (_head, roll_to) in sorted(rolls.iteritems()):
    full_dir = os.path.normpath(os.path.join(root_dir, dependency))
    check_call(['git', 'checkout', '--quiet', roll_to], cwd=full_dir)


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument(
      '--ignore-dirty-tree', action='store_true',
      help='Roll anyways, even if there is a diff.')
  parser.add_argument(
      '-r', '--reviewer',
      help='To specify multiple reviewers, use comma separated list, e.g. '
           '-r joe,jane,john. Defaults to @chromium.org')
  parser.add_argument('-b', '--bug', help='Associate a bug number to the roll')
  # It is important that --no-log continues to work, as it is used by
  # internal -> external rollers. Please do not remove or break it.
  parser.add_argument(
      '--no-log', action='store_true',
      help='Do not include the short log in the commit message')
  parser.add_argument(
      '--log-limit', type=int, default=100,
      help='Trim log after N commits (default: %(default)s)')
  parser.add_argument(
      '--roll-to', default='origin/master',
      help='Specify the new commit to roll to (default: %(default)s)')
  parser.add_argument(
      '--key', action='append', default=[],
      help='Regex(es) for dependency in DEPS file')
  parser.add_argument('dep_path', nargs='+', help='Path(s) to dependency')
  args = parser.parse_args()

  if len(args.dep_path) > 1:
    if args.roll_to != 'origin/master':
      parser.error(
          'Can\'t use multiple paths to roll simultaneously and --roll-to')
    if args.key:
      parser.error(
          'Can\'t use multiple paths to roll simultaneously and --key')
  reviewers = None
  if args.reviewer:
    reviewers = args.reviewer.split(',')
    for i, r in enumerate(reviewers):
      if not '@' in r:
        reviewers[i] = r + '@chromium.org'

  gclient_root = get_gclient_root()
  current_dir = os.getcwd()
  dependencies = sorted(d.rstrip('/').rstrip('\\') for d in args.dep_path)
  cmdline = 'roll-dep ' + ' '.join(dependencies) + ''.join(
      ' --key ' + k for k in args.key)
  try:
    if not args.ignore_dirty_tree and not is_pristine(current_dir):
      raise Error(
          'Ensure %s is clean first (no non-merged commits).' % current_dir)
    # First gather all the information without modifying anything, except for a
    # git fetch.
    deps_path, deps_content = get_deps(current_dir)
    gclient_dict = gclient_eval.Exec(deps_content, True, deps_path)
    is_relative = gclient_dict.get('use_relative_paths', False)
    root_dir = current_dir if is_relative else gclient_root
    rolls = {}
    for dependency in dependencies:
      full_dir = os.path.normpath(os.path.join(root_dir, dependency))
      if not os.path.isdir(full_dir):
        raise Error('Directory not found: %s (%s)' % (dependency, full_dir))
      head, roll_to = calculate_roll(
          full_dir, dependency, gclient_dict, args.roll_to)
      if roll_to == head:
        if len(dependencies) == 1:
          raise AlreadyRolledError('No revision to roll!')
        print('%s: Already at latest commit %s' % (dependency, roll_to))
      else:
        print(
            '%s: Rolling from %s to %s' % (dependency, head[:10], roll_to[:10]))
        rolls[dependency] = (head, roll_to)

    logs = []
    for dependency, (head, roll_to) in sorted(rolls.iteritems()):
      full_dir = os.path.normpath(os.path.join(root_dir, dependency))
      log = generate_commit_message(
          full_dir, dependency, head, roll_to, args.no_log, args.log_limit)
      logs.append(log)
      gclient_eval.SetRevision(gclient_dict, dependency, roll_to)

    deps_content = gclient_eval.RenderDEPSFile(gclient_dict)

    commit_msg = gen_commit_msg(logs, cmdline, rolls, reviewers, args.bug)
    finalize(commit_msg, deps_path, deps_content, rolls, is_relative, root_dir)
  except Error as e:
    sys.stderr.write('error: %s\n' % e)
    return 2 if isinstance(e, AlreadyRolledError) else 1

  print('')
  if not reviewers:
    print('You forgot to pass -r, make sure to insert a R=foo@example.com line')
    print('to the commit description before emailing.')
    print('')
  print('Run:')
  print('  git cl upload --send-mail')
  return 0


if __name__ == '__main__':
  sys.exit(main())
