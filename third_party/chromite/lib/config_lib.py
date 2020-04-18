# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Configuration options for various cbuildbot builders."""

from __future__ import print_function

import copy
import itertools
import json
import os

from chromite.lib.const import waterfall
from chromite.lib import constants
from chromite.lib import osutils


GS_PATH_DEFAULT = 'default' # Means gs://chromeos-image-archive/ + bot_id

# Contains the valid build config suffixes.
CONFIG_TYPE_PRECQ = 'pre-cq'
CONFIG_TYPE_PALADIN = 'paladin'
CONFIG_TYPE_RELEASE = 'release'
CONFIG_TYPE_FULL = 'full'
CONFIG_TYPE_FIRMWARE = 'firmware'
CONFIG_TYPE_FACTORY = 'factory'
CONFIG_TYPE_RELEASE_AFDO = 'release-afdo'
CONFIG_TYPE_TOOLCHAIN = 'toolchain'

# DISPLAY labels are used to group related builds together in the GE UI.

DISPLAY_LABEL_PRECQ = 'pre_cq'
DISPLAY_LABEL_TRYJOB = 'tryjob'

# These are the build groups against which tryjobs can be directly run. All
# other groups MUST be production builds (ie: use their -tryjob instead)
# TODO: crbug.com/776955 Make the above statement true.
TRYJOB_DISPLAY_LABEL = {
    DISPLAY_LABEL_PRECQ,
    DISPLAY_LABEL_TRYJOB,
}

DISPLAY_LABEL_INCREMENATAL = 'incremental'
DISPLAY_LABEL_FULL = 'full'
DISPLAY_LABEL_INFORMATIONAL = 'informational'
DISPLAY_LABEL_CQ = 'cq'
DISPLAY_LABEL_RELEASE = 'release'
DISPLAY_LABEL_CHROME_PFQ = 'chrome_pfq'
DISPLAY_LABEL_MST_ANDROID_PFQ = 'mst_android_pfq'
DISPLAY_LABEL_MNC_ANDROID_PFQ = 'mnc_android_pfq'
DISPLAY_LABEL_NYC_ANDROID_PFQ = 'nyc_android_pfq'
DISPLAY_LABEL_PI_ANDROID_PFQ = 'pi_android_pfq'
DISPLAY_LABEL_FIRMWARE = 'firmware'
DISPLAY_LABEL_FACTORY = 'factory'
DISPLAY_LABEL_TOOLCHAIN = 'toolchain'
DISPLAY_LABEL_UTILITY = 'utility'
DISPLAY_LABEL_PRODUCTION_TRYJOB = 'production_tryjob'

# This list of constants should be kept in sync with GoldenEye code.
ALL_DISPLAY_LABEL = TRYJOB_DISPLAY_LABEL | {
    DISPLAY_LABEL_INCREMENATAL,
    DISPLAY_LABEL_FULL,
    DISPLAY_LABEL_INFORMATIONAL,
    DISPLAY_LABEL_CQ,
    DISPLAY_LABEL_RELEASE,
    DISPLAY_LABEL_CHROME_PFQ,
    DISPLAY_LABEL_MST_ANDROID_PFQ,
    DISPLAY_LABEL_MNC_ANDROID_PFQ,
    DISPLAY_LABEL_NYC_ANDROID_PFQ,
    DISPLAY_LABEL_PI_ANDROID_PFQ,
    DISPLAY_LABEL_FIRMWARE,
    DISPLAY_LABEL_FACTORY,
    DISPLAY_LABEL_TOOLCHAIN,
    DISPLAY_LABEL_UTILITY,
    DISPLAY_LABEL_PRODUCTION_TRYJOB,
}

# These values must be kept in sync with the ChromeOS LUCI builders.
#
# https://chrome-internal.git.corp.google.com/chromeos/
#    manifest-internal/+/infra/config/cr-buildbucket.cfg
LUCI_BUILDER_TRY = 'Try'
LUCI_BUILDER_PRECQ = 'PreCQ'
LUCI_BUILDER_PROD = 'Prod'

ALL_LUCI_BUILDER = {
    LUCI_BUILDER_TRY,
    LUCI_BUILDER_PRECQ,
    LUCI_BUILDER_PROD,
}

def isTryjobConfig(build_config):
  """Is a given build config a tryjob config, or a production config?

  Args:
    build_config: A fully populated instance of BuildConfig.

  Returns:
    Boolean. True if it's a tryjob config.
  """
  return build_config.display_label in TRYJOB_DISPLAY_LABEL


# In the Json, this special build config holds the default values for all
# other configs.
DEFAULT_BUILD_CONFIG = '_default'

# We cache the config we load from disk to avoid reparsing.
_CACHED_CONFIG = None

# Constants for config template file
CONFIG_TEMPLATE_BOARDS = 'boards'
CONFIG_TEMPLATE_NAME = 'name'
CONFIG_TEMPLATE_EXPERIMENTAL = 'experimental'
CONFIG_TEMPLATE_LEADER_BOARD = 'leader_board'
CONFIG_TEMPLATE_BOARD_GROUP = 'board_group'
CONFIG_TEMPLATE_BUILDER = 'builder'
CONFIG_TEMPLATE_RELEASE = 'RELEASE'
CONFIG_TEMPLATE_CONFIGS = 'configs'
CONFIG_TEMPLATE_ARCH = 'arch'
CONFIG_TEMPLATE_RELEASE_BRANCH = 'release_branch'
CONFIG_TEMPLATE_REFERENCE_BOARD_NAME = 'reference_board_name'
CONFIG_TEMPLATE_MODELS = 'models'
CONFIG_TEMPLATE_MODEL_NAME = 'name'
CONFIG_TEMPLATE_MODEL_BOARD_NAME = 'board_name'
CONFIG_TEMPLATE_MODEL_TEST_SUITES = 'test_suites'
CONFIG_TEMPLATE_MODEL_CQ_TEST_ENABLED = 'cq_test_enabled'

CONFIG_X86_INTERNAL = 'X86_INTERNAL'
CONFIG_X86_EXTERNAL = 'X86_EXTERNAL'
CONFIG_ARM_INTERNAL = 'ARM_INTERNAL'
CONFIG_ARM_EXTERNAL = 'ARM_EXTERNAL'

def IsPFQType(b_type):
  """Returns True if this build type is a PFQ."""
  return b_type in (constants.PFQ_TYPE, constants.PALADIN_TYPE,
                    constants.CHROME_PFQ_TYPE, constants.ANDROID_PFQ_TYPE)

def IsCQType(b_type):
  """Returns True if this build type is a Commit Queue."""
  return b_type == constants.PALADIN_TYPE

def IsCanaryType(b_type):
  """Returns True if this build type is a Canary."""
  return b_type == constants.CANARY_TYPE

def IsMasterChromePFQ(config):
  """Returns True if this build is master chrome PFQ type."""
  return config.build_type == constants.CHROME_PFQ_TYPE and config.master

def IsMasterAndroidPFQ(config):
  """Returns True if this build is master Android PFQ type."""
  return config.build_type == constants.ANDROID_PFQ_TYPE and config.master

def IsMasterCQ(config):
  """Returns True if this build is master CQ."""
  return config.build_type == constants.PALADIN_TYPE and config.master

def IsMasterBuild(config):
  """Returns True if this build is master."""
  return config.master

def UseBuildbucketScheduler(config):
  """Returns True if this build uses Buildbucket to schedule builds."""
  return (config.active_waterfall in (waterfall.WATERFALL_INTERNAL,
                                      waterfall.WATERFALL_EXTERNAL,
                                      waterfall.WATERFALL_SWARMING,
                                      waterfall.WATERFALL_RELEASE) and
          config.name in (constants.CQ_MASTER,
                          constants.CANARY_MASTER,
                          constants.PFQ_MASTER,
                          constants.MST_ANDROID_PFQ_MASTER,
                          constants.NYC_ANDROID_PFQ_MASTER,
                          constants.PI_ANDROID_PFQ_MASTER,
                          constants.TOOLCHAIN_MASTTER,
                          constants.PRE_CQ_LAUNCHER_NAME))

