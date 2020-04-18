#!/usr/bin/env python
# Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.


"""
Script to automatically add specified chromium dependency to WebRTC repo.

If you want to add new chromium owned dependency foo to the WebRTC repo
you have to run this tool like this:
./checkin_chromium_deps.py -d foo

It will check in dependency into third_party directory and will add it into
git index. Also it will update chromium dependencies list with new dependency
to insure that it will be correctly auto updated in future.
"""

import argparse
import errno
import json
import logging
import os.path
import shutil
import subprocess
import sys
import tempfile

REMOTE_URL = 'https://chromium.googlesource.com/chromium/src/third_party'

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CHECKOUT_SRC_DIR = os.path.realpath(os.path.join(SCRIPT_DIR, os.pardir,
                                                 os.pardir))


class DependencyAlreadyCheckedIn(Exception):
  pass


class DependencyNotFound(Exception):
  pass


class Config(object):

  def __init__(self, src_root, remote_url, temp_dir):
    self.src_root = src_root
    self.dependencies_file = os.path.join(self.src_root,
                                          'THIRD_PARTY_CHROMIUM_DEPS.json')
    self.deps_file = os.path.join(self.src_root, 'DEPS')
    self.third_party_dir = os.path.join(self.src_root, 'third_party')
    self.remote_url = remote_url
    self.temp_dir = temp_dir


def VarLookup(local_scope):
  return lambda var_name: local_scope['vars'][var_name]


def ParseDepsDict(deps_content):
  local_scope = {}
  global_scope = {
    'Var': VarLookup(local_scope),
    'deps_os': {},
  }
  exec (deps_content, global_scope, local_scope)
  return local_scope


def ParseLocalDepsFile(filename):
  with open(filename, 'rb') as f:
    deps_content = f.read()
  return ParseDepsDict(deps_content)


def RunCommand(command, working_dir=None, ignore_exit_code=False,
    extra_env=None, input_data=None):
  """Runs a command and returns the output from that command.

  If the command fails (exit code != 0), the function will exit the process.

  Returns:
    A tuple containing the stdout and stderr outputs as strings.
  """
  working_dir = working_dir or CHECKOUT_SRC_DIR
  logging.debug('CMD: %s CWD: %s', ' '.join(command), working_dir)
  env = os.environ.copy()
  if extra_env:
    assert all(type(value) == str for value in extra_env.values())
    logging.debug('extra env: %s', extra_env)
    env.update(extra_env)
  p = subprocess.Popen(command,
                       stdin=subprocess.PIPE,
                       stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE, env=env,
                       cwd=working_dir, universal_newlines=True)
  std_output, err_output = p.communicate(input_data)
  p.stdout.close()
  p.stderr.close()
  if not ignore_exit_code and p.returncode != 0:
    logging.error('Command failed: %s\n'
                  'stdout:\n%s\n'
                  'stderr:\n%s\n', ' '.join(command), std_output, err_output)
    sys.exit(p.returncode)
  return std_output, err_output


def LoadThirdPartyRevision(deps_file):
  logging.debug('Loading chromium third_party revision from %s', deps_file)
  webrtc_deps = ParseLocalDepsFile(deps_file)
  return webrtc_deps['vars']['chromium_third_party_revision']


def CheckoutRequiredDependency(dep_name, config):
  third_party_revision = LoadThirdPartyRevision(config.deps_file)

  logging.debug('Initializing git repo in %s...', config.temp_dir)
  RunCommand(['git', 'init'], working_dir=config.temp_dir)

  logging.debug('Adding remote to %s. It may take some time...',
                config.remote_url)
  RunCommand(['git', 'remote', 'add', '-f', 'origin', config.remote_url],
             working_dir=config.temp_dir)

  logging.debug('Configuring sparse checkout...')
  RunCommand(['git', 'config', 'core.sparseCheckout', 'true'],
             working_dir=config.temp_dir)
  sparse_checkout_config_path = os.path.join(config.temp_dir, '.git', 'info',
                                             'sparse-checkout')
  with open(sparse_checkout_config_path, 'wb') as f:
    f.write(dep_name)

  logging.debug('Pulling changes...')
  _, stderr = RunCommand(['git', 'pull', 'origin', 'master'],
                         working_dir=config.temp_dir,
                         ignore_exit_code=True)
  if "Sparse checkout leaves no entry on working directory" in stderr:
    # There are no such dependency in chromium third_party
    raise DependencyNotFound(
        "Dependency %s not found in chromium repo" % dep_name)

  logging.debug('Switching to revision %s...', third_party_revision)
  RunCommand(['git', 'checkout', third_party_revision],
             working_dir=config.temp_dir)
  return os.path.join(config.temp_dir, dep_name)


