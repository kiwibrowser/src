#!/usr/bin/env python
# Copyright (c) 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""Wrapper for updating and calling infra.git tools.

This tool does a two things:
* Maintains a infra.git checkout pinned at "deployed" in the home dir
* Acts as an alias to infra.tools.*
* Acts as an alias to infra.git/cipd/<executable>
"""

# TODO(hinoka,iannucci): Pre-pack infra tools in cipd package with vpython spec.

import argparse
import sys
import os
import re

import subprocess2 as subprocess


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
GCLIENT = os.path.join(SCRIPT_DIR, 'gclient.py')
TARGET_DIR = os.path.expanduser(os.path.join('~', '.chrome-infra'))
INFRA_DIR = os.path.join(TARGET_DIR, 'infra')


def get_git_rev(target, branch):
  return subprocess.check_output(
      ['git', 'log', '--format=%B', '-n1', branch], cwd=target)


def need_to_update(branch):
  """Checks to see if we need to update the ~/.chrome-infra/infra checkout."""
  try:
    cmd = [sys.executable, GCLIENT, 'revinfo']
    subprocess.check_call(
        cmd, cwd=os.path.join(TARGET_DIR), stdout=subprocess.VOID)
  except subprocess.CalledProcessError:
    return True  # Gclient failed, definitely need to update.
  except OSError:
    return True  # Gclient failed, definitely need to update.

  if not os.path.isdir(INFRA_DIR):
    return True

  local_rev = get_git_rev(INFRA_DIR, 'HEAD')

  subprocess.check_call(
      ['git', 'fetch', 'origin'], cwd=INFRA_DIR,
      stdout=subprocess.VOID, stderr=subprocess.STDOUT)
  origin_rev = get_git_rev(INFRA_DIR, 'origin/%s' % (branch,))
  return origin_rev != local_rev


def ensure_infra(branch):
  """Ensures that infra.git is present in ~/.chrome-infra."""
  sys.stderr.write(
      'Fetching infra@%s into %s, may take a couple of minutes...' % (
      branch, TARGET_DIR))
  sys.stderr.flush()
  if not os.path.isdir(TARGET_DIR):
    os.mkdir(TARGET_DIR)
  if not os.path.exists(os.path.join(TARGET_DIR, '.gclient')):
    subprocess.check_call(
        [sys.executable, os.path.join(SCRIPT_DIR, 'fetch.py'), 'infra'],
        cwd=TARGET_DIR,
        stdout=subprocess.VOID)
  subprocess.check_call(
      [sys.executable, GCLIENT, 'sync', '--revision', 'origin/%s' % (branch,)],
      cwd=TARGET_DIR,
      stdout=subprocess.VOID)
  sys.stderr.write(' done.\n')
  sys.stderr.flush()


def is_exe(filename):
  """Given a full filepath, return true if the file is an executable."""
  if sys.platform.startswith('win'):
    return filename.endswith('.exe')
  else:
    return os.path.isfile(filename) and os.access(filename, os.X_OK)


def get_available_tools():
  """Returns a tuple of (list of infra tools, list of cipd tools)"""
  infra_tools = []
  cipd_tools = []
  starting = os.path.join(TARGET_DIR, 'infra', 'infra', 'tools')
  for root, _, files in os.walk(starting):
    if '__main__.py' in files:
      infra_tools.append(root[len(starting)+1:].replace(os.path.sep, '.'))
  cipd = os.path.join(TARGET_DIR, 'infra', 'cipd')
  for fn in os.listdir(cipd):
    if is_exe(os.path.join(cipd, fn)):
      cipd_tools.append(fn)
  return (sorted(infra_tools), sorted(cipd_tools))


def usage():
  infra_tools, cipd_tools = get_available_tools()
  print """usage: cit.py <name of tool> [args for tool]

  Wrapper for maintaining and calling tools in:
    "infra.git/run.py infra.tools.*"
    "infra.git/cipd/*"

  Available infra tools are:"""
  for tool in infra_tools:
    print '  * %s' % tool

  print """
  Available cipd tools are:"""
  for tool in cipd_tools:
    print '  * %s' % tool


def run(args):
  if not args:
    return usage()

  tool_name = args[0]
  # Check to see if it is a infra tool first.
  infra_dir = os.path.join(
    TARGET_DIR, 'infra', 'infra', 'tools', *tool_name.split('.'))
  cipd_file = os.path.join(TARGET_DIR, 'infra', 'cipd', tool_name)
  if sys.platform.startswith('win'):
    cipd_file += '.exe'
  if (os.path.isdir(infra_dir)
      and os.path.isfile(os.path.join(infra_dir, '__main__.py'))):
    cmd = [
        sys.executable, os.path.join(TARGET_DIR, 'infra', 'run.py'),
        'infra.tools.%s' % tool_name]
  elif os.path.isfile(cipd_file) and is_exe(cipd_file):
    cmd = [cipd_file]
  else:
    print >>sys.stderr, 'Unknown tool "%s"' % tool_name
    return usage()

  # Add the remaining arguments.
  cmd.extend(args[1:])
  return subprocess.call(cmd)


def main():
  parser = argparse.ArgumentParser("Chrome Infrastructure CLI.")
  parser.add_argument('-b', '--infra-branch', default='cit',
      help="The name of the 'infra' branch to use (default is %(default)s).")
  parser.add_argument('args', nargs=argparse.REMAINDER)

  args, extras = parser.parse_known_args()
  if args.args and args.args[0] == '--':
    args.args.pop(0)
  if extras:
    args.args = extras + args.args

  if need_to_update(args.infra_branch):
    ensure_infra(args.infra_branch)
  return run(args.args)

if __name__ == '__main__':
  sys.exit(main())