def RetryAlreadyStartedSlaves(config):
  """Returns True if wants to retry slaves which already start but fail.

  For a slave scheduled by Buildbucket, if the slave started cbuildbot
  and reported status to CIDB but failed to finish, its master may
  still want to retry the slave.
  """
  return config.name == constants.CQ_MASTER

def GetCriticalStageForRetry(config):
  """Get critical stage names for retry decisions.

  For a slave scheduled by Buildbucket, its master may want to retry it
  if it didn't pass the critical stage.

  Returns:
    A set of critical stage names (strings) for the config;
      default to an empty set.
  """
  if config.name == constants.CQ_MASTER:
    return {'CommitQueueSync', 'MasterSlaveLKGMSync'}
  else:
    return set()

def ScheduledByBuildbucket(config):
  """Returns True if this build is scheduled by Buildbucket."""
  return (config.build_type == constants.PALADIN_TYPE and
          config.name != constants.CQ_MASTER)


class AttrDict(dict):
  """Dictionary with 'attribute' access.

  This is identical to a dictionary, except that string keys can be addressed as
  read-only attributes.
  """
  def __getattr__(self, name):
    """Support attribute-like access to each dict entry."""
    if name in self:
      return self[name]

    # Super class (dict) has no __getattr__ method, so use __getattribute__.
    return super(AttrDict, self).__getattribute__(name)


class BuildConfig(AttrDict):
  """Dictionary of explicit configuration settings for a cbuildbot config

  Each dictionary entry is in turn a dictionary of config_param->value.

  See DefaultSettings for details on known configurations, and their
  documentation.
  """
  def deepcopy(self):
    """Create a deep copy of this object.

    This is a specialized version of copy.deepcopy() for BuildConfig objects. It
    speeds up deep copies by 10x because we know in advance what is stored
    inside a BuildConfig object and don't have to do as much introspection. This
    function is called a lot during setup of the config objects so optimizing it
    makes a big difference. (It saves seconds off the load time of this module!)
    """
    result = BuildConfig(self)

    # Here is where we handle all values that need deepcopy instead of shallow.
    for k, v in result.iteritems():
      if v is not None:
        if k == 'child_configs':
          result[k] = [x.deepcopy() for x in v]
        elif k in ('vm_tests', 'vm_tests_override',
                   'hw_tests', 'hw_tests_override',
                   'tast_vm_tests'):
          result[k] = [copy.copy(x) for x in v]
        # type(v) is faster than isinstance.
        elif type(v) is list:
          result[k] = v[:]

    return result

  def apply(self, *args, **kwargs):
    """Apply changes to this BuildConfig.

    Note: If an override is callable, it will be called and passed the prior
    value for the given key (or None) to compute the new value.

    Args:
      args: Dictionaries or templates to update this config with.
      kwargs: Settings to inject; see DefaultSettings for valid values.

    Returns:
      self after changes are applied.
    """
    inherits = list(args)
    inherits.append(kwargs)

    for update_config in inherits:
      for name, value in update_config.iteritems():
        if callable(value):
          # If we are applying to a fixed value, we resolve to a fixed value.
          # Otherwise, we save off a callable to apply later, perhaps with
          # nested callables (IE: we curry them). This allows us to use
          # callables in templates, and apply templates to each other and still
          # get the expected result when we use them later on.
          #
          # Delaying the resolution of callables is safe, because "Add()" always
          # applies against the default, which has fixed values for everything.

          if name in self:
            # apply it to the current value.
            if callable(self[name]):
              # If we have no fixed value to resolve with, stack the callables.
              def stack(new_callable, old_callable):
                """Helper method to isolate namespace for closure."""
                return lambda fixed: new_callable(old_callable(fixed))

              self[name] = stack(value, self[name])
            else:
              # If the current value was a fixed value, apply the callable.
              self[name] = value(self[name])
          else:
            # If we had no value to apply it to, save it for later.
            self[name] = value

        elif name == '_template':
          # We never apply _template. You have to set it through Add.
          pass

        else:
          # Simple values overwrite whatever we do or don't have.
          self[name] = value

    return self

  def derive(self, *args, **kwargs):
    """Create a new config derived from this one.

    Note: If an override is callable, it will be called and passed the prior
    value for the given key (or None) to compute the new value.

    Args:
      args: Mapping instances to mixin.
      kwargs: Settings to inject; see DefaultSettings for valid values.

    Returns:
      A new _config instance.
    """
    return self.deepcopy().apply(*args, **kwargs)

  def AddSlave(self, slave):
    """Assign slave config(s) to a build master.

    A helper for adding slave configs to a master config.
    """
    assert self.master
    if self['slave_configs'] is None:
      self['slave_configs'] = []
    self.slave_configs.append(slave.name)
    self.slave_configs.sort()

  def AddSlaves(self, slaves):
    """Assign slave config(s) to a build master.

    A helper for adding slave configs to a master config.
    """
    assert self.master
    if self['slave_configs'] is None:
      self['slave_configs'] = []
    self.slave_configs.extend(slave_config.name for slave_config in slaves)
    self.slave_configs.sort()


class VMTestConfig(object):
  """Config object for virtual machine tests suites.

  Members:
    test_type: Test type to be run.
    test_suite: Test suite to be run in VMTest.
    timeout: Number of seconds to wait before timing out waiting for
             results.
    retry: Whether we should retry tests that fail in a suite run.
    max_retries: Integer, maximum job retries allowed at suite level.
                 None for no max.
    warn_only: Boolean, failure on VM tests warns only.
  """
  DEFAULT_TEST_TIMEOUT = 90 * 60

  def __init__(self, test_type, test_suite=None,
               timeout=DEFAULT_TEST_TIMEOUT, retry=False,
               max_retries=constants.VM_TEST_MAX_RETRIES,
               warn_only=False):
    """Constructor -- see members above."""
    self.test_type = test_type
    self.test_suite = test_suite
    self.timeout = timeout
    self.retry = retry
    self.max_retries = max_retries
    self.warn_only = warn_only


  def __eq__(self, other):
    return self.__dict__ == other.__dict__


class GCETestConfig(object):
  """Config object for GCE tests suites.

  Members:
    test_type: Test type to be run.
    test_suite: Test suite to be run in GCETest.
    timeout: Number of seconds to wait before timing out waiting for
             results.
  """
  DEFAULT_TEST_TIMEOUT = 60 * 60

  def __init__(self, test_type, test_suite=None,
               timeout=DEFAULT_TEST_TIMEOUT):
    """Constructor -- see members above."""
    self.test_type = test_type
    self.test_suite = test_suite
    self.timeout = timeout

  def __eq__(self, other):
    return self.__dict__ == other.__dict__


class TastVMTestConfig(object):
  """Config object for a Tast virtual-machine-based test suite.

  Members:
    name: String containing short human-readable name describing test suite.
    test_exprs: List of string expressions describing which tests to run; this
                is passed directly to the 'tast run' command. See
                https://goo.gl/UPNEgT for info about test expressions.
    timeout: Number of seconds to wait before timing out waiting for
             results.
  """
  DEFAULT_TEST_TIMEOUT = 10 * 60

  def __init__(self, suite_name, test_exprs, timeout=DEFAULT_TEST_TIMEOUT):
    """Constructor -- see members above."""
    # This is an easy mistake to make and results in confusing errors later when
    # a list of one-character strings gets passed to the tast command.
    if not isinstance(test_exprs, list):
      raise TypeError('test_exprs must be list of strings')
    self.suite_name = suite_name
    self.test_exprs = test_exprs
    self.timeout = timeout

  def __eq__(self, other):
    return self.__dict__ == other.__dict__


class MoblabVMTestConfig(object):
  """Config object for moblab tests suites.

  Members:
    test_type: Test type to be run.
    timeout: Number of seconds to wait before timing out waiting for
             results.
  """
  DEFAULT_TEST_TIMEOUT = 60 * 60

  def __init__(self, test_type, timeout=DEFAULT_TEST_TIMEOUT):
    """Constructor -- see members above."""
    self.test_type = test_type
    self.timeout = timeout

  def __eq__(self, other):
    return self.__dict__ == other.__dict__