def CopyDependency(dep_name, source_path, third_party_dir):
  dest_path = os.path.join(third_party_dir, dep_name)
  logging.debug('Copying dependency from %s to %s...', source_path, dest_path)
  shutil.copytree(source_path, dest_path)


def AppendToChromiumOwnedDependenciesList(dep_name, dep_file):
  with open(dep_file, 'rb') as f:
    data = json.load(f)
    dep_list = data.get('dependencies', [])
    dep_list.append(dep_name)
    data['dependencies'] = dep_list

  with open(dep_file, 'wb') as f:
    json.dump(data, f, indent=2, sort_keys=True, separators=(',', ': '))


def AddToGitIndex(dep_name, config):
  logging.debug('Adding required changes to git index and commit set...')
  dest_path = os.path.join(config.third_party_dir, dep_name)
  RunCommand(['git', 'add', dest_path], working_dir=config.src_root)
  RunCommand(['git', 'add', config.dependencies_file],
             working_dir=config.src_root)


def CheckinDependency(dep_name, config):
  dep_path = CheckoutRequiredDependency(dep_name, config)
  CopyDependency(dep_name, dep_path, config.third_party_dir)
  AppendToChromiumOwnedDependenciesList(dep_name, config.dependencies_file)
  AddToGitIndex(dep_name, config)
  logging.info('Dependency checked into current working tree and added into\n'
               'git index. You have to commit generated changes and\n'
               'file the CL to finish adding the dependency')


def DefaultConfig(temp_dir):
  return Config(CHECKOUT_SRC_DIR, REMOTE_URL, temp_dir)


def CheckinDependencyWithNewTempDir(dep_name):
  temp_dir = tempfile.mkdtemp()
  try:
    logging.info('Using temp directory: %s', temp_dir)
    config = DefaultConfig(temp_dir)
    CheckinDependency(dep_name, config)
  finally:
    shutil.rmtree(temp_dir)


def CheckDependencyNotCheckedIn(dep_name):
  config = Config(CHECKOUT_SRC_DIR, REMOTE_URL, '')
  with open(config.dependencies_file, 'rb') as f:
    data = json.load(f)
    dep_list = data.get('dependencies', [])
    if dep_name in dep_list:
      raise DependencyAlreadyCheckedIn("Dependency %s has been already checked "
                                       "into WebRTC repo" % dep_name)
  if dep_name in os.listdir(config.third_party_dir):
    raise DependencyAlreadyCheckedIn("Directory for dependency %s already "
                                     "exists in third_party" % dep_name)


def main():
  p = argparse.ArgumentParser()
  p.add_argument('-d', '--dependency', required=True,
                 help='Name of chromium dependency to check in.')
  p.add_argument('--temp-dir',
                 help='Temp working directory to use. By default the one '
                      'provided via tempfile will be used')
  p.add_argument('-v', '--verbose', action='store_true', default=False,
                 help='Be extra verbose in printing of log messages.')
  args = p.parse_args()

  if args.verbose:
    logging.basicConfig(level=logging.DEBUG)
  else:
    logging.basicConfig(level=logging.INFO)

  CheckDependencyNotCheckedIn(args.dependency)

  if args.temp_dir:
    if not os.path.exists(args.temp_dir):
      # Raise system error "No such file or directory"
      raise OSError(
          errno.ENOENT, os.strerror(errno.ENOENT), args.temp_dir)
    config = DefaultConfig(args.temp_dir)
    CheckinDependency(args.dependency, config)
  else:
    CheckinDependencyWithNewTempDir(args.dependency)

  return 0


if __name__ == '__main__':
  sys.exit(main())
