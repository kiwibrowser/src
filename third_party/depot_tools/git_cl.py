#!/usr/bin/env python
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Copyright (C) 2008 Evan Martin <martine@danga.com>

"""A git-command for integrating reviews on Rietveld and Gerrit."""

from __future__ import print_function

from distutils.version import LooseVersion
from multiprocessing.pool import ThreadPool
import base64
import collections
import contextlib
import datetime
import fnmatch
import httplib
import itertools
import json
import logging
import multiprocessing
import optparse
import os
import re
import shutil
import stat
import sys
import tempfile
import textwrap
import urllib
import urllib2
import urlparse
import uuid
import webbrowser
import zlib

try:
  import readline  # pylint: disable=import-error,W0611
except ImportError:
  pass

from third_party import colorama
from third_party import httplib2
from third_party import upload
import auth
import checkout
import clang_format
import dart_format
import setup_color
import fix_encoding
import gclient_utils
import gerrit_util
import git_cache
import git_common
import git_footers
import owners
import owners_finder
import presubmit_support
import rietveld
import scm
import split_cl
import subcommand
import subprocess2
import watchlists

__version__ = '2.0'

COMMIT_BOT_EMAIL = 'commit-bot@chromium.org'
DEFAULT_SERVER = 'https://codereview.chromium.org'
POSTUPSTREAM_HOOK = '.git/hooks/post-cl-land'
DESCRIPTION_BACKUP_FILE = '~/.git_cl_description_backup'
REFS_THAT_ALIAS_TO_OTHER_REFS = {
    'refs/remotes/origin/lkgr': 'refs/remotes/origin/master',
    'refs/remotes/origin/lkcr': 'refs/remotes/origin/master',
}

# Valid extensions for files we want to lint.
DEFAULT_LINT_REGEX = r"(.*\.cpp|.*\.cc|.*\.h)"
DEFAULT_LINT_IGNORE_REGEX = r"$^"

# Buildbucket master name prefix.
MASTER_PREFIX = 'master.'

# Shortcut since it quickly becomes redundant.
Fore = colorama.Fore

# Initialized in main()
settings = None

# Used by tests/git_cl_test.py to add extra logging.
# Inside the weirdly failing test, add this:
# >>> self.mock(git_cl, '_IS_BEING_TESTED', True)
# And scroll up to see the stack trace printed.
_IS_BEING_TESTED = False


def DieWithError(message, change_desc=None):
  if change_desc:
    SaveDescriptionBackup(change_desc)

  print(message, file=sys.stderr)
  sys.exit(1)


def SaveDescriptionBackup(change_desc):
  backup_path = os.path.expanduser(DESCRIPTION_BACKUP_FILE)
  print('\nError after CL description prompt -- saving description to %s\n' %
        backup_path)
  backup_file = open(backup_path, 'w')
  backup_file.write(change_desc.description)
  backup_file.close()


def GetNoGitPagerEnv():
  env = os.environ.copy()
  # 'cat' is a magical git string that disables pagers on all platforms.
  env['GIT_PAGER'] = 'cat'
  return env


def RunCommand(args, error_ok=False, error_message=None, shell=False, **kwargs):
  try:
    return subprocess2.check_output(args, shell=shell, **kwargs)
  except subprocess2.CalledProcessError as e:
    logging.debug('Failed running %s', args)
    if not error_ok:
      DieWithError(
          'Command "%s" failed.\n%s' % (
            ' '.join(args), error_message or e.stdout or ''))
    return e.stdout


def RunGit(args, **kwargs):
  """Returns stdout."""
  return RunCommand(['git'] + args, **kwargs)


def RunGitWithCode(args, suppress_stderr=False):
  """Returns return code and stdout."""
  if suppress_stderr:
    stderr = subprocess2.VOID
  else:
    stderr = sys.stderr
  try:
    (out, _), code = subprocess2.communicate(['git'] + args,
                                             env=GetNoGitPagerEnv(),
                                             stdout=subprocess2.PIPE,
                                             stderr=stderr)
    return code, out
  except subprocess2.CalledProcessError as e:
    logging.debug('Failed running %s', ['git'] + args)
    return e.returncode, e.stdout


def RunGitSilent(args):
  """Returns stdout, suppresses stderr and ignores the return code."""
  return RunGitWithCode(args, suppress_stderr=True)[1]


def IsGitVersionAtLeast(min_version):
  prefix = 'git version '
  version = RunGit(['--version']).strip()
  return (version.startswith(prefix) and
      LooseVersion(version[len(prefix):]) >= LooseVersion(min_version))


def BranchExists(branch):
  """Return True if specified branch exists."""
  code, _ = RunGitWithCode(['rev-parse', '--verify', branch],
                           suppress_stderr=True)
  return not code


def time_sleep(seconds):
  # Use this so that it can be mocked in tests without interfering with python
  # system machinery.
  import time  # Local import to discourage others from importing time globally.
  return time.sleep(seconds)


def ask_for_data(prompt):
  try:
    return raw_input(prompt)
  except KeyboardInterrupt:
    # Hide the exception.
    sys.exit(1)


def confirm_or_exit(prefix='', action='confirm'):
  """Asks user to press enter to continue or press Ctrl+C to abort."""
  if not prefix or prefix.endswith('\n'):
    mid = 'Press'
  elif prefix.endswith('.') or prefix.endswith('?'):
    mid = ' Press'
  elif prefix.endswith(' '):
    mid = 'press'
  else:
    mid = ' press'
  ask_for_data('%s%s Enter to %s, or Ctrl+C to abort' % (prefix, mid, action))


def ask_for_explicit_yes(prompt):
  """Returns whether user typed 'y' or 'yes' to confirm the given prompt"""
  result = ask_for_data(prompt + ' [Yes/No]: ').lower()
  while True:
    if 'yes'.startswith(result):
      return True
    if 'no'.startswith(result):
      return False
    result = ask_for_data('Please, type yes or no: ').lower()


def _git_branch_config_key(branch, key):
  """Helper method to return Git config key for a branch."""
  assert branch, 'branch name is required to set git config for it'
  return 'branch.%s.%s' % (branch, key)


def _git_get_branch_config_value(key, default=None, value_type=str,
                                 branch=False):
  """Returns git config value of given or current branch if any.

  Returns default in all other cases.
  """
  assert value_type in (int, str, bool)
  if branch is False:  # Distinguishing default arg value from None.
    branch = GetCurrentBranch()

  if not branch:
    return default

  args = ['config']
  if value_type == bool:
    args.append('--bool')
  # git config also has --int, but apparently git config suffers from integer
  # overflows (http://crbug.com/640115), so don't use it.
  args.append(_git_branch_config_key(branch, key))
  code, out = RunGitWithCode(args)
  if code == 0:
    value = out.strip()
    if value_type == int:
      return int(value)
    if value_type == bool:
      return bool(value.lower() == 'true')
    return value
  return default


def _git_set_branch_config_value(key, value, branch=None, **kwargs):
  """Sets the value or unsets if it's None of a git branch config.

  Valid, though not necessarily existing, branch must be provided,
  otherwise currently checked out branch is used.
  """
  if not branch:
    branch = GetCurrentBranch()
    assert branch, 'a branch name OR currently checked out branch is required'
  args = ['config']
  # Check for boolean first, because bool is int, but int is not bool.
  if value is None:
    args.append('--unset')
  elif isinstance(value, bool):
    args.append('--bool')
    value = str(value).lower()
  else:
    # git config also has --int, but apparently git config suffers from integer
    # overflows (http://crbug.com/640115), so don't use it.
    value = str(value)
  args.append(_git_branch_config_key(branch, key))
  if value is not None:
    args.append(value)
  RunGit(args, **kwargs)


def _get_committer_timestamp(commit):
  """Returns Unix timestamp as integer of a committer in a commit.

  Commit can be whatever git show would recognize, such as HEAD, sha1 or ref.
  """
  # Git also stores timezone offset, but it only affects visual display,
  # actual point in time is defined by this timestamp only.
  return int(RunGit(['show', '-s', '--format=%ct', commit]).strip())


def _git_amend_head(message, committer_timestamp):
  """Amends commit with new message and desired committer_timestamp.

  Sets committer timezone to UTC.
  """
  env = os.environ.copy()
  env['GIT_COMMITTER_DATE'] = '%d+0000' % committer_timestamp
  return RunGit(['commit', '--amend', '-m', message], env=env)


def _get_properties_from_options(options):
  properties = dict(x.split('=', 1) for x in options.properties)
  for key, val in properties.iteritems():
    try:
      properties[key] = json.loads(val)
    except ValueError:
      pass  # If a value couldn't be evaluated, treat it as a string.
  return properties


def _prefix_master(master):
  """Convert user-specified master name to full master name.

  Buildbucket uses full master name(master.tryserver.chromium.linux) as bucket
  name, while the developers always use shortened master name
  (tryserver.chromium.linux) by stripping off the prefix 'master.'. This
  function does the conversion for buildbucket migration.
  """
  if master.startswith(MASTER_PREFIX):
    return master
  return '%s%s' % (MASTER_PREFIX, master)


def _unprefix_master(bucket):
  """Convert bucket name to shortened master name.

  Buildbucket uses full master name(master.tryserver.chromium.linux) as bucket
  name, while the developers always use shortened master name
  (tryserver.chromium.linux) by stripping off the prefix 'master.'. This
  function does the conversion for buildbucket migration.
  """
  if bucket.startswith(MASTER_PREFIX):
    return bucket[len(MASTER_PREFIX):]
  return bucket


def _buildbucket_retry(operation_name, http, *args, **kwargs):
  """Retries requests to buildbucket service and returns parsed json content."""
  try_count = 0
  while True:
    response, content = http.request(*args, **kwargs)
    try:
      content_json = json.loads(content)
    except ValueError:
      content_json = None

    # Buildbucket could return an error even if status==200.
    if content_json and content_json.get('error'):
      error = content_json.get('error')
      if error.get('code') == 403:
        raise BuildbucketResponseException(
            'Access denied: %s' % error.get('message', ''))
      msg = 'Error in response. Reason: %s. Message: %s.' % (
          error.get('reason', ''), error.get('message', ''))
      raise BuildbucketResponseException(msg)

    if response.status == 200:
      if not content_json:
        raise BuildbucketResponseException(
            'Buildbucket returns invalid json content: %s.\n'
            'Please file bugs at http://crbug.com, label "Infra-BuildBucket".' %
            content)
      return content_json
    if response.status < 500 or try_count >= 2:
      raise httplib2.HttpLib2Error(content)

    # status >= 500 means transient failures.
    logging.debug('Transient errors when %s. Will retry.', operation_name)
    time_sleep(0.5 + 1.5*try_count)
    try_count += 1
  assert False, 'unreachable'


def _get_bucket_map(changelist, options, option_parser):
  """Returns a dict mapping bucket names to builders and tests,
  for triggering try jobs.
  """
  # If no bots are listed, we try to get a set of builders and tests based
  # on GetPreferredTryMasters functions in PRESUBMIT.py files.
  if not options.bot:
    change = changelist.GetChange(
        changelist.GetCommonAncestorWithUpstream(), None)
    # Get try masters from PRESUBMIT.py files.
    masters = presubmit_support.DoGetTryMasters(
        change=change,
        changed_files=change.LocalPaths(),
        repository_root=settings.GetRoot(),
        default_presubmit=None,
        project=None,
        verbose=options.verbose,
        output_stream=sys.stdout)
    if masters is None:
      return None
    return {_prefix_master(m): b for m, b in masters.iteritems()}

  if options.bucket:
    return {options.bucket: {b: [] for b in options.bot}}
  if options.master:
    return {_prefix_master(options.master): {b: [] for b in options.bot}}

  # If bots are listed but no master or bucket, then we need to find out
  # the corresponding master for each bot.
  bucket_map, error_message = _get_bucket_map_for_builders(options.bot)
  if error_message:
    option_parser.error(
        'Tryserver master cannot be found because: %s\n'
        'Please manually specify the tryserver master, e.g. '
        '"-m tryserver.chromium.linux".' % error_message)
  return bucket_map


def _get_bucket_map_for_builders(builders):
  """Returns a map of buckets to builders for the given builders."""
  map_url = 'https://builders-map.appspot.com/'
  try:
    builders_map = json.load(urllib2.urlopen(map_url))
  except urllib2.URLError as e:
    return None, ('Failed to fetch builder-to-master map from %s. Error: %s.' %
                  (map_url, e))
  except ValueError as e:
    return None, ('Invalid json string from %s. Error: %s.' % (map_url, e))
  if not builders_map:
    return None, 'Failed to build master map.'

  bucket_map = {}
  for builder in builders:
    bucket = builders_map.get(builder, {}).get('bucket')
    if bucket:
      bucket_map.setdefault(bucket, {})[builder] = []
  return bucket_map, None


def _trigger_try_jobs(auth_config, changelist, buckets, options, patchset):
  """Sends a request to Buildbucket to trigger try jobs for a changelist.

  Args:
    auth_config: AuthConfig for Buildbucket.
    changelist: Changelist that the try jobs are associated with.
    buckets: A nested dict mapping bucket names to builders to tests.
    options: Command-line options.
  """
  assert changelist.GetIssue(), 'CL must be uploaded first'
  codereview_url = changelist.GetCodereviewServer()
  assert codereview_url, 'CL must be uploaded first'
  patchset = patchset or changelist.GetMostRecentPatchset()
  assert patchset, 'CL must be uploaded first'

  codereview_host = urlparse.urlparse(codereview_url).hostname
  # Cache the buildbucket credentials under the codereview host key, so that
  # users can use different credentials for different buckets.
  authenticator = auth.get_authenticator_for_host(codereview_host, auth_config)
  http = authenticator.authorize(httplib2.Http())
  http.force_exception_to_status_code = True

  buildbucket_put_url = (
      'https://{hostname}/_ah/api/buildbucket/v1/builds/batch'.format(
          hostname=options.buildbucket_host))
  buildset = 'patch/{codereview}/{hostname}/{issue}/{patch}'.format(
      codereview='gerrit' if changelist.IsGerrit() else 'rietveld',
      hostname=codereview_host,
      issue=changelist.GetIssue(),
      patch=patchset)

  shared_parameters_properties = changelist.GetTryJobProperties(patchset)
  shared_parameters_properties['category'] = options.category
  if options.clobber:
    shared_parameters_properties['clobber'] = True
  extra_properties = _get_properties_from_options(options)
  if extra_properties:
    shared_parameters_properties.update(extra_properties)

  batch_req_body = {'builds': []}
  print_text = []
  print_text.append('Tried jobs on:')
  for bucket, builders_and_tests in sorted(buckets.iteritems()):
    print_text.append('Bucket: %s' % bucket)
    master = None
    if bucket.startswith(MASTER_PREFIX):
      master = _unprefix_master(bucket)
    for builder, tests in sorted(builders_and_tests.iteritems()):
      print_text.append('  %s: %s' % (builder, tests))
      parameters = {
          'builder_name': builder,
          'changes': [{
              'author': {'email': changelist.GetIssueOwner()},
              'revision': options.revision,
          }],
          'properties': shared_parameters_properties.copy(),
      }
      if 'presubmit' in builder.lower():
        parameters['properties']['dry_run'] = 'true'
      if tests:
        parameters['properties']['testfilter'] = tests

      tags = [
          'builder:%s' % builder,
          'buildset:%s' % buildset,
          'user_agent:git_cl_try',
      ]
      if master:
        parameters['properties']['master'] = master
        tags.append('master:%s' % master)

      batch_req_body['builds'].append(
          {
              'bucket': bucket,
              'parameters_json': json.dumps(parameters),
              'client_operation_id': str(uuid.uuid4()),
              'tags': tags,
          }
      )

  _buildbucket_retry(
      'triggering try jobs',
      http,
      buildbucket_put_url,
      'PUT',
      body=json.dumps(batch_req_body),
      headers={'Content-Type': 'application/json'}
  )
  print_text.append('To see results here, run:        git cl try-results')
  print_text.append('To see results in browser, run:  git cl web')
  print('\n'.join(print_text))


def fetch_try_jobs(auth_config, changelist, buildbucket_host,
                   patchset=None):
  """Fetches try jobs from buildbucket.

  Returns a map from build id to build info as a dictionary.
  """
  assert buildbucket_host
  assert changelist.GetIssue(), 'CL must be uploaded first'
  assert changelist.GetCodereviewServer(), 'CL must be uploaded first'
  patchset = patchset or changelist.GetMostRecentPatchset()
  assert patchset, 'CL must be uploaded first'

  codereview_url = changelist.GetCodereviewServer()
  codereview_host = urlparse.urlparse(codereview_url).hostname
  authenticator = auth.get_authenticator_for_host(codereview_host, auth_config)
  if authenticator.has_cached_credentials():
    http = authenticator.authorize(httplib2.Http())
  else:
    print('Warning: Some results might be missing because %s' %
          # Get the message on how to login.
          (auth.LoginRequiredError(codereview_host).message,))
    http = httplib2.Http()

  http.force_exception_to_status_code = True

  buildset = 'patch/{codereview}/{hostname}/{issue}/{patch}'.format(
      codereview='gerrit' if changelist.IsGerrit() else 'rietveld',
      hostname=codereview_host,
      issue=changelist.GetIssue(),
      patch=patchset)
  params = {'tag': 'buildset:%s' % buildset}

  builds = {}
  while True:
    url = 'https://{hostname}/_ah/api/buildbucket/v1/search?{params}'.format(
        hostname=buildbucket_host,
        params=urllib.urlencode(params))
    content = _buildbucket_retry('fetching try jobs', http, url, 'GET')
    for build in content.get('builds', []):
      builds[build['id']] = build
    if 'next_cursor' in content:
      params['start_cursor'] = content['next_cursor']
    else:
      break
  return builds


def print_try_jobs(options, builds):
  """Prints nicely result of fetch_try_jobs."""
  if not builds:
    print('No try jobs scheduled.')
    return

  # Make a copy, because we'll be modifying builds dictionary.
  builds = builds.copy()
  builder_names_cache = {}

  def get_builder(b):
    try:
      return builder_names_cache[b['id']]
    except KeyError:
      try:
        parameters = json.loads(b['parameters_json'])
        name = parameters['builder_name']
      except (ValueError, KeyError) as error:
        print('WARNING: Failed to get builder name for build %s: %s' % (
              b['id'], error))
        name = None
      builder_names_cache[b['id']] = name
      return name

  def get_bucket(b):
    bucket = b['bucket']
    if bucket.startswith('master.'):
      return bucket[len('master.'):]
    return bucket

  if options.print_master:
    name_fmt = '%%-%ds %%-%ds' % (
        max(len(str(get_bucket(b))) for b in builds.itervalues()),
        max(len(str(get_builder(b))) for b in builds.itervalues()))
    def get_name(b):
      return name_fmt % (get_bucket(b), get_builder(b))
  else:
    name_fmt = '%%-%ds' % (
        max(len(str(get_builder(b))) for b in builds.itervalues()))
    def get_name(b):
      return name_fmt % get_builder(b)

  def sort_key(b):
    return b['status'], b.get('result'), get_name(b), b.get('url')

  def pop(title, f, color=None, **kwargs):
    """Pop matching builds from `builds` dict and print them."""

    if not options.color or color is None:
      colorize = str
    else:
      colorize = lambda x: '%s%s%s' % (color, x, Fore.RESET)

    result = []
    for b in builds.values():
      if all(b.get(k) == v for k, v in kwargs.iteritems()):
        builds.pop(b['id'])
        result.append(b)
    if result:
      print(colorize(title))
      for b in sorted(result, key=sort_key):
        print(' ', colorize('\t'.join(map(str, f(b)))))

  total = len(builds)
  pop(status='COMPLETED', result='SUCCESS',
      title='Successes:', color=Fore.GREEN,
      f=lambda b: (get_name(b), b.get('url')))
  pop(status='COMPLETED', result='FAILURE', failure_reason='INFRA_FAILURE',
      title='Infra Failures:', color=Fore.MAGENTA,
      f=lambda b: (get_name(b), b.get('url')))
  pop(status='COMPLETED', result='FAILURE', failure_reason='BUILD_FAILURE',
      title='Failures:', color=Fore.RED,
      f=lambda b: (get_name(b), b.get('url')))
  pop(status='COMPLETED', result='CANCELED',
      title='Canceled:', color=Fore.MAGENTA,
      f=lambda b: (get_name(b),))
  pop(status='COMPLETED', result='FAILURE',
      failure_reason='INVALID_BUILD_DEFINITION',
      title='Wrong master/builder name:', color=Fore.MAGENTA,
      f=lambda b: (get_name(b),))
  pop(status='COMPLETED', result='FAILURE',
      title='Other failures:',
      f=lambda b: (get_name(b), b.get('failure_reason'), b.get('url')))
  pop(status='COMPLETED',
      title='Other finished:',
      f=lambda b: (get_name(b), b.get('result'), b.get('url')))
  pop(status='STARTED',
      title='Started:', color=Fore.YELLOW,
      f=lambda b: (get_name(b), b.get('url')))
  pop(status='SCHEDULED',
      title='Scheduled:',
      f=lambda b: (get_name(b), 'id=%s' % b['id']))
  # The last section is just in case buildbucket API changes OR there is a bug.
  pop(title='Other:',
      f=lambda b: (get_name(b), 'id=%s' % b['id']))
  assert len(builds) == 0
  print('Total: %d try jobs' % total)


def write_try_results_json(output_file, builds):
  """Writes a subset of the data from fetch_try_jobs to a file as JSON.

  The input |builds| dict is assumed to be generated by Buildbucket.
  Buildbucket documentation: http://goo.gl/G0s101
  """

  def convert_build_dict(build):
    """Extracts some of the information from one build dict."""
    parameters = json.loads(build.get('parameters_json', '{}')) or {}
    return {
        'buildbucket_id': build.get('id'),
        'bucket': build.get('bucket'),
        'builder_name': parameters.get('builder_name'),
        'created_ts': build.get('created_ts'),
        'experimental': build.get('experimental'),
        'failure_reason': build.get('failure_reason'),
        'result': build.get('result'),
        'status': build.get('status'),
        'tags': build.get('tags'),
        'url': build.get('url'),
    }

  converted = []
  for _, build in sorted(builds.items()):
      converted.append(convert_build_dict(build))
  write_json(output_file, converted)


def print_stats(args):
  """Prints statistics about the change to the user."""
  # --no-ext-diff is broken in some versions of Git, so try to work around
  # this by overriding the environment (but there is still a problem if the
  # git config key "diff.external" is used).
  env = GetNoGitPagerEnv()
  if 'GIT_EXTERNAL_DIFF' in env:
    del env['GIT_EXTERNAL_DIFF']

  try:
    stdout = sys.stdout.fileno()
  except AttributeError:
    stdout = None
  return subprocess2.call(
      ['git', 'diff', '--no-ext-diff', '--stat', '-l100000', '-C50'] + args,
      stdout=stdout, env=env)


class BuildbucketResponseException(Exception):
  pass


class Settings(object):
  def __init__(self):
    self.default_server = None
    self.cc = None
    self.root = None
    self.tree_status_url = None
    self.viewvc_url = None
    self.updated = False
    self.is_gerrit = None
    self.squash_gerrit_uploads = None
    self.gerrit_skip_ensure_authenticated = None
    self.git_editor = None
    self.project = None
    self.force_https_commit_url = None

  def LazyUpdateIfNeeded(self):
    """Updates the settings from a codereview.settings file, if available."""
    if not self.updated:
      # The only value that actually changes the behavior is
      # autoupdate = "false". Everything else means "true".
      autoupdate = RunGit(['config', 'rietveld.autoupdate'],
                          error_ok=True
                          ).strip().lower()

      cr_settings_file = FindCodereviewSettingsFile()
      if autoupdate != 'false' and cr_settings_file:
        LoadCodereviewSettingsFromFile(cr_settings_file)
      self.updated = True

  def GetDefaultServerUrl(self, error_ok=False):
    if not self.default_server:
      self.LazyUpdateIfNeeded()
      self.default_server = gclient_utils.UpgradeToHttps(
          self._GetRietveldConfig('server', error_ok=True))
      if error_ok:
        return self.default_server
      if not self.default_server:
        error_message = ('Could not find settings file. You must configure '
                         'your review setup by running "git cl config".')
        self.default_server = gclient_utils.UpgradeToHttps(
            self._GetRietveldConfig('server', error_message=error_message))
    return self.default_server

  @staticmethod
  def GetRelativeRoot():
    return RunGit(['rev-parse', '--show-cdup']).strip()

  def GetRoot(self):
    if self.root is None:
      self.root = os.path.abspath(self.GetRelativeRoot())
    return self.root

  def GetGitMirror(self, remote='origin'):
    """If this checkout is from a local git mirror, return a Mirror object."""
    local_url = RunGit(['config', '--get', 'remote.%s.url' % remote]).strip()
    if not os.path.isdir(local_url):
      return None
    git_cache.Mirror.SetCachePath(os.path.dirname(local_url))
    remote_url = git_cache.Mirror.CacheDirToUrl(local_url)
    # Use the /dev/null print_func to avoid terminal spew.
    mirror = git_cache.Mirror(remote_url, print_func=lambda *args: None)
    if mirror.exists():
      return mirror
    return None

  def GetTreeStatusUrl(self, error_ok=False):
    if not self.tree_status_url:
      error_message = ('You must configure your tree status URL by running '
                       '"git cl config".')
      self.tree_status_url = self._GetRietveldConfig(
          'tree-status-url', error_ok=error_ok, error_message=error_message)
    return self.tree_status_url

  def GetViewVCUrl(self):
    if not self.viewvc_url:
      self.viewvc_url = self._GetRietveldConfig('viewvc-url', error_ok=True)
    return self.viewvc_url

  def GetBugPrefix(self):
    return self._GetRietveldConfig('bug-prefix', error_ok=True)

  def GetIsSkipDependencyUpload(self, branch_name):
    """Returns true if specified branch should skip dep uploads."""
    return self._GetBranchConfig(branch_name, 'skip-deps-uploads',
                                 error_ok=True)

  def GetRunPostUploadHook(self):
    run_post_upload_hook = self._GetRietveldConfig(
        'run-post-upload-hook', error_ok=True)
    return run_post_upload_hook == "True"

  def GetDefaultCCList(self):
    return self._GetRietveldConfig('cc', error_ok=True)

  def GetDefaultPrivateFlag(self):
    return self._GetRietveldConfig('private', error_ok=True)

  def GetIsGerrit(self):
    """Return true if this repo is associated with gerrit code review system."""
    if self.is_gerrit is None:
      self.is_gerrit = (
          self._GetConfig('gerrit.host', error_ok=True).lower() == 'true')
    return self.is_gerrit

  def GetSquashGerritUploads(self):
    """Return true if uploads to Gerrit should be squashed by default."""
    if self.squash_gerrit_uploads is None:
      self.squash_gerrit_uploads = self.GetSquashGerritUploadsOverride()
      if self.squash_gerrit_uploads is None:
        # Default is squash now (http://crbug.com/611892#c23).
        self.squash_gerrit_uploads = not (
            RunGit(['config', '--bool', 'gerrit.squash-uploads'],
                   error_ok=True).strip() == 'false')
    return self.squash_gerrit_uploads

  def GetSquashGerritUploadsOverride(self):
    """Return True or False if codereview.settings should be overridden.

    Returns None if no override has been defined.
    """
    # See also http://crbug.com/611892#c23
    result = RunGit(['config', '--bool', 'gerrit.override-squash-uploads'],
                    error_ok=True).strip()
    if result == 'true':
      return True
    if result == 'false':
      return False
    return None

  def GetGerritSkipEnsureAuthenticated(self):
    """Return True if EnsureAuthenticated should not be done for Gerrit
    uploads."""
    if self.gerrit_skip_ensure_authenticated is None:
      self.gerrit_skip_ensure_authenticated = (
          RunGit(['config', '--bool', 'gerrit.skip-ensure-authenticated'],
                 error_ok=True).strip() == 'true')
    return self.gerrit_skip_ensure_authenticated

  def GetGitEditor(self):
    """Return the editor specified in the git config, or None if none is."""
    if self.git_editor is None:
      self.git_editor = self._GetConfig('core.editor', error_ok=True)
    return self.git_editor or None

  def GetLintRegex(self):
    return (self._GetRietveldConfig('cpplint-regex', error_ok=True) or
            DEFAULT_LINT_REGEX)

  def GetLintIgnoreRegex(self):
    return (self._GetRietveldConfig('cpplint-ignore-regex', error_ok=True) or
            DEFAULT_LINT_IGNORE_REGEX)

  def GetProject(self):
    if not self.project:
      self.project = self._GetRietveldConfig('project', error_ok=True)
    return self.project

  def _GetRietveldConfig(self, param, **kwargs):
    return self._GetConfig('rietveld.' + param, **kwargs)

  def _GetBranchConfig(self, branch_name, param, **kwargs):
    return self._GetConfig('branch.' + branch_name + '.' + param, **kwargs)

  def _GetConfig(self, param, **kwargs):
    self.LazyUpdateIfNeeded()
    return RunGit(['config', param], **kwargs).strip()