class ModelTestConfig(object):
  """Model specific config that controls which test suites are executed.

  Members:
    name: The name of the model that will be tested (matches model label)
    lab_board_name: The name of the board in the lab (matches board label)
    test_suites: List of hardware test suites that will be executed.
  """
  def __init__(self, name, lab_board_name, test_suites=None):
    """Constructor -- see members above."""
    self.name = name
    self.lab_board_name = lab_board_name
    self.test_suites = test_suites

  def __eq__(self, other):
    return self.__dict__ == other.__dict__


class HWTestConfig(object):
  """Config object for hardware tests suites.

  Members:
    suite: Name of the test suite to run.
    timeout: Number of seconds to wait before timing out waiting for
             results.
    pool: Pool to use for hw testing.
    blocking: Setting this to true requires that this suite must PASS for suites
              scheduled after it to run. This also means any suites that are
              scheduled before a blocking one are also blocking ones scheduled
              after. This should be used when you want some suites to block
              whether or not others should run e.g. only run longer-running
              suites if some core ones pass first.

              Note, if you want multiple suites to block other suites but run
              in parallel, you should only mark the last one scheduled as
              blocking (it effectively serves as a thread/process join).
    async: Fire-and-forget suite.
    warn_only: Failure on HW tests warns only (does not generate error).
    critical: Usually we consider structural failures here as OK.
    priority:  Priority at which tests in the suite will be scheduled in
               the hw lab.
    file_bugs: Should we file bugs if a test fails in a suite run.
    minimum_duts: minimum number of DUTs required for testing in the hw lab.
    retry: Whether we should retry tests that fail in a suite run.
    max_retries: Integer, maximum job retries allowed at suite level.
                 None for no max.
    suite_min_duts: Preferred minimum duts. Lab will prioritize on getting such
                    number of duts even if the suite is competing with
                    other suites that have higher priority.
    suite_args: Arguments passed to the suite.  This should be a dict
                representing keyword arguments.  The value is marshalled
                using repr(), so the dict values should be basic types.

  Some combinations of member settings are invalid:
    * A suite config may not specify both blocking and async.
    * A suite config may not specify both warn_only and critical.
  """
  _MINUTE = 60
  _HOUR = 60 * _MINUTE
  # CTS timeout about 2 * expected runtime in case other tests are using the CTS
  # pool.
  CTS_QUAL_HW_TEST_TIMEOUT = int(48.0 * _HOUR)
  # GTS runs faster than CTS. But to avoid starving GTS by CTS we set both
  # timeouts equal.
  GTS_QUAL_HW_TEST_TIMEOUT = CTS_QUAL_HW_TEST_TIMEOUT
  SHARED_HW_TEST_TIMEOUT = int(3.0 * _HOUR)
  PALADIN_HW_TEST_TIMEOUT = int(1.5 * _HOUR)
  BRANCHED_HW_TEST_TIMEOUT = int(10.0 * _HOUR)

  # TODO(jrbarnette) Async HW test phases complete within seconds.
  # however, the tests they start can require hours to complete.
  # Chromite code doesn't distinguish "timeout for Autotest" from
  # timeout in the builder.  This is WRONG WRONG WRONG.  But, until
  # there's a better fix, we'll allow these phases hours to fail.
  ASYNC_HW_TEST_TIMEOUT = int(250.0 * _MINUTE)

  def __init__(self, suite,
               pool=constants.HWTEST_MACH_POOL,
               timeout=SHARED_HW_TEST_TIMEOUT,
               async=False,
               warn_only=False,
               critical=False,
               blocking=False,
               file_bugs=False,
               priority=constants.HWTEST_BUILD_PRIORITY,
               retry=True,
               max_retries=constants.HWTEST_MAX_RETRIES,
               minimum_duts=0,
               suite_min_duts=0,
               suite_args=None,
               offload_failures_only=False):
    """Constructor -- see members above."""
    assert not async or not blocking
    assert not warn_only or not critical
    self.suite = suite
    self.pool = pool
    self.timeout = timeout
    self.blocking = blocking
    self.async = async
    self.warn_only = warn_only
    self.critical = critical
    self.file_bugs = file_bugs
    self.priority = priority
    self.retry = retry
    self.max_retries = max_retries
    self.minimum_duts = minimum_duts
    self.suite_min_duts = suite_min_duts
    self.suite_args = suite_args
    self.offload_failures_only = offload_failures_only

  def SetBranchedValues(self):
    """Changes the HW Test timeout/priority values to branched values."""
    self.timeout = max(HWTestConfig.BRANCHED_HW_TEST_TIMEOUT, self.timeout)

    # Set minimum_duts default to 0, which means that lab will not check the
    # number of available duts to meet the minimum requirement before creating
    # a suite job for branched build.
    self.minimum_duts = 0

    # Only reduce priority if it's lower.
    new_priority = constants.HWTEST_PRIORITIES_MAP[
        constants.HWTEST_DEFAULT_PRIORITY]
    if isinstance(self.priority, (int, long)):
      self.priority = min(self.priority, new_priority)
    elif constants.HWTEST_PRIORITIES_MAP[self.priority] > new_priority:
      self.priority = new_priority

  @property
  def timeout_mins(self):
    return int(self.timeout / 60)

  def __eq__(self, other):
    return self.__dict__ == other.__dict__


