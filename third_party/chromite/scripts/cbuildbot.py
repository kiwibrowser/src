# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Main builder code for Chromium OS.

Used by Chromium OS buildbot configuration for all Chromium OS builds including
full and pre-flight-queue builds.
"""

from __future__ import print_function

import distutils.version # pylint: disable=import-error,no-name-in-module
import glob
import json
import optparse  # pylint: disable=deprecated-module
import os
import pickle
import sys

from chromite.cbuildbot import builders
from chromite.cbuildbot import cbuildbot_run
from chromite.cbuildbot import repository
from chromite.cbuildbot import tee
from chromite.cbuildbot import topology
from chromite.cbuildbot.stages import completion_stages
from chromite.lib import builder_status_lib
from chromite.lib import cidb
from chromite.lib import cgroups
from chromite.lib import cleanup
from chromite.lib import commandline
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import failures_lib
from chromite.lib import git
from chromite.lib import gob_util
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.lib import retry_stats
from chromite.lib import sudo
from chromite.lib import timeout_util
from chromite.lib import tree_status
from chromite.lib import ts_mon_config


_DEFAULT_LOG_DIR = 'cbuildbot_logs'
_BUILDBOT_LOG_FILE = 'cbuildbot.log'
_DEFAULT_EXT_BUILDROOT = 'trybot'
_DEFAULT_INT_BUILDROOT = 'trybot-internal'
_BUILDBOT_REQUIRED_BINARIES = ('pbzip2',)
_API_VERSION_ATTR = 'api_version'


def _BackupPreviousLog(log_file, backup_limit=25):
  """Rename previous log.

  Args:
    log_file: The absolute path to the previous log.
    backup_limit: Maximum number of old logs to keep.
  """
  if os.path.exists(log_file):
    old_logs = sorted(glob.glob(log_file + '.*'),
                      key=distutils.version.LooseVersion)

    if len(old_logs) >= backup_limit:
      os.remove(old_logs[0])

    last = 0
    if old_logs:
      last = int(old_logs.pop().rpartition('.')[2])

    os.rename(log_file, log_file + '.' + str(last + 1))


def _IsDistributedBuilder(options, chrome_rev, build_config):
  """Determines whether the builder should be a DistributedBuilder.

  Args:
    options: options passed on the commandline.
    chrome_rev: Chrome revision to build.
    build_config: Builder configuration dictionary.

  Returns:
    True if the builder should be a distributed_builder
  """
  if build_config['pre_cq']:
    return True
  elif not options.buildbot:
    return False
  elif chrome_rev in (constants.CHROME_REV_TOT,
                      constants.CHROME_REV_LOCAL,
                      constants.CHROME_REV_SPEC):
    # We don't do distributed logic to TOT Chrome PFQ's, nor local
    # chrome roots (e.g. chrome try bots)
    # TODO(davidjames): Update any builders that rely on this logic to use
    # manifest_version=False instead.
    return False
  elif build_config['manifest_version']:
    return True

  return False


def _RunBuildStagesWrapper(options, site_config, build_config):
  """Helper function that wraps RunBuildStages()."""
  logging.info('cbuildbot was executed with args %s' %
               cros_build_lib.CmdToStr(sys.argv))

  chrome_rev = build_config['chrome_rev']
  if options.chrome_rev:
    chrome_rev = options.chrome_rev
  if chrome_rev == constants.CHROME_REV_TOT:
    options.chrome_version = gob_util.GetTipOfTrunkRevision(
        constants.CHROMIUM_GOB_URL)
    options.chrome_rev = constants.CHROME_REV_SPEC

  # If it's likely we'll need to build Chrome, fetch the source.
  if build_config['sync_chrome'] is None:
    options.managed_chrome = (
        chrome_rev != constants.CHROME_REV_LOCAL and
        (not build_config['usepkg_build_packages'] or chrome_rev or
         build_config['profile']))
  else:
    options.managed_chrome = build_config['sync_chrome']

  if options.managed_chrome:
    # Tell Chrome to fetch the source locally.
    internal = constants.USE_CHROME_INTERNAL in build_config['useflags']
    chrome_src = 'chrome-src-internal' if internal else 'chrome-src'
    target_name = 'target'
    if options.branch:
      # Tie the cache per branch
      target_name = 'target-%s' % options.branch
    options.chrome_root = os.path.join(options.cache_dir, 'distfiles',
                                       target_name, chrome_src)
    # Create directory if in need
    osutils.SafeMakedirsNonRoot(options.chrome_root)

  # We are done munging options values, so freeze options object now to avoid
  # further abuse of it.
  # TODO(mtennant): one by one identify each options value override and see if
  # it can be handled another way.  Try to push this freeze closer and closer
  # to the start of the script (e.g. in or after _PostParseCheck).
  options.Freeze()

  metadata_dump_dict = {
      # A detected default has been set before now if it wasn't explicit.
      'branch': options.branch,
  }
  if options.metadata_dump:
    with open(options.metadata_dump, 'r') as metadata_file:
      metadata_dump_dict = json.loads(metadata_file.read())

  with parallel.Manager() as manager:
    builder_run = cbuildbot_run.BuilderRun(
        options, site_config, build_config, manager)
    if metadata_dump_dict:
      builder_run.attrs.metadata.UpdateWithDict(metadata_dump_dict)

    if builder_run.config.builder_class_name is None:
      # TODO: This should get relocated to chromeos_config.
      if _IsDistributedBuilder(options, chrome_rev, build_config):
        builder_cls_name = 'simple_builders.DistributedBuilder'
      else:
        builder_cls_name = 'simple_builders.SimpleBuilder'
      builder_cls = builders.GetBuilderClass(builder_cls_name)
      builder = builder_cls(builder_run)
    else:
      builder = builders.Builder(builder_run)

    if not builder.Run():
      sys.exit(1)


def _CheckChromeVersionOption(_option, _opt_str, value, parser):
  """Upgrade other options based on chrome_version being passed."""
  value = value.strip()

  if parser.values.chrome_rev is None and value:
    parser.values.chrome_rev = constants.CHROME_REV_SPEC

  parser.values.chrome_version = value


def _CheckChromeRootOption(_option, _opt_str, value, parser):
  """Validate and convert chrome_root to full-path form."""
  if parser.values.chrome_rev is None:
    parser.values.chrome_rev = constants.CHROME_REV_LOCAL

  parser.values.chrome_root = value


def FindCacheDir(_parser, _options):
  return None


class CustomGroup(optparse.OptionGroup):
  """Custom option group which supports arguments passed-through to trybot."""

  def add_remote_option(self, *args, **kwargs):
    """For arguments that are passed-through to remote trybot."""
    return optparse.OptionGroup.add_option(self, *args,
                                           remote_pass_through=True,
                                           **kwargs)


class CustomOption(commandline.FilteringOption):
  """Subclass FilteringOption class to implement pass-through and api."""

  def __init__(self, *args, **kwargs):
    # The remote_pass_through argument specifies whether we should directly
    # pass the argument (with its value) onto the remote trybot.
    self.pass_through = kwargs.pop('remote_pass_through', False)
    self.api_version = int(kwargs.pop('api', '0'))
    commandline.FilteringOption.__init__(self, *args, **kwargs)


class CustomParser(commandline.FilteringParser):
  """Custom option parser which supports arguments passed-trhough to trybot"""

  DEFAULT_OPTION_CLASS = CustomOption

  def add_remote_option(self, *args, **kwargs):
    """For arguments that are passed-through to remote trybot."""
    return self.add_option(*args, remote_pass_through=True, **kwargs)


def CreateParser():
  """Expose _CreateParser publicly."""
  # Name _CreateParser is needed for commandline library.
  return _CreateParser()


def _CreateParser():
  """Generate and return the parser with all the options."""
  # Parse options
  usage = 'usage: %prog [options] buildbot_config [buildbot_config ...]'
  parser = CustomParser(usage=usage, caching=FindCacheDir)

  # Main options
  parser.add_remote_option('-b', '--branch',
                           help='The manifest branch to test.  The branch to '
                                'check the buildroot out to.')
  parser.add_option('-r', '--buildroot', type='path', dest='buildroot',
                    help='Root directory where source is checked out to, and '
                         'where the build occurs. For external build configs, '
                         "defaults to 'trybot' directory at top level of your "
                         'repo-managed checkout.')
  parser.add_option('--bootstrap-dir', type='path',
                    help='Bootstrapping cbuildbot may involve checking out '
                         'multiple copies of chromite. All these checkouts '
                         'will be contained in the directory specified here. '
                         'Default:%s' % osutils.GetGlobalTempDir())
  parser.add_remote_option('--android_rev', type='choice',
                           choices=constants.VALID_ANDROID_REVISIONS,
                           help=('Revision of Android to use, of type [%s]'
                                 % '|'.join(constants.VALID_ANDROID_REVISIONS)))
  parser.add_remote_option('--chrome_rev', type='choice',
                           choices=constants.VALID_CHROME_REVISIONS,
                           help=('Revision of Chrome to use, of type [%s]'
                                 % '|'.join(constants.VALID_CHROME_REVISIONS)))
  parser.add_remote_option('--profile',
                           help='Name of profile to sub-specify board variant.')
  # TODO(crbug.com/279618): Running GOMA is under development. Following
  # flags are added for development purpose due to repository dependency,
  # but not officially supported yet.
  parser.add_option('--goma_dir', type='path',
                    api=constants.REEXEC_API_GOMA,
                    help='Specify a directory containing goma. When this is '
                         'set, GOMA is used to build Chrome.')
  parser.add_option('--goma_client_json', type='path',
                    api=constants.REEXEC_API_GOMA,
                    help='Specify a service-account-goma-client.json path. '
                         'The file is needed on bots to run GOMA.')

  group = CustomGroup(
      parser,
      'Deprecated Options')

  parser.add_option('--local', action='store_true', default=False,
                    help='Deprecated. See cros tryjob.')
  parser.add_option('--remote', action='store_true', default=False,
                    help='Deprecated. See cros tryjob.')

  #
  # Patch selection options.
  #

  group = CustomGroup(
      parser,
      'Patch Options')

  group.add_remote_option('-g', '--gerrit-patches', action='split_extend',
                          type='string', default=[],
                          metavar="'Id1 *int_Id2...IdN'",
                          help='Space-separated list of short-form Gerrit '
                               "Change-Id's or change numbers to patch. "
                               "Please prepend '*' to internal Change-Id's")

  parser.add_argument_group(group)

  #
  # Remote trybot options.
  #

  group = CustomGroup(
      parser,
      'Options used to configure tryjob behavior.')
  group.add_remote_option('--hwtest', action='store_true', default=False,
                          help='Run the HWTest stage (tests on real hardware)')
  group.add_remote_option('--channel', action='split_extend', dest='channels',
                          default=[],
                          help='Specify a channel for a payloads trybot. Can '
                               'be specified multiple times. No valid for '
                               'non-payloads configs.')

  parser.add_argument_group(group)

  #
  # Branch creation options.
  #

  group = CustomGroup(
      parser,
      'Branch Creation Options (used with branch-util)')

  group.add_remote_option('--branch-name',
                          help='The branch to create or delete.')
  group.add_remote_option('--delete-branch', action='store_true', default=False,
                          help='Delete the branch specified in --branch-name.')
  group.add_remote_option('--rename-to',
                          help='Rename a branch to the specified name.')
  group.add_remote_option('--force-create', action='store_true', default=False,
                          help='Overwrites an existing branch.')
  group.add_remote_option('--skip-remote-push', action='store_true',
                          default=False,
                          help='Do not actually push to remote git repos.  '
                               'Used for end-to-end testing branching.')

  parser.add_argument_group(group)

  #
  # Advanced options.
  #

  group = CustomGroup(
      parser,
      'Advanced Options',
      'Caution: use these options at your own risk.')

  group.add_remote_option('--bootstrap-args', action='append', default=[],
                          help='Args passed directly to the bootstrap re-exec '
                               'to skip verification by the bootstrap code')
  group.add_remote_option('--buildbot', action='store_true', dest='buildbot',
                          default=False,
                          help='This is running on a buildbot. '
                               'This can be used to make a build operate '
                               'like an official builder, e.g. generate '
                               'new version numbers and archive official '
                               'artifacts and such. This should only be '
                               'used if you are confident in what you are '
                               'doing, as it will make automated commits.')
  parser.add_remote_option('--repo-cache', type='path', dest='_repo_cache',
                           help='Present for backwards compatibility, ignored.')
  group.add_remote_option('--no-buildbot-tags', action='store_false',
                          dest='enable_buildbot_tags', default=True,
                          help='Suppress buildbot specific tags from log '
                               'output. This is used to hide recursive '
                               'cbuilbot runs on the waterfall.')
  group.add_remote_option('--buildnumber', type='int', default=0,
                          help='build number')
  group.add_option('--chrome_root', action='callback', type='path',
                   callback=_CheckChromeRootOption,
                   help='Local checkout of Chrome to use.')
  group.add_remote_option('--chrome_version', action='callback', type='string',
                          dest='chrome_version',
                          callback=_CheckChromeVersionOption,
                          help='Used with SPEC logic to force a particular '
                               'git revision of chrome rather than the '
                               'latest.')
  group.add_remote_option('--clobber', action='store_true', default=False,
                          help='Clears an old checkout before syncing')
  group.add_remote_option('--latest-toolchain', action='store_true',
                          default=False,
                          help='Use the latest toolchain.')
  parser.add_option('--log_dir', dest='log_dir', type='path',
                    help='Directory where logs are stored.')
  group.add_remote_option('--maxarchives', type='int',
                          dest='max_archive_builds', default=3,
                          help='Change the local saved build count limit.')
  parser.add_remote_option('--manifest-repo-url',
                           help='Overrides the default manifest repo url.')
  group.add_remote_option('--compilecheck', action='store_true', default=False,
                          help='Only verify compilation and unit tests.')
  group.add_remote_option('--noarchive', action='store_false', dest='archive',
                          default=True, help="Don't run archive stage.")
  group.add_remote_option('--nobootstrap', action='store_false',
                          dest='bootstrap', default=True,
                          help="Don't checkout and run from a standalone "
                               'chromite repo.')
  group.add_remote_option('--nobuild', action='store_false', dest='build',
                          default=True,
                          help="Don't actually build (for cbuildbot dev)")
  group.add_remote_option('--noclean', action='store_false', dest='clean',
                          default=True, help="Don't clean the buildroot")
  group.add_remote_option('--nocgroups', action='store_false', dest='cgroups',
                          default=True,
                          help='Disable cbuildbots usage of cgroups.')
  group.add_remote_option('--nochromesdk', action='store_false',
                          dest='chrome_sdk', default=True,
                          help="Don't run the ChromeSDK stage which builds "
                               'Chrome outside of the chroot.')
  group.add_remote_option('--noprebuilts', action='store_false',
                          dest='prebuilts', default=True,
                          help="Don't upload prebuilts.")
  group.add_remote_option('--nopatch', action='store_false',
                          dest='postsync_patch', default=True,
                          help="Don't run PatchChanges stage.  This does not "
                               'disable patching in of chromite patches '
                               'during BootstrapStage.')
  group.add_remote_option('--nopaygen', action='store_false',
                          dest='paygen', default=True,
                          help="Don't generate payloads.")
  group.add_remote_option('--noreexec', action='store_false',
                          dest='postsync_reexec', default=True,
                          help="Don't reexec into the buildroot after syncing.")
  group.add_remote_option('--nosdk', action='store_true', default=False,
                          help='Re-create the SDK from scratch.')
  group.add_remote_option('--nosync', action='store_false', dest='sync',
                          default=True, help="Don't sync before building.")
  group.add_remote_option('--notests', action='store_false', dest='tests',
                          default=True,
                          help='Override values from buildconfig, run no '
                               'tests, and build no autotest and artifacts.')
  group.add_remote_option('--novmtests', action='store_false', dest='vmtests',
                          default=True,
                          help='Override values from buildconfig, run no '
                               'vmtests.')
  group.add_remote_option('--noimagetests', action='store_false',
                          dest='image_test', default=True,
                          help='Override values from buildconfig and run no '
                               'image tests.')
  group.add_remote_option('--nouprev', action='store_false', dest='uprev',
                          default=True,
                          help='Override values from buildconfig and never '
                               'uprev.')
  group.add_option('--reference-repo',
                   help='Reuse git data stored in an existing repo '
                        'checkout. This can drastically reduce the network '
                        'time spent setting up the trybot checkout.  By '
                        "default, if this option isn't given but cbuildbot "
                        'is invoked from a repo checkout, cbuildbot will '
                        'use the repo root.')
  group.add_option('--resume', action='store_true', default=False,
                   help='Skip stages already successfully completed.')
  group.add_remote_option('--timeout', type='int', default=0,
                          help='Specify the maximum amount of time this job '
                               'can run for, at which point the build will be '
                               'aborted.  If set to zero, then there is no '
                               'timeout.')
  group.add_remote_option('--version', dest='force_version',
                          help='Used with manifest logic.  Forces use of this '
                               'version rather than create or get latest. '
                               'Examples: 4815.0.0-rc1, 4815.1.2')
  group.add_remote_option('--git-cache-dir', type='path',
                          api=constants.REEXEC_API_GIT_CACHE_DIR,
                          help='Specify the cache directory to store the '
                               'project caches populated by the git-cache '
                               'tool. Bootstrap the projects based on the git '
                               'cache files instead of fetching them directly '
                               'from the GoB servers.')
  group.add_remote_option('--sanity-check-build', action='store_true',
                          default=False, dest='sanity_check_build',
                          api=constants.REEXEC_API_SANITY_CHECK_BUILD,
                          help='Run the build as a sanity check build.')
  group.add_remote_option('--debug-cidb', action='store_true', default=False,
                          help='Force Debug CIDB to be used.')

  parser.add_argument_group(group)

  #
  # Internal options.
  #

  group = CustomGroup(
      parser,
      'Internal Chromium OS Build Team Options',
      'Caution: these are for meant for the Chromium OS build team only')

  group.add_remote_option('--archive-base', type='gs_path',
                          help='Base GS URL (gs://<bucket_name>/<path>) to '
                               'upload archive artifacts to')
  group.add_remote_option(
      '--cq-gerrit-query', dest='cq_gerrit_override',
      help='If given, this gerrit query will be used to find what patches to '
           "test, rather than the normal 'CommitQueue>=1 AND Verified=1 AND "
           "CodeReview=2' query it defaults to.  Use with care- note "
           'additionally this setting only has an effect if the buildbot '
           "target is a cq target, and we're in buildbot mode.")
  group.add_option('--pass-through', action='append', type='string',
                   dest='pass_through_args', default=[])
  group.add_option('--reexec-api-version', action='store_true',
                   dest='output_api_version', default=False,
                   help='Used for handling forwards/backwards compatibility '
                        'with --resume and --bootstrap')
  group.add_option('--remote-trybot', action='store_true', default=False,
                   help='Indicates this is running on a remote trybot machine')
  group.add_option('--buildbucket-id',
                   api=constants.REEXEC_API_GOMA, # Approximate.
                   help='The unique ID in buildbucket of current build '
                        'generated by buildbucket.')
  group.add_remote_option('--remote-patches', action='split_extend', default=[],
                          help='Patches uploaded by the trybot client when '
                               'run using the -p option')
  # Note the default here needs to be hardcoded to 3; that is the last version
  # that lacked this functionality.
  group.add_option('--remote-version', type='int', default=3,
                   help='Deprecated and ignored.')
  group.add_option('--sourceroot', type='path', default=constants.SOURCE_ROOT)
  group.add_option('--ts-mon-task-num', type='int', default=0,
                   api=constants.REEXEC_API_TSMON_TASK_NUM,
                   help='The task number of this process. Defaults to 0. '
                        'This argument is useful for running multiple copies '
                        'of cbuildbot without their metrics colliding.')
  group.add_remote_option('--test-bootstrap', action='store_true',
                          default=False,
                          help='Causes cbuildbot to bootstrap itself twice, '
                               'in the sequence A->B->C: A(unpatched) patches '
                               'and bootstraps B; B patches and bootstraps C')
  group.add_remote_option('--validation_pool',
                          help='Path to a pickled validation pool. Intended '
                               'for use only with the commit queue.')
  group.add_remote_option('--metadata_dump',
                          help='Path to a json dumped metadata file. This '
                               'will be used as the initial metadata.')
  group.add_remote_option('--master-build-id', type='int',
                          api=constants.REEXEC_API_MASTER_BUILD_ID,
                          help='cidb build id of the master build to this '
                               'slave build.')
  group.add_remote_option('--mock-tree-status',
                          help='Override the tree status value that would be '
                               'returned from the the actual tree. Example '
                               'values: open, closed, throttled. When used '
                               'in conjunction with --debug, the tree status '
                               'will not be ignored as it usually is in a '
                               '--debug run.')
  # TODO(nxia): crbug.com/778838
  # cbuildbot doesn't use pickle files anymore, remove this.
  group.add_remote_option('--mock-slave-status',
                          metavar='MOCK_SLAVE_STATUS_PICKLE_FILE',
                          help='Override the result of the _FetchSlaveStatuses '
                               'method of MasterSlaveSyncCompletionStage, by '
                               'specifying a file with a pickle of the result '
                               'to be returned.')
  group.add_option('--previous-build-state', type='string', default='',
                   api=constants.REEXEC_API_PREVIOUS_BUILD_STATE,
                   help='A base64-encoded BuildSummary object describing the '
                        'previous build run on the same build machine.')

  parser.add_argument_group(group)

  #
  # Debug options
  #
  # Temporary hack; in place till --dry-run replaces --debug.
  # pylint: disable=W0212
  group = parser.debug_group
  debug = [x for x in group.option_list if x._long_opts == ['--debug']][0]
  debug.help += '  Currently functions as --dry-run in addition.'
  debug.pass_through = True
  group.add_option('--notee', action='store_false', dest='tee', default=True,
                   help='Disable logging and internal tee process.  Primarily '
                        'used for debugging cbuildbot itself.')
  return parser


def _FinishParsing(options):
  """Perform some parsing tasks that need to take place after optparse.

  This function needs to be easily testable!  Keep it free of
  environment-dependent code.  Put more detailed usage validation in
  _PostParseCheck().

  Args:
    options: The options object returned by optparse
  """
  # Populate options.pass_through_args.
  accepted, _ = commandline.FilteringParser.FilterArgs(
      options.parsed_args, lambda x: x.opt_inst.pass_through)
  options.pass_through_args.extend(accepted)

  if options.local or options.remote:
    cros_build_lib.Die('Deprecated usage. Please use cros tryjob instead.')

  if not options.buildroot:
    cros_build_lib.Die('A buildroot is required to build.')

  if options.chrome_root:
    if options.chrome_rev != constants.CHROME_REV_LOCAL:
      cros_build_lib.Die('Chrome rev must be %s if chrome_root is set.' %
                         constants.CHROME_REV_LOCAL)
  elif options.chrome_rev == constants.CHROME_REV_LOCAL:
    cros_build_lib.Die('Chrome root must be set if chrome_rev is %s.' %
                       constants.CHROME_REV_LOCAL)

  if options.chrome_version:
    if options.chrome_rev != constants.CHROME_REV_SPEC:
      cros_build_lib.Die('Chrome rev must be %s if chrome_version is set.' %
                         constants.CHROME_REV_SPEC)
  elif options.chrome_rev == constants.CHROME_REV_SPEC:
    cros_build_lib.Die(
        'Chrome rev must not be %s if chrome_version is not set.'
        % constants.CHROME_REV_SPEC)

  patches = bool(options.gerrit_patches)

  # When running in release mode, make sure we are running with checked-in code.
  # We want checked-in cbuildbot/scripts to prevent errors, and we want to build
  # a release image with checked-in code for CrOS packages.
  if options.buildbot and patches and not options.debug:
    cros_build_lib.Die(
        'Cannot provide patches when running with --buildbot!')

  if options.buildbot and options.remote_trybot:
    cros_build_lib.Die(
        '--buildbot and --remote-trybot cannot be used together.')

  # Record whether --debug was set explicitly vs. it was inferred.
  options.debug_forced = options.debug
  # We force --debug to be set for builds that are not 'official'.
  options.debug = options.debug or not options.buildbot

  if options.build_config_name in (constants.BRANCH_UTIL_CONFIG,
                                   'branch-util-tryjob'):
    if not options.branch_name:
      cros_build_lib.Die(
          'Must specify --branch-name with the %s config.',
          constants.BRANCH_UTIL_CONFIG)
    if (options.branch and options.branch != 'master' and
        options.branch != options.branch_name):
      cros_build_lib.Die(
          'If --branch is specified with the %s config, it must'
          ' have the same value as --branch-name.',
          constants.BRANCH_UTIL_CONFIG)

    exclusive_opts = {'--version': options.force_version,
                      '--delete-branch': options.delete_branch,
                      '--rename-to': options.rename_to}
    if 1 != sum(1 for x in exclusive_opts.values() if x):
      cros_build_lib.Die('When using the %s config, you must'
                         ' specifiy one and only one of the following'
                         ' options: %s.', constants.BRANCH_UTIL_CONFIG,
                         ', '.join(exclusive_opts.keys()))

    # When deleting or renaming a branch, the --branch and --nobootstrap
    # options are implied.
    if options.delete_branch or options.rename_to:
      if not options.branch:
        logging.info('Automatically enabling sync to branch %s for this %s '
                     'flow.', options.branch_name,
                     constants.BRANCH_UTIL_CONFIG)
        options.branch = options.branch_name
      if options.bootstrap:
        logging.info('Automatically disabling bootstrap step for this %s flow.',
                     constants.BRANCH_UTIL_CONFIG)
        options.bootstrap = False

  elif any([options.delete_branch, options.rename_to, options.branch_name]):
    cros_build_lib.Die(
        'Cannot specify --delete-branch, --rename-to or --branch-name when not '
        'running the %s config', constants.BRANCH_UTIL_CONFIG)


# pylint: disable=W0613
def _PostParseCheck(parser, options, site_config):
  """Perform some usage validation after we've parsed the arguments

  Args:
    parser: Option parser that was used to parse arguments.
    options: The options returned by optparse.
    site_config: config_lib.SiteConfig containing all config info.
  """

  if not options.branch:
    options.branch = git.GetChromiteTrackingBranch()

  # Because the default cache dir depends on other options, FindCacheDir
  # always returns None, and we setup the default here.
  if options.cache_dir is None:
    # Note, options.sourceroot is set regardless of the path
    # actually existing.
    options.cache_dir = os.path.join(options.buildroot, '.cache')
    options.cache_dir = os.path.abspath(options.cache_dir)
    parser.ConfigureCacheDir(options.cache_dir)

  osutils.SafeMakedirsNonRoot(options.cache_dir)

  # Ensure that all args are legitimate config targets.
  if options.build_config_name not in site_config:
    cros_build_lib.Die('Unkonwn build config: "%s"' % options.build_config_name)

  build_config = site_config[options.build_config_name]
  is_payloads_build = build_config.build_type == constants.PAYLOADS_TYPE

  if options.channels and not is_payloads_build:
    cros_build_lib.Die('--channel must only be used with a payload config,'
                       ' not target (%s).' % options.build_config_name)

  if not options.channels and is_payloads_build:
    cros_build_lib.Die('payload configs (%s) require --channel to do anything'
                       ' useful.' % options.build_config_name)

  # If the build config explicitly forces the debug flag, set the debug flag
  # as if it was set from the command line.
  if build_config.debug:
    options.debug = True

  if not (config_lib.isTryjobConfig(build_config) or options.buildbot):
    cros_build_lib.Die(
        'Refusing to run non-tryjob config as a tryjob.\n'
        'Please "repo sync && cros tryjob --list %s" for alternatives.\n'
        'See go/cros-explicit-tryjob-build-configs-psa.',
        build_config.name)

  # The --version option is not compatible with an external target unless the
  # --buildbot option is specified.  More correctly, only "paladin versions"
  # will work with external targets, and those are only used with --buildbot.
  # If --buildbot is specified, then user should know what they are doing and
  # only specify a version that will work.  See crbug.com/311648.
  if (options.force_version and
      not (options.buildbot or build_config.internal)):
    cros_build_lib.Die('Cannot specify --version without --buildbot for an'
                       ' external target (%s).' % options.build_config_name)


def ParseCommandLine(parser, argv):
  """Completely parse the commandline arguments"""
  (options, args) = parser.parse_args(argv)

  # Handle the request for the reexec command line API version number.
  if options.output_api_version:
    print(constants.REEXEC_API_VERSION)
    sys.exit(0)

  # Record the configs targeted. Strip out null arguments.
  build_config_names = [x for x in args if x]
  if len(build_config_names) != 1:
    cros_build_lib.Die('Expected exactly one build config. Got: %r',
                       build_config_names)
  options.build_config_name = build_config_names[-1]

  _FinishParsing(options)
  return options


_ENVIRONMENT_PROD = 'prod'
_ENVIRONMENT_DEBUG = 'debug'
_ENVIRONMENT_STANDALONE = 'standalone'


def _GetRunEnvironment(options, build_config):
  """Determine whether this is a prod/debug/standalone run."""
  if options.debug_cidb:
    return _ENVIRONMENT_DEBUG

  # One of these arguments should always be set if running on a real builder.
  # If we aren't on a real builder, we are standalone.
  if not options.buildbot and not options.remote_trybot:
    return _ENVIRONMENT_STANDALONE

  if build_config['debug_cidb']:
    return _ENVIRONMENT_DEBUG

  return _ENVIRONMENT_PROD


def _SetupConnections(options, build_config):
  """Set up CIDB connections using the appropriate Setup call.

  Args:
    options: Command line options structure.
    build_config: Config object for this build.
  """
  # Outline:
  # 1) Based on options and build_config, decide whether we are a production
  # run, debug run, or standalone run.
  # 2) Set up cidb instance accordingly.
  # 3) Update topology info from cidb, so that any other service set up can use
  # topology.
  # 4) Set up any other services.
  run_type = _GetRunEnvironment(options, build_config)

  if run_type == _ENVIRONMENT_PROD:
    cidb.CIDBConnectionFactory.SetupProdCidb()
    context = ts_mon_config.SetupTsMonGlobalState(
        'cbuildbot', indirect=True, task_num=options.ts_mon_task_num)
  elif run_type == _ENVIRONMENT_DEBUG:
    cidb.CIDBConnectionFactory.SetupDebugCidb()
    context = ts_mon_config.TrivialContextManager()
  else:
    cidb.CIDBConnectionFactory.SetupNoCidb()
    context = ts_mon_config.TrivialContextManager()

  db = cidb.CIDBConnectionFactory.GetCIDBConnectionForBuilder()
  topology.FetchTopologyFromCIDB(db)

  return context


class _MockMethodWithReturnValue(object):
  """A method mocker which just returns the specific value."""
  def __init__(self, return_value):
    self.return_value = return_value

  def __call__(self, *args, **kwargs):
    return self.return_value


class _ObjectMethodPatcher(object):
  """A simplified mock.object.patch.

  It is a context manager that patches an object's method with specified
  return value.
  """
  def __init__(self, target, attr, return_value=None):
    """Constructor.

    Args:
      target: object to patch.
      attr: method name of the object to patch.
      return_value: the return value when calling target.attr
    """
    self.target = target
    self.attr = attr
    self.return_value = return_value
    self.original_attr = None
    self.new_attr = _MockMethodWithReturnValue(self.return_value)

  def __enter__(self):
    self.original_attr = self.target.__dict__[self.attr]
    setattr(self.target, self.attr, self.new_attr)

  def __exit__(self, *args):
    if self.target and self.original_attr:
      setattr(self.target, self.attr, self.original_attr)


# TODO(build): This function is too damn long.
def main(argv):
  # We get false positives with the options object.
  # pylint: disable=attribute-defined-outside-init

  # Turn on strict sudo checks.
  cros_build_lib.STRICT_SUDO = True

  # Set umask to 022 so files created by buildbot are readable.
  os.umask(0o22)

  parser = _CreateParser()
  options = ParseCommandLine(parser, argv)

  # Fetch our site_config now, because we need it to do anything else.
  site_config = config_lib.GetConfig()

  _PostParseCheck(parser, options, site_config)

  cros_build_lib.AssertOutsideChroot()

  if options.enable_buildbot_tags:
    logging.EnableBuildbotMarkers()

  if (options.buildbot and
      not options.debug and
      not options.build_config_name == constants.BRANCH_UTIL_CONFIG and
      not cros_build_lib.HostIsCIBuilder()):
    # --buildbot can only be used on a real builder, unless it's debug, or
    # 'branch-util'.
    cros_build_lib.Die('This host is not a supported build machine.')

  # Only one config arg is allowed in this mode, which was confirmed earlier.
  build_config = site_config[options.build_config_name]

  # TODO: Re-enable this block when reference_repo support handles this
  #       properly. (see chromium:330775)
  # if options.reference_repo is None:
  #   repo_path = os.path.join(options.sourceroot, '.repo')
  #   # If we're being run from a repo checkout, reuse the repo's git pool to
  #   # cut down on sync time.
  #   if os.path.exists(repo_path):
  #     options.reference_repo = options.sourceroot

  if options.reference_repo:
    if not os.path.exists(options.reference_repo):
      parser.error('Reference path %s does not exist'
                   % (options.reference_repo,))
    elif not os.path.exists(os.path.join(options.reference_repo, '.repo')):
      parser.error('Reference path %s does not look to be the base of a '
                   'repo checkout; no .repo exists in the root.'
                   % (options.reference_repo,))

  if (options.buildbot or options.remote_trybot) and not options.resume:
    if not options.cgroups:
      parser.error('Options --buildbot/--remote-trybot and --nocgroups cannot '
                   'be used together.  Cgroup support is required for '
                   'buildbot/remote-trybot mode.')
    if not cgroups.Cgroup.IsSupported():
      parser.error('Option --buildbot/--remote-trybot was given, but this '
                   'system does not support cgroups.  Failing.')

    missing = osutils.FindMissingBinaries(_BUILDBOT_REQUIRED_BINARIES)
    if missing:
      parser.error('Option --buildbot/--remote-trybot requires the following '
                   "binaries which couldn't be found in $PATH: %s"
                   % (', '.join(missing)))

  if options.reference_repo:
    options.reference_repo = os.path.abspath(options.reference_repo)

  # Sanity check of buildroot- specifically that it's not pointing into the
  # midst of an existing repo since git-repo doesn't support nesting.
  if (not repository.IsARepoRoot(options.buildroot) and
      git.FindRepoDir(options.buildroot)):
    cros_build_lib.Die(
        'Configured buildroot %s is a subdir of an existing repo checkout.'
        % options.buildroot)

  if not options.log_dir:
    options.log_dir = os.path.join(options.buildroot, _DEFAULT_LOG_DIR)

  log_file = None
  if options.tee:
    log_file = os.path.join(options.log_dir, _BUILDBOT_LOG_FILE)
    osutils.SafeMakedirs(options.log_dir)
    _BackupPreviousLog(log_file)

  with cros_build_lib.ContextManagerStack() as stack:
    options.preserve_paths = set()
    if log_file is not None:
      # We don't want the critical section to try to clean up the tee process,
      # so we run Tee (forked off) outside of it. This prevents a deadlock
      # because the Tee process only exits when its pipe is closed, and the
      # critical section accidentally holds on to that file handle.
      stack.Add(tee.Tee, log_file)
      options.preserve_paths.add(_DEFAULT_LOG_DIR)

    critical_section = stack.Add(cleanup.EnforcedCleanupSection)
    stack.Add(sudo.SudoKeepAlive)

    if not options.resume:
      # If we're in resume mode, use our parents tempdir rather than
      # nesting another layer.
      stack.Add(osutils.TempDir, prefix='cbuildbot-tmp', set_global=True)
      logging.debug('Cbuildbot tempdir is %r.', os.environ.get('TMP'))

    if options.cgroups:
      stack.Add(cgroups.SimpleContainChildren, 'cbuildbot')

    # Mark everything between EnforcedCleanupSection and here as having to
    # be rolled back via the contextmanager cleanup handlers.  This
    # ensures that sudo bits cannot outlive cbuildbot, that anything
    # cgroups would kill gets killed, etc.
    stack.Add(critical_section.ForkWatchdog)

    if options.mock_tree_status is not None:
      stack.Add(_ObjectMethodPatcher, tree_status, '_GetStatus',
                return_value=options.mock_tree_status)

    if options.mock_slave_status is not None:
      with open(options.mock_slave_status, 'r') as f:
        mock_statuses = pickle.load(f)
        for key, value in mock_statuses.iteritems():
          mock_statuses[key] = builder_status_lib.BuilderStatus(**value)
      stack.Add(_ObjectMethodPatcher,
                completion_stages.MasterSlaveSyncCompletionStage,
                '_FetchSlaveStatuses',
                return_value=mock_statuses)

    stack.Add(_SetupConnections, options, build_config)
    retry_stats.SetupStats()

    timeout_display_message = None
    # For master-slave builds: Update slave's timeout using master's published
    # deadline.
    if options.buildbot and options.master_build_id is not None:
      slave_timeout = None
      if cidb.CIDBConnectionFactory.IsCIDBSetup():
        cidb_handle = cidb.CIDBConnectionFactory.GetCIDBConnectionForBuilder()
        if cidb_handle:
          slave_timeout = cidb_handle.GetTimeToDeadline(options.master_build_id)

      if slave_timeout is not None:
        # We artificially set a minimum slave_timeout because '0' is handled
        # specially, and because we don't want to timeout while trying to set
        # things up.
        slave_timeout = max(slave_timeout, 20)
        if options.timeout == 0 or slave_timeout < options.timeout:
          logging.info('Updating slave build timeout to %d seconds enforced '
                       'by the master', slave_timeout)
          options.timeout = slave_timeout
          timeout_display_message = (
              'This build has reached the timeout deadline set by the master. '
              'Either this stage or a previous one took too long (see stage '
              'timing historical summary in ReportStage) or the build failed '
              'to start on time.')
      else:
        logging.warning('Could not get master deadline for master-slave build. '
                        'Can not set slave timeout.')

    if options.timeout > 0:
      stack.Add(timeout_util.FatalTimeout, options.timeout,
                timeout_display_message)
    try:
      _RunBuildStagesWrapper(options, site_config, build_config)
    except failures_lib.ExitEarlyException as ex:
      # This build finished successfully. Do not re-raise ExitEarlyException.
      logging.info('One stage exited early: %s', ex)