@contextlib.contextmanager
def _get_gerrit_project_config_file(remote_url):
  """Context manager to fetch and store Gerrit's project.config from
  refs/meta/config branch and store it in temp file.

  Provides a temporary filename or None if there was error.
  """
  error, _ = RunGitWithCode([
      'fetch', remote_url,
      '+refs/meta/config:refs/git_cl/meta/config'])
  if error:
    # Ref doesn't exist or isn't accessible to current user.
    print('WARNING: Failed to fetch project config for %s: %s' %
          (remote_url, error))
    yield None
    return

  error, project_config_data = RunGitWithCode(
      ['show', 'refs/git_cl/meta/config:project.config'])
  if error:
    print('WARNING: project.config file not found')
    yield None
    return

  with gclient_utils.temporary_directory() as tempdir:
    project_config_file = os.path.join(tempdir, 'project.config')
    gclient_utils.FileWrite(project_config_file, project_config_data)
    yield project_config_file


def _is_git_numberer_enabled(remote_url, remote_ref):
  """Returns True if Git Numberer is enabled on this ref."""
  # TODO(tandrii): this should be deleted once repos below are 100% on Gerrit.
  KNOWN_PROJECTS_WHITELIST = [
      'chromium/src',
      'external/webrtc',
      'v8/v8',
      'infra/experimental',
      # For webrtc.googlesource.com/src.
      'src',
  ]

  assert remote_ref and remote_ref.startswith('refs/'), remote_ref
  url_parts = urlparse.urlparse(remote_url)
  project_name = url_parts.path.lstrip('/').rstrip('git./')
  for known in KNOWN_PROJECTS_WHITELIST:
    if project_name.endswith(known):
      break
  else:
    # Early exit to avoid extra fetches for repos that aren't using Git
    # Numberer.
    return False

  with _get_gerrit_project_config_file(remote_url) as project_config_file:
    if project_config_file is None:
      # Failed to fetch project.config, which shouldn't happen on open source
      # repos KNOWN_PROJECTS_WHITELIST.
      return False
    def get_opts(x):
      code, out = RunGitWithCode(
          ['config', '-f', project_config_file, '--get-all',
           'plugin.git-numberer.validate-%s-refglob' % x])
      if code == 0:
        return out.strip().splitlines()
      return []
    enabled, disabled = map(get_opts, ['enabled', 'disabled'])

  logging.info('validator config enabled %s disabled %s refglobs for '
               '(this ref: %s)', enabled, disabled, remote_ref)

  def match_refglobs(refglobs):
    for refglob in refglobs:
      if remote_ref == refglob or fnmatch.fnmatch(remote_ref, refglob):
        return True
    return False

  if match_refglobs(disabled):
    return False
  return match_refglobs(enabled)


def ShortBranchName(branch):
  """Convert a name like 'refs/heads/foo' to just 'foo'."""
  return branch.replace('refs/heads/', '', 1)


def GetCurrentBranchRef():
  """Returns branch ref (e.g., refs/heads/master) or None."""
  return RunGit(['symbolic-ref', 'HEAD'],
                stderr=subprocess2.VOID, error_ok=True).strip() or None


def GetCurrentBranch():
  """Returns current branch or None.

  For refs/heads/* branches, returns just last part. For others, full ref.
  """
  branchref = GetCurrentBranchRef()
  if branchref:
    return ShortBranchName(branchref)
  return None


class _CQState(object):
  """Enum for states of CL with respect to Commit Queue."""
  NONE = 'none'
  DRY_RUN = 'dry_run'
  COMMIT = 'commit'

  ALL_STATES = [NONE, DRY_RUN, COMMIT]


class _ParsedIssueNumberArgument(object):
  def __init__(self, issue=None, patchset=None, hostname=None, codereview=None):
    self.issue = issue
    self.patchset = patchset
    self.hostname = hostname
    assert codereview in (None, 'rietveld', 'gerrit')
    self.codereview = codereview

  @property
  def valid(self):
    return self.issue is not None


def ParseIssueNumberArgument(arg, codereview=None):
  """Parses the issue argument and returns _ParsedIssueNumberArgument."""
  fail_result = _ParsedIssueNumberArgument()

  if arg.isdigit():
    return _ParsedIssueNumberArgument(issue=int(arg), codereview=codereview)
  if not arg.startswith('http'):
    return fail_result

  url = gclient_utils.UpgradeToHttps(arg)
  try:
    parsed_url = urlparse.urlparse(url)
  except ValueError:
    return fail_result

  if codereview is not None:
    parsed = _CODEREVIEW_IMPLEMENTATIONS[codereview].ParseIssueURL(parsed_url)
    return parsed or fail_result

  results = {}
  for name, cls in _CODEREVIEW_IMPLEMENTATIONS.iteritems():
    parsed = cls.ParseIssueURL(parsed_url)
    if parsed is not None:
      results[name] = parsed

  if not results:
    return fail_result
  if len(results) == 1:
    return results.values()[0]

  if parsed_url.netloc and parsed_url.netloc.split('.')[0].endswith('-review'):
    # This is likely Gerrit.
    return results['gerrit']
  # Choose Rietveld as before if URL can parsed by either.
  return results['rietveld']


class GerritChangeNotExists(Exception):
  def __init__(self, issue, url):
    self.issue = issue
    self.url = url
    super(GerritChangeNotExists, self).__init__()

  def __str__(self):
    return 'change %s at %s does not exist or you have no access to it' % (
        self.issue, self.url)


_CommentSummary = collections.namedtuple(
    '_CommentSummary', ['date', 'message', 'sender',
                        # TODO(tandrii): these two aren't known in Gerrit.
                        'approval', 'disapproval'])


class Changelist(object):
  """Changelist works with one changelist in local branch.

  Supports two codereview backends: Rietveld or Gerrit, selected at object
  creation.

  Notes:
    * Not safe for concurrent multi-{thread,process} use.
    * Caches values from current branch. Therefore, re-use after branch change
      with great care.
  """

  def __init__(self, branchref=None, issue=None, codereview=None, **kwargs):
    """Create a new ChangeList instance.

    If issue is given, the codereview must be given too.

    If `codereview` is given, it must be 'rietveld' or 'gerrit'.
    Otherwise, it's decided based on current configuration of the local branch,
    with default being 'rietveld' for backwards compatibility.
    See _load_codereview_impl for more details.

    **kwargs will be passed directly to codereview implementation.
    """
    # Poke settings so we get the "configure your server" message if necessary.
    global settings
    if not settings:
      # Happens when git_cl.py is used as a utility library.
      settings = Settings()

    if issue:
      assert codereview, 'codereview must be known, if issue is known'

    self.branchref = branchref
    if self.branchref:
      assert branchref.startswith('refs/heads/')
      self.branch = ShortBranchName(self.branchref)
    else:
      self.branch = None
    self.upstream_branch = None
    self.lookedup_issue = False
    self.issue = issue or None
    self.has_description = False
    self.description = None
    self.lookedup_patchset = False
    self.patchset = None
    self.cc = None
    self.more_cc = []
    self._remote = None

    self._codereview_impl = None
    self._codereview = None
    self._load_codereview_impl(codereview, **kwargs)
    assert self._codereview_impl
    assert self._codereview in _CODEREVIEW_IMPLEMENTATIONS

  def _load_codereview_impl(self, codereview=None, **kwargs):
    if codereview:
      assert codereview in _CODEREVIEW_IMPLEMENTATIONS
      cls = _CODEREVIEW_IMPLEMENTATIONS[codereview]
      self._codereview = codereview
      self._codereview_impl = cls(self, **kwargs)
      return

    # Automatic selection based on issue number set for a current branch.
    # Rietveld takes precedence over Gerrit.
    assert not self.issue
    # Whether we find issue or not, we are doing the lookup.
    self.lookedup_issue = True
    if self.GetBranch():
      for codereview, cls in _CODEREVIEW_IMPLEMENTATIONS.iteritems():
        issue = _git_get_branch_config_value(
            cls.IssueConfigKey(), value_type=int, branch=self.GetBranch())
        if issue:
          self._codereview = codereview
          self._codereview_impl = cls(self, **kwargs)
          self.issue = int(issue)
          return

    # No issue is set for this branch, so decide based on repo-wide settings.
    return self._load_codereview_impl(
        codereview='gerrit' if settings.GetIsGerrit() else 'rietveld',
        **kwargs)

  def IsGerrit(self):
    return self._codereview == 'gerrit'

  def GetCCList(self):
    """Returns the users cc'd on this CL.

    The return value is a string suitable for passing to git cl with the --cc
    flag.
    """
    if self.cc is None:
      base_cc = settings.GetDefaultCCList()
      more_cc = ','.join(self.more_cc)
      self.cc = ','.join(filter(None, (base_cc, more_cc))) or ''
    return self.cc

  def GetCCListWithoutDefault(self):
    """Return the users cc'd on this CL excluding default ones."""
    if self.cc is None:
      self.cc = ','.join(self.more_cc)
    return self.cc

  def ExtendCC(self, more_cc):
    """Extends the list of users to cc on this CL based on the changed files."""
    self.more_cc.extend(more_cc)

  def GetBranch(self):
    """Returns the short branch name, e.g. 'master'."""
    if not self.branch:
      branchref = GetCurrentBranchRef()
      if not branchref:
        return None
      self.branchref = branchref
      self.branch = ShortBranchName(self.branchref)
    return self.branch

  def GetBranchRef(self):
    """Returns the full branch name, e.g. 'refs/heads/master'."""
    self.GetBranch()  # Poke the lazy loader.
    return self.branchref

  def ClearBranch(self):
    """Clears cached branch data of this object."""
    self.branch = self.branchref = None

  def _GitGetBranchConfigValue(self, key, default=None, **kwargs):
    assert 'branch' not in kwargs, 'this CL branch is used automatically'
    kwargs['branch'] = self.GetBranch()
    return _git_get_branch_config_value(key, default, **kwargs)

  def _GitSetBranchConfigValue(self, key, value, **kwargs):
    assert 'branch' not in kwargs, 'this CL branch is used automatically'
    assert self.GetBranch(), (
        'this CL must have an associated branch to %sset %s%s' %
          ('un' if value is None else '',
           key,
           '' if value is None else ' to %r' % value))
    kwargs['branch'] = self.GetBranch()
    return _git_set_branch_config_value(key, value, **kwargs)

  @staticmethod
  def FetchUpstreamTuple(branch):
    """Returns a tuple containing remote and remote ref,
       e.g. 'origin', 'refs/heads/master'
    """
    remote = '.'
    upstream_branch = _git_get_branch_config_value('merge', branch=branch)

    if upstream_branch:
      remote = _git_get_branch_config_value('remote', branch=branch)
    else:
      upstream_branch = RunGit(['config', 'rietveld.upstream-branch'],
                               error_ok=True).strip()
      if upstream_branch:
        remote = RunGit(['config', 'rietveld.upstream-remote']).strip()
      else:
        # Else, try to guess the origin remote.
        remote_branches = RunGit(['branch', '-r']).split()
        if 'origin/master' in remote_branches:
          # Fall back on origin/master if it exits.
          remote = 'origin'
          upstream_branch = 'refs/heads/master'
        else:
          DieWithError(
             'Unable to determine default branch to diff against.\n'
             'Either pass complete "git diff"-style arguments, like\n'
             '  git cl upload origin/master\n'
             'or verify this branch is set up to track another \n'
             '(via the --track argument to "git checkout -b ...").')

    return remote, upstream_branch

  def GetCommonAncestorWithUpstream(self):
    upstream_branch = self.GetUpstreamBranch()
    if not BranchExists(upstream_branch):
      DieWithError('The upstream for the current branch (%s) does not exist '
                   'anymore.\nPlease fix it and try again.' % self.GetBranch())
    return git_common.get_or_create_merge_base(self.GetBranch(),
                                               upstream_branch)

  def GetUpstreamBranch(self):
    if self.upstream_branch is None:
      remote, upstream_branch = self.FetchUpstreamTuple(self.GetBranch())
      if remote is not '.':
        upstream_branch = upstream_branch.replace('refs/heads/',
                                                  'refs/remotes/%s/' % remote)
        upstream_branch = upstream_branch.replace('refs/branch-heads/',
                                                  'refs/remotes/branch-heads/')
      self.upstream_branch = upstream_branch
    return self.upstream_branch

  def GetRemoteBranch(self):
    if not self._remote:
      remote, branch = None, self.GetBranch()
      seen_branches = set()
      while branch not in seen_branches:
        seen_branches.add(branch)
        remote, branch = self.FetchUpstreamTuple(branch)
        branch = ShortBranchName(branch)
        if remote != '.' or branch.startswith('refs/remotes'):
          break
      else:
        remotes = RunGit(['remote'], error_ok=True).split()
        if len(remotes) == 1:
          remote, = remotes
        elif 'origin' in remotes:
          remote = 'origin'
          logging.warn('Could not determine which remote this change is '
                       'associated with, so defaulting to "%s".' % self._remote)
        else:
          logging.warn('Could not determine which remote this change is '
                       'associated with.')
        branch = 'HEAD'
      if branch.startswith('refs/remotes'):
        self._remote = (remote, branch)
      elif branch.startswith('refs/branch-heads/'):
        self._remote = (remote, branch.replace('refs/', 'refs/remotes/'))
      else:
        self._remote = (remote, 'refs/remotes/%s/%s' % (remote, branch))
    return self._remote

  def GitSanityChecks(self, upstream_git_obj):
    """Checks git repo status and ensures diff is from local commits."""

    if upstream_git_obj is None:
      if self.GetBranch() is None:
        print('ERROR: Unable to determine current branch (detached HEAD?)',
              file=sys.stderr)
      else:
        print('ERROR: No upstream branch.', file=sys.stderr)
      return False

    # Verify the commit we're diffing against is in our current branch.
    upstream_sha = RunGit(['rev-parse', '--verify', upstream_git_obj]).strip()
    common_ancestor = RunGit(['merge-base', upstream_sha, 'HEAD']).strip()
    if upstream_sha != common_ancestor:
      print('ERROR: %s is not in the current branch.  You may need to rebase '
            'your tracking branch' % upstream_sha, file=sys.stderr)
      return False

    # List the commits inside the diff, and verify they are all local.
    commits_in_diff = RunGit(
        ['rev-list', '^%s' % upstream_sha, 'HEAD']).splitlines()
    code, remote_branch = RunGitWithCode(['config', 'gitcl.remotebranch'])
    remote_branch = remote_branch.strip()
    if code != 0:
      _, remote_branch = self.GetRemoteBranch()

    commits_in_remote = RunGit(
        ['rev-list', '^%s' % upstream_sha, remote_branch]).splitlines()

    common_commits = set(commits_in_diff) & set(commits_in_remote)
    if common_commits:
      print('ERROR: Your diff contains %d commits already in %s.\n'
            'Run "git log --oneline %s..HEAD" to get a list of commits in '
            'the diff.  If you are using a custom git flow, you can override'
            ' the reference used for this check with "git config '
            'gitcl.remotebranch <git-ref>".' % (
                len(common_commits), remote_branch, upstream_git_obj),
            file=sys.stderr)
      return False
    return True

  def GetGitBaseUrlFromConfig(self):
    """Return the configured base URL from branch.<branchname>.baseurl.

    Returns None if it is not set.
    """
    return self._GitGetBranchConfigValue('base-url')

  def GetRemoteUrl(self):
    """Return the configured remote URL, e.g. 'git://example.org/foo.git/'.

    Returns None if there is no remote.
    """
    remote, _ = self.GetRemoteBranch()
    url = RunGit(['config', 'remote.%s.url' % remote], error_ok=True).strip()

    # If URL is pointing to a local directory, it is probably a git cache.
    if os.path.isdir(url):
      url = RunGit(['config', 'remote.%s.url' % remote],
                   error_ok=True,
                   cwd=url).strip()
    return url

  def GetIssue(self):
    """Returns the issue number as a int or None if not set."""
    if self.issue is None and not self.lookedup_issue:
      self.issue = self._GitGetBranchConfigValue(
          self._codereview_impl.IssueConfigKey(), value_type=int)
      self.lookedup_issue = True
    return self.issue

  def GetIssueURL(self):
    """Get the URL for a particular issue."""
    issue = self.GetIssue()
    if not issue:
      return None
    return '%s/%s' % (self._codereview_impl.GetCodereviewServer(), issue)

  def GetDescription(self, pretty=False, force=False):
    if not self.has_description or force:
      if self.GetIssue():
        self.description = self._codereview_impl.FetchDescription(force=force)
      self.has_description = True
    if pretty:
      # Set width to 72 columns + 2 space indent.
      wrapper = textwrap.TextWrapper(width=74, replace_whitespace=True)
      wrapper.initial_indent = wrapper.subsequent_indent = '  '
      lines = self.description.splitlines()
      return '\n'.join([wrapper.fill(line) for line in lines])
    return self.description

  def GetDescriptionFooters(self):
    """Returns (non_footer_lines, footers) for the commit message.

    Returns:
      non_footer_lines (list(str)) - Simple list of description lines without
        any footer. The lines do not contain newlines, nor does the list contain
        the empty line between the message and the footers.
      footers (list(tuple(KEY, VALUE))) - List of parsed footers, e.g.
        [("Change-Id", "Ideadbeef...."), ...]
    """
    raw_description = self.GetDescription()
    msg_lines, _, footers = git_footers.split_footers(raw_description)
    if footers:
      msg_lines = msg_lines[:len(msg_lines)-1]
    return msg_lines, footers

  def GetPatchset(self):
    """Returns the patchset number as a int or None if not set."""
    if self.patchset is None and not self.lookedup_patchset:
      self.patchset = self._GitGetBranchConfigValue(
          self._codereview_impl.PatchsetConfigKey(), value_type=int)
      self.lookedup_patchset = True
    return self.patchset

  def SetPatchset(self, patchset):
    """Set this branch's patchset. If patchset=0, clears the patchset."""
    assert self.GetBranch()
    if not patchset:
      self.patchset = None
    else:
      self.patchset = int(patchset)
    self._GitSetBranchConfigValue(
        self._codereview_impl.PatchsetConfigKey(), self.patchset)

  def SetIssue(self, issue=None):
    """Set this branch's issue. If issue isn't given, clears the issue."""
    assert self.GetBranch()
    if issue:
      issue = int(issue)
      self._GitSetBranchConfigValue(
          self._codereview_impl.IssueConfigKey(), issue)
      self.issue = issue
      codereview_server = self._codereview_impl.GetCodereviewServer()
      if codereview_server:
        self._GitSetBranchConfigValue(
            self._codereview_impl.CodereviewServerConfigKey(),
            codereview_server)
    else:
      # Reset all of these just to be clean.
      reset_suffixes = [
          'last-upload-hash',
          self._codereview_impl.IssueConfigKey(),
          self._codereview_impl.PatchsetConfigKey(),
          self._codereview_impl.CodereviewServerConfigKey(),
      ] + self._PostUnsetIssueProperties()
      for prop in reset_suffixes:
        self._GitSetBranchConfigValue(prop, None, error_ok=True)
      msg = RunGit(['log', '-1', '--format=%B']).strip()
      if msg and git_footers.get_footer_change_id(msg):
        print('WARNING: The change patched into this branch has a Change-Id. '
              'Removing it.')
        RunGit(['commit', '--amend', '-m',
                git_footers.remove_footer(msg, 'Change-Id')])
      self.issue = None
      self.patchset = None

  def GetChange(self, upstream_branch, author, local_description=False):
    if not self.GitSanityChecks(upstream_branch):
      DieWithError('\nGit sanity check failure')

    root = settings.GetRelativeRoot()
    if not root:
      root = '.'
    absroot = os.path.abspath(root)

    # We use the sha1 of HEAD as a name of this change.
    name = RunGitWithCode(['rev-parse', 'HEAD'])[1].strip()
    # Need to pass a relative path for msysgit.
    try:
      files = scm.GIT.CaptureStatus([root], '.', upstream_branch)
    except subprocess2.CalledProcessError:
      DieWithError(
          ('\nFailed to diff against upstream branch %s\n\n'
           'This branch probably doesn\'t exist anymore. To reset the\n'
           'tracking branch, please run\n'
           '    git branch --set-upstream-to origin/master %s\n'
           'or replace origin/master with the relevant branch') %
          (upstream_branch, self.GetBranch()))

    issue = self.GetIssue()
    patchset = self.GetPatchset()
    if issue and not local_description:
      description = self.GetDescription()
    else:
      # If the change was never uploaded, use the log messages of all commits
      # up to the branch point, as git cl upload will prefill the description
      # with these log messages.
      args = ['log', '--pretty=format:%s%n%n%b', '%s...' % (upstream_branch)]
      description = RunGitWithCode(args)[1].strip()

    if not author:
      author = RunGit(['config', 'user.email']).strip() or None
    return presubmit_support.GitChange(
        name,
        description,
        absroot,
        files,
        issue,
        patchset,
        author,
        upstream=upstream_branch)

  def UpdateDescription(self, description, force=False):
    self._codereview_impl.UpdateDescriptionRemote(description, force=force)
    self.description = description
    self.has_description = True

  def UpdateDescriptionFooters(self, description_lines, footers, force=False):
    """Sets the description for this CL remotely.

    You can get description_lines and footers with GetDescriptionFooters.

    Args:
      description_lines (list(str)) - List of CL description lines without
        newline characters.
      footers (list(tuple(KEY, VALUE))) - List of footers, as returned by
        GetDescriptionFooters. Key must conform to the git footers format (i.e.
        `List-Of-Tokens`). It will be case-normalized so that each token is
        title-cased.
    """
    new_description = '\n'.join(description_lines)
    if footers:
      new_description += '\n'
      for k, v in footers:
        foot = '%s: %s' % (git_footers.normalize_name(k), v)
        if not git_footers.FOOTER_PATTERN.match(foot):
          raise ValueError('Invalid footer %r' % foot)
        new_description += foot + '\n'
    self.UpdateDescription(new_description, force)

  def RunHook(self, committing, may_prompt, verbose, change, parallel):
    """Calls sys.exit() if the hook fails; returns a HookResults otherwise."""
    try:
      return presubmit_support.DoPresubmitChecks(change, committing,
          verbose=verbose, output_stream=sys.stdout, input_stream=sys.stdin,
          default_presubmit=None, may_prompt=may_prompt,
          gerrit_obj=self._codereview_impl.GetGerritObjForPresubmit(),
          parallel=parallel)
    except presubmit_support.PresubmitFailure as e:
      DieWithError('%s\nMaybe your depot_tools is out of date?' % e)

  def CMDPatchIssue(self, issue_arg, reject, nocommit, directory):
    """Fetches and applies the issue patch from codereview to local branch."""
    if isinstance(issue_arg, (int, long)) or issue_arg.isdigit():
      parsed_issue_arg = _ParsedIssueNumberArgument(int(issue_arg))
    else:
      # Assume url.
      parsed_issue_arg = self._codereview_impl.ParseIssueURL(
          urlparse.urlparse(issue_arg))
    if not parsed_issue_arg or not parsed_issue_arg.valid:
      DieWithError('Failed to parse issue argument "%s". '
                   'Must be an issue number or a valid URL.' % issue_arg)
    return self._codereview_impl.CMDPatchWithParsedIssue(
        parsed_issue_arg, reject, nocommit, directory, False)

  def CMDUpload(self, options, git_diff_args, orig_args):
    """Uploads a change to codereview."""
    custom_cl_base = None
    if git_diff_args:
      custom_cl_base = base_branch = git_diff_args[0]
    else:
      if self.GetBranch() is None:
        DieWithError('Can\'t upload from detached HEAD state. Get on a branch!')

      # Default to diffing against common ancestor of upstream branch
      base_branch = self.GetCommonAncestorWithUpstream()
      git_diff_args = [base_branch, 'HEAD']

    # Warn about Rietveld deprecation for initial uploads to Rietveld.
    if not self.IsGerrit() and not self.GetIssue():
      print('=====================================')
      print('NOTICE: Rietveld is being deprecated. '
            'You can upload changes to Gerrit with')
      print('  git cl upload --gerrit')
      print('or set Gerrit to be your default code review tool with')
      print('  git config gerrit.host true')
      print('=====================================')

    # Fast best-effort checks to abort before running potentially
    # expensive hooks if uploading is likely to fail anyway. Passing these
    # checks does not guarantee that uploading will not fail.
    self._codereview_impl.EnsureAuthenticated(force=options.force)
    self._codereview_impl.EnsureCanUploadPatchset(force=options.force)

    # Apply watchlists on upload.
    change = self.GetChange(base_branch, None)
    watchlist = watchlists.Watchlists(change.RepositoryRoot())
    files = [f.LocalPath() for f in change.AffectedFiles()]
    if not options.bypass_watchlists:
      self.ExtendCC(watchlist.GetWatchersForPaths(files))

    if not options.bypass_hooks:
      if options.reviewers or options.tbrs or options.add_owners_to:
        # Set the reviewer list now so that presubmit checks can access it.
        change_description = ChangeDescription(change.FullDescriptionText())
        change_description.update_reviewers(options.reviewers,
                                            options.tbrs,
                                            options.add_owners_to,
                                            change)
        change.SetDescriptionText(change_description.description)
      hook_results = self.RunHook(committing=False,
                                  may_prompt=not options.force,
                                  verbose=options.verbose,
                                  change=change, parallel=options.parallel)
      if not hook_results.should_continue():
        return 1
      if not options.reviewers and hook_results.reviewers:
        options.reviewers = hook_results.reviewers.split(',')
      self.ExtendCC(hook_results.more_cc)

    # TODO(tandrii): Checking local patchset against remote patchset is only
    # supported for Rietveld. Extend it to Gerrit or remove it completely.
    if self.GetIssue() and not self.IsGerrit():
      latest_patchset = self.GetMostRecentPatchset()
      local_patchset = self.GetPatchset()
      if (latest_patchset and local_patchset and
          local_patchset != latest_patchset):
        print('The last upload made from this repository was patchset #%d but '
              'the most recent patchset on the server is #%d.'
              % (local_patchset, latest_patchset))
        print('Uploading will still work, but if you\'ve uploaded to this '
              'issue from another machine or branch the patch you\'re '
              'uploading now might not include those changes.')
        confirm_or_exit(action='upload')

    print_stats(git_diff_args)
    ret = self.CMDUploadChange(options, git_diff_args, custom_cl_base, change)
    if not ret:
      if self.IsGerrit():
        self.SetLabels(options.enable_auto_submit, options.use_commit_queue,
                       options.cq_dry_run);
      else:
        if options.use_commit_queue:
          self.SetCQState(_CQState.COMMIT)
        elif options.cq_dry_run:
          self.SetCQState(_CQState.DRY_RUN)

      _git_set_branch_config_value('last-upload-hash',
                                   RunGit(['rev-parse', 'HEAD']).strip())
      # Run post upload hooks, if specified.
      if settings.GetRunPostUploadHook():
        presubmit_support.DoPostUploadExecuter(
            change,
            self,
            settings.GetRoot(),
            options.verbose,
            sys.stdout)

      # Upload all dependencies if specified.
      if options.dependencies:
        print()
        print('--dependencies has been specified.')
        print('All dependent local branches will be re-uploaded.')
        print()
        # Remove the dependencies flag from args so that we do not end up in a
        # loop.
        orig_args.remove('--dependencies')
        ret = upload_branch_deps(self, orig_args)
    return ret

  def SetLabels(self, enable_auto_submit, use_commit_queue, cq_dry_run):
    """Sets labels on the change based on the provided flags.

    Sets labels if issue is already uploaded and known, else returns without
    doing anything.

    Args:
      enable_auto_submit: Sets Auto-Submit+1 on the change.
      use_commit_queue: Sets Commit-Queue+2 on the change.
      cq_dry_run: Sets Commit-Queue+1 on the change. Overrides Commit-Queue+2 if
                  both use_commit_queue and cq_dry_run are true.
    """
    if not self.GetIssue():
      return
    try:
      self._codereview_impl.SetLabels(enable_auto_submit, use_commit_queue,
                                      cq_dry_run)
      return 0
    except KeyboardInterrupt:
      raise
    except:
      labels = []
      if enable_auto_submit:
        labels.append('Auto-Submit')
      if use_commit_queue or cq_dry_run:
        labels.append('Commit-Queue')
      print('WARNING: Failed to set label(s) on your change: %s\n'
            'Either:\n'
            ' * Your project does not have the above label(s),\n'
            ' * You don\'t have permission to set the above label(s),\n'
            ' * There\'s a bug in this code (see stack trace below).\n' %
            (', '.join(labels)))
      # Still raise exception so that stack trace is printed.
      raise

  def SetCQState(self, new_state):
    """Updates the CQ state for the latest patchset.

    Issue must have been already uploaded and known.
    """
    assert new_state in _CQState.ALL_STATES
    assert self.GetIssue()
    try:
      self._codereview_impl.SetCQState(new_state)
      return 0
    except KeyboardInterrupt:
      raise
    except:
      print('WARNING: Failed to %s.\n'
            'Either:\n'
            ' * Your project has no CQ,\n'
            ' * You don\'t have permission to change the CQ state,\n'
            ' * There\'s a bug in this code (see stack trace below).\n'
            'Consider specifying which bots to trigger manually or asking your '
            'project owners for permissions or contacting Chrome Infra at:\n'
            'https://www.chromium.org/infra\n\n' %
            ('cancel CQ' if new_state == _CQState.NONE else 'trigger CQ'))
      # Still raise exception so that stack trace is printed.
      raise

  # Forward methods to codereview specific implementation.

  def AddComment(self, message, publish=None):
    return self._codereview_impl.AddComment(message, publish=publish)

  def GetCommentsSummary(self, readable=True):
    """Returns list of _CommentSummary for each comment.

    args:
    readable: determines whether the output is designed for a human or a machine
    """
    return self._codereview_impl.GetCommentsSummary(readable)

  def CloseIssue(self):
    return self._codereview_impl.CloseIssue()

  def GetStatus(self):
    return self._codereview_impl.GetStatus()

  def GetCodereviewServer(self):
    return self._codereview_impl.GetCodereviewServer()

  def GetIssueOwner(self):
    """Get owner from codereview, which may differ from this checkout."""
    return self._codereview_impl.GetIssueOwner()

  def GetReviewers(self):
    return self._codereview_impl.GetReviewers()

  def GetMostRecentPatchset(self):
    return self._codereview_impl.GetMostRecentPatchset()

  def CannotTriggerTryJobReason(self):
    """Returns reason (str) if unable trigger try jobs on this CL or None."""
    return self._codereview_impl.CannotTriggerTryJobReason()

  def GetTryJobProperties(self, patchset=None):
    """Returns dictionary of properties to launch try job."""
    return self._codereview_impl.GetTryJobProperties(patchset=patchset)

  def __getattr__(self, attr):
    # This is because lots of untested code accesses Rietveld-specific stuff
    # directly, and it's hard to fix for sure. So, just let it work, and fix
    # on a case by case basis.
    # Note that child method defines __getattr__ as well, and forwards it here,
    # because _RietveldChangelistImpl is not cleaned up yet, and given
    # deprecation of Rietveld, it should probably be just removed.
    # Until that time, avoid infinite recursion by bypassing __getattr__
    # of implementation class.
    return self._codereview_impl.__getattribute__(attr)