def DefaultSettings():
  # Enumeration of valid settings; any/all config settings must be in this.
  # All settings must be documented.
  return dict(
      # The name of the template we inherit settings from.
      _template=None,

      # The name of the config.
      name=None,

      # What type of builder is used for this build? This is a hint sent to
      # the waterfall code. It is ignored by the trybot waterfall.
      #   constants.VALID_BUILD_SLAVE_TYPES
      buildslave_type=constants.GCE_BEEFY_BUILD_SLAVE_TYPE,

      # A list of boards to build.
      boards=None,

      # A list of ModelTestConfig objects that represent all of the models
      # supported by a given unified build and their corresponding test config.
      models=[],

      # This value defines what part of the Golden Eye UI is responsible for
      # displaying builds of this build config. The value is required, and
      # must be in ALL_DISPLAY_LABEL.
      # TODO: Make the value required after crbug.com/776955 is finished.
      display_label=None,

      # This defines which LUCI Builder to use. It must match an entry in:
      #
      # https://chrome-internal.git.corp.google.com/chromeos/
      #    manifest-internal/+/infra/config/cr-buildbucket.cfg
      #
      luci_builder=LUCI_BUILDER_PROD,

      # The profile of the variant to set up and build.
      profile=None,

      # This bot pushes changes to the overlays.
      master=False,

      # If this bot triggers slave builds, this will contain a list of
      # slave config names.
      slave_configs=None,

      # If False, this flag indicates that the CQ should not check whether
      # this bot passed or failed. Set this to False if you are setting up a
      # new bot. Once the bot is on the waterfall and is consistently green,
      # mark the builder as important=True.
      important=False,

      # If True, build config should always be run as if --debug was set
      # on the cbuildbot command line. This is different from 'important'
      # and is usually correlated with tryjob build configs.
      debug=False,

      # If True, use the debug instance of CIDB instead of prod.
      debug_cidb=False,

      # Timeout for the build as a whole (in seconds).
      build_timeout=(4 * 60 + 30) * 60,

      # An integer. If this builder fails this many times consecutively, send
      # an alert email to the recipients health_alert_recipients. This does
      # not apply to tryjobs. This feature is similar to the ERROR_WATERMARK
      # feature of upload_symbols, and it may make sense to merge the features
      # at some point.
      health_threshold=0,

      # If this build_config fails this many times consecutively, trigger a
      # sanity-check build on this build_config. A sanity-check-pre-cq is a
      # pre-cq build without patched CLs.
      sanity_check_threshold=0,

      # List of email addresses to send health alerts to for this builder. It
      # supports automatic email address lookup for the following sheriff
      # types:
      #     'tree': tree sheriffs
      #     'chrome': chrome gardeners
      health_alert_recipients=[],

      # Whether this is an internal build config.
      internal=False,

      # Whether this is a branched build config. Used for pfq logic.
      branch=False,

      # The name of the manifest to use. E.g., to use the buildtools manifest,
      # specify 'buildtools'.
      manifest=constants.DEFAULT_MANIFEST,

      # The name of the manifest to use if we're building on a local trybot.
      # This should only require elevated access if it's really needed to
      # build this config.
      dev_manifest=constants.DEFAULT_MANIFEST,

      # Applies only to paladin builders. If true, Sync to the manifest
      # without applying any test patches, then do a fresh build in a new
      # chroot. Then, apply the patches and build in the existing chroot.
      build_before_patching=False,

      # Applies only to paladin builders. If True, Sync to the master manifest
      # without applying any of the test patches, rather than running
      # CommitQueueSync. This is basically ToT immediately prior to the
      # current commit queue run.
      do_not_apply_cq_patches=False,

      # emerge use flags to use while setting up the board, building packages,
      # making images, etc.
      useflags=[],

      # Set the variable CHROMEOS_OFFICIAL for the build. Known to affect
      # parallel_emerge, cros_set_lsb_release, and chromeos_version.sh. See
      # bug chromium-os:14649
      chromeos_official=False,

      # Use binary packages for building the toolchain. (emerge --getbinpkg)
      usepkg_toolchain=True,

      # Use binary packages for build_packages and setup_board.
      usepkg_build_packages=True,

      # If set, run BuildPackages in the background and allow subsequent
      # stages to run in parallel with this one.
      #
      # For each release group, the first builder should be set to run in the
      # foreground (to build binary packages), and the remainder of the
      # builders should be set to run in parallel (to install the binary
      # packages.)
      build_packages_in_background=False,

      # Only use binaries in build_packages for Chrome itself.
      chrome_binhost_only=False,

      # Does this profile need to sync chrome?  If None, we guess based on
      # other factors.  If True/False, we always do that.
      sync_chrome=None,

      # Use the newest ebuilds for all the toolchain packages.
      latest_toolchain=False,

      # This is only valid when latest_toolchain is True. If you set this to a
      # commit-ish, the gcc ebuild will use it to build the toolchain
      # compiler.
      gcc_githash=None,

      # Wipe and replace the board inside the chroot.
      board_replace=False,

      # Wipe and replace chroot, but not source.
      chroot_replace=True,

      # Create the chroot on a loopback-mounted chroot.img instead of a bare
      # directory.  Required for snapshots; otherwise optional.
      chroot_use_image=True,

      # Uprevs the local ebuilds to build new changes since last stable.
      # build.  If master then also pushes these changes on success. Note that
      # we uprev on just about every bot config because it gives us a more
      # deterministic build system (the tradeoff being that some bots build
      # from source more frequently than if they never did an uprev). This way
      # the release/factory/etc... builders will pick up changes that devs
      # pushed before it runs, but after the correspoding PFQ bot ran (which
      # is what creates+uploads binpkgs).  The incremental bots are about the
      # only ones that don't uprev because they mimic the flow a developer
      # goes through on their own local systems.
      uprev=True,

      # Select what overlays to look at for revving and prebuilts. This can be
      # any constants.VALID_OVERLAYS.
      overlays=constants.PUBLIC_OVERLAYS,

      # Select what overlays to push at. This should be a subset of overlays
      # for the particular builder.  Must be None if not a master.  There
      # should only be one master bot pushing changes to each overlay per
      # branch.
      push_overlays=None,

      # Uprev Android, values of 'latest_release', or None.
      android_rev=None,

      # Which Android branch build do we try to uprev from.
      android_import_branch=None,

      # Android package name.
      android_package=None,

      # Android GTS package branch name, if it is necessary to uprev.
      android_gts_build_branch=None,

      # Uprev Chrome, values of 'tot', 'stable_release', or None.
      chrome_rev=None,

      # Exit the builder right after checking compilation.
      # TODO(mtennant): Should be something like "compile_check_only".
      compilecheck=False,

      # Test CLs to verify they're ready for the commit queue.
      pre_cq=False,

      # Runs the tests that the signer would run. This should only be set if
      # 'recovery' is in images.
      signer_tests=False,

      # Runs unittests for packages.
      unittests=True,

      # A list of the packages to blacklist from unittests.
      unittest_blacklist=[],

      # Generates AFDO data. Will capture a profile of chrome using a hwtest
      # to run a predetermined set of benchmarks.
      afdo_generate=False,

      # Generates AFDO data, builds the minimum amount of artifacts and
      # assumes a non-distributed builder (i.e.: the whole process in a single
      # builder).
      afdo_generate_min=False,

      # Update the Chrome ebuild with the AFDO profile info.
      afdo_update_ebuild=False,

      # Uses AFDO data. The Chrome build will be optimized using the AFDO
      # profile information found in the chrome ebuild file.
      afdo_use=False,

      # A list of VMTestConfig objects to run by default.
      vm_tests=[VMTestConfig(constants.VM_SUITE_TEST_TYPE, test_suite='smoke'),
                VMTestConfig(constants.SIMPLE_AU_TEST_TYPE)],

      # A list of all VMTestConfig objects to use if VM Tests are forced on
      # (--vmtest command line or trybot). None means no override.
      vm_tests_override=None,

      # If true, in addition to upload vm test result to artifact folder, report
      # results to other dashboard as well.
      vm_test_report_to_dashboards=False,

      # The number of times to run the VMTest stage. If this is >1, then we
      # will run the stage this many times, stopping if we encounter any
      # failures.
      vm_test_runs=1,

      # A list of HWTestConfig objects to run.
      hw_tests=[],

      # A list of all HWTestConfig objects to use if HW Tests are forced on
      # (--hwtest command line or trybot). None means no override.
      hw_tests_override=None,

      # If true, uploads artifacts for hw testing. Upload payloads for test
      # image if the image is built. If not, dev image is used and then base
      # image.
      upload_hw_test_artifacts=True,

      # If true, uploads individual image tarballs.
      upload_standalone_images=True,

      # A list of GCETestConfig objects to use. Currently only some lakitu
      # builders run gce tests.
      gce_tests=[],

      # A list of TastVMTestConfig objects describing Tast-based test suites
      # that should be run in a VM.
      tast_vm_tests=[],

      # Default to not run moblab tests. Currently the blessed moblab board runs
      # these tests.
      moblab_vm_tests=[],

      # List of patterns for portage packages for which stripped binpackages
      # should be uploaded to GS. The patterns are used to search for packages
      # via `equery list`.
      upload_stripped_packages=[
          # Used by SimpleChrome workflow.
          'chromeos-base/chromeos-chrome',
          'sys-kernel/*kernel*',
      ],

      # Google Storage path to offload files to.
      #   None - No upload
      #   GS_PATH_DEFAULT - 'gs://chromeos-image-archive/' + bot_id
      #   value - Upload to explicit path
      gs_path=GS_PATH_DEFAULT,

      # TODO(sosa): Deprecate binary.
      # Type of builder.  Check constants.VALID_BUILD_TYPES.
      build_type=constants.PFQ_TYPE,

      # Whether to schedule test suites by suite_scheduler. Generally only
      # True for "release" builders.
      suite_scheduling=False,

      # The class name used to build this config.  See the modules in
      # cbuildbot / builders/*_builders.py for possible values.  This should
      # be the name in string form -- e.g. "simple_builders.SimpleBuilder" to
      # get the SimpleBuilder class in the simple_builders module.  If not
      # specified, we'll fallback to legacy probing behavior until everyone
      # has been converted (see the scripts/cbuildbot.py file for details).
      builder_class_name=None,

      # List of images we want to build -- see build_image for more details.
      images=['test'],

      # Image from which we will build update payloads.  Must either be None
      # or name one of the images in the 'images' list, above.
      payload_image=None,

      # Whether to build a netboot image.
      factory_install_netboot=True,

      # Whether to build the factory toolkit.
      factory_toolkit=True,

      # Whether to build factory packages in BuildPackages.
      factory=True,

      # Tuple of specific packages we want to build.  Most configs won't
      # specify anything here and instead let build_packages calculate.
      packages=[],

      # Do we push a final release image to chromeos-images.
      push_image=False,

      # Do we upload debug symbols.
      upload_symbols=False,

      # Whether we upload a hwqual tarball.
      hwqual=False,

      # Run a stage that generates release payloads for signed images.
      paygen=False,

      # If the paygen stage runs, generate tests, and schedule auto-tests for
      # them.
      paygen_skip_testing=False,

      # If the paygen stage runs, don't generate any delta payloads. This is
      # only done if deltas are broken for a given board.
      paygen_skip_delta_payloads=False,

      # Run a stage that generates and uploads package CPE information.
      cpe_export=True,

      # Run a stage that generates and uploads debug symbols.
      debug_symbols=True,

      # Do not package the debug symbols in the binary package. The debug
      # symbols will be in an archive with the name cpv.debug.tbz2 in
      # /build/${BOARD}/packages and uploaded with the prebuilt.
      separate_debug_symbols=True,

      # Include *.debug files for debugging core files with gdb in debug.tgz.
      # These are very large. This option only has an effect if debug_symbols
      # and archive are set.
      archive_build_debug=False,

      # Run a stage that archives build and test artifacts for developer
      # consumption.
      archive=True,

      # Git repository URL for our manifests.
      #  https://chromium.googlesource.com/chromiumos/manifest
      #  https://chrome-internal.googlesource.com/chromeos/manifest-internal
      manifest_repo_url=None,

      # Whether we are using the manifest_version repo that stores per-build
      # manifests.
      manifest_version=False,

      # Use a different branch of the project manifest for the build.
      manifest_branch=None,

      # Use the Last Known Good Manifest blessed by Paladin.
      use_lkgm=False,

      # If we use_lkgm -- What is the name of the manifest to look for?
      lkgm_manifest=constants.LKGM_MANIFEST,

      # LKGM for Chrome OS generated for Chrome builds that are blessed from
      # canary runs.
      use_chrome_lkgm=False,

      # True if this build config is critical for the chrome_lkgm decision.
      critical_for_chrome=False,

      # Upload prebuilts for this build. Valid values are PUBLIC, PRIVATE, or
      # False.
      prebuilts=False,

      # Use SDK as opposed to building the chroot from source.
      use_sdk=True,

      # The description string to print out for config when user runs --list.
      description=None,

      # Boolean that enables parameter --git-sync for upload_prebuilts.
      git_sync=False,

      # A list of the child config groups, if applicable. See the AddGroup
      # method.
      child_configs=[],

      # Set shared user password for "chronos" user in built images. Use
      # "None" (default) to remove the shared user password. Note that test
      # images will always set the password to "test0000".
      shared_user_password=None,

      # Whether this config belongs to a config group.
      grouped=False,

      # layout of build_image resulting image. See
      # scripts/build_library/legacy_disk_layout.json or
      # overlay-<board>/scripts/disk_layout.json for possible values.
      disk_layout=None,

      # If enabled, run the PatchChanges stage.  Enabled by default. Can be
      # overridden by the --nopatch flag.
      postsync_patch=True,

      # Reexec into the buildroot after syncing.  Enabled by default.
      postsync_reexec=True,

      # Create delta sysroot during ArchiveStage. Disabled by default.
      create_delta_sysroot=False,

      # Run the binhost_test stage. Only makes sense for builders that have no
      # boards.
      binhost_test=False,

      # Run the BranchUtilTestStage. Useful for builders that publish new
      # manifest versions that we may later want to branch off of.
      branch_util_test=False,

      # If specified, it is passed on to the PushImage script as '--sign-types'
      # commandline argument.  Must be either None or a list of image types.
      sign_types=None,

      # TODO(sosa): Collapse to one option.
      # ========== Dev installer prebuilts options =======================

      # Upload prebuilts for this build to this bucket. If it equals None the
      # default buckets are used.
      binhost_bucket=None,

      # Parameter --key for upload_prebuilts. If it equals None, the default
      # values are used, which depend on the build type.
      binhost_key=None,

      # Parameter --binhost-base-url for upload_prebuilts. If it equals None,
      # the default value is used.
      binhost_base_url=None,

      # Upload dev installer prebuilts.
      dev_installer_prebuilts=False,

      # Enable rootfs verification on the image.
      rootfs_verification=True,

      # Build the Chrome SDK.
      chrome_sdk=False,

      # If chrome_sdk is set to True, this determines whether we attempt to
      # build Chrome itself with the generated SDK.
      chrome_sdk_build_chrome=True,

      # If chrome_sdk is set to True, this determines whether we use goma to
      # build chrome.
      chrome_sdk_goma=True,

      # Run image tests. This should only be set if 'base' is in our list of
      # images.
      image_test=False,

      # ==================================================================
      # The documentation associated with the config.
      doc=None,

      # ==================================================================
      # Hints to Buildbot master UI

      # If set, tells buildbot what name to give to the corresponding builder
      # on its waterfall.
      buildbot_waterfall_name=None,

      # If not None, the name (in waterfall.CIDB_KNOWN_WATERFALLS) of the
      # waterfall that this target should be active on.
      active_waterfall=None,

      # If true, skip package retries in BuildPackages step.
      nobuildretry=False,

      # If false, turn off rebooting between builds
      auto_reboot=True,
  )


def GerritInstanceParameters(name, instance, defaults=False):
  GOB_HOST = '%s.googlesource.com'
  param_names = ['_GOB_INSTANCE', '_GERRIT_INSTANCE', '_GOB_HOST',
                 '_GERRIT_HOST', '_GOB_URL', '_GERRIT_URL']
  if defaults:
    return dict([('%s%s' % (name, x), None) for x in param_names])

  gob_instance = instance
  gerrit_instance = '%s-review' % instance
  gob_host = GOB_HOST % gob_instance
  gerrit_host = GOB_HOST % gerrit_instance
  gob_url = 'https://%s' % gob_host
  gerrit_url = 'https://%s' % gerrit_host

  params = [gob_instance, gerrit_instance, gob_host, gerrit_host,
            gob_url, gerrit_url]

  return dict([('%s%s' % (name, pn), p) for pn, p in zip(param_names, params)])


def DefaultSiteParameters():
  # Enumeration of valid site parameters; any/all site parameters must be here.
  # All site parameters should be documented.
  default_site_params = {}

  # Helper variables for defining site parameters.
  gob_host = '%s.googlesource.com'

  external_remote = 'cros'
  internal_remote = 'cros-internal'
  chromium_remote = 'chromium'
  chrome_remote = 'chrome'

  internal_change_prefix = '*'
  external_change_prefix = ''

  # Gerrit instance site parameters.
  default_site_params.update(GOB_HOST=gob_host)
  default_site_params.update(
      GerritInstanceParameters('EXTERNAL', 'chromium'))
  default_site_params.update(
      GerritInstanceParameters('INTERNAL', 'chrome-internal'))
  default_site_params.update(
      GerritInstanceParameters('AOSP', 'android', defaults=True))
  default_site_params.update(
      GerritInstanceParameters('WEAVE', 'weave', defaults=True))

  default_site_params.update(
      # Parameters to define which manifests to use.
      MANIFEST_PROJECT=None,
      MANIFEST_INT_PROJECT=None,
      MANIFEST_PROJECTS=None,
      MANIFEST_URL=None,
      MANIFEST_INT_URL=None,

      # CrOS remotes specified in the manifests.
      EXTERNAL_REMOTE=external_remote,
      INTERNAL_REMOTE=internal_remote,
      GOB_REMOTES=None,
      KAYLE_INTERNAL_REMOTE=None,
      CHROMIUM_REMOTE=None,
      CHROME_REMOTE=None,
      AOSP_REMOTE=None,
      WEAVE_REMOTE=None,

      # Only remotes listed in CROS_REMOTES are considered branchable.
      # CROS_REMOTES and BRANCHABLE_PROJECTS must be kept in sync.
      GERRIT_HOSTS={
          external_remote: default_site_params['EXTERNAL_GERRIT_HOST'],
          internal_remote: default_site_params['INTERNAL_GERRIT_HOST']
      },
      CROS_REMOTES={
          external_remote: default_site_params['EXTERNAL_GOB_URL'],
          internal_remote: default_site_params['INTERNAL_GOB_URL']
      },
      GIT_REMOTES={
          chromium_remote: default_site_params['EXTERNAL_GOB_URL'],
          chrome_remote: default_site_params['INTERNAL_GOB_URL'],
          external_remote: default_site_params['EXTERNAL_GOB_URL'],
          internal_remote: default_site_params['INTERNAL_GOB_URL'],
      },

      # Prefix to distinguish internal and external changes. This is used
      # when a user specifies a patch with "-g", when generating a key for
      # a patch to use in our PatchCache, and when displaying a custom
      # string for the patch.
      INTERNAL_CHANGE_PREFIX=internal_change_prefix,
      EXTERNAL_CHANGE_PREFIX=external_change_prefix,
      CHANGE_PREFIX={
          external_remote: internal_change_prefix,
          internal_remote: external_change_prefix
      },

      # List of remotes that are okay to include in the external manifest.
      EXTERNAL_REMOTES=None,

      # Mapping 'remote name' -> regexp that matches names of repositories on
      # that remote that can be branched when creating CrOS branch.
      # Branching script will actually create a new git ref when branching
      # these projects. It won't attempt to create a git ref for other projects
      # that may be mentioned in a manifest. If a remote is missing from this
      # dictionary, all projects on that remote are considered to not be
      # branchable.
      BRANCHABLE_PROJECTS={
          external_remote: r'(chromiumos|aosp)/(.+)',
          internal_remote: r'chromeos/(.+)'
      },

      # Additional parameters used to filter manifests, create modified
      # manifests, and to branch manifests.
      MANIFEST_VERSIONS_GOB_URL=None,
      MANIFEST_VERSIONS_GOB_URL_TEST=None,
      MANIFEST_VERSIONS_INT_GOB_URL=None,
      MANIFEST_VERSIONS_INT_GOB_URL_TEST=None,
      MANIFEST_VERSIONS_GS_URL=None,

      # Standard directories under buildroot for cloning these repos.
      EXTERNAL_MANIFEST_VERSIONS_PATH=None,
      INTERNAL_MANIFEST_VERSIONS_PATH=None,

      # URL of the repo project.
      REPO_URL='https://chromium.googlesource.com/external/repo',

      # GS URL in which to archive build artifacts.
      ARCHIVE_URL='gs://chromeos-image-archive',
  )

  return default_site_params