class _ChangelistCodereviewBase(object):
  """Abstract base class encapsulating codereview specifics of a changelist."""
  def __init__(self, changelist):
    self._changelist = changelist  # instance of Changelist

  def __getattr__(self, attr):
    # Forward methods to changelist.
    # TODO(tandrii): maybe clean up _GerritChangelistImpl and
    # _RietveldChangelistImpl to avoid this hack?
    return getattr(self._changelist, attr)

  def GetStatus(self):
    """Apply a rough heuristic to give a simple summary of an issue's review
    or CQ status, assuming adherence to a common workflow.

    Returns None if no issue for this branch, or specific string keywords.
    """
    raise NotImplementedError()

  def GetCodereviewServer(self):
    """Returns server URL without end slash, like "https://codereview.com"."""
    raise NotImplementedError()

  def FetchDescription(self, force=False):
    """Fetches and returns description from the codereview server."""
    raise NotImplementedError()

  @classmethod
  def IssueConfigKey(cls):
    """Returns branch setting storing issue number."""
    raise NotImplementedError()

  @classmethod
  def PatchsetConfigKey(cls):
    """Returns branch setting storing patchset number."""
    raise NotImplementedError()

  @classmethod
  def CodereviewServerConfigKey(cls):
    """Returns branch setting storing codereview server."""
    raise NotImplementedError()

  def _PostUnsetIssueProperties(self):
    """Which branch-specific properties to erase when unsetting issue."""
    return []

  def GetGerritObjForPresubmit(self):
    # None is valid return value, otherwise presubmit_support.GerritAccessor.
    return None

  def UpdateDescriptionRemote(self, description, force=False):
    """Update the description on codereview site."""
    raise NotImplementedError()

  def AddComment(self, message, publish=None):
    """Posts a comment to the codereview site."""
    raise NotImplementedError()

  def GetCommentsSummary(self, readable=True):
    raise NotImplementedError()

  def CloseIssue(self):
    """Closes the issue."""
    raise NotImplementedError()

  def GetMostRecentPatchset(self):
    """Returns the most recent patchset number from the codereview site."""
    raise NotImplementedError()

  def CMDPatchWithParsedIssue(self, parsed_issue_arg, reject, nocommit,
                              directory, force):
    """Fetches and applies the issue.

    Arguments:
      parsed_issue_arg: instance of _ParsedIssueNumberArgument.
      reject: if True, reject the failed patch instead of switching to 3-way
        merge. Rietveld only.
      nocommit: do not commit the patch, thus leave the tree dirty. Rietveld
        only.
      directory: switch to directory before applying the patch. Rietveld only.
      force: if true, overwrites existing local state.
    """
    raise NotImplementedError()

  @staticmethod
  def ParseIssueURL(parsed_url):
    """Parses url and returns instance of _ParsedIssueNumberArgument or None if
    failed."""
    raise NotImplementedError()

  def EnsureAuthenticated(self, force, refresh=False):
    """Best effort check that user is authenticated with codereview server.

    Arguments:
      force: whether to skip confirmation questions.
      refresh: whether to attempt to refresh credentials. Ignored if not
        applicable.
    """
    raise NotImplementedError()

  def EnsureCanUploadPatchset(self, force):
    """Best effort check that uploading isn't supposed to fail for predictable
    reasons.

    This method should raise informative exception if uploading shouldn't
    proceed.

    Arguments:
      force: whether to skip confirmation questions.
    """
    raise NotImplementedError()

  def CMDUploadChange(self, options, git_diff_args, custom_cl_base, change):
    """Uploads a change to codereview."""
    raise NotImplementedError()

  def SetLabels(self, enable_auto_submit, use_commit_queue, cq_dry_run):
    """Sets labels on the change based on the provided flags.

    Issue must have been already uploaded and known.
    """
    raise NotImplementedError()

  def SetCQState(self, new_state):
    """Updates the CQ state for the latest patchset.

    Issue must have been already uploaded and known.
    """
    raise NotImplementedError()

  def CannotTriggerTryJobReason(self):
    """Returns reason (str) if unable trigger try jobs on this CL or None."""
    raise NotImplementedError()

  def GetIssueOwner(self):
    raise NotImplementedError()

  def GetReviewers(self):
    raise NotImplementedError()

  def GetTryJobProperties(self, patchset=None):
    raise NotImplementedError()


class _RietveldChangelistImpl(_ChangelistCodereviewBase):

  def __init__(self, changelist, auth_config=None, codereview_host=None):
    super(_RietveldChangelistImpl, self).__init__(changelist)
    assert settings, 'must be initialized in _ChangelistCodereviewBase'
    if not codereview_host:
      settings.GetDefaultServerUrl()

    self._rietveld_server = codereview_host
    self._auth_config = auth_config or auth.make_auth_config()
    self._props = None
    self._rpc_server = None

  def GetCodereviewServer(self):
    if not self._rietveld_server:
      # If we're on a branch then get the server potentially associated
      # with that branch.
      if self.GetIssue():
        self._rietveld_server = gclient_utils.UpgradeToHttps(
            self._GitGetBranchConfigValue(self.CodereviewServerConfigKey()))
      if not self._rietveld_server:
        self._rietveld_server = settings.GetDefaultServerUrl()
    return self._rietveld_server

  def EnsureAuthenticated(self, force, refresh=False):
    """Best effort check that user is authenticated with Rietveld server."""
    if self._auth_config.use_oauth2:
      authenticator = auth.get_authenticator_for_host(
          self.GetCodereviewServer(), self._auth_config)
      if not authenticator.has_cached_credentials():
        raise auth.LoginRequiredError(self.GetCodereviewServer())
      if refresh:
        authenticator.get_access_token()

  def EnsureCanUploadPatchset(self, force):
    # No checks for Rietveld because we are deprecating Rietveld.
    pass

  def FetchDescription(self, force=False):
    issue = self.GetIssue()
    assert issue
    try:
      return self.RpcServer().get_description(issue, force=force).strip()
    except urllib2.HTTPError as e:
      if e.code == 404:
        DieWithError(
            ('\nWhile fetching the description for issue %d, received a '
             '404 (not found)\n'
             'error. It is likely that you deleted this '
             'issue on the server. If this is the\n'
             'case, please run\n\n'
             '    git cl issue 0\n\n'
             'to clear the association with the deleted issue. Then run '
             'this command again.') % issue)
      else:
        DieWithError(
            '\nFailed to fetch issue description. HTTP error %d' % e.code)
    except urllib2.URLError as e:
      print('Warning: Failed to retrieve CL description due to network '
            'failure.', file=sys.stderr)
      return ''

  def GetMostRecentPatchset(self):
    return self.GetIssueProperties()['patchsets'][-1]

  def GetIssueProperties(self):
    if self._props is None:
      issue = self.GetIssue()
      if not issue:
        self._props = {}
      else:
        self._props = self.RpcServer().get_issue_properties(issue, True)
    return self._props

  def CannotTriggerTryJobReason(self):
    props = self.GetIssueProperties()
    if not props:
      return 'Rietveld doesn\'t know about your issue %s' % self.GetIssue()
    if props.get('closed'):
      return 'CL %s is closed' % self.GetIssue()
    if props.get('private'):
      return 'CL %s is private' % self.GetIssue()
    return None

  def GetTryJobProperties(self, patchset=None):
    """Returns dictionary of properties to launch try job."""
    project = (self.GetIssueProperties() or {}).get('project')
    return {
      'issue': self.GetIssue(),
      'patch_project': project,
      'patch_storage': 'rietveld',
      'patchset': patchset or self.GetPatchset(),
      'rietveld': self.GetCodereviewServer(),
    }

  def GetIssueOwner(self):
    return (self.GetIssueProperties() or {}).get('owner_email')

  def GetReviewers(self):
    return (self.GetIssueProperties() or {}).get('reviewers')

  def AddComment(self, message, publish=None):
    return self.RpcServer().add_comment(self.GetIssue(), message)

  def GetCommentsSummary(self, _readable=True):
    summary = []
    for message in self.GetIssueProperties().get('messages', []):
      date = datetime.datetime.strptime(message['date'], '%Y-%m-%d %H:%M:%S.%f')
      summary.append(_CommentSummary(
        date=date,
        disapproval=bool(message['disapproval']),
        approval=bool(message['approval']),
        sender=message['sender'],
        message=message['text'],
      ))
    return summary

  def GetStatus(self):
    """Applies a rough heuristic to give a simple summary of an issue's review
    or CQ status, assuming adherence to a common workflow.

    Returns None if no issue for this branch, or one of the following keywords:
      * 'error'    - error from review tool (including deleted issues)
      * 'unsent'   - not sent for review
      * 'waiting'  - waiting for review
      * 'reply'    - waiting for owner to reply to review
      * 'not lgtm' - Code-Review label has been set negatively
      * 'lgtm'     - LGTM from at least one approved reviewer
      * 'commit'   - in the commit queue
      * 'closed'   - closed
    """
    if not self.GetIssue():
      return None

    try:
      props = self.GetIssueProperties()
    except urllib2.HTTPError:
      return 'error'

    if props.get('closed'):
      # Issue is closed.
      return 'closed'
    if props.get('commit') and not props.get('cq_dry_run', False):
      # Issue is in the commit queue.
      return 'commit'

    messages = props.get('messages') or []
    if not messages:
      # No message was sent.
      return 'unsent'

    if get_approving_reviewers(props):
      return 'lgtm'
    elif get_approving_reviewers(props, disapproval=True):
      return 'not lgtm'

    # Skip CQ messages that don't require owner's action.
    while messages and messages[-1]['sender'] == COMMIT_BOT_EMAIL:
      if 'Dry run:' in messages[-1]['text']:
        messages.pop()
      elif 'The CQ bit was unchecked' in messages[-1]['text']:
        # This message always follows prior messages from CQ,
        # so skip this too.
        messages.pop()
      else:
        # This is probably a CQ messages warranting user attention.
        break

    if messages[-1]['sender'] != props.get('owner_email'):
      # Non-LGTM reply from non-owner and not CQ bot.
      return 'reply'
    return 'waiting'

  def UpdateDescriptionRemote(self, description, force=False):
    self.RpcServer().update_description(self.GetIssue(), description)

  def CloseIssue(self):
    return self.RpcServer().close_issue(self.GetIssue())

  def SetFlag(self, flag, value):
    return self.SetFlags({flag: value})

  def SetFlags(self, flags):
    """Sets flags on this CL/patchset in Rietveld.
    """
    patchset = self.GetPatchset() or self.GetMostRecentPatchset()
    try:
      return self.RpcServer().set_flags(
          self.GetIssue(), patchset, flags)
    except urllib2.HTTPError as e:
      if e.code == 404:
        DieWithError('The issue %s doesn\'t exist.' % self.GetIssue())
      if e.code == 403:
        DieWithError(
            ('Access denied to issue %s. Maybe the patchset %s doesn\'t '
             'match?') % (self.GetIssue(), patchset))
      raise

  def RpcServer(self):
    """Returns an upload.RpcServer() to access this review's rietveld instance.
    """
    if not self._rpc_server:
      self._rpc_server = rietveld.CachingRietveld(
          self.GetCodereviewServer(),
          self._auth_config)
    return self._rpc_server

  @classmethod
  def IssueConfigKey(cls):
    return 'rietveldissue'

  @classmethod
  def PatchsetConfigKey(cls):
    return 'rietveldpatchset'

  @classmethod
  def CodereviewServerConfigKey(cls):
    return 'rietveldserver'

  def SetLabels(self, enable_auto_submit, use_commit_queue, cq_dry_run):
    raise NotImplementedError()

  def SetCQState(self, new_state):
    props = self.GetIssueProperties()
    if props.get('private'):
      DieWithError('Cannot set-commit on private issue')

    if new_state == _CQState.COMMIT:
      self.SetFlags({'commit': '1', 'cq_dry_run': '0'})
    elif new_state == _CQState.NONE:
      self.SetFlags({'commit': '0', 'cq_dry_run': '0'})
    else:
      assert new_state == _CQState.DRY_RUN
      self.SetFlags({'commit': '1', 'cq_dry_run': '1'})

  def CMDPatchWithParsedIssue(self, parsed_issue_arg, reject, nocommit,
                              directory, force):
    # PatchIssue should never be called with a dirty tree.  It is up to the
    # caller to check this, but just in case we assert here since the
    # consequences of the caller not checking this could be dire.
    assert(not git_common.is_dirty_git_tree('apply'))
    assert(parsed_issue_arg.valid)
    self._changelist.issue = parsed_issue_arg.issue
    if parsed_issue_arg.hostname:
      self._rietveld_server = 'https://%s' % parsed_issue_arg.hostname

    patchset = parsed_issue_arg.patchset or self.GetMostRecentPatchset()
    patchset_object = self.RpcServer().get_patch(self.GetIssue(), patchset)
    scm_obj = checkout.GitCheckout(settings.GetRoot(), None, None, None, None)
    try:
      scm_obj.apply_patch(patchset_object)
    except Exception as e:
      print(str(e))
      return 1

    # If we had an issue, commit the current state and register the issue.
    if not nocommit:
      self.SetIssue(self.GetIssue())
      self.SetPatchset(patchset)
      RunGit(['commit', '-m', (self.GetDescription() + '\n\n' +
                               'patch from issue %(i)s at patchset '
                               '%(p)s (http://crrev.com/%(i)s#ps%(p)s)'
                               % {'i': self.GetIssue(), 'p': patchset})])
      print('Committed patch locally.')
    else:
      print('Patch applied to index.')
    return 0

  @staticmethod
  def ParseIssueURL(parsed_url):
    if not parsed_url.scheme or not parsed_url.scheme.startswith('http'):
      return None
    # Rietveld patch: https://domain/<number>/#ps<patchset>
    match = re.match(r'/(\d+)/$', parsed_url.path)
    match2 = re.match(r'ps(\d+)$', parsed_url.fragment)
    if match and match2:
      return _ParsedIssueNumberArgument(
          issue=int(match.group(1)),
          patchset=int(match2.group(1)),
          hostname=parsed_url.netloc,
          codereview='rietveld')
    # Typical url: https://domain/<issue_number>[/[other]]
    match = re.match('/(\d+)(/.*)?$', parsed_url.path)
    if match:
      return _ParsedIssueNumberArgument(
          issue=int(match.group(1)),
          hostname=parsed_url.netloc,
          codereview='rietveld')
    # Rietveld patch: https://domain/download/issue<number>_<patchset>.diff
    match = re.match(r'/download/issue(\d+)_(\d+).diff$', parsed_url.path)
    if match:
      return _ParsedIssueNumberArgument(
          issue=int(match.group(1)),
          patchset=int(match.group(2)),
          hostname=parsed_url.netloc,
          codereview='rietveld')
    return None

  def CMDUploadChange(self, options, args, custom_cl_base, change):
    """Upload the patch to Rietveld."""
    upload_args = ['--assume_yes']  # Don't ask about untracked files.
    upload_args.extend(['--server', self.GetCodereviewServer()])
    upload_args.extend(auth.auth_config_to_command_options(self._auth_config))
    if options.emulate_svn_auto_props:
      upload_args.append('--emulate_svn_auto_props')

    change_desc = None

    if options.email is not None:
      upload_args.extend(['--email', options.email])

    if self.GetIssue():
      if options.title is not None:
        upload_args.extend(['--title', options.title])
      if options.message:
        upload_args.extend(['--message', options.message])
      upload_args.extend(['--issue', str(self.GetIssue())])
      print('This branch is associated with issue %s. '
            'Adding patch to that issue.' % self.GetIssue())
    else:
      if options.title is not None:
        upload_args.extend(['--title', options.title])
      if options.message:
        message = options.message
      else:
        message = CreateDescriptionFromLog(args)
        if options.title:
          message = options.title + '\n\n' + message
      change_desc = ChangeDescription(message)
      if options.reviewers or options.add_owners_to:
        change_desc.update_reviewers(options.reviewers, options.tbrs,
                                     options.add_owners_to, change)
      if not options.force:
        change_desc.prompt(bug=options.bug, git_footer=False)

      if not change_desc.description:
        print('Description is empty; aborting.')
        return 1

      upload_args.extend(['--message', change_desc.description])
      if change_desc.get_reviewers():
        upload_args.append('--reviewers=%s' % ','.join(
            change_desc.get_reviewers()))
      if options.send_mail:
        if not change_desc.get_reviewers():
          DieWithError("Must specify reviewers to send email.", change_desc)
        upload_args.append('--send_mail')

      # We check this before applying rietveld.private assuming that in
      # rietveld.cc only addresses which we can send private CLs to are listed
      # if rietveld.private is set, and so we should ignore rietveld.cc only
      # when --private is specified explicitly on the command line.
      if options.private:
        logging.warn('rietveld.cc is ignored since private flag is specified.  '
                     'You need to review and add them manually if necessary.')
        cc = self.GetCCListWithoutDefault()
      else:
        cc = self.GetCCList()
      cc = ','.join(filter(None, (cc, ','.join(options.cc))))
      if change_desc.get_cced():
        cc = ','.join(filter(None, (cc, ','.join(change_desc.get_cced()))))
      if cc:
        upload_args.extend(['--cc', cc])

    if options.private or settings.GetDefaultPrivateFlag() == "True":
      upload_args.append('--private')

    # Include the upstream repo's URL in the change -- this is useful for
    # projects that have their source spread across multiple repos.
    remote_url = self.GetGitBaseUrlFromConfig()
    if not remote_url:
      if self.GetRemoteUrl() and '/' in self.GetUpstreamBranch():
        remote_url = '%s@%s' % (self.GetRemoteUrl(),
                                self.GetUpstreamBranch().split('/')[-1])
    if remote_url:
      remote, remote_branch = self.GetRemoteBranch()
      target_ref = GetTargetRef(remote, remote_branch, options.target_branch)
      if target_ref:
        upload_args.extend(['--target_ref', target_ref])

      # Look for dependent patchsets. See crbug.com/480453 for more details.
      remote, upstream_branch = self.FetchUpstreamTuple(self.GetBranch())
      upstream_branch = ShortBranchName(upstream_branch)
      if remote is '.':
        # A local branch is being tracked.
        local_branch = upstream_branch
        if settings.GetIsSkipDependencyUpload(local_branch):
          print()
          print('Skipping dependency patchset upload because git config '
                'branch.%s.skip-deps-uploads is set to True.' % local_branch)
          print()
        else:
          auth_config = auth.extract_auth_config_from_options(options)
          branch_cl = Changelist(branchref='refs/heads/'+local_branch,
                                 auth_config=auth_config)
          branch_cl_issue_url = branch_cl.GetIssueURL()
          branch_cl_issue = branch_cl.GetIssue()
          branch_cl_patchset = branch_cl.GetPatchset()
          if branch_cl_issue_url and branch_cl_issue and branch_cl_patchset:
            upload_args.extend(
                ['--depends_on_patchset', '%s:%s' % (
                     branch_cl_issue, branch_cl_patchset)])
            print(
                '\n'
                'The current branch (%s) is tracking a local branch (%s) with '
                'an associated CL.\n'
                'Adding %s/#ps%s as a dependency patchset.\n'
                '\n' % (self.GetBranch(), local_branch, branch_cl_issue_url,
                        branch_cl_patchset))

    project = settings.GetProject()
    if project:
      upload_args.extend(['--project', project])
    else:
      print()
      print('WARNING: Uploading without a project specified. Please ensure '
            'your repo\'s codereview.settings has a "PROJECT: foo" line.')
      print()

    try:
      upload_args = ['upload'] + upload_args + args
      logging.info('upload.RealMain(%s)', upload_args)
      issue, patchset = upload.RealMain(upload_args)
      issue = int(issue)
      patchset = int(patchset)
    except KeyboardInterrupt:
      sys.exit(1)
    except:
      # If we got an exception after the user typed a description for their
      # change, back up the description before re-raising.
      if change_desc:
        SaveDescriptionBackup(change_desc)
      raise

    if not self.GetIssue():
      self.SetIssue(issue)
    self.SetPatchset(patchset)
    return 0