class SiteParameters(dict):
  """This holds the site-wide configuration parameters for a SiteConfig."""

  def __getattr__(self, name):
    """Support attribute-like access to each SiteValue entry."""
    if name in self:
      return self[name]

    return super(SiteParameters, self).__getattribute__(name)

  @classmethod
  def HideDefaults(cls, site_params):
    """Hide default valued site parameters.

    Args:
      site_params: A dictionary of site parameters.

    Returns:
      A dictionary of site parameters containing only non-default
      valued entries.
    """
    defaults = DefaultSiteParameters()
    return {k: v for k, v in site_params.iteritems() if defaults.get(k) != v}


class SiteConfig(dict):
  """This holds a set of named BuildConfig values."""

  def __init__(self, defaults=None, templates=None, site_params=None):
    """Init.

    Args:
      defaults: Dictionary of key value pairs to use as BuildConfig values.
                All BuildConfig values should be defined here. If None,
                the DefaultSettings() is used. Most sites should use
                DefaultSettings(), and then update to add any site specific
                values needed.
      templates: Dictionary of template names to partial BuildConfigs
                 other BuildConfigs can be based on. Mostly used to reduce
                 verbosity of the config dump file format.
      site_params: Dictionary of site-wide configuration parameters. Keys
                   of the site_params dictionary should be strings.
    """
    super(SiteConfig, self).__init__()
    self._defaults = DefaultSettings()
    if defaults:
      self._defaults.update(defaults)
    self._templates = AttrDict() if templates is None else AttrDict(templates)
    self._site_params = DefaultSiteParameters()
    if site_params:
      self._site_params.update(site_params)

  def GetDefault(self):
    """Create the canonical default build configuration."""
    # Enumeration of valid settings; any/all config settings must be in this.
    # All settings must be documented.
    return BuildConfig(**self._defaults)

  def GetTemplates(self):
    """Get the templates of the build configs"""
    return self._templates

  @property
  def templates(self):
    return self._templates

  @property
  def params(self):
    """Get the site-wide configuration parameters."""
    return SiteParameters(**self._site_params)

  #
  # Methods for searching a SiteConfig's contents.
  #
  def GetBoards(self):
    """Return an iterable of all boards in the SiteConfig."""
    return set(itertools.chain.from_iterable(
        x.boards for x in self.itervalues() if x.boards))

  def FindFullConfigsForBoard(self, board=None):
    """Returns full builder configs for a board.

    Args:
      board: The board to match. By default, match all boards.

    Returns:
      A tuple containing a list of matching external configs and a list of
      matching internal release configs for a board.
    """
    ext_cfgs = []
    int_cfgs = []

    for name, c in self.iteritems():
      if c['boards'] and (board is None or board in c['boards']):
        if (name.endswith('-%s' % CONFIG_TYPE_RELEASE) and
            c['internal']):
          int_cfgs.append(c.deepcopy())
        elif (name.endswith('-%s' % CONFIG_TYPE_FULL) and
              not c['internal']):
          ext_cfgs.append(c.deepcopy())

    return ext_cfgs, int_cfgs

  def FindCanonicalConfigForBoard(self, board, allow_internal=True):
    """Get the canonical cbuildbot builder config for a board."""
    ext_cfgs, int_cfgs = self.FindFullConfigsForBoard(board)
    # If both external and internal builds exist for this board, prefer the
    # internal one unless instructed otherwise.
    both = (int_cfgs if allow_internal else []) + ext_cfgs

    if not both:
      raise ValueError('Invalid board specified: %s.' % board)
    return both[0]

  def GetSlaveConfigMapForMaster(self, master_config, options=None,
                                 important_only=True):
    """Gets the slave builds triggered by a master config.

    If a master builder also performs a build, it can (incorrectly) return
    itself.

    Args:
      master_config: A build config for a master builder.
      options: The options passed on the commandline. This argument is required
      for normal operation, but we accept None to assist with testing.
      important_only: If True, only get the important slaves.

    Returns:
      A slave_name to slave_config map, corresponding to the slaves for the
      master represented by master_config.

    Raises:
      AssertionError if the given config is not a master config or it does
        not have a manifest_version.
    """
    assert master_config.manifest_version
    assert master_config.master
    assert master_config.slave_configs is not None

    slave_name_config_map = {}
    if options is not None and options.remote_trybot:
      return {}

    # Look up the build configs for all slaves named by the master.
    slave_name_config_map = {
        name: self[name] for name in master_config.slave_configs
    }

    if important_only:
      # Remove unimportant configs from the result.
      slave_name_config_map = {
          k: v for k, v in slave_name_config_map.iteritems() if v.important
      }

    return slave_name_config_map

  def GetSlavesForMaster(self, master_config, options=None,
                         important_only=True):
    """Get a list of qualified build slave configs given the master_config.

    Args:
      master_config: A build config for a master builder.
      options: The options passed on the commandline. This argument is optional,
               and only makes sense when called from cbuildbot.
      important_only: If True, only get the important slaves.
    """
    slave_map = self.GetSlaveConfigMapForMaster(
        master_config, options=options, important_only=important_only)
    return slave_map.values()

  #
  # Methods used when creating a Config programatically.
  #
  def Add(self, name, template=None, *args, **kwargs):
    """Add a new BuildConfig to the SiteConfig.

    Example usage:
      # Creates default build named foo.
      site_config.Add('foo')

      # Creates default build with board 'foo_board'
      site_config.Add('foo',
                      boards=['foo_board'])

      # Creates build based on template_build for 'foo_board'.
      site_config.Add('foo',
                      template_build,
                      boards=['foo_board'])

      # Creates build based on template for 'foo_board'. with mixin.
      # Inheritance order is default, template, mixin, arguments.
      site_config.Add('foo',
                      template_build,
                      mixin_build_config,
                      boards=['foo_board'])

      # Creates build without a template but with mixin.
      # Inheritance order is default, template, mixin, arguments.
      site_config.Add('foo',
                      None,
                      mixin_build_config,
                      boards=['foo_board'])

    Args:
      name: The name to label this configuration; this is what cbuildbot
            would see.
      template: BuildConfig to use as a template for this build.
      args: BuildConfigs to patch into this config. First one (if present) is
            considered the template. See AddTemplate for help on templates.
      kwargs: BuildConfig values to explicitly set on this config.

    Returns:
      The BuildConfig just added to the SiteConfig.
    """
    assert name not in self, ('%s already exists.' % name)

    inherits, overrides = args, kwargs
    if template:
      inherits = (template,) + inherits

    # Make sure we don't ignore that argument silently.
    if '_template' in overrides:
      raise ValueError('_template cannot be explicitly set.')

    result = self.GetDefault()
    result.apply(*inherits, **overrides)

    # Select the template name based on template argument, or nothing.
    resolved_template = template.get('_template') if template else None
    assert not resolved_template or resolved_template in self.templates, \
        '%s inherits from non-template %s' % (name, resolved_template)

    # Our name is passed as an explicit argument. We use the first build
    # config as our template, or nothing.
    result['name'] = name
    result['_template'] = resolved_template
    self[name] = result
    return result

  def AddWithoutTemplate(self, name, *args, **kwargs):
    """Add a config containing only explicitly listed values (no defaults)."""
    self.Add(name, None, *args, **kwargs)

  def AddGroup(self, name, *args, **kwargs):
    """Create a new group of build configurations.

    Args:
      name: The name to label this configuration; this is what cbuildbot
            would see.
      args: Configurations to build in this group. The first config in
            the group is considered the primary configuration and is used
            for syncing and creating the chroot.
      kwargs: Override values to use for the parent config.

    Returns:
      A new BuildConfig instance.
    """
    child_configs = [x.deepcopy().apply(grouped=True) for x in args]
    return self.Add(name, args[0], child_configs=child_configs, **kwargs)

  def AddForBoards(self, suffix, boards, per_board=None,
                   template=None, *args, **kwargs):
    """Create configs for all boards in |boards|.

    Args:
      suffix: Config name is <board>-<suffix>.
      boards: A list of board names as strings.
      per_board: A dictionary of board names to BuildConfigs, or None.
      template: The template to use for all configs created.
      *args: Mixin templates to apply.
      **kwargs: Additional keyword arguments to be used in AddConfig.

    Returns:
      List of the configs created.
    """
    result = []

    for board in boards:
      config_name = '%s-%s' % (board, suffix)

      # Insert the per_board value as the last mixin, if it exists.
      mixins = args + (dict(boards=[board]),)
      if per_board and board in per_board:
        mixins = mixins + (per_board[board],)

      # Create the new config for this board.
      result.append(
          self.Add(config_name, template, *mixins, **kwargs))

    return result

  def AddTemplate(self, name, *args, **kwargs):
    """Create a template named |name|.

    Templates are used to define common settings that are shared across types
    of builders. They help reduce duplication in config_dump.json, because we
    only define the template and its settings once.

    Args:
      name: The name of the template.
      args: See the docstring of BuildConfig.derive.
      kwargs: See the docstring of BuildConfig.derive.
    """
    assert name not in self._templates, ('Template %s already exists.' % name)

    template = BuildConfig()
    template.apply(*args, **kwargs)
    template['_template'] = name
    self._templates[name] = template

    return template

  def _MarshalBuildConfig(self, name, config):
    """Hide the defaults from a given config entry.

    Args:
      name: Default build name (usually dictionary key).
      config: A config entry.

    Returns:
      The same config entry, but without any defaults.
    """
    defaults = self.GetDefault()
    defaults['name'] = name

    template = config.get('_template')
    if template:
      defaults.apply(self._templates[template])
      defaults['_template'] = None

    result = {}
    for k, v in config.iteritems():
      if defaults.get(k) != v:
        if k == 'child_configs':
          result['child_configs'] = [self._MarshalBuildConfig(name, child)
                                     for child in v]
        else:
          result[k] = v

    return result

  def _MarshalTemplates(self):
    """Return a version of self._templates with only used templates.

    Templates have callables/delete keys resolved against GetDefault() to
    ensure they can be safely saved to json.

    Returns:
      Dict copy of self._templates with all unreferenced templates removed.
    """
    defaults = self.GetDefault()

    # All templates used. We ignore child configs since they
    # should exist at top level.
    used = set(c.get('_template', None) for c in self.itervalues())
    used.discard(None)

    result = {}

    for name in used:
      # Expand any special values (callables, etc)
      expanded = defaults.derive(self._templates[name])
      # Recover the '_template' value which is filtered out by derive.
      expanded['_template'] = name
      # Hide anything that matches the default.
      save = {k: v for k, v in expanded.iteritems() if defaults.get(k) != v}
      result[name] = save

    return result

  def SaveConfigToString(self):
    """Save this Config object to a Json format string."""
    default = self.GetDefault()
    site_params = self.params

    config_dict = {}
    config_dict['_default'] = default
    config_dict['_templates'] = self._MarshalTemplates()
    config_dict['_site_params'] = SiteParameters.HideDefaults(site_params)
    for k, v in self.iteritems():
      config_dict[k] = self._MarshalBuildConfig(k, v)

    return PrettyJsonDict(config_dict)

  def SaveConfigToFile(self, config_file):
    """Save this Config to a Json file.

    Args:
      config_file: The file to write too.
    """
    json_string = self.SaveConfigToString()
    osutils.WriteFile(config_file, json_string)

  def DumpExpandedConfigToString(self):
    """Dump the SiteConfig to Json with all configs full expanded.

    This is intended for debugging default/template behavior. The dumped JSON
    can't be reloaded (at least not reliably).
    """
    return PrettyJsonDict(self)