class _GerritChangelistImpl(_ChangelistCodereviewBase):
  def __init__(self, changelist, auth_config=None, codereview_host=None):
    # auth_config is Rietveld thing, kept here to preserve interface only.
    super(_GerritChangelistImpl, self).__init__(changelist)
    self._change_id = None
    # Lazily cached values.
    self._gerrit_host = None    # e.g. chromium-review.googlesource.com
    self._gerrit_server = None  # e.g. https://chromium-review.googlesource.com
    # Map from change number (issue) to its detail cache.
    self._detail_cache = {}

    if codereview_host is not None:
      assert not codereview_host.startswith('https://'), codereview_host
      self._gerrit_host = codereview_host
      self._gerrit_server = 'https://%s' % codereview_host

  def _GetGerritHost(self):
    # Lazy load of configs.
    self.GetCodereviewServer()
    if self._gerrit_host and '.' not in self._gerrit_host:
      # Abbreviated domain like "chromium" instead of chromium.googlesource.com.
      # This happens for internal stuff http://crbug.com/614312.
      parsed = urlparse.urlparse(self.GetRemoteUrl())
      if parsed.scheme == 'sso':
        print('WARNING: using non-https URLs for remote is likely broken\n'
              '  Your current remote is: %s'  % self.GetRemoteUrl())
        self._gerrit_host = '%s.googlesource.com' % self._gerrit_host
        self._gerrit_server = 'https://%s' % self._gerrit_host
    return self._gerrit_host

  def _GetGitHost(self):
    """Returns git host to be used when uploading change to Gerrit."""
    return urlparse.urlparse(self.GetRemoteUrl()).netloc

  def GetCodereviewServer(self):
    if not self._gerrit_server:
      # If we're on a branch then get the server potentially associated
      # with that branch.
      if self.GetIssue():
        self._gerrit_server = self._GitGetBranchConfigValue(
            self.CodereviewServerConfigKey())
        if self._gerrit_server:
          self._gerrit_host = urlparse.urlparse(self._gerrit_server).netloc
      if not self._gerrit_server:
        # We assume repo to be hosted on Gerrit, and hence Gerrit server
        # has "-review" suffix for lowest level subdomain.
        parts = self._GetGitHost().split('.')
        parts[0] = parts[0] + '-review'
        self._gerrit_host = '.'.join(parts)
        self._gerrit_server = 'https://%s' % self._gerrit_host
    return self._gerrit_server

  @classmethod
  def IssueConfigKey(cls):
    return 'gerritissue'

  @classmethod
  def PatchsetConfigKey(cls):
    return 'gerritpatchset'

  @classmethod
  def CodereviewServerConfigKey(cls):
    return 'gerritserver'

  def EnsureAuthenticated(self, force, refresh=None):
    """Best effort check that user is authenticated with Gerrit server."""
    if settings.GetGerritSkipEnsureAuthenticated():
      # For projects with unusual authentication schemes.
      # See http://crbug.com/603378.
      return
    # Lazy-loader to identify Gerrit and Git hosts.
    if gerrit_util.GceAuthenticator.is_gce():
      return
    self.GetCodereviewServer()
    git_host = self._GetGitHost()
    assert self._gerrit_server and self._gerrit_host
    cookie_auth = gerrit_util.CookiesAuthenticator()

    gerrit_auth = cookie_auth.get_auth_header(self._gerrit_host)
    git_auth = cookie_auth.get_auth_header(git_host)
    if gerrit_auth and git_auth:
      if gerrit_auth == git_auth:
        return
      all_gsrc = cookie_auth.get_auth_header('d0esN0tEx1st.googlesource.com')
      print((
          'WARNING: You have different credentials for Gerrit and git hosts:\n'
          '           %s\n'
          '           %s\n'
          '        Consider running the following command:\n'
          '          git cl creds-check\n'
          '        %s\n'
          '        %s') %
          (git_host, self._gerrit_host,
           ('Hint: delete creds for .googlesource.com' if all_gsrc else ''),
           cookie_auth.get_new_password_message(git_host)))
      if not force:
        confirm_or_exit('If you know what you are doing', action='continue')
      return
    else:
      missing = (
          ([] if gerrit_auth else [self._gerrit_host]) +
          ([] if git_auth else [git_host]))
      DieWithError('Credentials for the following hosts are required:\n'
                   '  %s\n'
                   'These are read from %s (or legacy %s)\n'
                   '%s' % (
                     '\n  '.join(missing),
                     cookie_auth.get_gitcookies_path(),
                     cookie_auth.get_netrc_path(),
                     cookie_auth.get_new_password_message(git_host)))

  def EnsureCanUploadPatchset(self, force):
    if not self.GetIssue():
      return

    # Warm change details cache now to avoid RPCs later, reducing latency for
    # developers.
    self._GetChangeDetail(
        ['DETAILED_ACCOUNTS', 'CURRENT_REVISION', 'CURRENT_COMMIT'])

    status = self._GetChangeDetail()['status']
    if status in ('MERGED', 'ABANDONED'):
      DieWithError('Change %s has been %s, new uploads are not allowed' %
                   (self.GetIssueURL(),
                    'submitted' if status == 'MERGED' else 'abandoned'))

    if gerrit_util.GceAuthenticator.is_gce():
      return
    cookies_user = gerrit_util.CookiesAuthenticator().get_auth_email(
        self._GetGerritHost())
    if self.GetIssueOwner() == cookies_user:
      return
    logging.debug('change %s owner is %s, cookies user is %s',
                  self.GetIssue(), self.GetIssueOwner(), cookies_user)
    # Maybe user has linked accounts or something like that,
    # so ask what Gerrit thinks of this user.
    details = gerrit_util.GetAccountDetails(self._GetGerritHost(), 'self')
    if details['email'] == self.GetIssueOwner():
      return
    if not force:
      print('WARNING: Change %s is owned by %s, but you authenticate to Gerrit '
            'as %s.\n'
            'Uploading may fail due to lack of permissions.' %
            (self.GetIssue(), self.GetIssueOwner(), details['email']))
      confirm_or_exit(action='upload')


  def _PostUnsetIssueProperties(self):
    """Which branch-specific properties to erase when unsetting issue."""
    return ['gerritsquashhash']

  def GetGerritObjForPresubmit(self):
    return presubmit_support.GerritAccessor(self._GetGerritHost())

  def GetStatus(self):
    """Apply a rough heuristic to give a simple summary of an issue's review
    or CQ status, assuming adherence to a common workflow.

    Returns None if no issue for this branch, or one of the following keywords:
      * 'error'   - error from review tool (including deleted issues)
      * 'unsent'  - no reviewers added
      * 'waiting' - waiting for review
      * 'reply'   - waiting for uploader to reply to review
      * 'lgtm'    - Code-Review label has been set
      * 'commit'  - in the commit queue
      * 'closed'  - successfully submitted or abandoned
    """
    if not self.GetIssue():
      return None

    try:
      data = self._GetChangeDetail([
          'DETAILED_LABELS', 'CURRENT_REVISION', 'SUBMITTABLE'])
    except (httplib.HTTPException, GerritChangeNotExists):
      return 'error'

    if data['status'] in ('ABANDONED', 'MERGED'):
      return 'closed'

    if data['labels'].get('Commit-Queue', {}).get('approved'):
      # The section will have an "approved" subsection if anyone has voted
      # the maximum value on the label.
      return 'commit'

    if data['labels'].get('Code-Review', {}).get('approved'):
      return 'lgtm'

    if not data.get('reviewers', {}).get('REVIEWER', []):
      return 'unsent'

    owner = data['owner'].get('_account_id')
    messages = sorted(data.get('messages', []), key=lambda m: m.get('updated'))
    last_message_author = messages.pop().get('author', {})
    while last_message_author:
      if last_message_author.get('email') == COMMIT_BOT_EMAIL:
        # Ignore replies from CQ.
        last_message_author = messages.pop().get('author', {})
        continue
      if last_message_author.get('_account_id') == owner:
        # Most recent message was by owner.
        return 'waiting'
      else:
        # Some reply from non-owner.
        return 'reply'

    # Somehow there are no messages even though there are reviewers.
    return 'unsent'

  def GetMostRecentPatchset(self):
    data = self._GetChangeDetail(['CURRENT_REVISION'])
    patchset = data['revisions'][data['current_revision']]['_number']
    self.SetPatchset(patchset)
    return patchset

  def FetchDescription(self, force=False):
    data = self._GetChangeDetail(['CURRENT_REVISION', 'CURRENT_COMMIT'],
                                 no_cache=force)
    current_rev = data['current_revision']
    return data['revisions'][current_rev]['commit']['message']

  def UpdateDescriptionRemote(self, description, force=False):
    if gerrit_util.HasPendingChangeEdit(self._GetGerritHost(), self.GetIssue()):
      if not force:
        confirm_or_exit(
            'The description cannot be modified while the issue has a pending '
            'unpublished edit. Either publish the edit in the Gerrit web UI '
            'or delete it.\n\n', action='delete the unpublished edit')

      gerrit_util.DeletePendingChangeEdit(self._GetGerritHost(),
                                          self.GetIssue())
    gerrit_util.SetCommitMessage(self._GetGerritHost(), self.GetIssue(),
                                 description, notify='NONE')

  def AddComment(self, message, publish=None):
    gerrit_util.SetReview(self._GetGerritHost(), self.GetIssue(),
                          msg=message, ready=publish)

  def GetCommentsSummary(self, readable=True):
    # DETAILED_ACCOUNTS is to get emails in accounts.
    messages = self._GetChangeDetail(
        options=['MESSAGES', 'DETAILED_ACCOUNTS']).get('messages', [])
    file_comments = gerrit_util.GetChangeComments(
        self._GetGerritHost(), self.GetIssue())

    # Build dictionary of file comments for easy access and sorting later.
    # {author+date: {path: {patchset: {line: url+message}}}}
    comments = collections.defaultdict(
        lambda: collections.defaultdict(lambda: collections.defaultdict(dict)))
    for path, line_comments in file_comments.iteritems():
      for comment in line_comments:
        if comment.get('tag', '').startswith('autogenerated'):
          continue
        key = (comment['author']['email'], comment['updated'])
        if comment.get('side', 'REVISION') == 'PARENT':
          patchset = 'Base'
        else:
          patchset = 'PS%d' % comment['patch_set']
        line = comment.get('line', 0)
        url = ('https://%s/c/%s/%s/%s#%s%s' %
            (self._GetGerritHost(), self.GetIssue(), comment['patch_set'], path,
             'b' if comment.get('side') == 'PARENT' else '',
             str(line) if line else ''))
        comments[key][path][patchset][line] = (url, comment['message'])

    summary = []
    for msg in messages:
      # Don't bother showing autogenerated messages.
      if msg.get('tag') and msg.get('tag').startswith('autogenerated'):
        continue
      # Gerrit spits out nanoseconds.
      assert len(msg['date'].split('.')[-1]) == 9
      date = datetime.datetime.strptime(msg['date'][:-3],
                                        '%Y-%m-%d %H:%M:%S.%f')
      message = msg['message']
      key = (msg['author']['email'], msg['date'])
      if key in comments:
        message += '\n'
      for path, patchsets in sorted(comments.get(key, {}).items()):
        if readable:
          message += '\n%s' % path
        for patchset, lines in sorted(patchsets.items()):
          for line, (url, content) in sorted(lines.items()):
            if line:
              line_str = 'Line %d' % line
              path_str = '%s:%d:' % (path, line)
            else:
              line_str = 'File comment'
              path_str = '%s:0:' % path
            if readable:
              message += '\n  %s, %s: %s' % (patchset, line_str, url)
              message += '\n  %s\n' % content
            else:
              message += '\n%s ' % path_str
              message += '\n%s\n' % content

      summary.append(_CommentSummary(
        date=date,
        message=message,
        sender=msg['author']['email'],
        # These could be inferred from the text messages and correlated with
        # Code-Review label maximum, however this is not reliable.
        # Leaving as is until the need arises.
        approval=False,
        disapproval=False,
      ))
    return summary

  def CloseIssue(self):
    gerrit_util.AbandonChange(self._GetGerritHost(), self.GetIssue(), msg='')

  def SubmitIssue(self, wait_for_merge=True):
    gerrit_util.SubmitChange(self._GetGerritHost(), self.GetIssue(),
                             wait_for_merge=wait_for_merge)

  def _GetChangeDetail(self, options=None, issue=None,
                       no_cache=False):
    """Returns details of the issue by querying Gerrit and caching results.

    If fresh data is needed, set no_cache=True which will clear cache and
    thus new data will be fetched from Gerrit.
    """
    options = options or []
    issue = issue or self.GetIssue()
    assert issue, 'issue is required to query Gerrit'

    # Optimization to avoid multiple RPCs:
    if (('CURRENT_REVISION' in options or 'ALL_REVISIONS' in options) and
        'CURRENT_COMMIT' not in options):
      options.append('CURRENT_COMMIT')

    # Normalize issue and options for consistent keys in cache.
    issue = str(issue)
    options = [o.upper() for o in options]

    # Check in cache first unless no_cache is True.
    if no_cache:
      self._detail_cache.pop(issue, None)
    else:
      options_set = frozenset(options)
      for cached_options_set, data in self._detail_cache.get(issue, []):
        # Assumption: data fetched before with extra options is suitable
        # for return for a smaller set of options.
        # For example, if we cached data for
        #     options=[CURRENT_REVISION, DETAILED_FOOTERS]
        #   and request is for options=[CURRENT_REVISION],
        # THEN we can return prior cached data.
        if options_set.issubset(cached_options_set):
          return data

    try:
      data = gerrit_util.GetChangeDetail(
          self._GetGerritHost(), str(issue), options)
    except gerrit_util.GerritError as e:
      if e.http_status == 404:
        raise GerritChangeNotExists(issue, self.GetCodereviewServer())
      raise

    self._detail_cache.setdefault(issue, []).append((frozenset(options), data))
    return data

  def _GetChangeCommit(self, issue=None):
    issue = issue or self.GetIssue()
    assert issue, 'issue is required to query Gerrit'
    try:
      data = gerrit_util.GetChangeCommit(self._GetGerritHost(), str(issue))
    except gerrit_util.GerritError as e:
      if e.http_status == 404:
        raise GerritChangeNotExists(issue, self.GetCodereviewServer())
      raise
    return data

  def CMDLand(self, force, bypass_hooks, verbose, parallel):
    if git_common.is_dirty_git_tree('land'):
      return 1
    detail = self._GetChangeDetail(['CURRENT_REVISION', 'LABELS'])
    if u'Commit-Queue' in detail.get('labels', {}):
      if not force:
        confirm_or_exit('\nIt seems this repository has a Commit Queue, '
                        'which can test and land changes for you. '
                        'Are you sure you wish to bypass it?\n',
                        action='bypass CQ')

    differs = True
    last_upload = self._GitGetBranchConfigValue('gerritsquashhash')
    # Note: git diff outputs nothing if there is no diff.
    if not last_upload or RunGit(['diff', last_upload]).strip():
      print('WARNING: Some changes from local branch haven\'t been uploaded.')
    else:
      if detail['current_revision'] == last_upload:
        differs = False
      else:
        print('WARNING: Local branch contents differ from latest uploaded '
              'patchset.')
    if differs:
      if not force:
        confirm_or_exit(
            'Do you want to submit latest Gerrit patchset and bypass hooks?\n',
            action='submit')
      print('WARNING: Bypassing hooks and submitting latest uploaded patchset.')
    elif not bypass_hooks:
      hook_results = self.RunHook(
          committing=True,
          may_prompt=not force,
          verbose=verbose,
          change=self.GetChange(self.GetCommonAncestorWithUpstream(), None),
          parallel=parallel)
      if not hook_results.should_continue():
        return 1

    self.SubmitIssue(wait_for_merge=True)
    print('Issue %s has been submitted.' % self.GetIssueURL())
    links = self._GetChangeCommit().get('web_links', [])
    for link in links:
      if link.get('name') == 'gitiles' and link.get('url'):
        print('Landed as: %s' % link.get('url'))
        break
    return 0

  def CMDPatchWithParsedIssue(self, parsed_issue_arg, reject, nocommit,
                              directory, force):
    assert not reject
    assert not directory
    assert parsed_issue_arg.valid

    self._changelist.issue = parsed_issue_arg.issue

    if parsed_issue_arg.hostname:
      self._gerrit_host = parsed_issue_arg.hostname
      self._gerrit_server = 'https://%s' % self._gerrit_host

    try:
      detail = self._GetChangeDetail(['ALL_REVISIONS'])
    except GerritChangeNotExists as e:
      DieWithError(str(e))

    if not parsed_issue_arg.patchset:
      # Use current revision by default.
      revision_info = detail['revisions'][detail['current_revision']]
      patchset = int(revision_info['_number'])
    else:
      patchset = parsed_issue_arg.patchset
      for revision_info in detail['revisions'].itervalues():
        if int(revision_info['_number']) == parsed_issue_arg.patchset:
          break
      else:
        DieWithError('Couldn\'t find patchset %i in change %i' %
                     (parsed_issue_arg.patchset, self.GetIssue()))

    remote_url = self._changelist.GetRemoteUrl()
    if remote_url.endswith('.git'):
      remote_url = remote_url[:-len('.git')]
    fetch_info = revision_info['fetch']['http']

    if remote_url != fetch_info['url']:
      DieWithError('Trying to patch a change from %s but this repo appears '
                   'to be %s.' % (fetch_info['url'], remote_url))

    RunGit(['fetch', fetch_info['url'], fetch_info['ref']])

    if force:
      RunGit(['reset', '--hard', 'FETCH_HEAD'])
      print('Checked out commit for change %i patchset %i locally' %
            (parsed_issue_arg.issue, patchset))
    elif nocommit:
      RunGit(['cherry-pick', '--no-commit', 'FETCH_HEAD'])
      print('Patch applied to index.')
    else:
      RunGit(['cherry-pick', 'FETCH_HEAD'])
      print('Committed patch for change %i patchset %i locally.' %
            (parsed_issue_arg.issue, patchset))
      print('Note: this created a local commit which does not have '
            'the same hash as the one uploaded for review. This will make '
            'uploading changes based on top of this branch difficult.\n'
            'If you want to do that, use "git cl patch --force" instead.')

    if self.GetBranch():
      self.SetIssue(parsed_issue_arg.issue)
      self.SetPatchset(patchset)
      fetched_hash = RunGit(['rev-parse', 'FETCH_HEAD']).strip()
      self._GitSetBranchConfigValue('last-upload-hash', fetched_hash)
      self._GitSetBranchConfigValue('gerritsquashhash', fetched_hash)
    else:
      print('WARNING: You are in detached HEAD state.\n'
            'The patch has been applied to your checkout, but you will not be '
            'able to upload a new patch set to the gerrit issue.\n'
            'Try using the \'-b\' option if you would like to work on a '
            'branch and/or upload a new patch set.')

    return 0

  @staticmethod
  def ParseIssueURL(parsed_url):
    if not parsed_url.scheme or not parsed_url.scheme.startswith('http'):
      return None
    # Gerrit's new UI is https://domain/c/project/+/<issue_number>[/[patchset]]
    # But old GWT UI is https://domain/#/c/project/+/<issue_number>[/[patchset]]
    # Short urls like https://domain/<issue_number> can be used, but don't allow
    # specifying the patchset (you'd 404), but we allow that here.
    if parsed_url.path == '/':
      part = parsed_url.fragment
    else:
      part = parsed_url.path
    match = re.match('(/c(/.*/\+)?)?/(\d+)(/(\d+)?/?)?$', part)
    if match:
      return _ParsedIssueNumberArgument(
          issue=int(match.group(3)),
          patchset=int(match.group(5)) if match.group(5) else None,
          hostname=parsed_url.netloc,
          codereview='gerrit')
    return None

  def _GerritCommitMsgHookCheck(self, offer_removal):
    hook = os.path.join(settings.GetRoot(), '.git', 'hooks', 'commit-msg')
    if not os.path.exists(hook):
      return
    # Crude attempt to distinguish Gerrit Codereview hook from potentially
    # custom developer made one.
    data = gclient_utils.FileRead(hook)
    if not('From Gerrit Code Review' in data and 'add_ChangeId()' in data):
      return
    print('WARNING: You have Gerrit commit-msg hook installed.\n'
          'It is not necessary for uploading with git cl in squash mode, '
          'and may interfere with it in subtle ways.\n'
          'We recommend you remove the commit-msg hook.')
    if offer_removal:
      if ask_for_explicit_yes('Do you want to remove it now?'):
        gclient_utils.rm_file_or_tree(hook)
        print('Gerrit commit-msg hook removed.')
      else:
        print('OK, will keep Gerrit commit-msg hook in place.')

  def CMDUploadChange(self, options, git_diff_args, custom_cl_base, change):
    """Upload the current branch to Gerrit."""
    if options.squash and options.no_squash:
      DieWithError('Can only use one of --squash or --no-squash')

    if not options.squash and not options.no_squash:
      # Load default for user, repo, squash=true, in this order.
      options.squash = settings.GetSquashGerritUploads()
    elif options.no_squash:
      options.squash = False

    remote, remote_branch = self.GetRemoteBranch()
    branch = GetTargetRef(remote, remote_branch, options.target_branch)

    # This may be None; default fallback value is determined in logic below.
    title = options.title

    # Extract bug number from branch name.
    bug = options.bug
    match = re.match(r'(?:bug|fix)[_-]?(\d+)', self.GetBranch())
    if not bug and match:
      bug = match.group(1)

    if options.squash:
      self._GerritCommitMsgHookCheck(offer_removal=not options.force)
      if self.GetIssue():
        # Try to get the message from a previous upload.
        message = self.GetDescription()
        if not message:
          DieWithError(
              'failed to fetch description from current Gerrit change %d\n'
              '%s' % (self.GetIssue(), self.GetIssueURL()))
        if not title:
          if options.message:
            # When uploading a subsequent patchset, -m|--message is taken
            # as the patchset title if --title was not provided.
            title = options.message.strip()
          else:
            default_title = RunGit(
                ['show', '-s', '--format=%s', 'HEAD']).strip()
            if options.force:
              title = default_title
            else:
              title = ask_for_data(
                  'Title for patchset [%s]: ' % default_title) or default_title
        change_id = self._GetChangeDetail()['change_id']
        while True:
          footer_change_ids = git_footers.get_footer_change_id(message)
          if footer_change_ids == [change_id]:
            break
          if not footer_change_ids:
            message = git_footers.add_footer_change_id(message, change_id)
            print('WARNING: appended missing Change-Id to change description.')
            continue
          # There is already a valid footer but with different or several ids.
          # Doing this automatically is non-trivial as we don't want to lose
          # existing other footers, yet we want to append just 1 desired
          # Change-Id. Thus, just create a new footer, but let user verify the
          # new description.
          message = '%s\n\nChange-Id: %s' % (message, change_id)
          print(
              'WARNING: change %s has Change-Id footer(s):\n'
              '  %s\n'
              'but change has Change-Id %s, according to Gerrit.\n'
              'Please, check the proposed correction to the description, '
              'and edit it if necessary but keep the "Change-Id: %s" footer\n'
              % (self.GetIssue(), '\n  '.join(footer_change_ids), change_id,
                 change_id))
          confirm_or_exit(action='edit')
          if not options.force:
            change_desc = ChangeDescription(message)
            change_desc.prompt(bug=bug)
            message = change_desc.description
            if not message:
              DieWithError("Description is empty. Aborting...")
          # Continue the while loop.
        # Sanity check of this code - we should end up with proper message
        # footer.
        assert [change_id] == git_footers.get_footer_change_id(message)
        change_desc = ChangeDescription(message)
      else:  # if not self.GetIssue()
        if options.message:
          message = options.message
        else:
          message = CreateDescriptionFromLog(git_diff_args)
          if options.title:
            message = options.title + '\n\n' + message
        change_desc = ChangeDescription(message)

        if not options.force:
          change_desc.prompt(bug=bug)
        # On first upload, patchset title is always this string, while
        # --title flag gets converted to first line of message.
        title = 'Initial upload'
        if not change_desc.description:
          DieWithError("Description is empty. Aborting...")
        change_ids = git_footers.get_footer_change_id(change_desc.description)
        if len(change_ids) > 1:
          DieWithError('too many Change-Id footers, at most 1 allowed.')
        if not change_ids:
          # Generate the Change-Id automatically.
          change_desc.set_description(git_footers.add_footer_change_id(
              change_desc.description,
              GenerateGerritChangeId(change_desc.description)))
          change_ids = git_footers.get_footer_change_id(change_desc.description)
          assert len(change_ids) == 1
        change_id = change_ids[0]

      if options.reviewers or options.tbrs or options.add_owners_to:
        change_desc.update_reviewers(options.reviewers, options.tbrs,
                                     options.add_owners_to, change)

      remote, upstream_branch = self.FetchUpstreamTuple(self.GetBranch())
      parent = self._ComputeParent(remote, upstream_branch, custom_cl_base,
                                   options.force, change_desc)
      tree = RunGit(['rev-parse', 'HEAD:']).strip()
      with tempfile.NamedTemporaryFile(delete=False) as desc_tempfile:
        desc_tempfile.write(change_desc.description)
        desc_tempfile.close()
        ref_to_push = RunGit(['commit-tree', tree, '-p', parent,
                              '-F', desc_tempfile.name]).strip()
        os.remove(desc_tempfile.name)
    else:
      change_desc = ChangeDescription(
          options.message or CreateDescriptionFromLog(git_diff_args))
      if not change_desc.description:
        DieWithError("Description is empty. Aborting...")

      if not git_footers.get_footer_change_id(change_desc.description):
        DownloadGerritHook(False)
        change_desc.set_description(
            self._AddChangeIdToCommitMessage(options, git_diff_args))
      if options.reviewers or options.tbrs or options.add_owners_to:
        change_desc.update_reviewers(options.reviewers, options.tbrs,
                                     options.add_owners_to, change)
      ref_to_push = 'HEAD'
      # For no-squash mode, we assume the remote called "origin" is the one we
      # want. It is not worthwhile to support different workflows for
      # no-squash mode.
      parent = 'origin/%s' % branch
      change_id = git_footers.get_footer_change_id(change_desc.description)[0]

    assert change_desc
    commits = RunGitSilent(['rev-list', '%s..%s' % (parent,
                                                    ref_to_push)]).splitlines()
    if len(commits) > 1:
      print('WARNING: This will upload %d commits. Run the following command '
            'to see which commits will be uploaded: ' % len(commits))
      print('git log %s..%s' % (parent, ref_to_push))
      print('You can also use `git squash-branch` to squash these into a '
            'single commit.')
      confirm_or_exit(action='upload')

    if options.reviewers or options.tbrs or options.add_owners_to:
      change_desc.update_reviewers(options.reviewers, options.tbrs,
                                   options.add_owners_to, change)

    # Extra options that can be specified at push time. Doc:
    # https://gerrit-review.googlesource.com/Documentation/user-upload.html
    refspec_opts = []

    # By default, new changes are started in WIP mode, and subsequent patchsets
    # don't send email. At any time, passing --send-mail will mark the change
    # ready and send email for that particular patch.
    if options.send_mail:
      refspec_opts.append('ready')
      refspec_opts.append('notify=ALL')
    elif not self.GetIssue() and options.squash:
      refspec_opts.append('wip')
    else:
      refspec_opts.append('notify=NONE')

    # TODO(tandrii): options.message should be posted as a comment
    # if --send-mail is set on non-initial upload as Rietveld used to do it.

    if title:
      # Punctuation and whitespace in |title| must be percent-encoded.
      refspec_opts.append('m=' + gerrit_util.PercentEncodeForGitRef(title))

    if options.private:
      refspec_opts.append('private')

    if options.topic:
      # Documentation on Gerrit topics is here:
      # https://gerrit-review.googlesource.com/Documentation/user-upload.html#topic
      refspec_opts.append('topic=%s' % options.topic)

    # Gerrit sorts hashtags, so order is not important.
    hashtags = {change_desc.sanitize_hash_tag(t) for t in options.hashtags}
    if not self.GetIssue():
      hashtags.update(change_desc.get_hash_tags())
    refspec_opts += ['hashtag=%s' % t for t in sorted(hashtags)]

    refspec_suffix = ''
    if refspec_opts:
      refspec_suffix = '%' + ','.join(refspec_opts)
      assert ' ' not in refspec_suffix, (
          'spaces not allowed in refspec: "%s"' % refspec_suffix)
    refspec = '%s:refs/for/%s%s' % (ref_to_push, branch, refspec_suffix)

    try:
      push_stdout = gclient_utils.CheckCallAndFilter(
          ['git', 'push', self.GetRemoteUrl(), refspec],
          print_stdout=True,
          # Flush after every line: useful for seeing progress when running as
          # recipe.
          filter_fn=lambda _: sys.stdout.flush())
    except subprocess2.CalledProcessError:
      DieWithError('Failed to create a change. Please examine output above '
                   'for the reason of the failure.\n'
                   'Hint: run command below to diagnose common Git/Gerrit '
                   'credential problems:\n'
                   '  git cl creds-check\n',
                   change_desc)

    if options.squash:
      regex = re.compile(r'remote:\s+https?://[\w\-\.\+\/#]*/(\d+)\s.*')
      change_numbers = [m.group(1)
                        for m in map(regex.match, push_stdout.splitlines())
                        if m]
      if len(change_numbers) != 1:
        DieWithError(
          ('Created|Updated %d issues on Gerrit, but only 1 expected.\n'
           'Change-Id: %s') % (len(change_numbers), change_id), change_desc)
      self.SetIssue(change_numbers[0])
      self._GitSetBranchConfigValue('gerritsquashhash', ref_to_push)

    reviewers = sorted(change_desc.get_reviewers())

    # Add cc's from the CC_LIST and --cc flag (if any).
    if not options.private:
      cc = self.GetCCList().split(',')
    else:
      cc = []
    if options.cc:
      cc.extend(options.cc)
    cc = filter(None, [email.strip() for email in cc])
    if change_desc.get_cced():
      cc.extend(change_desc.get_cced())

    gerrit_util.AddReviewers(
        self._GetGerritHost(), self.GetIssue(), reviewers, cc,
        notify=bool(options.send_mail))

    if change_desc.get_reviewers(tbr_only=True):
      labels = self._GetChangeDetail(['LABELS']).get('labels', {})
      score = 1
      if 'Code-Review' in labels and 'values' in labels['Code-Review']:
        score = max([int(x) for x in labels['Code-Review']['values'].keys()])
      print('Adding self-LGTM (Code-Review +%d) because of TBRs.' % score)
      gerrit_util.SetReview(
          self._GetGerritHost(), self.GetIssue(),
          msg='Self-approving for TBR',
          labels={'Code-Review': score})

    return 0

  def _ComputeParent(self, remote, upstream_branch, custom_cl_base, force,
                     change_desc):
    """Computes parent of the generated commit to be uploaded to Gerrit.

    Returns revision or a ref name.
    """
    if custom_cl_base:
      # Try to avoid creating additional unintended CLs when uploading, unless
      # user wants to take this risk.
      local_ref_of_target_remote = self.GetRemoteBranch()[1]
      code, _ = RunGitWithCode(['merge-base', '--is-ancestor', custom_cl_base,
                                local_ref_of_target_remote])
      if code == 1:
        print('\nWARNING: Manually specified base of this CL `%s` '
              'doesn\'t seem to belong to target remote branch `%s`.\n\n'
              'If you proceed with upload, more than 1 CL may be created by '
              'Gerrit as a result, in turn confusing or crashing git cl.\n\n'
              'If you are certain that specified base `%s` has already been '
              'uploaded to Gerrit as another CL, you may proceed.\n' %
              (custom_cl_base, local_ref_of_target_remote, custom_cl_base))
        if not force:
          confirm_or_exit(
              'Do you take responsibility for cleaning up potential mess '
              'resulting from proceeding with upload?',
              action='upload')
      return custom_cl_base

    if remote != '.':
      return self.GetCommonAncestorWithUpstream()

    # If our upstream branch is local, we base our squashed commit on its
    # squashed version.
    upstream_branch_name = scm.GIT.ShortBranchName(upstream_branch)

    if upstream_branch_name == 'master':
      return self.GetCommonAncestorWithUpstream()

    # Check the squashed hash of the parent.
    # TODO(tandrii): consider checking parent change in Gerrit and using its
    # hash if tree hash of latest parent revision (patchset) in Gerrit matches
    # the tree hash of the parent branch. The upside is less likely bogus
    # requests to reupload parent change just because it's uploadhash is
    # missing, yet the downside likely exists, too (albeit unknown to me yet).
    parent = RunGit(['config',
                     'branch.%s.gerritsquashhash' % upstream_branch_name],
                    error_ok=True).strip()
    # Verify that the upstream branch has been uploaded too, otherwise
    # Gerrit will create additional CLs when uploading.
    if not parent or (RunGitSilent(['rev-parse', upstream_branch + ':']) !=
                      RunGitSilent(['rev-parse', parent + ':'])):
      DieWithError(
          '\nUpload upstream branch %s first.\n'
          'It is likely that this branch has been rebased since its last '
          'upload, so you just need to upload it again.\n'
          '(If you uploaded it with --no-squash, then branch dependencies '
          'are not supported, and you should reupload with --squash.)'
          % upstream_branch_name,
          change_desc)
    return parent

  def _AddChangeIdToCommitMessage(self, options, args):
    """Re-commits using the current message, assumes the commit hook is in
    place.
    """
    log_desc = options.message or CreateDescriptionFromLog(args)
    git_command = ['commit', '--amend', '-m', log_desc]
    RunGit(git_command)
    new_log_desc = CreateDescriptionFromLog(args)
    if git_footers.get_footer_change_id(new_log_desc):
      print('git-cl: Added Change-Id to commit message.')
      return new_log_desc
    else:
      DieWithError('ERROR: Gerrit commit-msg hook not installed.')

  def SetLabels(self, enable_auto_submit, use_commit_queue, cq_dry_run):
    """Sets labels on the change based on the provided flags."""
    labels = {}
    notify = None;
    if enable_auto_submit:
      labels['Auto-Submit'] = 1
    if use_commit_queue:
      labels['Commit-Queue'] = 2
    elif cq_dry_run:
      labels['Commit-Queue'] = 1
      notify = False
    if labels:
      gerrit_util.SetReview(self._GetGerritHost(), self.GetIssue(),
                            labels=labels, notify=notify)

  def SetCQState(self, new_state):
    """Sets the Commit-Queue label assuming canonical CQ config for Gerrit."""
    vote_map = {
        _CQState.NONE:    0,
        _CQState.DRY_RUN: 1,
        _CQState.COMMIT:  2,
    }
    labels = {'Commit-Queue': vote_map[new_state]}
    notify = False if new_state == _CQState.DRY_RUN else None
    gerrit_util.SetReview(self._GetGerritHost(), self.GetIssue(),
                          labels=labels, notify=notify)

  def CannotTriggerTryJobReason(self):
    try:
      data = self._GetChangeDetail()
    except GerritChangeNotExists:
      return 'Gerrit doesn\'t know about your change %s' % self.GetIssue()

    if data['status'] in ('ABANDONED', 'MERGED'):
      return 'CL %s is closed' % self.GetIssue()

  def GetTryJobProperties(self, patchset=None):
    """Returns dictionary of properties to launch try job."""
    data = self._GetChangeDetail(['ALL_REVISIONS'])
    patchset = int(patchset or self.GetPatchset())
    assert patchset
    revision_data = None  # Pylint wants it to be defined.
    for revision_data in data['revisions'].itervalues():
      if int(revision_data['_number']) == patchset:
        break
    else:
      raise Exception('Patchset %d is not known in Gerrit change %d' %
                      (patchset, self.GetIssue()))
    return {
      'patch_issue': self.GetIssue(),
      'patch_set': patchset or self.GetPatchset(),
      'patch_project': data['project'],
      'patch_storage': 'gerrit',
      'patch_ref': revision_data['fetch']['http']['ref'],
      'patch_repository_url': revision_data['fetch']['http']['url'],
      'patch_gerrit_url': self.GetCodereviewServer(),
    }

  def GetIssueOwner(self):
    return self._GetChangeDetail(['DETAILED_ACCOUNTS'])['owner']['email']

  def GetReviewers(self):
    details = self._GetChangeDetail(['DETAILED_ACCOUNTS'])
    return [reviewer['email'] for reviewer in details['reviewers']['REVIEWER']]


_CODEREVIEW_IMPLEMENTATIONS = {
  'rietveld': _RietveldChangelistImpl,
  'gerrit': _GerritChangelistImpl,
}


def _add_codereview_issue_select_options(parser, extra=""):
  _add_codereview_select_options(parser)

  text = ('Operate on this issue number instead of the current branch\'s '
          'implicit issue.')
  if extra:
    text += ' '+extra
  parser.add_option('-i', '--issue', type=int, help=text)


def _process_codereview_issue_select_options(parser, options):
  _process_codereview_select_options(parser, options)
  if options.issue is not None and not options.forced_codereview:
    parser.error('--issue must be specified with either --rietveld or --gerrit')


def _add_codereview_select_options(parser):
  """Appends --gerrit and --rietveld options to force specific codereview."""
  parser.codereview_group = optparse.OptionGroup(
      parser, 'EXPERIMENTAL! Codereview override options')
  parser.add_option_group(parser.codereview_group)
  parser.codereview_group.add_option(
      '--gerrit', action='store_true',
      help='Force the use of Gerrit for codereview')
  parser.codereview_group.add_option(
      '--rietveld', action='store_true',
      help='Force the use of Rietveld for codereview')


def _process_codereview_select_options(parser, options):
  if options.gerrit and options.rietveld:
    parser.error('Options --gerrit and --rietveld are mutually exclusive')
  options.forced_codereview = None
  if options.gerrit:
    options.forced_codereview = 'gerrit'
  elif options.rietveld:
    options.forced_codereview = 'rietveld'


def _get_bug_line_values(default_project, bugs):
  """Given default_project and comma separated list of bugs, yields bug line
  values.

  Each bug can be either:
    * a number, which is combined with default_project
    * string, which is left as is.

  This function may produce more than one line, because bugdroid expects one
  project per line.

  >>> list(_get_bug_line_values('v8', '123,chromium:789'))
      ['v8:123', 'chromium:789']
  """
  default_bugs = []
  others = []
  for bug in bugs.split(','):
    bug = bug.strip()
    if bug:
      try:
        default_bugs.append(int(bug))
      except ValueError:
        others.append(bug)

  if default_bugs:
    default_bugs = ','.join(map(str, default_bugs))
    if default_project:
      yield '%s:%s' % (default_project, default_bugs)
    else:
      yield default_bugs
  for other in sorted(others):
    # Don't bother finding common prefixes, CLs with >2 bugs are very very rare.
    yield other


class ChangeDescription(object):
  """Contains a parsed form of the change description."""
  R_LINE = r'^[ \t]*(TBR|R)[ \t]*=[ \t]*(.*?)[ \t]*$'
  CC_LINE = r'^[ \t]*(CC)[ \t]*=[ \t]*(.*?)[ \t]*$'
  BUG_LINE = r'^[ \t]*(?:(BUG)[ \t]*=|Bug:)[ \t]*(.*?)[ \t]*$'
  CHERRY_PICK_LINE = r'^\(cherry picked from commit [a-fA-F0-9]{40}\)$'
  STRIP_HASH_TAG_PREFIX = r'^(\s*(revert|reland)( "|:)?\s*)*'
  BRACKET_HASH_TAG = r'\s*\[([^\[\]]+)\]'
  COLON_SEPARATED_HASH_TAG = r'^([a-zA-Z0-9_\- ]+):'
  BAD_HASH_TAG_CHUNK = r'[^a-zA-Z0-9]+'

  def __init__(self, description):
    self._description_lines = (description or '').strip().splitlines()

  @property               # www.logilab.org/ticket/89786
  def description(self):  # pylint: disable=method-hidden
    return '\n'.join(self._description_lines)

  def set_description(self, desc):
    if isinstance(desc, basestring):
      lines = desc.splitlines()
    else:
      lines = [line.rstrip() for line in desc]
    while lines and not lines[0]:
      lines.pop(0)
    while lines and not lines[-1]:
      lines.pop(-1)
    self._description_lines = lines

  def update_reviewers(self, reviewers, tbrs, add_owners_to=None, change=None):
    """Rewrites the R=/TBR= line(s) as a single line each.

    Args:
      reviewers (list(str)) - list of additional emails to use for reviewers.
      tbrs (list(str)) - list of additional emails to use for TBRs.
      add_owners_to (None|'R'|'TBR') - Pass to do an OWNERS lookup for files in
        the change that are missing OWNER coverage. If this is not None, you
        must also pass a value for `change`.
      change (Change) - The Change that should be used for OWNERS lookups.
    """
    assert isinstance(reviewers, list), reviewers
    assert isinstance(tbrs, list), tbrs

    assert add_owners_to in (None, 'TBR', 'R'), add_owners_to
    assert not add_owners_to or change, add_owners_to

    if not reviewers and not tbrs and not add_owners_to:
      return

    reviewers = set(reviewers)
    tbrs = set(tbrs)
    LOOKUP = {
      'TBR': tbrs,
      'R': reviewers,
    }

    # Get the set of R= and TBR= lines and remove them from the description.
    regexp = re.compile(self.R_LINE)
    matches = [regexp.match(line) for line in self._description_lines]
    new_desc = [l for i, l in enumerate(self._description_lines)
                if not matches[i]]
    self.set_description(new_desc)

    # Construct new unified R= and TBR= lines.

    # First, update tbrs/reviewers with names from the R=/TBR= lines (if any).
    for match in matches:
      if not match:
        continue
      LOOKUP[match.group(1)].update(cleanup_list([match.group(2).strip()]))

    # Next, maybe fill in OWNERS coverage gaps to either tbrs/reviewers.
    if add_owners_to:
      owners_db = owners.Database(change.RepositoryRoot(),
                                  fopen=file, os_path=os.path)
      missing_files = owners_db.files_not_covered_by(change.LocalPaths(),
                                                     (tbrs | reviewers))
      LOOKUP[add_owners_to].update(
        owners_db.reviewers_for(missing_files, change.author_email))

    # If any folks ended up in both groups, remove them from tbrs.
    tbrs -= reviewers

    new_r_line = 'R=' + ', '.join(sorted(reviewers)) if reviewers else None
    new_tbr_line = 'TBR=' + ', '.join(sorted(tbrs)) if tbrs else None

    # Put the new lines in the description where the old first R= line was.
    line_loc = next((i for i, match in enumerate(matches) if match), -1)
    if 0 <= line_loc < len(self._description_lines):
      if new_tbr_line:
        self._description_lines.insert(line_loc, new_tbr_line)
      if new_r_line:
        self._description_lines.insert(line_loc, new_r_line)
    else:
      if new_r_line:
        self.append_footer(new_r_line)
      if new_tbr_line:
        self.append_footer(new_tbr_line)

  def prompt(self, bug=None, git_footer=True):
    """Asks the user to update the description."""
    self.set_description([
      '# Enter a description of the change.',
      '# This will be displayed on the codereview site.',
      '# The first line will also be used as the subject of the review.',
      '#--------------------This line is 72 characters long'
      '--------------------',
    ] + self._description_lines)

    regexp = re.compile(self.BUG_LINE)
    if not any((regexp.match(line) for line in self._description_lines)):
      prefix = settings.GetBugPrefix()
      values = list(_get_bug_line_values(prefix, bug or '')) or [prefix]
      if git_footer:
        self.append_footer('Bug: %s' % ', '.join(values))
      else:
        for value in values:
          self.append_footer('BUG=%s' % value)

    content = gclient_utils.RunEditor(self.description, True,
                                      git_editor=settings.GetGitEditor())
    if not content:
      DieWithError('Running editor failed')
    lines = content.splitlines()

    # Strip off comments and default inserted "Bug:" line.
    clean_lines = [line.rstrip() for line in lines if not
                   (line.startswith('#') or line.rstrip() == "Bug:")]
    if not clean_lines:
      DieWithError('No CL description, aborting')
    self.set_description(clean_lines)

  def append_footer(self, line):
    """Adds a footer line to the description.

    Differentiates legacy "KEY=xxx" footers (used to be called tags) and
    Gerrit's footers in the form of "Footer-Key: footer any value" and ensures
    that Gerrit footers are always at the end.
    """
    parsed_footer_line = git_footers.parse_footer(line)
    if parsed_footer_line:
      # Line is a gerrit footer in the form: Footer-Key: any value.
      # Thus, must be appended observing Gerrit footer rules.
      self.set_description(
          git_footers.add_footer(self.description,
                                 key=parsed_footer_line[0],
                                 value=parsed_footer_line[1]))
      return

    if not self._description_lines:
      self._description_lines.append(line)
      return

    top_lines, gerrit_footers, _ = git_footers.split_footers(self.description)
    if gerrit_footers:
      # git_footers.split_footers ensures that there is an empty line before
      # actual (gerrit) footers, if any. We have to keep it that way.
      assert top_lines and top_lines[-1] == ''
      top_lines, separator = top_lines[:-1], top_lines[-1:]
    else:
      separator = []  # No need for separator if there are no gerrit_footers.

    prev_line = top_lines[-1] if top_lines else ''
    if (not presubmit_support.Change.TAG_LINE_RE.match(prev_line) or
        not presubmit_support.Change.TAG_LINE_RE.match(line)):
      top_lines.append('')
    top_lines.append(line)
    self._description_lines = top_lines + separator + gerrit_footers

  def get_reviewers(self, tbr_only=False):
    """Retrieves the list of reviewers."""
    matches = [re.match(self.R_LINE, line) for line in self._description_lines]
    reviewers = [match.group(2).strip()
                 for match in matches
                 if match and (not tbr_only or match.group(1).upper() == 'TBR')]
    return cleanup_list(reviewers)

  def get_cced(self):
    """Retrieves the list of reviewers."""
    matches = [re.match(self.CC_LINE, line) for line in self._description_lines]
    cced = [match.group(2).strip() for match in matches if match]
    return cleanup_list(cced)

  def get_hash_tags(self):
    """Extracts and sanitizes a list of Gerrit hashtags."""
    subject = (self._description_lines or ('',))[0]
    subject = re.sub(
        self.STRIP_HASH_TAG_PREFIX, '', subject, flags=re.IGNORECASE)

    tags = []
    start = 0
    bracket_exp = re.compile(self.BRACKET_HASH_TAG)
    while True:
      m = bracket_exp.match(subject, start)
      if not m:
        break
      tags.append(self.sanitize_hash_tag(m.group(1)))
      start = m.end()

    if not tags:
      # Try "Tag: " prefix.
      m = re.match(self.COLON_SEPARATED_HASH_TAG, subject)
      if m:
        tags.append(self.sanitize_hash_tag(m.group(1)))
    return tags

  @classmethod
  def sanitize_hash_tag(cls, tag):
    """Returns a sanitized Gerrit hash tag.

    A sanitized hashtag can be used as a git push refspec parameter value.
    """
    return re.sub(cls.BAD_HASH_TAG_CHUNK, '-', tag).strip('-').lower()

  def update_with_git_number_footers(self, parent_hash, parent_msg, dest_ref):
    """Updates this commit description given the parent.

    This is essentially what Gnumbd used to do.
    Consult https://goo.gl/WMmpDe for more details.
    """
    assert parent_msg  # No, orphan branch creation isn't supported.
    assert parent_hash
    assert dest_ref
    parent_footer_map = git_footers.parse_footers(parent_msg)
    # This will also happily parse svn-position, which GnumbD is no longer
    # supporting. While we'd generate correct footers, the verifier plugin
    # installed in Gerrit will block such commit (ie git push below will fail).
    parent_position = git_footers.get_position(parent_footer_map)

    # Cherry-picks may have last line obscuring their prior footers,
    # from git_footers perspective. This is also what Gnumbd did.
    cp_line = None
    if (self._description_lines and
        re.match(self.CHERRY_PICK_LINE, self._description_lines[-1])):
      cp_line = self._description_lines.pop()

    top_lines, footer_lines, _ = git_footers.split_footers(self.description)

    # Original-ify all Cr- footers, to avoid re-lands, cherry-picks, or just
    # user interference with actual footers we'd insert below.
    for i, line in enumerate(footer_lines):
      k, v = git_footers.parse_footer(line) or (None, None)
      if k and k.startswith('Cr-'):
        footer_lines[i] = '%s: %s' % ('Cr-Original-' + k[len('Cr-'):], v)

    # Add Position and Lineage footers based on the parent.
    lineage = list(reversed(parent_footer_map.get('Cr-Branched-From', [])))
    if parent_position[0] == dest_ref:
      # Same branch as parent.
      number = int(parent_position[1]) + 1
    else:
      number = 1  # New branch, and extra lineage.
      lineage.insert(0, '%s-%s@{#%d}' % (parent_hash, parent_position[0],
                                         int(parent_position[1])))

    footer_lines.append('Cr-Commit-Position: %s@{#%d}' % (dest_ref, number))
    footer_lines.extend('Cr-Branched-From: %s' % v for v in lineage)

    self._description_lines = top_lines
    if cp_line:
      self._description_lines.append(cp_line)
    if self._description_lines[-1] != '':
      self._description_lines.append('')  # Ensure footer separator.
    self._description_lines.extend(footer_lines)


def get_approving_reviewers(props, disapproval=False):
  """Retrieves the reviewers that approved a CL from the issue properties with
  messages.

  Note that the list may contain reviewers that are not committer, thus are not
  considered by the CQ.

  If disapproval is True, instead returns reviewers who 'not lgtm'd the CL.
  """
  approval_type = 'disapproval' if disapproval else 'approval'
  return sorted(
      set(
        message['sender']
        for message in props['messages']
        if message[approval_type] and message['sender'] in props['reviewers']
      )
  )


def FindCodereviewSettingsFile(filename='codereview.settings'):
  """Finds the given file starting in the cwd and going up.

  Only looks up to the top of the repository unless an
  'inherit-review-settings-ok' file exists in the root of the repository.
  """
  inherit_ok_file = 'inherit-review-settings-ok'
  cwd = os.getcwd()
  root = settings.GetRoot()
  if os.path.isfile(os.path.join(root, inherit_ok_file)):
    root = '/'
  while True:
    if filename in os.listdir(cwd):
      if os.path.isfile(os.path.join(cwd, filename)):
        return open(os.path.join(cwd, filename))
    if cwd == root:
      break
    cwd = os.path.dirname(cwd)


def LoadCodereviewSettingsFromFile(fileobj):
  """Parse a codereview.settings file and updates hooks."""
  keyvals = gclient_utils.ParseCodereviewSettingsContent(fileobj.read())

  def SetProperty(name, setting, unset_error_ok=False):
    fullname = 'rietveld.' + name
    if setting in keyvals:
      RunGit(['config', fullname, keyvals[setting]])
    else:
      RunGit(['config', '--unset-all', fullname], error_ok=unset_error_ok)

  if not keyvals.get('GERRIT_HOST', False):
    SetProperty('server', 'CODE_REVIEW_SERVER')
  # Only server setting is required. Other settings can be absent.
  # In that case, we ignore errors raised during option deletion attempt.
  SetProperty('cc', 'CC_LIST', unset_error_ok=True)
  SetProperty('private', 'PRIVATE', unset_error_ok=True)
  SetProperty('tree-status-url', 'STATUS', unset_error_ok=True)
  SetProperty('viewvc-url', 'VIEW_VC', unset_error_ok=True)
  SetProperty('bug-prefix', 'BUG_PREFIX', unset_error_ok=True)
  SetProperty('cpplint-regex', 'LINT_REGEX', unset_error_ok=True)
  SetProperty('cpplint-ignore-regex', 'LINT_IGNORE_REGEX', unset_error_ok=True)
  SetProperty('project', 'PROJECT', unset_error_ok=True)
  SetProperty('run-post-upload-hook', 'RUN_POST_UPLOAD_HOOK',
              unset_error_ok=True)

  if 'GERRIT_HOST' in keyvals:
    RunGit(['config', 'gerrit.host', keyvals['GERRIT_HOST']])

  if 'GERRIT_SQUASH_UPLOADS' in keyvals:
    RunGit(['config', 'gerrit.squash-uploads',
            keyvals['GERRIT_SQUASH_UPLOADS']])

  if 'GERRIT_SKIP_ENSURE_AUTHENTICATED' in keyvals:
    RunGit(['config', 'gerrit.skip-ensure-authenticated',
            keyvals['GERRIT_SKIP_ENSURE_AUTHENTICATED']])

  if 'PUSH_URL_CONFIG' in keyvals and 'ORIGIN_URL_CONFIG' in keyvals:
    # should be of the form
    # PUSH_URL_CONFIG: url.ssh://gitrw.chromium.org.pushinsteadof
    # ORIGIN_URL_CONFIG: http://src.chromium.org/git
    RunGit(['config', keyvals['PUSH_URL_CONFIG'],
            keyvals['ORIGIN_URL_CONFIG']])


def urlretrieve(source, destination):
  """urllib is broken for SSL connections via a proxy therefore we
  can't use urllib.urlretrieve()."""
  with open(destination, 'w') as f:
    f.write(urllib2.urlopen(source).read())


def hasSheBang(fname):
  """Checks fname is a #! script."""
  with open(fname) as f:
    return f.read(2).startswith('#!')


# TODO(bpastene) Remove once a cleaner fix to crbug.com/600473 presents itself.
def DownloadHooks(*args, **kwargs):
  pass


def DownloadGerritHook(force):
  """Download and install Gerrit commit-msg hook.

  Args:
    force: True to update hooks. False to install hooks if not present.
  """
  if not settings.GetIsGerrit():
    return
  src = 'https://gerrit-review.googlesource.com/tools/hooks/commit-msg'
  dst = os.path.join(settings.GetRoot(), '.git', 'hooks', 'commit-msg')
  if not os.access(dst, os.X_OK):
    if os.path.exists(dst):
      if not force:
        return
    try:
      urlretrieve(src, dst)
      if not hasSheBang(dst):
        DieWithError('Not a script: %s\n'
                     'You need to download from\n%s\n'
                     'into .git/hooks/commit-msg and '
                     'chmod +x .git/hooks/commit-msg' % (dst, src))
      os.chmod(dst, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR)
    except Exception:
      if os.path.exists(dst):
        os.remove(dst)
      DieWithError('\nFailed to download hooks.\n'
                   'You need to download from\n%s\n'
                   'into .git/hooks/commit-msg and '
                   'chmod +x .git/hooks/commit-msg' % src)