#
# Methods related to working with GE Data.
#

def LoadGEBuildConfigFromFile(
    build_settings_file=constants.GE_BUILD_CONFIG_FILE):
  """Load template config dict from a Json encoded file."""
  json_string = osutils.ReadFile(build_settings_file)
  return json.loads(json_string)


def GeBuildConfigAllBoards(ge_build_config):
  """Extract a list of board names from the GE Build Config.

  Args:
    ge_build_config: Dictionary containing the decoded GE configuration file.

  Returns:
    A list of board names as strings.
  """
  return [b['name'] for b in ge_build_config['boards']]

def GetUnifiedBuildConfigAllBuilds(ge_build_config):
  """Extract a list of all unified build configurations.

  This dictionary is based on the JSON defined by the proto generated from
  GoldenEye.  See cs/crosbuilds.proto

  Args:
    ge_build_config: Dictionary containing the decoded GE configuration file.

  Returns:
    A list of unified build configurations (json configs)
  """
  return ge_build_config.get('reference_board_unified_builds', [])

class BoardGroup(object):
  """Class holds leader_boards and follower_boards for grouped boards"""

  def __init__(self):
    self.leader_boards = []
    self.follower_boards = []

  def AddLeaderBoard(self, board):
    self.leader_boards.append(board)

  def AddFollowerBoard(self, board):
    self.follower_boards.append(board)

  def __str__(self):
    return ('Leader_boards: %s Follower_boards: %s' %
            (self.leader_boards, self.follower_boards))

def GroupBoardsByBuilderAndBoardGroup(board_list):
  """Group boards by builder and board_group.

  Args:
    board_list: board list from the template file.

  Returns:
    builder_group_dict: maps builder to {group_n: board_group_n}
    builder_ungrouped_dict: maps builder to a list of ungrouped boards
  """
  builder_group_dict = {}
  builder_ungrouped_dict = {}

  for b in board_list:
    name = b[CONFIG_TEMPLATE_NAME]
    for config in b[CONFIG_TEMPLATE_CONFIGS]:
      board = {'name': name}
      board.update(config)

      builder = config[CONFIG_TEMPLATE_BUILDER]
      if builder not in builder_group_dict:
        builder_group_dict[builder] = {}
      if builder not in builder_ungrouped_dict:
        builder_ungrouped_dict[builder] = []

      board_group = config[CONFIG_TEMPLATE_BOARD_GROUP]
      if not board_group:
        builder_ungrouped_dict[builder].append(board)
        continue
      if board_group not in builder_group_dict[builder]:
        builder_group_dict[builder][board_group] = BoardGroup()
      if config[CONFIG_TEMPLATE_LEADER_BOARD]:
        builder_group_dict[builder][board_group].AddLeaderBoard(board)
      else:
        builder_group_dict[builder][board_group].AddFollowerBoard(board)

  return (builder_group_dict, builder_ungrouped_dict)


def GroupBoardsByBuilder(board_list):
  """Group boards by the 'builder' flag."""
  builder_to_boards_dict = {}

  for b in board_list:
    for config in b[CONFIG_TEMPLATE_CONFIGS]:
      builder = config[CONFIG_TEMPLATE_BUILDER]
      if builder not in builder_to_boards_dict:
        builder_to_boards_dict[builder] = set()
      builder_to_boards_dict[builder].add(b[CONFIG_TEMPLATE_NAME])

  return builder_to_boards_dict

def GetArchBoardDict(ge_build_config):
  """Get a dict mapping arch types to board names.

  Args:
    ge_build_config: Dictionary containing the decoded GE configuration file.

  Returns:
    A dict mapping arch types to board names.
  """
  arch_board_dict = {}

  for b in ge_build_config[CONFIG_TEMPLATE_BOARDS]:
    board_name = b[CONFIG_TEMPLATE_NAME]
    for config in b[CONFIG_TEMPLATE_CONFIGS]:
      arch = config[CONFIG_TEMPLATE_ARCH]
      arch_board_dict.setdefault(arch, set()).add(board_name)

  for b in GetUnifiedBuildConfigAllBuilds(ge_build_config):
    board_name = b[CONFIG_TEMPLATE_REFERENCE_BOARD_NAME]
    arch = b[CONFIG_TEMPLATE_ARCH]
    arch_board_dict.setdefault(arch, set()).add(board_name)

  return arch_board_dict

#
# Methods related to loading/saving Json.
#
class ObjectJSONEncoder(json.JSONEncoder):
  """Json Encoder that encodes objects as their dictionaries."""
  # pylint: disable=method-hidden
  def default(self, obj):
    return self.encode(obj.__dict__)


def PrettyJsonDict(dictionary):
  """Returns a pretty-ified json dump of a dictionary."""
  return json.dumps(dictionary, cls=ObjectJSONEncoder,
                    sort_keys=True, indent=4, separators=(',', ': '))


def LoadConfigFromFile(config_file=constants.CHROMEOS_CONFIG_FILE):
  """Load a Config a Json encoded file."""
  json_string = osutils.ReadFile(config_file)
  return LoadConfigFromString(json_string)


def LoadConfigFromString(json_string):
  """Load a cbuildbot config from it's Json encoded string."""
  config_dict = json.loads(json_string)

  # Use standard defaults, but allow the config to override.
  defaults = DefaultSettings()
  defaults.update(config_dict.pop(DEFAULT_BUILD_CONFIG))
  _DeserializeTestConfigs(defaults)

  templates = config_dict.pop('_templates', {})
  for t in templates.itervalues():
    _DeserializeTestConfigs(t)

  site_params = DefaultSiteParameters()
  site_params.update(config_dict.pop('_site_params', {}))

  defaultBuildConfig = BuildConfig(**defaults)

  builds = {n: _CreateBuildConfig(n, defaultBuildConfig, v, templates)
            for n, v in config_dict.iteritems()}

  # config is the struct that holds the complete cbuildbot config.
  result = SiteConfig(defaults=defaults, templates=templates,
                      site_params=site_params)
  result.update(builds)

  return result


def _DeserializeTestConfig(build_dict, config_key, test_class,
                           preserve_none=False):
  """Deserialize test config of given type inside build_dict.

  Args:
    build_dict: The build_dict to update (in place)
    config_key: Key for the config inside build_dict.
    test_class: The class to instantiate for the config.
    preserve_none: If True, None values are preserved as is. By default, they
        are dropped.
  """
  serialized_test_configs = build_dict.pop(config_key, None)
  if serialized_test_configs is None:
    if preserve_none:
      build_dict[config_key] = None
    return

  test_configs = []
  for test_config_string in serialized_test_configs:
    if isinstance(test_config_string, test_class):
      test_config = test_config_string
    else:
      # Each test config is dumped as a json string embedded in json.
      embedded_configs = json.loads(test_config_string)
      test_config = test_class(**embedded_configs)
    test_configs.append(test_config)
  build_dict[config_key] = test_configs


def _DeserializeTestConfigs(build_dict):
  """Updates a config dictionary with recreated objects.

  Various test configs are serialized as strings (rather than JSON objects), so
  we need to turn them into real objects before they can be consumed.

  Args:
    build_dict: The config dictionary to update (in place).
  """
  _DeserializeTestConfig(build_dict, 'vm_tests', VMTestConfig)
  _DeserializeTestConfig(build_dict, 'vm_tests_override', VMTestConfig,
                         preserve_none=True)
  _DeserializeTestConfig(build_dict, 'models', ModelTestConfig)
  _DeserializeTestConfig(build_dict, 'hw_tests', HWTestConfig)
  _DeserializeTestConfig(build_dict, 'hw_tests_override', HWTestConfig,
                         preserve_none=True)
  _DeserializeTestConfig(build_dict, 'gce_tests', GCETestConfig)
  _DeserializeTestConfig(build_dict, 'tast_vm_tests', TastVMTestConfig)
  _DeserializeTestConfig(build_dict, 'moblab_vm_tests', MoblabVMTestConfig)


def _CreateBuildConfig(name, default, build_dict, templates):
  """Create a BuildConfig object from it's parsed JSON dictionary encoding."""
  # These build config values need special handling.
  child_configs = build_dict.pop('child_configs', None)
  template = build_dict.get('_template')

  # Use the name passed in as the default build name.
  build_dict.setdefault('name', name)

  result = default.deepcopy()
  # Use update to explicitly avoid apply's special handing.
  if template:
    result.update(templates[template])
  result.update(build_dict)

  _DeserializeTestConfigs(result)

  if child_configs is not None:
    result['child_configs'] = [
        _CreateBuildConfig(name, default, child, templates)
        for child in child_configs
    ]

  return result


def ClearConfigCache():
  """Clear the currently cached SiteConfig.

  This is intended to be used very early in the startup, after we fetch/update
  the site config information available to us.

  However, this operation is never 100% safe, since the Chrome OS config, or an
  outdated config was availble to any code that ran before (including on
  import), and that code might have used or cached related values.
  """
  # pylint: disable=global-statement
  global _CACHED_CONFIG
  _CACHED_CONFIG = None


def GetConfig():
  """Load the current SiteConfig.

  Returns:
    SiteConfig instance to use for this build.
  """
  # pylint: disable=global-statement
  global _CACHED_CONFIG

  if _CACHED_CONFIG is None:
    if os.path.exists(constants.SITE_CONFIG_FILE):
      # Use a site specific config, if present.
      filename = constants.SITE_CONFIG_FILE
    else:
      # Fall back to default Chrome OS configuration.
      filename = constants.CHROMEOS_CONFIG_FILE

    _CACHED_CONFIG = LoadConfigFromFile(filename)

  return _CACHED_CONFIG