def GetRietveldCodereviewSettingsInteractively():
  """Prompt the user for settings."""
  server = settings.GetDefaultServerUrl(error_ok=True)
  prompt = 'Rietveld server (host[:port])'
  prompt += ' [%s]' % (server or DEFAULT_SERVER)
  newserver = ask_for_data(prompt + ':')
  if not server and not newserver:
    newserver = DEFAULT_SERVER
  if newserver:
    newserver = gclient_utils.UpgradeToHttps(newserver)
    if newserver != server:
      RunGit(['config', 'rietveld.server', newserver])

  def SetProperty(initial, caption, name, is_url):
    prompt = caption
    if initial:
      prompt += ' ("x" to clear) [%s]' % initial
    new_val = ask_for_data(prompt + ':')
    if new_val == 'x':
      RunGit(['config', '--unset-all', 'rietveld.' + name], error_ok=True)
    elif new_val:
      if is_url:
        new_val = gclient_utils.UpgradeToHttps(new_val)
      if new_val != initial:
        RunGit(['config', 'rietveld.' + name, new_val])

  SetProperty(settings.GetDefaultCCList(), 'CC list', 'cc', False)
  SetProperty(settings.GetDefaultPrivateFlag(),
              'Private flag (rietveld only)', 'private', False)
  SetProperty(settings.GetTreeStatusUrl(error_ok=True), 'Tree status URL',
              'tree-status-url', False)
  SetProperty(settings.GetViewVCUrl(), 'ViewVC URL', 'viewvc-url', True)
  SetProperty(settings.GetBugPrefix(), 'Bug Prefix', 'bug-prefix', False)
  SetProperty(settings.GetRunPostUploadHook(), 'Run Post Upload Hook',
              'run-post-upload-hook', False)


class _GitCookiesChecker(object):
  """Provides facilities for validating and suggesting fixes to .gitcookies."""

  _GOOGLESOURCE = 'googlesource.com'

  def __init__(self):
    # Cached list of [host, identity, source], where source is either
    # .gitcookies or .netrc.
    self._all_hosts = None

  def ensure_configured_gitcookies(self):
    """Runs checks and suggests fixes to make git use .gitcookies from default
    path."""
    default = gerrit_util.CookiesAuthenticator.get_gitcookies_path()
    configured_path = RunGitSilent(
        ['config', '--global', 'http.cookiefile']).strip()
    configured_path = os.path.expanduser(configured_path)
    if configured_path:
      self._ensure_default_gitcookies_path(configured_path, default)
    else:
      self._configure_gitcookies_path(default)

  @staticmethod
  def _ensure_default_gitcookies_path(configured_path, default_path):
    assert configured_path
    if configured_path == default_path:
      print('git is already configured to use your .gitcookies from %s' %
            configured_path)
      return

    print('WARNING: You have configured custom path to .gitcookies: %s\n'
          'Gerrit and other depot_tools expect .gitcookies at %s\n' %
          (configured_path, default_path))

    if not os.path.exists(configured_path):
      print('However, your configured .gitcookies file is missing.')
      confirm_or_exit('Reconfigure git to use default .gitcookies?',
                      action='reconfigure')
      RunGit(['config', '--global', 'http.cookiefile', default_path])
      return

    if os.path.exists(default_path):
      print('WARNING: default .gitcookies file already exists %s' %
            default_path)
      DieWithError('Please delete %s manually and re-run git cl creds-check' %
                   default_path)

    confirm_or_exit('Move existing .gitcookies to default location?',
                    action='move')
    shutil.move(configured_path, default_path)
    RunGit(['config', '--global', 'http.cookiefile', default_path])
    print('Moved and reconfigured git to use .gitcookies from %s' %
          default_path)

  @staticmethod
  def _configure_gitcookies_path(default_path):
    netrc_path = gerrit_util.CookiesAuthenticator.get_netrc_path()
    if os.path.exists(netrc_path):
      print('You seem to be using outdated .netrc for git credentials: %s' %
            netrc_path)
    print('This tool will guide you through setting up recommended '
          '.gitcookies store for git credentials.\n'
          '\n'
          'IMPORTANT: If something goes wrong and you decide to go back, do:\n'
          '  git config --global --unset http.cookiefile\n'
          '  mv %s %s.backup\n\n' % (default_path, default_path))
    confirm_or_exit(action='setup .gitcookies')
    RunGit(['config', '--global', 'http.cookiefile', default_path])
    print('Configured git to use .gitcookies from %s' % default_path)

  def get_hosts_with_creds(self, include_netrc=False):
    if self._all_hosts is None:
      a = gerrit_util.CookiesAuthenticator()
      self._all_hosts = [
          (h, u, s)
          for h, u, s in itertools.chain(
              ((h, u, '.netrc') for h, (u, _, _) in a.netrc.hosts.iteritems()),
              ((h, u, '.gitcookies') for h, (u, _) in a.gitcookies.iteritems())
          )
          if h.endswith(self._GOOGLESOURCE)
      ]

    if include_netrc:
      return self._all_hosts
    return [(h, u, s) for h, u, s in self._all_hosts if s != '.netrc']

  def print_current_creds(self, include_netrc=False):
    hosts = sorted(self.get_hosts_with_creds(include_netrc=include_netrc))
    if not hosts:
      print('No Git/Gerrit credentials found')
      return
    lengths = [max(map(len, (row[i] for row in hosts))) for i in xrange(3)]
    header = [('Host', 'User', 'Which file'),
              ['=' * l for l in lengths]]
    for row in (header + hosts):
      print('\t'.join((('%%+%ds' % l) % s)
                       for l, s in zip(lengths, row)))

  @staticmethod
  def _parse_identity(identity):
    """Parses identity "git-<username>.domain" into <username> and domain."""
    # Special case: usernames that contain ".", which are generally not
    # distinguishable from sub-domains. But we do know typical domains:
    if identity.endswith('.chromium.org'):
      domain = 'chromium.org'
      username = identity[:-len('.chromium.org')]
    else:
      username, domain = identity.split('.', 1)
    if username.startswith('git-'):
      username = username[len('git-'):]
    return username, domain

  def _get_usernames_of_domain(self, domain):
    """Returns list of usernames referenced by .gitcookies in a given domain."""
    identities_by_domain = {}
    for _, identity, _ in self.get_hosts_with_creds():
      username, domain = self._parse_identity(identity)
      identities_by_domain.setdefault(domain, []).append(username)
    return identities_by_domain.get(domain)

  def _canonical_git_googlesource_host(self, host):
    """Normalizes Gerrit hosts (with '-review') to Git host."""
    assert host.endswith(self._GOOGLESOURCE)
    # Prefix doesn't include '.' at the end.
    prefix = host[:-(1 + len(self._GOOGLESOURCE))]
    if prefix.endswith('-review'):
      prefix = prefix[:-len('-review')]
    return prefix + '.' + self._GOOGLESOURCE

  def _canonical_gerrit_googlesource_host(self, host):
    git_host = self._canonical_git_googlesource_host(host)
    prefix = git_host.split('.', 1)[0]
    return prefix + '-review.' + self._GOOGLESOURCE

  def _get_counterpart_host(self, host):
    assert host.endswith(self._GOOGLESOURCE)
    git = self._canonical_git_googlesource_host(host)
    gerrit = self._canonical_gerrit_googlesource_host(git)
    return git if gerrit == host else gerrit

  def has_generic_host(self):
    """Returns whether generic .googlesource.com has been configured.

    Chrome Infra recommends to use explicit ${host}.googlesource.com instead.
    """
    for host, _, _ in self.get_hosts_with_creds(include_netrc=False):
      if host == '.' + self._GOOGLESOURCE:
        return True
    return False

  def _get_git_gerrit_identity_pairs(self):
    """Returns map from canonic host to pair of identities (Git, Gerrit).

    One of identities might be None, meaning not configured.
    """
    host_to_identity_pairs = {}
    for host, identity, _ in self.get_hosts_with_creds():
      canonical = self._canonical_git_googlesource_host(host)
      pair = host_to_identity_pairs.setdefault(canonical, [None, None])
      idx = 0 if canonical == host else 1
      pair[idx] = identity
    return host_to_identity_pairs

  def get_partially_configured_hosts(self):
    return set(
        (host if i1 else self._canonical_gerrit_googlesource_host(host))
        for host, (i1, i2) in self._get_git_gerrit_identity_pairs().iteritems()
        if None in (i1, i2) and host != '.' + self._GOOGLESOURCE)

  def get_conflicting_hosts(self):
    return set(
        host
        for host, (i1, i2) in self._get_git_gerrit_identity_pairs().iteritems()
        if None not in (i1, i2) and i1 != i2)

  def get_duplicated_hosts(self):
    counters = collections.Counter(h for h, _, _ in self.get_hosts_with_creds())
    return set(host for host, count in counters.iteritems() if count > 1)

  _EXPECTED_HOST_IDENTITY_DOMAINS = {
    'chromium.googlesource.com': 'chromium.org',
    'chrome-internal.googlesource.com': 'google.com',
  }

  def get_hosts_with_wrong_identities(self):
    """Finds hosts which **likely** reference wrong identities.

    Note: skips hosts which have conflicting identities for Git and Gerrit.
    """
    hosts = set()
    for host, expected in self._EXPECTED_HOST_IDENTITY_DOMAINS.iteritems():
      pair = self._get_git_gerrit_identity_pairs().get(host)
      if pair and pair[0] == pair[1]:
        _, domain = self._parse_identity(pair[0])
        if domain != expected:
          hosts.add(host)
    return hosts

  @staticmethod
  def _format_hosts(hosts, extra_column_func=None):
    hosts = sorted(hosts)
    assert hosts
    if extra_column_func is None:
      extras = [''] * len(hosts)
    else:
      extras = [extra_column_func(host) for host in hosts]
    tmpl = '%%-%ds    %%-%ds' % (max(map(len, hosts)), max(map(len, extras)))
    lines = []
    for he in zip(hosts, extras):
      lines.append(tmpl % he)
    return lines

  def _find_problems(self):
    if self.has_generic_host():
      yield ('.googlesource.com wildcard record detected',
             ['Chrome Infrastructure team recommends to list full host names '
              'explicitly.'],
             None)

    dups = self.get_duplicated_hosts()
    if dups:
      yield ('The following hosts were defined twice',
             self._format_hosts(dups),
             None)

    partial = self.get_partially_configured_hosts()
    if partial:
      yield ('Credentials should come in pairs for Git and Gerrit hosts. '
             'These hosts are missing',
             self._format_hosts(partial, lambda host: 'but %s defined' %
                self._get_counterpart_host(host)),
             partial)

    conflicting = self.get_conflicting_hosts()
    if conflicting:
      yield ('The following Git hosts have differing credentials from their '
             'Gerrit counterparts',
             self._format_hosts(conflicting, lambda host: '%s vs %s' %
                 tuple(self._get_git_gerrit_identity_pairs()[host])),
             conflicting)

    wrong = self.get_hosts_with_wrong_identities()
    if wrong:
      yield ('These hosts likely use wrong identity',
             self._format_hosts(wrong, lambda host: '%s but %s recommended' %
                (self._get_git_gerrit_identity_pairs()[host][0],
                 self._EXPECTED_HOST_IDENTITY_DOMAINS[host])),
             wrong)

  def find_and_report_problems(self):
    """Returns True if there was at least one problem, else False."""
    found = False
    bad_hosts = set()
    for title, sublines, hosts in self._find_problems():
      if not found:
        found = True
        print('\n\n.gitcookies problem report:\n')
      bad_hosts.update(hosts or [])
      print('  %s%s' % (title , (':' if sublines else '')))
      if sublines:
        print()
        print('    %s' % '\n    '.join(sublines))
      print()

    if bad_hosts:
      assert found
      print('  You can manually remove corresponding lines in your %s file and '
            'visit the following URLs with correct account to generate '
            'correct credential lines:\n' %
            gerrit_util.CookiesAuthenticator.get_gitcookies_path())
      print('    %s' % '\n    '.join(sorted(set(
          gerrit_util.CookiesAuthenticator().get_new_password_url(
              self._canonical_git_googlesource_host(host))
          for host in bad_hosts
      ))))
    return found


def CMDcreds_check(parser, args):
  """Checks credentials and suggests changes."""
  _, _ = parser.parse_args(args)

  if gerrit_util.GceAuthenticator.is_gce():
    DieWithError(
        'This command is not designed for GCE, are you on a bot?\n'
        'If you need to run this, export SKIP_GCE_AUTH_FOR_GIT=1 in your env.')

  checker = _GitCookiesChecker()
  checker.ensure_configured_gitcookies()

  print('Your .netrc and .gitcookies have credentials for these hosts:')
  checker.print_current_creds(include_netrc=True)

  if not checker.find_and_report_problems():
    print('\nNo problems detected in your .gitcookies file.')
    return 0
  return 1


@subcommand.usage('[repo root containing codereview.settings]')
def CMDconfig(parser, args):
  """Edits configuration for this tree."""

  print('WARNING: git cl config works for Rietveld only.')
  # TODO(tandrii): remove this once we switch to Gerrit.
  # See bugs http://crbug.com/637561 and http://crbug.com/600469.
  parser.add_option('--activate-update', action='store_true',
                    help='activate auto-updating [rietveld] section in '
                         '.git/config')
  parser.add_option('--deactivate-update', action='store_true',
                    help='deactivate auto-updating [rietveld] section in '
                         '.git/config')
  options, args = parser.parse_args(args)

  if options.deactivate_update:
    RunGit(['config', 'rietveld.autoupdate', 'false'])
    return

  if options.activate_update:
    RunGit(['config', '--unset', 'rietveld.autoupdate'])
    return

  if len(args) == 0:
    GetRietveldCodereviewSettingsInteractively()
    return 0

  url = args[0]
  if not url.endswith('codereview.settings'):
    url = os.path.join(url, 'codereview.settings')

  # Load code review settings and download hooks (if available).
  LoadCodereviewSettingsFromFile(urllib2.urlopen(url))
  return 0


def CMDbaseurl(parser, args):
  """Gets or sets base-url for this branch."""
  branchref = RunGit(['symbolic-ref', 'HEAD']).strip()
  branch = ShortBranchName(branchref)
  _, args = parser.parse_args(args)
  if not args:
    print('Current base-url:')
    return RunGit(['config', 'branch.%s.base-url' % branch],
                  error_ok=False).strip()
  else:
    print('Setting base-url to %s' % args[0])
    return RunGit(['config', 'branch.%s.base-url' % branch, args[0]],
                  error_ok=False).strip()


def color_for_status(status):
  """Maps a Changelist status to color, for CMDstatus and other tools."""
  return {
    'unsent': Fore.YELLOW,
    'waiting': Fore.BLUE,
    'reply': Fore.YELLOW,
    'not lgtm': Fore.RED,
    'lgtm': Fore.GREEN,
    'commit': Fore.MAGENTA,
    'closed': Fore.CYAN,
    'error': Fore.WHITE,
  }.get(status, Fore.WHITE)


def get_cl_statuses(changes, fine_grained, max_processes=None):
  """Returns a blocking iterable of (cl, status) for given branches.

  If fine_grained is true, this will fetch CL statuses from the server.
  Otherwise, simply indicate if there's a matching url for the given branches.

  If max_processes is specified, it is used as the maximum number of processes
  to spawn to fetch CL status from the server. Otherwise 1 process per branch is
  spawned.

  See GetStatus() for a list of possible statuses.
  """
  # Silence upload.py otherwise it becomes unwieldy.
  upload.verbosity = 0

  if not changes:
    raise StopIteration()

  if not fine_grained:
    # Fast path which doesn't involve querying codereview servers.
    # Do not use get_approving_reviewers(), since it requires an HTTP request.
    for cl in changes:
      yield (cl, 'waiting' if cl.GetIssueURL() else 'error')
    return

  # First, sort out authentication issues.
  logging.debug('ensuring credentials exist')
  for cl in changes:
    cl.EnsureAuthenticated(force=False, refresh=True)

  def fetch(cl):
    try:
      return (cl, cl.GetStatus())
    except:
      # See http://crbug.com/629863.
      logging.exception('failed to fetch status for cl %s:', cl.GetIssue())
      raise

  threads_count = len(changes)
  if max_processes:
    threads_count = max(1, min(threads_count, max_processes))
  logging.debug('querying %d CLs using %d threads', len(changes), threads_count)

  pool = ThreadPool(threads_count)
  fetched_cls = set()
  try:
    it = pool.imap_unordered(fetch, changes).__iter__()
    while True:
      try:
        cl, status = it.next(timeout=5)
      except multiprocessing.TimeoutError:
        break
      fetched_cls.add(cl)
      yield cl, status
  finally:
    pool.close()

  # Add any branches that failed to fetch.
  for cl in set(changes) - fetched_cls:
    yield (cl, 'error')


def upload_branch_deps(cl, args):
  """Uploads CLs of local branches that are dependents of the current branch.

  If the local branch dependency tree looks like:
  test1 -> test2.1 -> test3.1
                   -> test3.2
        -> test2.2 -> test3.3

  and you run "git cl upload --dependencies" from test1 then "git cl upload" is
  run on the dependent branches in this order:
  test2.1, test3.1, test3.2, test2.2, test3.3

  Note: This function does not rebase your local dependent branches. Use it when
        you make a change to the parent branch that will not conflict with its
        dependent branches, and you would like their dependencies updated in
        Rietveld.
  """
  if git_common.is_dirty_git_tree('upload-branch-deps'):
    return 1

  root_branch = cl.GetBranch()
  if root_branch is None:
    DieWithError('Can\'t find dependent branches from detached HEAD state. '
                 'Get on a branch!')
  if not cl.GetIssue() or (not cl.IsGerrit() and not cl.GetPatchset()):
    DieWithError('Current branch does not have an uploaded CL. We cannot set '
                 'patchset dependencies without an uploaded CL.')

  branches = RunGit(['for-each-ref',
                     '--format=%(refname:short) %(upstream:short)',
                     'refs/heads'])
  if not branches:
    print('No local branches found.')
    return 0

  # Create a dictionary of all local branches to the branches that are dependent
  # on it.
  tracked_to_dependents = collections.defaultdict(list)
  for b in branches.splitlines():
    tokens = b.split()
    if len(tokens) == 2:
      branch_name, tracked = tokens
      tracked_to_dependents[tracked].append(branch_name)

  print()
  print('The dependent local branches of %s are:' % root_branch)
  dependents = []
  def traverse_dependents_preorder(branch, padding=''):
    dependents_to_process = tracked_to_dependents.get(branch, [])
    padding += '  '
    for dependent in dependents_to_process:
      print('%s%s' % (padding, dependent))
      dependents.append(dependent)
      traverse_dependents_preorder(dependent, padding)
  traverse_dependents_preorder(root_branch)
  print()

  if not dependents:
    print('There are no dependent local branches for %s' % root_branch)
    return 0

  confirm_or_exit('This command will checkout all dependent branches and run '
                  '"git cl upload".', action='continue')

  # Add a default patchset title to all upload calls in Rietveld.
  if not cl.IsGerrit():
    args.extend(['-t', 'Updated patchset dependency'])

  # Record all dependents that failed to upload.
  failures = {}
  # Go through all dependents, checkout the branch and upload.
  try:
    for dependent_branch in dependents:
      print()
      print('--------------------------------------')
      print('Running "git cl upload" from %s:' % dependent_branch)
      RunGit(['checkout', '-q', dependent_branch])
      print()
      try:
        if CMDupload(OptionParser(), args) != 0:
          print('Upload failed for %s!' % dependent_branch)
          failures[dependent_branch] = 1
      except:  # pylint: disable=bare-except
        failures[dependent_branch] = 1
      print()
  finally:
    # Swap back to the original root branch.
    RunGit(['checkout', '-q', root_branch])

  print()
  print('Upload complete for dependent branches!')
  for dependent_branch in dependents:
    upload_status = 'failed' if failures.get(dependent_branch) else 'succeeded'
    print('  %s : %s' % (dependent_branch, upload_status))
  print()

  return 0


def CMDarchive(parser, args):
  """Archives and deletes branches associated with closed changelists."""
  parser.add_option(
      '-j', '--maxjobs', action='store', type=int,
      help='The maximum number of jobs to use when retrieving review status.')
  parser.add_option(
      '-f', '--force', action='store_true',
      help='Bypasses the confirmation prompt.')
  parser.add_option(
      '-d', '--dry-run', action='store_true',
      help='Skip the branch tagging and removal steps.')
  parser.add_option(
      '-t', '--notags', action='store_true',
      help='Do not tag archived branches. '
           'Note: local commit history may be lost.')

  auth.add_auth_options(parser)
  options, args = parser.parse_args(args)
  if args:
    parser.error('Unsupported args: %s' % ' '.join(args))
  auth_config = auth.extract_auth_config_from_options(options)

  branches = RunGit(['for-each-ref', '--format=%(refname)', 'refs/heads'])
  if not branches:
    return 0

  print('Finding all branches associated with closed issues...')
  changes = [Changelist(branchref=b, auth_config=auth_config)
              for b in branches.splitlines()]
  alignment = max(5, max(len(c.GetBranch()) for c in changes))
  statuses = get_cl_statuses(changes,
                             fine_grained=True,
                             max_processes=options.maxjobs)
  proposal = [(cl.GetBranch(),
               'git-cl-archived-%s-%s' % (cl.GetIssue(), cl.GetBranch()))
              for cl, status in statuses
              if status == 'closed']
  proposal.sort()

  if not proposal:
    print('No branches with closed codereview issues found.')
    return 0

  current_branch = GetCurrentBranch()

  print('\nBranches with closed issues that will be archived:\n')
  if options.notags:
    for next_item in proposal:
      print('  ' + next_item[0])
  else:
    print('%*s | %s' % (alignment, 'Branch name', 'Archival tag name'))
    for next_item in proposal:
      print('%*s   %s' % (alignment, next_item[0], next_item[1]))

  # Quit now on precondition failure or if instructed by the user, either
  # via an interactive prompt or by command line flags.
  if options.dry_run:
    print('\nNo changes were made (dry run).\n')
    return 0
  elif any(branch == current_branch for branch, _ in proposal):
    print('You are currently on a branch \'%s\' which is associated with a '
          'closed codereview issue, so archive cannot proceed. Please '
          'checkout another branch and run this command again.' %
          current_branch)
    return 1
  elif not options.force:
    answer = ask_for_data('\nProceed with deletion (Y/n)? ').lower()
    if answer not in ('y', ''):
      print('Aborted.')
      return 1

  for branch, tagname in proposal:
    if not options.notags:
      RunGit(['tag', tagname, branch])
    RunGit(['branch', '-D', branch])

  print('\nJob\'s done!')

  return 0


def CMDstatus(parser, args):
  """Show status of changelists.

  Colors are used to tell the state of the CL unless --fast is used:
    - Blue     waiting for review
    - Yellow   waiting for you to reply to review, or not yet sent
    - Green    LGTM'ed
    - Red      'not LGTM'ed
    - Magenta  in the commit queue
    - Cyan     was committed, branch can be deleted
    - White    error, or unknown status

  Also see 'git cl comments'.
  """
  parser.add_option('--field',
                    help='print only specific field (desc|id|patch|status|url)')
  parser.add_option('-f', '--fast', action='store_true',
                    help='Do not retrieve review status')
  parser.add_option(
      '-j', '--maxjobs', action='store', type=int,
      help='The maximum number of jobs to use when retrieving review status')

  auth.add_auth_options(parser)
  _add_codereview_issue_select_options(
    parser, 'Must be in conjunction with --field.')
  options, args = parser.parse_args(args)
  _process_codereview_issue_select_options(parser, options)
  if args:
    parser.error('Unsupported args: %s' % args)
  auth_config = auth.extract_auth_config_from_options(options)

  if options.issue is not None and not options.field:
    parser.error('--field must be specified with --issue')

  if options.field:
    cl = Changelist(auth_config=auth_config, issue=options.issue,
                    codereview=options.forced_codereview)
    if options.field.startswith('desc'):
      print(cl.GetDescription())
    elif options.field == 'id':
      issueid = cl.GetIssue()
      if issueid:
        print(issueid)
    elif options.field == 'patch':
      patchset = cl.GetMostRecentPatchset()
      if patchset:
        print(patchset)
    elif options.field == 'status':
      print(cl.GetStatus())
    elif options.field == 'url':
      url = cl.GetIssueURL()
      if url:
        print(url)
    return 0

  branches = RunGit(['for-each-ref', '--format=%(refname)', 'refs/heads'])
  if not branches:
    print('No local branch found.')
    return 0

  changes = [
      Changelist(branchref=b, auth_config=auth_config)
      for b in branches.splitlines()]
  print('Branches associated with reviews:')
  output = get_cl_statuses(changes,
                           fine_grained=not options.fast,
                           max_processes=options.maxjobs)

  branch_statuses = {}
  alignment = max(5, max(len(ShortBranchName(c.GetBranch())) for c in changes))
  for cl in sorted(changes, key=lambda c: c.GetBranch()):
    branch = cl.GetBranch()
    while branch not in branch_statuses:
      c, status = output.next()
      branch_statuses[c.GetBranch()] = status
    status = branch_statuses.pop(branch)
    url = cl.GetIssueURL()
    if url and (not status or status == 'error'):
      # The issue probably doesn't exist anymore.
      url += ' (broken)'

    color = color_for_status(status)
    reset = Fore.RESET
    if not setup_color.IS_TTY:
      color = ''
      reset = ''
    status_str = '(%s)' % status if status else ''
    print('  %*s : %s%s %s%s' % (
          alignment, ShortBranchName(branch), color, url,
          status_str, reset))


  branch = GetCurrentBranch()
  print()
  print('Current branch: %s' % branch)
  for cl in changes:
    if cl.GetBranch() == branch:
      break
  if not cl.GetIssue():
    print('No issue assigned.')
    return 0
  print('Issue number: %s (%s)' % (cl.GetIssue(), cl.GetIssueURL()))
  if not options.fast:
    print('Issue description:')
    print(cl.GetDescription(pretty=True))
  return 0


def colorize_CMDstatus_doc():
  """To be called once in main() to add colors to git cl status help."""
  colors = [i for i in dir(Fore) if i[0].isupper()]

  def colorize_line(line):
    for color in colors:
      if color in line.upper():
        # Extract whitespace first and the leading '-'.
        indent = len(line) - len(line.lstrip(' ')) + 1
        return line[:indent] + getattr(Fore, color) + line[indent:] + Fore.RESET
    return line

  lines = CMDstatus.__doc__.splitlines()
  CMDstatus.__doc__ = '\n'.join(colorize_line(l) for l in lines)


def write_json(path, contents):
  if path == '-':
    json.dump(contents, sys.stdout)
  else:
    with open(path, 'w') as f:
      json.dump(contents, f)


@subcommand.usage('[issue_number]')
def CMDissue(parser, args):
  """Sets or displays the current code review issue number.

  Pass issue number 0 to clear the current issue.
  """
  parser.add_option('-r', '--reverse', action='store_true',
                    help='Lookup the branch(es) for the specified issues. If '
                         'no issues are specified, all branches with mapped '
                         'issues will be listed.')
  parser.add_option('--json',
                    help='Path to JSON output file, or "-" for stdout.')
  _add_codereview_select_options(parser)
  options, args = parser.parse_args(args)
  _process_codereview_select_options(parser, options)

  if options.reverse:
    branches = RunGit(['for-each-ref', 'refs/heads',
                       '--format=%(refname)']).splitlines()
    # Reverse issue lookup.
    issue_branch_map = {}
    for branch in branches:
      cl = Changelist(branchref=branch)
      issue_branch_map.setdefault(cl.GetIssue(), []).append(branch)
    if not args:
      args = sorted(issue_branch_map.iterkeys())
    result = {}
    for issue in args:
      if not issue:
        continue
      result[int(issue)] = issue_branch_map.get(int(issue))
      print('Branch for issue number %s: %s' % (
          issue, ', '.join(issue_branch_map.get(int(issue)) or ('None',))))
    if options.json:
      write_json(options.json, result)
    return 0

  if len(args) > 0:
    issue = ParseIssueNumberArgument(args[0], options.forced_codereview)
    if not issue.valid:
      DieWithError('Pass a url or number to set the issue, 0 to unset it, '
                   'or no argument to list it.\n'
                   'Maybe you want to run git cl status?')
    cl = Changelist(codereview=issue.codereview)
    cl.SetIssue(issue.issue)
  else:
    cl = Changelist(codereview=options.forced_codereview)
  print('Issue number: %s (%s)' % (cl.GetIssue(), cl.GetIssueURL()))
  if options.json:
    write_json(options.json, {
      'issue': cl.GetIssue(),
      'issue_url': cl.GetIssueURL(),
    })
  return 0


def CMDcomments(parser, args):
  """Shows or posts review comments for any changelist."""
  parser.add_option('-a', '--add-comment', dest='comment',
                    help='comment to add to an issue')
  parser.add_option('-i', '--issue', dest='issue',
                    help='review issue id (defaults to current issue). '
                         'If given, requires --rietveld or --gerrit')
  parser.add_option('-m', '--machine-readable', dest='readable',
                    action='store_false', default=True,
                    help='output comments in a format compatible with '
                         'editor parsing')
  parser.add_option('-j', '--json-file',
                    help='File to write JSON summary to, or "-" for stdout')
  auth.add_auth_options(parser)
  _add_codereview_select_options(parser)
  options, args = parser.parse_args(args)
  _process_codereview_select_options(parser, options)
  auth_config = auth.extract_auth_config_from_options(options)

  issue = None
  if options.issue:
    try:
      issue = int(options.issue)
    except ValueError:
      DieWithError('A review issue id is expected to be a number')
    if not options.forced_codereview:
      parser.error('--gerrit or --rietveld is required if --issue is specified')

  cl = Changelist(issue=issue,
                  codereview=options.forced_codereview,
                  auth_config=auth_config)

  if options.comment:
    cl.AddComment(options.comment)
    return 0

  summary = sorted(cl.GetCommentsSummary(readable=options.readable),
                   key=lambda c: c.date)
  for comment in summary:
    if comment.disapproval:
      color = Fore.RED
    elif comment.approval:
      color = Fore.GREEN
    elif comment.sender == cl.GetIssueOwner():
      color = Fore.MAGENTA
    else:
      color = Fore.BLUE
    print('\n%s%s   %s%s\n%s' % (
      color,
      comment.date.strftime('%Y-%m-%d %H:%M:%S UTC'),
      comment.sender,
      Fore.RESET,
      '\n'.join('  ' + l for l in comment.message.strip().splitlines())))

  if options.json_file:
    def pre_serialize(c):
      dct = c.__dict__.copy()
      dct['date'] = dct['date'].strftime('%Y-%m-%d %H:%M:%S.%f')
      return dct
    with open(options.json_file, 'wb') as f:
      json.dump(map(pre_serialize, summary), f)
  return 0


@subcommand.usage('[codereview url or issue id]')
def CMDdescription(parser, args):
  """Brings up the editor for the current CL's description."""
  parser.add_option('-d', '--display', action='store_true',
                    help='Display the description instead of opening an editor')
  parser.add_option('-n', '--new-description',
                    help='New description to set for this issue (- for stdin, '
                         '+ to load from local commit HEAD)')
  parser.add_option('-f', '--force', action='store_true',
                    help='Delete any unpublished Gerrit edits for this issue '
                         'without prompting')

  _add_codereview_select_options(parser)
  auth.add_auth_options(parser)
  options, args = parser.parse_args(args)
  _process_codereview_select_options(parser, options)

  target_issue_arg = None
  if len(args) > 0:
    target_issue_arg = ParseIssueNumberArgument(args[0],
                                                options.forced_codereview)
    if not target_issue_arg.valid:
      parser.error('invalid codereview url or CL id')

  auth_config = auth.extract_auth_config_from_options(options)

  kwargs = {
      'auth_config': auth_config,
      'codereview': options.forced_codereview,
  }
  detected_codereview_from_url = False
  if target_issue_arg:
    kwargs['issue'] = target_issue_arg.issue
    kwargs['codereview_host'] = target_issue_arg.hostname
    if target_issue_arg.codereview and not options.forced_codereview:
      detected_codereview_from_url = True
      kwargs['codereview'] = target_issue_arg.codereview

  cl = Changelist(**kwargs)
  if not cl.GetIssue():
    assert not detected_codereview_from_url
    DieWithError('This branch has no associated changelist.')

  if detected_codereview_from_url:
    logging.info('canonical issue/change URL: %s (type: %s)\n',
                 cl.GetIssueURL(), target_issue_arg.codereview)

  description = ChangeDescription(cl.GetDescription())

  if options.display:
    print(description.description)
    return 0

  if options.new_description:
    text = options.new_description
    if text == '-':
      text = '\n'.join(l.rstrip() for l in sys.stdin)
    elif text == '+':
      base_branch = cl.GetCommonAncestorWithUpstream()
      change = cl.GetChange(base_branch, None, local_description=True)
      text = change.FullDescriptionText()

    description.set_description(text)
  else:
    description.prompt(git_footer=cl.IsGerrit())

  if cl.GetDescription().strip() != description.description:
    cl.UpdateDescription(description.description, force=options.force)
  return 0


def CreateDescriptionFromLog(args):
  """Pulls out the commit log to use as a base for the CL description."""
  log_args = []
  if len(args) == 1 and not args[0].endswith('.'):
    log_args = [args[0] + '..']
  elif len(args) == 1 and args[0].endswith('...'):
    log_args = [args[0][:-1]]
  elif len(args) == 2:
    log_args = [args[0] + '..' + args[1]]
  else:
    log_args = args[:]  # Hope for the best!
  return RunGit(['log', '--pretty=format:%s\n\n%b'] + log_args)


def CMDlint(parser, args):
  """Runs cpplint on the current changelist."""
  parser.add_option('--filter', action='append', metavar='-x,+y',
                    help='Comma-separated list of cpplint\'s category-filters')
  auth.add_auth_options(parser)
  options, args = parser.parse_args(args)
  auth_config = auth.extract_auth_config_from_options(options)

  # Access to a protected member _XX of a client class
  # pylint: disable=protected-access
  try:
    import cpplint
    import cpplint_chromium
  except ImportError:
    print('Your depot_tools is missing cpplint.py and/or cpplint_chromium.py.')
    return 1

  # Change the current working directory before calling lint so that it
  # shows the correct base.
  previous_cwd = os.getcwd()
  os.chdir(settings.GetRoot())
  try:
    cl = Changelist(auth_config=auth_config)
    change = cl.GetChange(cl.GetCommonAncestorWithUpstream(), None)
    files = [f.LocalPath() for f in change.AffectedFiles()]
    if not files:
      print('Cannot lint an empty CL')
      return 1

    # Process cpplints arguments if any.
    command = args + files
    if options.filter:
      command = ['--filter=' + ','.join(options.filter)] + command
    filenames = cpplint.ParseArguments(command)

    white_regex = re.compile(settings.GetLintRegex())
    black_regex = re.compile(settings.GetLintIgnoreRegex())
    extra_check_functions = [cpplint_chromium.CheckPointerDeclarationWhitespace]
    for filename in filenames:
      if white_regex.match(filename):
        if black_regex.match(filename):
          print('Ignoring file %s' % filename)
        else:
          cpplint.ProcessFile(filename, cpplint._cpplint_state.verbose_level,
                              extra_check_functions)
      else:
        print('Skipping file %s' % filename)
  finally:
    os.chdir(previous_cwd)
  print('Total errors found: %d\n' % cpplint._cpplint_state.error_count)
  if cpplint._cpplint_state.error_count != 0:
    return 1
  return 0


def CMDpresubmit(parser, args):
  """Runs presubmit tests on the current changelist."""
  parser.add_option('-u', '--upload', action='store_true',
                    help='Run upload hook instead of the push hook')
  parser.add_option('-f', '--force', action='store_true',
                    help='Run checks even if tree is dirty')
  parser.add_option('--all', action='store_true',
                    help='Run checks against all files, not just modified ones')
  parser.add_option('--parallel', action='store_true',
                    help='Run all tests specified by input_api.RunTests in all '
                         'PRESUBMIT files in parallel.')
  auth.add_auth_options(parser)
  options, args = parser.parse_args(args)
  auth_config = auth.extract_auth_config_from_options(options)

  if not options.force and git_common.is_dirty_git_tree('presubmit'):
    print('use --force to check even if tree is dirty.')
    return 1

  cl = Changelist(auth_config=auth_config)
  if args:
    base_branch = args[0]
  else:
    # Default to diffing against the common ancestor of the upstream branch.
    base_branch = cl.GetCommonAncestorWithUpstream()

  if options.all:
    base_change = cl.GetChange(base_branch, None)
    files = [('M', f) for f in base_change.AllFiles()]
    change = presubmit_support.GitChange(
        base_change.Name(),
        base_change.FullDescriptionText(),
        base_change.RepositoryRoot(),
        files,
        base_change.issue,
        base_change.patchset,
        base_change.author_email,
        base_change._upstream)
  else:
    change = cl.GetChange(base_branch, None)

  cl.RunHook(
      committing=not options.upload,
      may_prompt=False,
      verbose=options.verbose,
      change=change,
      parallel=options.parallel)
  return 0


def GenerateGerritChangeId(message):
  """Returns Ixxxxxx...xxx change id.

  Works the same way as
  https://gerrit-review.googlesource.com/tools/hooks/commit-msg
  but can be called on demand on all platforms.

  The basic idea is to generate git hash of a state of the tree, original commit
  message, author/committer info and timestamps.
  """
  lines = []
  tree_hash = RunGitSilent(['write-tree'])
  lines.append('tree %s' % tree_hash.strip())
  code, parent = RunGitWithCode(['rev-parse', 'HEAD~0'], suppress_stderr=False)
  if code == 0:
    lines.append('parent %s' % parent.strip())
  author = RunGitSilent(['var', 'GIT_AUTHOR_IDENT'])
  lines.append('author %s' % author.strip())
  committer = RunGitSilent(['var', 'GIT_COMMITTER_IDENT'])
  lines.append('committer %s' % committer.strip())
  lines.append('')
  # Note: Gerrit's commit-hook actually cleans message of some lines and
  # whitespace. This code is not doing this, but it clearly won't decrease
  # entropy.
  lines.append(message)
  change_hash = RunCommand(['git', 'hash-object', '-t', 'commit', '--stdin'],
                           stdin='\n'.join(lines))
  return 'I%s' % change_hash.strip()


def GetTargetRef(remote, remote_branch, target_branch):
  """Computes the remote branch ref to use for the CL.

  Args:
    remote (str): The git remote for the CL.
    remote_branch (str): The git remote branch for the CL.
    target_branch (str): The target branch specified by the user.
  """
  if not (remote and remote_branch):
    return None

  if target_branch:
    # Canonicalize branch references to the equivalent local full symbolic
    # refs, which are then translated into the remote full symbolic refs
    # below.
    if '/' not in target_branch:
      remote_branch = 'refs/remotes/%s/%s' % (remote, target_branch)
    else:
      prefix_replacements = (
        ('^((refs/)?remotes/)?branch-heads/', 'refs/remotes/branch-heads/'),
        ('^((refs/)?remotes/)?%s/' % remote,  'refs/remotes/%s/' % remote),
        ('^(refs/)?heads/',                   'refs/remotes/%s/' % remote),
      )
      match = None
      for regex, replacement in prefix_replacements:
        match = re.search(regex, target_branch)
        if match:
          remote_branch = target_branch.replace(match.group(0), replacement)
          break
      if not match:
        # This is a branch path but not one we recognize; use as-is.
        remote_branch = target_branch
  elif remote_branch in REFS_THAT_ALIAS_TO_OTHER_REFS:
    # Handle the refs that need to land in different refs.
    remote_branch = REFS_THAT_ALIAS_TO_OTHER_REFS[remote_branch]

  # Create the true path to the remote branch.
  # Does the following translation:
  # * refs/remotes/origin/refs/diff/test -> refs/diff/test
  # * refs/remotes/origin/master -> refs/heads/master
  # * refs/remotes/branch-heads/test -> refs/branch-heads/test
  if remote_branch.startswith('refs/remotes/%s/refs/' % remote):
    remote_branch = remote_branch.replace('refs/remotes/%s/' % remote, '')
  elif remote_branch.startswith('refs/remotes/%s/' % remote):
    remote_branch = remote_branch.replace('refs/remotes/%s/' % remote,
                                          'refs/heads/')
  elif remote_branch.startswith('refs/remotes/branch-heads'):
    remote_branch = remote_branch.replace('refs/remotes/', 'refs/')

  return remote_branch


def cleanup_list(l):
  """Fixes a list so that comma separated items are put as individual items.

  So that "--reviewers joe@c,john@c --reviewers joa@c" results in
  options.reviewers == sorted(['joe@c', 'john@c', 'joa@c']).
  """
  items = sum((i.split(',') for i in l), [])
  stripped_items = (i.strip() for i in items)
  return sorted(filter(None, stripped_items))


@subcommand.usage('[flags]')
def CMDupload(parser, args):
  """Uploads the current changelist to codereview.

  Can skip dependency patchset uploads for a branch by running:
    git config branch.branch_name.skip-deps-uploads True
  To unset run:
    git config --unset branch.branch_name.skip-deps-uploads
  Can also set the above globally by using the --global flag.

  If the name of the checked out branch starts with "bug-" or "fix-" followed by
  a bug number, this bug number is automatically populated in the CL
  description.

  If subject contains text in square brackets or has "<text>: " prefix, such
  text(s) is treated as Gerrit hashtags. For example, CLs with subjects
    [git-cl] add support for hashtags
    Foo bar: implement foo
  will be hashtagged with "git-cl" and "foo-bar" respectively.
  """
  parser.add_option('--bypass-hooks', action='store_true', dest='bypass_hooks',
                    help='bypass upload presubmit hook')
  parser.add_option('--bypass-watchlists', action='store_true',
                    dest='bypass_watchlists',
                    help='bypass watchlists auto CC-ing reviewers')
  parser.add_option('-f', '--force', action='store_true', dest='force',
                    help="force yes to questions (don't prompt)")
  parser.add_option('--message', '-m', dest='message',
                    help='message for patchset')
  parser.add_option('-b', '--bug',
                    help='pre-populate the bug number(s) for this issue. '
                         'If several, separate with commas')
  parser.add_option('--message-file', dest='message_file',
                    help='file which contains message for patchset')
  parser.add_option('--title', '-t', dest='title',
                    help='title for patchset')
  parser.add_option('-r', '--reviewers',
                    action='append', default=[],
                    help='reviewer email addresses')
  parser.add_option('--tbrs',
                    action='append', default=[],
                    help='TBR email addresses')
  parser.add_option('--cc',
                    action='append', default=[],
                    help='cc email addresses')
  parser.add_option('--hashtag', dest='hashtags',
                    action='append', default=[],
                    help=('Gerrit hashtag for new CL; '
                          'can be applied multiple times'))
  parser.add_option('-s', '--send-mail', action='store_true',
                    help='send email to reviewer(s) and cc(s) immediately')
  parser.add_option('--emulate_svn_auto_props',
                    '--emulate-svn-auto-props',
                    action="store_true",
                    dest="emulate_svn_auto_props",
                    help="Emulate Subversion's auto properties feature.")
  parser.add_option('-c', '--use-commit-queue', action='store_true',
                    help='tell the commit queue to commit this patchset; '
                          'implies --send-mail')
  parser.add_option('--target_branch',
                    '--target-branch',
                    metavar='TARGET',
                    help='Apply CL to remote ref TARGET.  ' +
                         'Default: remote branch head, or master')
  parser.add_option('--squash', action='store_true',
                    help='Squash multiple commits into one')
  parser.add_option('--no-squash', action='store_true',
                    help='Don\'t squash multiple commits into one')
  parser.add_option('--topic', default=None,
                    help='Topic to specify when uploading')
  parser.add_option('--tbr-owners', dest='add_owners_to', action='store_const',
                    const='TBR', help='add a set of OWNERS to TBR')
  parser.add_option('--r-owners', dest='add_owners_to', action='store_const',
                    const='R', help='add a set of OWNERS to R')
  parser.add_option('-d', '--cq-dry-run', dest='cq_dry_run',
                    action='store_true',
                    help='Send the patchset to do a CQ dry run right after '
                         'upload.')
  parser.add_option('--dependencies', action='store_true',
                    help='Uploads CLs of all the local branches that depend on '
                         'the current branch')
  parser.add_option('-a', '--enable-auto-submit', action='store_true',
                    help='Sends your change to the CQ after an approval. Only '
                         'works on repos that have the Auto-Submit label '
                         'enabled')
  parser.add_option('--parallel', action='store_true',
                    help='Run all tests specified by input_api.RunTests in all '
                         'PRESUBMIT files in parallel.')

  # TODO: remove Rietveld flags
  parser.add_option('--private', action='store_true',
                    help='set the review private (rietveld only)')
  parser.add_option('--email', default=None,
                    help='email address to use to connect to Rietveld')

  orig_args = args
  auth.add_auth_options(parser)
  _add_codereview_select_options(parser)
  (options, args) = parser.parse_args(args)
  _process_codereview_select_options(parser, options)
  auth_config = auth.extract_auth_config_from_options(options)

  if git_common.is_dirty_git_tree('upload'):
    return 1

  options.reviewers = cleanup_list(options.reviewers)
  options.tbrs = cleanup_list(options.tbrs)
  options.cc = cleanup_list(options.cc)

  if options.message_file:
    if options.message:
      parser.error('only one of --message and --message-file allowed.')
    options.message = gclient_utils.FileRead(options.message_file)
    options.message_file = None

  if options.cq_dry_run and options.use_commit_queue:
    parser.error('only one of --use-commit-queue and --cq-dry-run allowed.')

  if options.use_commit_queue:
    options.send_mail = True

  # For sanity of test expectations, do this otherwise lazy-loading *now*.
  settings.GetIsGerrit()

  cl = Changelist(auth_config=auth_config, codereview=options.forced_codereview)
  return cl.CMDUpload(options, args, orig_args)


@subcommand.usage('--description=<description file>')
def CMDsplit(parser, args):
  """Splits a branch into smaller branches and uploads CLs.

  Creates a branch and uploads a CL for each group of files modified in the
  current branch that share a common OWNERS file. In the CL description and
  comment, the string '$directory', is replaced with the directory containing
  the shared OWNERS file.
  """
  parser.add_option("-d", "--description", dest="description_file",
                    help="A text file containing a CL description in which "
                         "$directory will be replaced by each CL's directory.")
  parser.add_option("-c", "--comment", dest="comment_file",
                    help="A text file containing a CL comment.")
  parser.add_option("-n", "--dry-run", dest="dry_run", action='store_true',
                    default=False,
                    help="List the files and reviewers for each CL that would "
                         "be created, but don't create branches or CLs.")
  options, _ = parser.parse_args(args)

  if not options.description_file:
    parser.error('No --description flag specified.')

  def WrappedCMDupload(args):
    return CMDupload(OptionParser(), args)

  return split_cl.SplitCl(options.description_file, options.comment_file,
                          Changelist, WrappedCMDupload, options.dry_run)


@subcommand.usage('DEPRECATED')
def CMDdcommit(parser, args):
  """DEPRECATED: Used to commit the current changelist via git-svn."""
  message = ('git-cl no longer supports committing to SVN repositories via '
             'git-svn. You probably want to use `git cl land` instead.')
  print(message)
  return 1


# Two special branches used by git cl land.
MERGE_BRANCH = 'git-cl-commit'
CHERRY_PICK_BRANCH = 'git-cl-cherry-pick'


@subcommand.usage('[upstream branch to apply against]')
def CMDland(parser, args):
  """Commits the current changelist via git.

  In case of Gerrit, uses Gerrit REST api to "submit" the issue, which pushes
  upstream and closes the issue automatically and atomically.

  Otherwise (in case of Rietveld):
    Squashes branch into a single commit.
    Updates commit message with metadata (e.g. pointer to review).
    Pushes the code upstream.
    Updates review and closes.
  """
  parser.add_option('--bypass-hooks', action='store_true', dest='bypass_hooks',
                    help='bypass upload presubmit hook')
  parser.add_option('-m', dest='message',
                    help="override review description")
  parser.add_option('-f', '--force', action='store_true', dest='force',
                    help="force yes to questions (don't prompt)")
  parser.add_option('-c', dest='contributor',
                    help="external contributor for patch (appended to " +
                         "description and used as author for git). Should be " +
                         "formatted as 'First Last <email@example.com>'")
  parser.add_option('--parallel', action='store_true',
                    help='Run all tests specified by input_api.RunTests in all '
                         'PRESUBMIT files in parallel.')
  auth.add_auth_options(parser)
  (options, args) = parser.parse_args(args)
  auth_config = auth.extract_auth_config_from_options(options)

  cl = Changelist(auth_config=auth_config)

  if not cl.IsGerrit():
    parser.error('rietveld is not supported')

  if options.message:
    # This could be implemented, but it requires sending a new patch to
    # Gerrit, as Gerrit unlike Rietveld versions messages with patchsets.
    # Besides, Gerrit has the ability to change the commit message on submit
    # automatically, thus there is no need to support this option (so far?).
    parser.error('-m MESSAGE option is not supported for Gerrit.')
  if options.contributor:
    parser.error(
        '-c CONTRIBUTOR option is not supported for Gerrit.\n'
        'Before uploading a commit to Gerrit, ensure it\'s author field is '
        'the contributor\'s "name <email>". If you can\'t upload such a '
        'commit for review, contact your repository admin and request'
        '"Forge-Author" permission.')
  if not cl.GetIssue():
    DieWithError('You must upload the change first to Gerrit.\n'
                 '  If you would rather have `git cl land` upload '
                 'automatically for you, see http://crbug.com/642759')
  return cl._codereview_impl.CMDLand(options.force, options.bypass_hooks,
                                     options.verbose, options.parallel)


def PushToGitWithAutoRebase(remote, branch, original_description,
                            git_numberer_enabled, max_attempts=3):
  """Pushes current HEAD commit on top of remote's branch.

  Attempts to fetch and autorebase on push failures.
  Adds git number footers on the fly.

  Returns integer code from last command.
  """
  cherry = RunGit(['rev-parse', 'HEAD']).strip()
  code = 0
  attempts_left = max_attempts
  while attempts_left:
    attempts_left -= 1
    print('Attempt %d of %d' % (max_attempts - attempts_left, max_attempts))

    # Fetch remote/branch into local cherry_pick_branch, overriding the latter.
    # If fetch fails, retry.
    print('Fetching %s/%s...' % (remote, branch))
    code, out = RunGitWithCode(
        ['retry', 'fetch', remote,
         '+%s:refs/heads/%s' % (branch, CHERRY_PICK_BRANCH)])
    if code:
      print('Fetch failed with exit code %d.' % code)
      print(out.strip())
      continue

    print('Cherry-picking commit on top of latest %s' % branch)
    RunGitWithCode(['checkout', 'refs/heads/%s' % CHERRY_PICK_BRANCH],
                   suppress_stderr=True)
    parent_hash = RunGit(['rev-parse', 'HEAD']).strip()
    code, out = RunGitWithCode(['cherry-pick', cherry])
    if code:
      print('Your patch doesn\'t apply cleanly to \'%s\' HEAD @ %s, '
            'the following files have merge conflicts:' %
            (branch, parent_hash))
      print(RunGit(['-c', 'core.quotePath=false', 'diff',
                    '--name-status', '--diff-filter=U']).strip())
      print('Please rebase your patch and try again.')
      RunGitWithCode(['cherry-pick', '--abort'])
      break

    commit_desc = ChangeDescription(original_description)
    if git_numberer_enabled:
      logging.debug('Adding git number footers')
      parent_msg = RunGit(['show', '-s', '--format=%B', parent_hash]).strip()
      commit_desc.update_with_git_number_footers(parent_hash, parent_msg,
                                                 branch)
      # Ensure timestamps are monotonically increasing.
      timestamp = max(1 + _get_committer_timestamp(parent_hash),
                      _get_committer_timestamp('HEAD'))
      _git_amend_head(commit_desc.description, timestamp)

    code, out = RunGitWithCode(
        ['push', '--porcelain', remote, 'HEAD:%s' % branch])
    print(out)
    if code == 0:
      break
    if IsFatalPushFailure(out):
      print('Fatal push error. Make sure your .netrc credentials and git '
            'user.email are correct and you have push access to the repo.\n'
            'Hint: run command below to diangose common Git/Gerrit credential '
            'problems:\n'
            '  git cl creds-check\n')
      break
  return code


def IsFatalPushFailure(push_stdout):
  """True if retrying push won't help."""
  return '(prohibited by Gerrit)' in push_stdout


@subcommand.usage('<patch url or issue id or issue url>')
def CMDpatch(parser, args):
  """Patches in a code review."""
  parser.add_option('-b', dest='newbranch',
                    help='create a new branch off trunk for the patch')
  parser.add_option('-f', '--force', action='store_true',
                    help='overwrite state on the current or chosen branch')
  parser.add_option('-d', '--directory', action='store', metavar='DIR',
                    help='change to the directory DIR immediately, '
                         'before doing anything else. Rietveld only.')
  parser.add_option('--reject', action='store_true',
                    help='failed patches spew .rej files rather than '
                        'attempting a 3-way merge. Rietveld only.')
  parser.add_option('-n', '--no-commit', action='store_true', dest='nocommit',
                    help='don\'t commit after patch applies. Rietveld only.')


  group = optparse.OptionGroup(
      parser,
      'Options for continuing work on the current issue uploaded from a '
      'different clone (e.g. different machine). Must be used independently '
      'from the other options. No issue number should be specified, and the '
      'branch must have an issue number associated with it')
  group.add_option('--reapply', action='store_true', dest='reapply',
                   help='Reset the branch and reapply the issue.\n'
                        'CAUTION: This will undo any local changes in this '
                        'branch')

  group.add_option('--pull', action='store_true', dest='pull',
                    help='Performs a pull before reapplying.')
  parser.add_option_group(group)

  auth.add_auth_options(parser)
  _add_codereview_select_options(parser)
  (options, args) = parser.parse_args(args)
  _process_codereview_select_options(parser, options)
  auth_config = auth.extract_auth_config_from_options(options)

  if options.reapply:
    if options.newbranch:
      parser.error('--reapply works on the current branch only')
    if len(args) > 0:
      parser.error('--reapply implies no additional arguments')

    cl = Changelist(auth_config=auth_config,
                    codereview=options.forced_codereview)
    if not cl.GetIssue():
      parser.error('current branch must have an associated issue')

    upstream = cl.GetUpstreamBranch()
    if upstream is None:
      parser.error('No upstream branch specified. Cannot reset branch')

    RunGit(['reset', '--hard', upstream])
    if options.pull:
      RunGit(['pull'])

    return cl.CMDPatchIssue(cl.GetIssue(), options.reject, options.nocommit,
                            options.directory)

  if len(args) != 1 or not args[0]:
    parser.error('Must specify issue number or url')

  target_issue_arg = ParseIssueNumberArgument(args[0],
                                              options.forced_codereview)
  if not target_issue_arg.valid:
    parser.error('invalid codereview url or CL id')

  cl_kwargs = {
      'auth_config': auth_config,
      'codereview_host': target_issue_arg.hostname,
      'codereview': options.forced_codereview,
  }
  detected_codereview_from_url = False
  if target_issue_arg.codereview and not options.forced_codereview:
    detected_codereview_from_url = True
    cl_kwargs['codereview'] = target_issue_arg.codereview
    cl_kwargs['issue'] = target_issue_arg.issue

  # We don't want uncommitted changes mixed up with the patch.
  if git_common.is_dirty_git_tree('patch'):
    return 1

  if options.newbranch:
    if options.force:
      RunGit(['branch', '-D', options.newbranch],
             stderr=subprocess2.PIPE, error_ok=True)
    RunGit(['new-branch', options.newbranch])

  cl = Changelist(**cl_kwargs)

  if cl.IsGerrit():
    if options.reject:
      parser.error('--reject is not supported with Gerrit codereview.')
    if options.directory:
      parser.error('--directory is not supported with Gerrit codereview.')

  if detected_codereview_from_url:
    print('canonical issue/change URL: %s (type: %s)\n' %
          (cl.GetIssueURL(), target_issue_arg.codereview))

  return cl.CMDPatchWithParsedIssue(target_issue_arg, options.reject,
                                    options.nocommit, options.directory,
                                    options.force)


def GetTreeStatus(url=None):
  """Fetches the tree status and returns either 'open', 'closed',
  'unknown' or 'unset'."""
  url = url or settings.GetTreeStatusUrl(error_ok=True)
  if url:
    status = urllib2.urlopen(url).read().lower()
    if status.find('closed') != -1 or status == '0':
      return 'closed'
    elif status.find('open') != -1 or status == '1':
      return 'open'
    return 'unknown'
  return 'unset'


def GetTreeStatusReason():
  """Fetches the tree status from a json url and returns the message
  with the reason for the tree to be opened or closed."""
  url = settings.GetTreeStatusUrl()
  json_url = urlparse.urljoin(url, '/current?format=json')
  connection = urllib2.urlopen(json_url)
  status = json.loads(connection.read())
  connection.close()
  return status['message']


def CMDtree(parser, args):
  """Shows the status of the tree."""
  _, args = parser.parse_args(args)
  status = GetTreeStatus()
  if 'unset' == status:
    print('You must configure your tree status URL by running "git cl config".')
    return 2

  print('The tree is %s' % status)
  print()
  print(GetTreeStatusReason())
  if status != 'open':
    return 1
  return 0


def CMDtry(parser, args):
  """Triggers try jobs using either BuildBucket or CQ dry run."""
  group = optparse.OptionGroup(parser, 'Try job options')
  group.add_option(
      '-b', '--bot', action='append',
      help=('IMPORTANT: specify ONE builder per --bot flag. Use it multiple '
            'times to specify multiple builders. ex: '
            '"-b win_rel -b win_layout". See '
            'the try server waterfall for the builders name and the tests '
            'available.'))
  group.add_option(
      '-B', '--bucket', default='',
      help=('Buildbucket bucket to send the try requests.'))
  group.add_option(
      '-m', '--master', default='',
      help=('DEPRECATED, use -B. The try master where to run the builds.'))
  group.add_option(
      '-r', '--revision',
      help='Revision to use for the try job; default: the revision will '
           'be determined by the try recipe that builder runs, which usually '
           'defaults to HEAD of origin/master')
  group.add_option(
      '-c', '--clobber', action='store_true', default=False,
      help='Force a clobber before building; that is don\'t do an '
           'incremental build')
  group.add_option(
      '--category', default='git_cl_try', help='Specify custom build category.')
  group.add_option(
      '--project',
      help='Override which project to use. Projects are defined '
           'in recipe to determine to which repository or directory to '
           'apply the patch')
  group.add_option(
      '-p', '--property', dest='properties', action='append', default=[],
      help='Specify generic properties in the form -p key1=value1 -p '
           'key2=value2 etc. The value will be treated as '
           'json if decodable, or as string otherwise. '
           'NOTE: using this may make your try job not usable for CQ, '
           'which will then schedule another try job with default properties')
  group.add_option(
      '--buildbucket-host', default='cr-buildbucket.appspot.com',
      help='Host of buildbucket. The default host is %default.')
  parser.add_option_group(group)
  auth.add_auth_options(parser)
  _add_codereview_issue_select_options(parser)
  options, args = parser.parse_args(args)
  _process_codereview_issue_select_options(parser, options)
  auth_config = auth.extract_auth_config_from_options(options)

  if options.master and options.master.startswith('luci.'):
    parser.error(
        '-m option does not support LUCI. Please pass -B %s' % options.master)
  # Make sure that all properties are prop=value pairs.
  bad_params = [x for x in options.properties if '=' not in x]
  if bad_params:
    parser.error('Got properties with missing "=": %s' % bad_params)

  if args:
    parser.error('Unknown arguments: %s' % args)

  cl = Changelist(auth_config=auth_config, issue=options.issue,
                  codereview=options.forced_codereview)
  if not cl.GetIssue():
    parser.error('Need to upload first')

  if cl.IsGerrit():
    # HACK: warm up Gerrit change detail cache to save on RPCs.
    cl._codereview_impl._GetChangeDetail(['DETAILED_ACCOUNTS', 'ALL_REVISIONS'])

  error_message = cl.CannotTriggerTryJobReason()
  if error_message:
    parser.error('Can\'t trigger try jobs: %s' % error_message)

  if options.bucket and options.master:
    parser.error('Only one of --bucket and --master may be used.')

  buckets = _get_bucket_map(cl, options, parser)

  # If no bots are listed and we couldn't get a list based on PRESUBMIT files,
  # then we default to triggering a CQ dry run (see http://crbug.com/625697).
  if not buckets:
    if options.verbose:
      print('git cl try with no bots now defaults to CQ dry run.')
    print('Scheduling CQ dry run on: %s' % cl.GetIssueURL())
    return cl.SetCQState(_CQState.DRY_RUN)

  for builders in buckets.itervalues():
    if any('triggered' in b for b in builders):
      print('ERROR You are trying to send a job to a triggered bot. This type '
            'of bot requires an initial job from a parent (usually a builder). '
            'Instead send your job to the parent.\n'
            'Bot list: %s' % builders, file=sys.stderr)
      return 1

  patchset = cl.GetMostRecentPatchset()
  # TODO(tandrii): Checking local patchset against remote patchset is only
  # supported for Rietveld. Extend it to Gerrit or remove it completely.
  if not cl.IsGerrit() and patchset != cl.GetPatchset():
    print('Warning: Codereview server has newer patchsets (%s) than most '
          'recent upload from local checkout (%s). Did a previous upload '
          'fail?\n'
          'By default, git cl try uses the latest patchset from '
          'codereview, continuing to use patchset %s.\n' %
          (patchset, cl.GetPatchset(), patchset))

  try:
    _trigger_try_jobs(auth_config, cl, buckets, options, patchset)
  except BuildbucketResponseException as ex:
    print('ERROR: %s' % ex)
    return 1
  return 0


def CMDtry_results(parser, args):
  """Prints info about try jobs associated with current CL."""
  group = optparse.OptionGroup(parser, 'Try job results options')
  group.add_option(
      '-p', '--patchset', type=int, help='patchset number if not current.')
  group.add_option(
      '--print-master', action='store_true', help='print master name as well.')
  group.add_option(
      '--color', action='store_true', default=setup_color.IS_TTY,
      help='force color output, useful when piping output.')
  group.add_option(
      '--buildbucket-host', default='cr-buildbucket.appspot.com',
      help='Host of buildbucket. The default host is %default.')
  group.add_option(
      '--json', help=('Path of JSON output file to write try job results to,'
                      'or "-" for stdout.'))
  parser.add_option_group(group)
  auth.add_auth_options(parser)
  _add_codereview_issue_select_options(parser)
  options, args = parser.parse_args(args)
  _process_codereview_issue_select_options(parser, options)
  if args:
    parser.error('Unrecognized args: %s' % ' '.join(args))

  auth_config = auth.extract_auth_config_from_options(options)
  cl = Changelist(
      issue=options.issue, codereview=options.forced_codereview,
      auth_config=auth_config)
  if not cl.GetIssue():
    parser.error('Need to upload first')

  patchset = options.patchset
  if not patchset:
    patchset = cl.GetMostRecentPatchset()
    if not patchset:
      parser.error('Codereview doesn\'t know about issue %s. '
                   'No access to issue or wrong issue number?\n'
                   'Either upload first, or pass --patchset explicitly' %
                   cl.GetIssue())

    # TODO(tandrii): Checking local patchset against remote patchset is only
    # supported for Rietveld. Extend it to Gerrit or remove it completely.
    if not cl.IsGerrit() and patchset != cl.GetPatchset():
      print('Warning: Codereview server has newer patchsets (%s) than most '
            'recent upload from local checkout (%s). Did a previous upload '
            'fail?\n'
            'By default, git cl try-results uses the latest patchset from '
            'codereview, continuing to use patchset %s.\n' %
            (patchset, cl.GetPatchset(), patchset))
  try:
    jobs = fetch_try_jobs(auth_config, cl, options.buildbucket_host, patchset)
  except BuildbucketResponseException as ex:
    print('Buildbucket error: %s' % ex)
    return 1
  if options.json:
    write_try_results_json(options.json, jobs)
  else:
    print_try_jobs(options, jobs)
  return 0


@subcommand.usage('[new upstream branch]')
def CMDupstream(parser, args):
  """Prints or sets the name of the upstream branch, if any."""
  _, args = parser.parse_args(args)
  if len(args) > 1:
    parser.error('Unrecognized args: %s' % ' '.join(args))

  cl = Changelist()
  if args:
    # One arg means set upstream branch.
    branch = cl.GetBranch()
    RunGit(['branch', '--set-upstream-to', args[0], branch])
    cl = Changelist()
    print('Upstream branch set to %s' % (cl.GetUpstreamBranch(),))

    # Clear configured merge-base, if there is one.
    git_common.remove_merge_base(branch)
  else:
    print(cl.GetUpstreamBranch())
  return 0


def CMDweb(parser, args):
  """Opens the current CL in the web browser."""
  _, args = parser.parse_args(args)
  if args:
    parser.error('Unrecognized args: %s' % ' '.join(args))

  issue_url = Changelist().GetIssueURL()
  if not issue_url:
    print('ERROR No issue to open', file=sys.stderr)
    return 1

  webbrowser.open(issue_url)
  return 0


def CMDset_commit(parser, args):
  """Sets the commit bit to trigger the Commit Queue."""
  parser.add_option('-d', '--dry-run', action='store_true',
                    help='trigger in dry run mode')
  parser.add_option('-c', '--clear', action='store_true',
                    help='stop CQ run, if any')
  auth.add_auth_options(parser)
  _add_codereview_issue_select_options(parser)
  options, args = parser.parse_args(args)
  _process_codereview_issue_select_options(parser, options)
  auth_config = auth.extract_auth_config_from_options(options)
  if args:
    parser.error('Unrecognized args: %s' % ' '.join(args))
  if options.dry_run and options.clear:
    parser.error('Make up your mind: both --dry-run and --clear not allowed')

  cl = Changelist(auth_config=auth_config, issue=options.issue,
                  codereview=options.forced_codereview)
  if options.clear:
    state = _CQState.NONE
  elif options.dry_run:
    state = _CQState.DRY_RUN
  else:
    state = _CQState.COMMIT
  if not cl.GetIssue():
    parser.error('Must upload the issue first')
  cl.SetCQState(state)
  return 0


def CMDset_close(parser, args):
  """Closes the issue."""
  _add_codereview_issue_select_options(parser)
  auth.add_auth_options(parser)
  options, args = parser.parse_args(args)
  _process_codereview_issue_select_options(parser, options)
  auth_config = auth.extract_auth_config_from_options(options)
  if args:
    parser.error('Unrecognized args: %s' % ' '.join(args))
  cl = Changelist(auth_config=auth_config, issue=options.issue,
                  codereview=options.forced_codereview)
  # Ensure there actually is an issue to close.
  if not cl.GetIssue():
    DieWithError('ERROR No issue to close')
  cl.CloseIssue()
  return 0


def CMDdiff(parser, args):
  """Shows differences between local tree and last upload."""
  parser.add_option(
      '--stat',
      action='store_true',
      dest='stat',
      help='Generate a diffstat')
  auth.add_auth_options(parser)
  options, args = parser.parse_args(args)
  auth_config = auth.extract_auth_config_from_options(options)
  if args:
    parser.error('Unrecognized args: %s' % ' '.join(args))

  cl = Changelist(auth_config=auth_config)
  issue = cl.GetIssue()
  branch = cl.GetBranch()
  if not issue:
    DieWithError('No issue found for current branch (%s)' % branch)

  base = cl._GitGetBranchConfigValue('last-upload-hash')
  if not base:
    base = cl._GitGetBranchConfigValue('gerritsquashhash')
  if not base:
    detail = cl._GetChangeDetail(['CURRENT_REVISION', 'CURRENT_COMMIT'])
    revision_info = detail['revisions'][detail['current_revision']]
    fetch_info = revision_info['fetch']['http']
    RunGit(['fetch', fetch_info['url'], fetch_info['ref']])
    base = 'FETCH_HEAD'

  cmd = ['git', 'diff']
  if options.stat:
    cmd.append('--stat')
  cmd.append(base)
  subprocess2.check_call(cmd)

  return 0


def CMDowners(parser, args):
  """Finds potential owners for reviewing."""
  parser.add_option(
      '--no-color',
      action='store_true',
      help='Use this option to disable color output')
  parser.add_option(
      '--batch',
      action='store_true',
      help='Do not run interactively, just suggest some')
  auth.add_auth_options(parser)
  options, args = parser.parse_args(args)
  auth_config = auth.extract_auth_config_from_options(options)

  author = RunGit(['config', 'user.email']).strip() or None

  cl = Changelist(auth_config=auth_config)

  if args:
    if len(args) > 1:
      parser.error('Unknown args')
    base_branch = args[0]
  else:
    # Default to diffing against the common ancestor of the upstream branch.
    base_branch = cl.GetCommonAncestorWithUpstream()

  change = cl.GetChange(base_branch, None)
  affected_files = [f.LocalPath() for f in change.AffectedFiles()]

  if options.batch:
    db = owners.Database(change.RepositoryRoot(), file, os.path)
    print('\n'.join(db.reviewers_for(affected_files, author)))
    return 0

  return owners_finder.OwnersFinder(
      affected_files,
      change.RepositoryRoot(),
      author,
      cl.GetReviewers(),
      fopen=file, os_path=os.path,
      disable_color=options.no_color,
      override_files=change.OriginalOwnersFiles()).run()


def BuildGitDiffCmd(diff_type, upstream_commit, args):
  """Generates a diff command."""
  # Generate diff for the current branch's changes.
  diff_cmd = ['-c', 'core.quotePath=false', 'diff',
              '--no-ext-diff', '--no-prefix', diff_type,
              upstream_commit, '--']

  if args:
    for arg in args:
      if os.path.isdir(arg) or os.path.isfile(arg):
        diff_cmd.append(arg)
      else:
        DieWithError('Argument "%s" is not a file or a directory' % arg)

  return diff_cmd


def MatchingFileType(file_name, extensions):
  """Returns true if the file name ends with one of the given extensions."""
  return bool([ext for ext in extensions if file_name.lower().endswith(ext)])


@subcommand.usage('[files or directories to diff]')
def CMDformat(parser, args):
  """Runs auto-formatting tools (clang-format etc.) on the diff."""
  CLANG_EXTS = ['.cc', '.cpp', '.h', '.m', '.mm', '.proto', '.java']
  GN_EXTS = ['.gn', '.gni', '.typemap']
  parser.add_option('--full', action='store_true',
                    help='Reformat the full content of all touched files')
  parser.add_option('--dry-run', action='store_true',
                    help='Don\'t modify any file on disk.')
  parser.add_option('--python', action='store_true',
                    help='Format python code with yapf (experimental).')
  parser.add_option('--js', action='store_true',
                    help='Format javascript code with clang-format.')
  parser.add_option('--diff', action='store_true',
                    help='Print diff to stdout rather than modifying files.')
  parser.add_option('--presubmit', action='store_true',
                    help='Used when running the script from a presubmit.')
  opts, args = parser.parse_args(args)

  # Normalize any remaining args against the current path, so paths relative to
  # the current directory are still resolved as expected.
  args = [os.path.join(os.getcwd(), arg) for arg in args]

  # git diff generates paths against the root of the repository.  Change
  # to that directory so clang-format can find files even within subdirs.
  rel_base_path = settings.GetRelativeRoot()
  if rel_base_path:
    os.chdir(rel_base_path)

  # Grab the merge-base commit, i.e. the upstream commit of the current
  # branch when it was created or the last time it was rebased. This is
  # to cover the case where the user may have called "git fetch origin",
  # moving the origin branch to a newer commit, but hasn't rebased yet.
  upstream_commit = None
  cl = Changelist()
  upstream_branch = cl.GetUpstreamBranch()
  if upstream_branch:
    upstream_commit = RunGit(['merge-base', 'HEAD', upstream_branch])
    upstream_commit = upstream_commit.strip()

  if not upstream_commit:
    DieWithError('Could not find base commit for this branch. '
                 'Are you in detached state?')

  changed_files_cmd = BuildGitDiffCmd('--name-only', upstream_commit, args)
  diff_output = RunGit(changed_files_cmd)
  diff_files = diff_output.splitlines()
  # Filter out files deleted by this CL
  diff_files = [x for x in diff_files if os.path.isfile(x)]

  if opts.js:
    CLANG_EXTS.append('.js')

  clang_diff_files = [x for x in diff_files if MatchingFileType(x, CLANG_EXTS)]
  python_diff_files = [x for x in diff_files if MatchingFileType(x, ['.py'])]
  dart_diff_files = [x for x in diff_files if MatchingFileType(x, ['.dart'])]
  gn_diff_files = [x for x in diff_files if MatchingFileType(x, GN_EXTS)]

  top_dir = os.path.normpath(
      RunGit(["rev-parse", "--show-toplevel"]).rstrip('\n'))

  # Set to 2 to signal to CheckPatchFormatted() that this patch isn't
  # formatted. This is used to block during the presubmit.
  return_value = 0

  if clang_diff_files:
    # Locate the clang-format binary in the checkout
    try:
      clang_format_tool = clang_format.FindClangFormatToolInChromiumTree()
    except clang_format.NotFoundError as e:
      DieWithError(e)

    if opts.full:
      cmd = [clang_format_tool]
      if not opts.dry_run and not opts.diff:
        cmd.append('-i')
      stdout = RunCommand(cmd + clang_diff_files, cwd=top_dir)
      if opts.diff:
        sys.stdout.write(stdout)
    else:
      env = os.environ.copy()
      env['PATH'] = str(os.path.dirname(clang_format_tool))
      try:
        script = clang_format.FindClangFormatScriptInChromiumTree(
            'clang-format-diff.py')
      except clang_format.NotFoundError as e:
        DieWithError(e)

      cmd = [sys.executable, script, '-p0']
      if not opts.dry_run and not opts.diff:
        cmd.append('-i')

      diff_cmd = BuildGitDiffCmd('-U0', upstream_commit, clang_diff_files)
      diff_output = RunGit(diff_cmd)

      stdout = RunCommand(cmd, stdin=diff_output, cwd=top_dir, env=env)
      if opts.diff:
        sys.stdout.write(stdout)
      if opts.dry_run and len(stdout) > 0:
        return_value = 2

  # Similar code to above, but using yapf on .py files rather than clang-format
  # on C/C++ files
  if opts.python:
    yapf_tool = gclient_utils.FindExecutable('yapf')
    if yapf_tool is None:
      DieWithError('yapf not found in PATH')

    if opts.full:
      if python_diff_files:
        cmd = [yapf_tool]
        if not opts.dry_run and not opts.diff:
          cmd.append('-i')
        stdout = RunCommand(cmd + python_diff_files, cwd=top_dir)
        if opts.diff:
          sys.stdout.write(stdout)
    else:
      # TODO(sbc): yapf --lines mode still has some issues.
      # https://github.com/google/yapf/issues/154
      DieWithError('--python currently only works with --full')

  # Dart's formatter does not have the nice property of only operating on
  # modified chunks, so hard code full.
  if dart_diff_files:
    try:
      command = [dart_format.FindDartFmtToolInChromiumTree()]
      if not opts.dry_run and not opts.diff:
        command.append('-w')
      command.extend(dart_diff_files)

      stdout = RunCommand(command, cwd=top_dir)
      if opts.dry_run and stdout:
        return_value = 2
    except dart_format.NotFoundError as e:
      print('Warning: Unable to check Dart code formatting. Dart SDK not '
            'found in this checkout. Files in other languages are still '
            'formatted.')

  # Format GN build files. Always run on full build files for canonical form.
  if gn_diff_files:
    cmd = ['gn', 'format']
    if opts.dry_run or opts.diff:
      cmd.append('--dry-run')
    for gn_diff_file in gn_diff_files:
      gn_ret = subprocess2.call(cmd + [gn_diff_file],
                                shell=sys.platform == 'win32',
                                cwd=top_dir)
      if opts.dry_run and gn_ret == 2:
        return_value = 2  # Not formatted.
      elif opts.diff and gn_ret == 2:
        # TODO this should compute and print the actual diff.
        print("This change has GN build file diff for " + gn_diff_file)
      elif gn_ret != 0:
        # For non-dry run cases (and non-2 return values for dry-run), a
        # nonzero error code indicates a failure, probably because the file
        # doesn't parse.
        DieWithError("gn format failed on " + gn_diff_file +
                     "\nTry running 'gn format' on this file manually.")

  # Skip the metrics formatting from the global presubmit hook. These files have
  # a separate presubmit hook that issues an error if the files need formatting,
  # whereas the top-level presubmit script merely issues a warning. Formatting
  # these files is somewhat slow, so it's important not to duplicate the work.
  if not opts.presubmit:
    for xml_dir in GetDirtyMetricsDirs(diff_files):
      tool_dir = os.path.join(top_dir, xml_dir)
      cmd = [os.path.join(tool_dir, 'pretty_print.py'), '--non-interactive']
      if opts.dry_run or opts.diff:
        cmd.append('--diff')
      stdout = RunCommand(cmd, cwd=top_dir)
      if opts.diff:
        sys.stdout.write(stdout)
      if opts.dry_run and stdout:
        return_value = 2  # Not formatted.

  return return_value

def GetDirtyMetricsDirs(diff_files):
  xml_diff_files = [x for x in diff_files if MatchingFileType(x, ['.xml'])]
  metrics_xml_dirs = [
    os.path.join('tools', 'metrics', 'actions'),
    os.path.join('tools', 'metrics', 'histograms'),
    os.path.join('tools', 'metrics', 'rappor'),
    os.path.join('tools', 'metrics', 'ukm')]
  for xml_dir in metrics_xml_dirs:
    if any(file.startswith(xml_dir) for file in xml_diff_files):
      yield xml_dir


@subcommand.usage('<codereview url or issue id>')
def CMDcheckout(parser, args):
  """Checks out a branch associated with a given Rietveld or Gerrit issue."""
  _, args = parser.parse_args(args)

  if len(args) != 1:
    parser.print_help()
    return 1

  issue_arg = ParseIssueNumberArgument(args[0])
  if not issue_arg.valid:
    parser.error('invalid codereview url or CL id')

  target_issue = str(issue_arg.issue)

  def find_issues(issueprefix):
    output = RunGit(['config', '--local', '--get-regexp',
                     r'branch\..*\.%s' % issueprefix],
                     error_ok=True)
    for key, issue in [x.split() for x in output.splitlines()]:
      if issue == target_issue:
        yield re.sub(r'branch\.(.*)\.%s' % issueprefix, r'\1', key)

  branches = []
  for cls in _CODEREVIEW_IMPLEMENTATIONS.values():
    branches.extend(find_issues(cls.IssueConfigKey()))
  if len(branches) == 0:
    print('No branch found for issue %s.' % target_issue)
    return 1
  if len(branches) == 1:
    RunGit(['checkout', branches[0]])
  else:
    print('Multiple branches match issue %s:' % target_issue)
    for i in range(len(branches)):
      print('%d: %s' % (i, branches[i]))
    which = raw_input('Choose by index: ')
    try:
      RunGit(['checkout', branches[int(which)]])
    except (IndexError, ValueError):
      print('Invalid selection, not checking out any branch.')
      return 1

  return 0


def CMDlol(parser, args):
  # This command is intentionally undocumented.
  print(zlib.decompress(base64.b64decode(
      'eNptkLEOwyAMRHe+wupCIqW57v0Vq84WqWtXyrcXnCBsmgMJ+/SSAxMZgRB6NzE'
      'E2ObgCKJooYdu4uAQVffUEoE1sRQLxAcqzd7uK2gmStrll1ucV3uZyaY5sXyDd9'
      'JAnN+lAXsOMJ90GANAi43mq5/VeeacylKVgi8o6F1SC63FxnagHfJUTfUYdCR/W'
      'Ofe+0dHL7PicpytKP750Fh1q2qnLVof4w8OZWNY')))
  return 0


class OptionParser(optparse.OptionParser):
  """Creates the option parse and add --verbose support."""
  def __init__(self, *args, **kwargs):
    optparse.OptionParser.__init__(
        self, *args, prog='git cl', version=__version__, **kwargs)
    self.add_option(
        '-v', '--verbose', action='count', default=0,
        help='Use 2 times for more debugging info')

  def parse_args(self, args=None, values=None):
    options, args = optparse.OptionParser.parse_args(self, args, values)
    levels = [logging.WARNING, logging.INFO, logging.DEBUG]
    logging.basicConfig(
        level=levels[min(options.verbose, len(levels) - 1)],
        format='[%(levelname).1s%(asctime)s %(process)d %(thread)d '
               '%(filename)s] %(message)s')
    return options, args


def main(argv):
  if sys.hexversion < 0x02060000:
    print('\nYour python version %s is unsupported, please upgrade.\n' %
          (sys.version.split(' ', 1)[0],), file=sys.stderr)
    return 2

  # Reload settings.
  global settings
  settings = Settings()

  colorize_CMDstatus_doc()
  dispatcher = subcommand.CommandDispatcher(__name__)
  try:
    return dispatcher.execute(OptionParser(), argv)
  except auth.AuthenticationError as e:
    DieWithError(str(e))
  except urllib2.HTTPError as e:
    if e.code != 500:
      raise
    DieWithError(
        ('AppEngine is misbehaving and returned HTTP %d, again. Keep faith '
          'and retry or visit go/isgaeup.\n%s') % (e.code, str(e)))
  return 0


if __name__ == '__main__':
  # These affect sys.stdout so do it outside of main() to simplify mocks in
  # unit testing.
  fix_encoding.fix_encoding()
  setup_color.init()
  try:
    sys.exit(main(sys.argv[1:]))
  except KeyboardInterrupt:
    sys.stderr.write('interrupted\n')
    sys.exit(1)
