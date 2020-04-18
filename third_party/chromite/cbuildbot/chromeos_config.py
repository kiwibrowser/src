# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Configuration options for various cbuildbot builders."""

from __future__ import print_function

import copy
import os

from chromite.lib.const import waterfall
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import factory


def GetDefaultWaterfall(build_config, is_release_branch):
  if not (build_config['important'] or build_config['master']):
    return None
  if build_config['branch']:
    return None
  b_type = build_config['build_type']

  # Build types that, absent other cues, go on the internal waterfall.
  INTERNAL_TYPES = (constants.PRE_CQ_LAUNCHER_TYPE, constants.TOOLCHAIN_TYPE)

  if config_lib.IsCanaryType(b_type):
    # If this is a canary build, it may fall on different waterfalls:
    # - If we're building for a release branch, it belongs on a release
    #   waterfall.
    # - Otherwise, it belongs on the internal waterfall.
    if is_release_branch:
      return waterfall.WATERFALL_RELEASE
    else:
      return waterfall.WATERFALL_INTERNAL
  elif config_lib.IsCQType(b_type):
    # A Paladin can appear on the public or internal waterfall depending on its
    # 'internal' status.
    return (waterfall.WATERFALL_INTERNAL if build_config['internal'] else
            waterfall.WATERFALL_EXTERNAL)
  elif config_lib.IsPFQType(b_type) or b_type in INTERNAL_TYPES:
    # These builder types belong on the internal waterfall.
    return waterfall.WATERFALL_INTERNAL
  else:
    # No default active waterfall.
    return None


def getInfoVMTest():
  suites = [
      'vmtest-informational4',
      'arc-gts']
  ret = []
  for suite in suites:
    ret.append(config_lib.VMTestConfig(
        constants.VM_SUITE_TEST_TYPE,
        test_suite=suite,
        warn_only=True,
        timeout=12 * 60 * 60))
  return ret


class HWTestList(object):
  """Container for methods to generate HWTest lists."""

  def __init__(self, ge_build_config):
    """Helper class for creating hwtests.

    Args:
      ge_build_config: Dictionary containing the decoded GE configuration file.
    """
    self.is_release_branch = ge_build_config[
        config_lib.CONFIG_TEMPLATE_RELEASE_BRANCH]

  def DefaultList(self, **kwargs):
    """Returns a default list of HWTestConfigs for a build.

    Args:
      *kwargs: overrides for the configs
    """
    installer_kwargs = kwargs.copy()
    # Force au suite to run first.
    installer_kwargs['priority'] = constants.HWTEST_CQ_PRIORITY

    async_kwargs = kwargs.copy()
    async_kwargs['priority'] = constants.HWTEST_POST_BUILD_PRIORITY
    async_kwargs['async'] = True
    async_kwargs['suite_min_duts'] = 1
    async_kwargs['timeout'] = config_lib.HWTestConfig.ASYNC_HW_TEST_TIMEOUT

    if self.is_release_branch:
      bvt_inline_kwargs = async_kwargs
    else:
      bvt_inline_kwargs = kwargs.copy()
      bvt_inline_kwargs['timeout'] = (
          config_lib.HWTestConfig.SHARED_HW_TEST_TIMEOUT)

    # BVT + INSTALLER suite.
    return [config_lib.HWTestConfig(constants.HWTEST_BVT_SUITE,
                                    **bvt_inline_kwargs),
            config_lib.HWTestConfig(constants.HWTEST_ARC_COMMIT_SUITE,
                                    **bvt_inline_kwargs),
            config_lib.HWTestConfig(constants.HWTEST_INSTALLER_SUITE,
                                    blocking=True, **installer_kwargs),
            config_lib.HWTestConfig(constants.HWTEST_COMMIT_SUITE,
                                    **async_kwargs),
            config_lib.HWTestConfig(constants.HWTEST_CANARY_SUITE,
                                    **async_kwargs)]

  def DefaultListCanary(self, **kwargs):
    """Returns a default list of config_lib.HWTestConfig's for a canary build.

    Args:
      *kwargs: overrides for the configs
    """
    # Set minimum_duts default to 4, which means that lab will check the
    # number of available duts to meet the minimum requirement before creating
    # the suite job for canary builds.
    kwargs.setdefault('minimum_duts', 4)
    kwargs.setdefault('file_bugs', True)
    return self.DefaultList(**kwargs)

  def AFDOList(self, **kwargs):
    """Returns a default list of HWTestConfigs for a AFDO build.

    For more details on AFDO:
    - https://gcc.gnu.org/wiki/AutoFDO
    - https://sites.google.com/a/google.com/chromeos-toolchain-team-home2
      -> 11. AFDO PROFILE-GUIDED OPTIMIZATIONS

    Args:
      *kwargs: overrides for the configs
    """
    afdo_dict = dict(pool=constants.HWTEST_SUITES_POOL,
                     timeout=120 * 60, async=True, retry=False,
                     max_retries=None)
    afdo_dict.update(kwargs)
    return [config_lib.HWTestConfig('perf_v2', **afdo_dict)]

  def DefaultListNonCanary(self, **kwargs):
    """Return a default list of HWTestConfigs for a non-canary build.

    Optional arguments may be overridden in `kwargs`, except that
    the `blocking` setting cannot be provided.
    """
    # N.B.  The ordering here is coupled with the column order of
    # entries in _paladin_hwtest_assignments, below.  If you change the
    # ordering here you must also change the ordering there.
    return [
        config_lib.HWTestConfig(
            constants.HWTEST_PROVISION_SUITE,
            blocking=True,
            suite_args={'num_required': 1},
            **kwargs),
        config_lib.HWTestConfig(constants.HWTEST_BVT_SUITE,
                                **kwargs),
        config_lib.HWTestConfig(constants.HWTEST_COMMIT_SUITE,
                                **kwargs),
        config_lib.HWTestConfig(constants.HWTEST_ARC_COMMIT_SUITE,
                                **kwargs)]

  def DefaultListCQ(self, **kwargs):
    """Return a default list of HWTestConfigs for a CQ build.

    Optional arguments may be overridden in `kwargs`, except that
    the `blocking` setting cannot be provided.
    """
    default_dict = dict(pool=constants.HWTEST_PALADIN_POOL,
                        timeout=config_lib.HWTestConfig.PALADIN_HW_TEST_TIMEOUT,
                        file_bugs=False, priority=constants.HWTEST_CQ_PRIORITY,
                        minimum_duts=4)
    # Allows kwargs overrides to default_dict for cq.
    default_dict.update(kwargs)
    return self.DefaultListNonCanary(**default_dict)

  def DefaultListPFQ(self, **kwargs):
    """Return a default list of HWTestConfigs for a PFQ build.

    Optional arguments may be overridden in `kwargs`, except that
    the `blocking` setting cannot be provided.
    """
    default_dict = dict(pool=constants.HWTEST_PFQ_POOL, file_bugs=True,
                        priority=constants.HWTEST_PFQ_PRIORITY,
                        minimum_duts=4)
    # Allows kwargs overrides to default_dict for pfq.
    default_dict.update(kwargs)
    return self.DefaultListNonCanary(**default_dict)

  def DefaultListChromePFQInformational(self, **kwargs):
    """Return a default list of HWTestConfigs for an inform. PFQ build.

    Optional arguments may be overridden in `kwargs`, except that
    the `blocking` setting cannot be provided.
    """
    # The informational PFQ does not honor to retry jobs. This reduces
    # hwtest times and shows flakes clearer.
    default_dict = dict(pool=constants.HWTEST_PFQ_POOL, file_bugs=True,
                        priority=constants.HWTEST_PFQ_PRIORITY,
                        retry=False, max_retries=None, minimum_duts=1)
    # Allows kwargs overrides to default_dict for pfq.
    default_dict.update(kwargs)
    suite_list = self.DefaultListNonCanary(**default_dict)
    suite_list.append(config_lib.HWTestConfig(
        constants.HWTEST_CHROME_INFORMATIONAL, **default_dict))
    return suite_list

  def SharedPoolPFQ(self, **kwargs):
    """Return a list of HWTestConfigs for PFQ which uses a shared pool.

    The returned suites will run in pool:critical by default, which is
    shared with other types of builders (canaries, cq). The first suite in the
    list is a blocking sanity suite that verifies the build will not break dut.
    """
    sanity_dict = dict(pool=constants.HWTEST_MACH_POOL,
                       file_bugs=True, priority=constants.HWTEST_PFQ_PRIORITY)
    sanity_dict.update(kwargs)
    sanity_dict.update(dict(minimum_duts=1, suite_min_duts=1,
                            blocking=True))
    default_dict = dict(pool=constants.HWTEST_MACH_POOL,
                        suite_min_duts=3)
    default_dict.update(kwargs)
    suite_list = [config_lib.HWTestConfig(constants.HWTEST_SANITY_SUITE,
                                          **sanity_dict)]
    suite_list.extend(self.DefaultListPFQ(**default_dict))
    return suite_list

  def DefaultListAndroidPFQ(self, **kwargs):
    """Return a default list of HWTestConfig's for a PFQ build.

    Optional arguments may be overridden in `kwargs`, except that
    the `blocking` setting cannot be provided.
    """
    default_dict = dict(file_bugs=True,
                        timeout=config_lib.HWTestConfig.ASYNC_HW_TEST_TIMEOUT,
                        priority=constants.HWTEST_PFQ_PRIORITY, minimum_duts=3)
    # Allows kwargs overrides to default_dict for pfq.
    default_dict.update(kwargs)

    # TODO(crbug.com/610807): Disable the HWTests for now, since we are having
    # issues getting them to run and complete in time.
    # return [config_lib.HWTestConfig(constants.HWTEST_COMMIT_SUITE,
    #                                pool=constants.HWTEST_MACH_POOL,
    #                                **default_dict),
    return [config_lib.HWTestConfig(constants.HWTEST_ARC_COMMIT_SUITE,
                                    pool=constants.HWTEST_MACH_POOL,
                                    **default_dict)]

  def SharedPoolAndroidPFQ(self, **kwargs):
    """Return a list of HWTestConfigs for PFQ which uses a shared pool.

    The returned suites will run in pool:critical by default, which is
    shared with other types of builders (canaries, cq). The first suite in the
    list is a blocking sanity suite that verifies the build will not break dut.
    """
    sanity_dict = dict(pool=constants.HWTEST_MACH_POOL,
                       file_bugs=True, priority=constants.HWTEST_PFQ_PRIORITY)
    sanity_dict.update(kwargs)
    sanity_dict.update(dict(minimum_duts=1, suite_min_duts=1,
                            blocking=True))
    default_dict = dict(suite_min_duts=3)
    default_dict.update(kwargs)
    suite_list = [config_lib.HWTestConfig(constants.HWTEST_SANITY_SUITE,
                                          **sanity_dict)]
    suite_list.extend(self.DefaultListAndroidPFQ(**default_dict))
    return suite_list

  def SharedPoolCQ(self, **kwargs):
    """Return a list of HWTestConfigs for CQ which uses a shared pool.

    The returned suites will run in pool:critical by default, which is
    shared with other types of builder (canaries, pfq). The first suite in the
    list is a blocking sanity suite that verifies the build will not break dut.
    """
    sanity_dict = dict(pool=constants.HWTEST_MACH_POOL,
                       timeout=config_lib.HWTestConfig.SHARED_HW_TEST_TIMEOUT,
                       file_bugs=False, priority=constants.HWTEST_CQ_PRIORITY)
    sanity_dict.update(kwargs)
    sanity_dict.update(dict(minimum_duts=1, suite_min_duts=1,
                            blocking=True))
    default_dict = dict(pool=constants.HWTEST_MACH_POOL,
                        suite_min_duts=10)
    default_dict.update(kwargs)
    suite_list = [config_lib.HWTestConfig(constants.HWTEST_SANITY_SUITE,
                                          **sanity_dict)]
    suite_list.extend(self.DefaultListCQ(**default_dict))
    return suite_list

  def SharedPoolCanary(self, **kwargs):
    """Return a list of HWTestConfigs for Canary which uses a shared pool.

    The returned suites will run in pool:critical by default, which is
    shared with CQs. The first suite in the list is a blocking sanity suite
    that verifies the build will not break dut.
    """
    sanity_dict = dict(pool=constants.HWTEST_MACH_POOL, file_bugs=True)
    sanity_dict.update(kwargs)
    sanity_dict.update(dict(minimum_duts=1, suite_min_duts=1,
                            blocking=True))
    default_dict = dict(pool=constants.HWTEST_MACH_POOL,
                        suite_min_duts=6)
    default_dict.update(kwargs)
    suite_list = [config_lib.HWTestConfig(constants.HWTEST_SANITY_SUITE,
                                          **sanity_dict)]
    suite_list.extend(self.DefaultListCanary(**default_dict))
    return suite_list

  def AFDORecordTest(self, **kwargs):
    default_dict = dict(pool=constants.HWTEST_MACH_POOL,
                        warn_only=True, file_bugs=True,
                        timeout=constants.AFDO_GENERATE_TIMEOUT,
                        priority=constants.HWTEST_PFQ_PRIORITY)
    # Allows kwargs overrides to default_dict for cq.
    default_dict.update(kwargs)
    return config_lib.HWTestConfig(constants.HWTEST_AFDO_SUITE, **default_dict)

  def WiFiCellPoolPreCQ(self, **kwargs):
    """Return a list of HWTestConfigs which run wifi tests.

    This should be used by the ChromeOS WiFi team to ensure changes pass the
    wifi tests as a pre-cq sanity check.
    """
    default_dict = dict(pool=constants.HWTEST_WIFICELL_PRE_CQ_POOL,
                        file_bugs=False,
                        priority=constants.HWTEST_DEFAULT_PRIORITY,
                        retry=False, max_retries=None, minimum_duts=1)
    default_dict.update(kwargs)
    suite_list = [config_lib.HWTestConfig(constants.WIFICELL_PRE_CQ,
                                          **default_dict)]
    return suite_list

  def BluestreakPoolPreCQ(self, **kwargs):
    """Return a list of HWTestConfigs which run bluestreak tests.

    This should be used by the ChromeOS MRHW team to ensure changes pass the
    CFM tests as a pre-cq sanity check.
    """
    default_dict = dict(pool=constants.HWTEST_BLUESTREAK_PRE_CQ_POOL,
                        file_bugs=False,
                        priority=constants.HWTEST_DEFAULT_PRIORITY,
                        retry=False, max_retries=None, minimum_duts=1)
    default_dict.update(kwargs)
    suite_list = [config_lib.HWTestConfig(constants.BLUESTREAK_PRE_CQ,
                                          **default_dict)]
    return suite_list

  def AsanTest(self, **kwargs):
    """Return a list of HWTESTConfigs which run asan tests."""
    default_dict = dict(pool=constants.HWTEST_MACH_POOL, file_bugs=False,
                        priority=constants.HWTEST_DEFAULT_PRIORITY)
    default_dict.update(kwargs)
    return [config_lib.HWTestConfig(constants.HWTEST_BVT_SUITE,
                                    **default_dict),
            config_lib.HWTestConfig(constants.HWTEST_COMMIT_SUITE,
                                    **default_dict)]

  def ToolchainTestFull(self, machine_pool, **kwargs):
    """Return full set of HWTESTConfigs to run toolchain correctness tests."""
    default_dict = dict(pool=machine_pool, async=False,
                        file_bugs=False,
                        priority=constants.HWTEST_DEFAULT_PRIORITY)
    default_dict.update(kwargs)
    return [config_lib.HWTestConfig(constants.HWTEST_BVT_SUITE,
                                    **default_dict),
            config_lib.HWTestConfig(constants.HWTEST_COMMIT_SUITE,
                                    **default_dict),
            config_lib.HWTestConfig('security',
                                    **default_dict),
            config_lib.HWTestConfig('kernel_daily_regression',
                                    **default_dict),
            config_lib.HWTestConfig('kernel_daily_benchmarks',
                                    **default_dict)]

  def ToolchainTestMedium(self, machine_pool, **kwargs):
    """Return list of HWTESTConfigs to run toolchain LLVM correctness tests.

    Since the kernel is not built with LLVM, it makes no sense for the
    toolchain to run kernel tests on LLVM builds.
    """
    default_dict = dict(pool=machine_pool, async=False,
                        file_bugs=False,
                        priority=constants.HWTEST_DEFAULT_PRIORITY)
    default_dict.update(kwargs)
    return [config_lib.HWTestConfig(constants.HWTEST_BVT_SUITE,
                                    **default_dict),
            config_lib.HWTestConfig(constants.HWTEST_COMMIT_SUITE,
                                    **default_dict),
            config_lib.HWTestConfig('security',
                                    **default_dict)]

  # pylint: disable=unused-argument
  def CtsGtsQualTests(self, **kwargs):
    """Return a list of HWTestConfigs for CTS, GTS tests."""
    cts_config = dict(
        pool=constants.HWTEST_CTS_POOL,
        timeout=config_lib.HWTestConfig.CTS_QUAL_HW_TEST_TIMEOUT,
        priority=constants.HWTEST_CTS_PRIORITY,
        async=True)
    cts_config.update(kwargs)

    gts_config = dict(
        pool=constants.HWTEST_GTS_POOL,
        timeout=config_lib.HWTestConfig.GTS_QUAL_HW_TEST_TIMEOUT,
        priority=constants.HWTEST_GTS_PRIORITY,
        async=True)
    gts_config.update(kwargs)

    return [config_lib.HWTestConfig(constants.HWTEST_CTS_FOLLOWER_SUITE,
                                    **cts_config),
            config_lib.HWTestConfig(constants.HWTEST_CTS_QUAL_SUITE,
                                    **cts_config),
            config_lib.HWTestConfig(constants.HWTEST_GTS_QUAL_SUITE,
                                    **gts_config)]



def append_useflags(useflags):
  """Used to append a set of useflags to existing useflags.

  Useflags that shadow prior use flags will cause the prior flag to be removed.
  (e.g. appending '-foo' to 'foo' will cause 'foo' to be removed)

  Usage:
    new_config = base_config.derive(useflags=append_useflags(['foo', '-bar'])

  Args:
    useflags: List of string useflags to append.
  """
  assert isinstance(useflags, (list, set))
  shadowed_useflags = {'-' + flag for flag in useflags
                       if not flag.startswith('-')}
  shadowed_useflags.update({flag[1:] for flag in useflags
                            if flag.startswith('-')})
  def handler(old_useflags):
    new_useflags = set(old_useflags or [])
    new_useflags.update(useflags)
    new_useflags.difference_update(shadowed_useflags)
    return sorted(list(new_useflags))

  return handler


def remove_images(unsupported_images):
  """Remove unsupported images when applying changes to a BuildConfig.

  Used similarly to append_useflags.

  Args:
    unsupported_images: A list of image names that should not be present
                        in the final build config.

  Returns:
    A callable suitable for use with BuildConfig.apply.
  """
  unsupported = set(unsupported_images)

  def handler(old_images):
    if not old_images:
      old_images = []
    return [i for i in old_images if i not in unsupported]

  return handler


TRADITIONAL_VM_TESTS_SUPPORTED = [
    config_lib.VMTestConfig(constants.VM_SUITE_TEST_TYPE,
                            test_suite='smoke'),
    config_lib.VMTestConfig(constants.SIMPLE_AU_TEST_TYPE),
    config_lib.VMTestConfig(constants.CROS_VM_TEST_TYPE)]

#
# Define assorted constants describing various sets of boards.
#

# Base per-board configuration.
# Every board must appear in exactly 1 of the following sets.

_arm_internal_release_boards = frozenset([
    'arkham',
    'beaglebone',
    'beaglebone_servo',
    'bob',
    'capri',
    'capri-zfpga',
    'cheza',
    'cobblepot',
    'daisy',
    'daisy_skate',
    'daisy_spring',
    'elm',
    'gale',
    'gonzo',
    'gru',
    'hana',
    'kevin',
    'kevin-arcnext',
    'lasilla-ground',
    'lasilla-sky',
    'macchiato-ground',
    'nyan',
    'nyan_big',
    'nyan_blaze',
    'nyan_kitty',
    'oak',
    'octavius',
    'peach_pi',
    'peach_pit',
    'rainier',
    'romer',
    'rowan',
    'scarlet',
    'smaug',
    'veyron_fievel',
    'veyron_jaq',
    'veyron_jerry',
    'veyron_mickey',
    'veyron_mighty',
    'veyron_minnie',
    'veyron_pinky',
    'veyron_rialto',
    'veyron_speedy',
    'veyron_tiger',
    'whirlwind',
    'wooten',
])

_arm_external_boards = frozenset([
    'arm-generic',
    'arm64-generic',
    'arm64-llvmpipe',
    'tael',
])

_x86_internal_release_boards = frozenset([
    'amd64-generic-cheets',
    'amd64-generic-goofy',
    'asuka',
    'atlas',
    'auron',
    'auron_paine',
    'auron_yuna',
    'banjo',
    'banon',
    'betty',
    'betty-arc64',
    'betty-arcnext',
    'buddy',
    'candy',
    'caroline',
    'caroline-arcnext',
    'cave',
    'celes',
    'chell',
    'clapper',
    'coral',
    'cyan',
    'edgar',
    'enguarde',
    'eve',
    'eve-arcnext',
    'eve-campfire',
    'expresso',
    'falco',
    'falco_li',
    'fizz',
    'fizz-accelerator',
    'fizz-moblab',
    'gandof',
    'glados',
    'glimmer',
    'gnawty',
    'grunt',
    'guado',
    'guado-accelerator',
    'guado_labstation',
    'guado_moblab',
    'heli',
    'jadeite',
    'jecht',
    'kahlee',
    'kefka',
    'kip',
    'kunimitsu',
    'lakitu',
    'lakitu-gpu',
    'lakitu-nc',
    'lakitu-st',
    'lakitu_next',
    'lars',
    'leon',
    'link',
    'lulu',
    'mccloud',
    'meowth',
    'monroe',
    'nami',
    'nautilus',
    'newbie',
    'ninja',
    'nocturne',
    'novato',
    'novato-arc64',
    'octopus',
    'orco',
    'panther',
    'parrot_ivb',
    'peppy',
    'plaso',
    'poppy',
    'pyro',
    'quawks',
    'rambi',
    'reef',
    'reks',
    'relm',
    'rikku',
    'samus',
    'sand',
    'sentry',
    'setzer',
    'slippy',
    'snappy',
    'soraka',
    'squawks',
    'stout',
    'sumo',
    'swanky',
    'terra',
    'tidus',
    'tricky',
    'ultima',
    'umaro',
    'winky',
    'wizpig',
    'wolf',
    'x86-alex',
    'x86-alex_he',
    'x86-mario',
    'x86-zgb',
    'x86-zgb_he',
    'zako',
    'zoombini',
])

_x86_external_boards = frozenset([
    'amd64-generic',
    'moblab-generic-vm',
    'tatl',
    'x32-generic',
    'x86-generic',
])

# Board can appear in 1 or more of the following sets.
_brillo_boards = frozenset([
    'arkham',
    'gale',
    'whirlwind',
])

_accelerator_boards = frozenset([
    'fizz-accelerator',
    'guado-accelerator',
])

_beaglebone_boards = frozenset([
    'beaglebone',
    'beaglebone_servo',
])

_lakitu_boards = frozenset([
    'lakitu',
    'lakitu-gpu',
    'lakitu-nc',
    'lakitu-st',
    'lakitu_next',
])

_lassen_boards = frozenset([
    'lassen',
])

_loonix_boards = frozenset([
    'capri',
    'capri-zfpga',
    'cobblepot',
    'gonzo',
    'lasilla-ground',
    'lasilla-sky',
    'macchiato-ground',
    'octavius',
    'romer',
    'wooten',
])

_moblab_boards = frozenset([
    'fizz-moblab',
    'guado_moblab',
    'moblab-generic-vm',
])

_scribe_boards = frozenset([
    'guado-macrophage',
])

_termina_boards = frozenset([
    'tatl',
    'tael',
])

_nofactory_boards = (
    _lakitu_boards | _termina_boards | _lassen_boards | frozenset([
        'smaug',
        'x30evb',
    ])
)

_toolchains_from_source = frozenset([
    'x32-generic',
])

_noimagetest_boards = (_lakitu_boards | _loonix_boards | _termina_boards
                       | _scribe_boards)

_nohwqual_boards = (_lakitu_boards | _lassen_boards | _loonix_boards
                    | _termina_boards | _beaglebone_boards)

_norootfs_verification_boards = frozenset([
])

_base_layout_boards = _lakitu_boards | _termina_boards

_no_unittest_boards = frozenset((
))

_vmtest_boards = frozenset([
    # Full VMTest support on ChromeOS is currently limited to devices derived
    # from betty & co.
    'amd64-generic', # Has kernel 4.4, used with public Chromium.
    'betty',         # amd64 Chrome OS VM board with 32 bit arm/x86 ARC++ ABI.
    'betty-arc64',   # Chrome OS VM board with 64 bit x86_64 ARC++ ABI.
    'betty-arcnext', # Like betty but with the next version of ARC++.
    'newbie',        # AOSP variant of betty.
    'novato',        # Like betty but with GMSCore but not the Play Store
    'novato-arc64',  # 64 bit x86_64 ARC++ ABI
]) | _lakitu_boards  # All lakitu boards have VM support.

# This is a list of configs that should be included on the main waterfall, but
# aren't included by default (see IsDefaultMainWaterfall). This loosely
# corresponds to the set of experimental or self-standing configs.
_waterfall_config_map = {
    waterfall.WATERFALL_EXTERNAL: frozenset([
        # Full
        'amd64-generic-full',
        'arm-generic-full',
        'daisy-full',
        'kevin-full',
        'lakitu-full',
        'oak-full',
        'tael-full',
        'tatl-full',

        # ASAN.
        'amd64-generic-asan',
        'amd64-generic-fuzzer',
    ]),

    waterfall.WATERFALL_INTERNAL: frozenset([
        # Firmware Builders.
        'link-depthcharge-full-firmware',
        'betty-asan',
    ]),
}


def GetBoardTypeToBoardsDict(ge_build_config):
  """Get board type to board names dict.

  Args:
    ge_build_config: Dictionary containing the decoded GE configuration file.

  Returns:
    A dict mapping board types to board name collections.
    The dict contains board types including distinct_board_sets,
    all_release_boards, all_full_boards, all_boards, internal_boards,
    and no_vmtest_boards.
  """
  ge_arch_board_dict = config_lib.GetArchBoardDict(ge_build_config)

  boards_dict = {}

  arm_internal_release_boards = (
      _arm_internal_release_boards |
      ge_arch_board_dict.get(config_lib.CONFIG_ARM_INTERNAL, set())
  )
  arm_external_boards = (
      _arm_external_boards |
      ge_arch_board_dict.get(config_lib.CONFIG_ARM_EXTERNAL, set())
  )

  x86_internal_release_boards = (
      _x86_internal_release_boards |
      ge_arch_board_dict.get(config_lib.CONFIG_X86_INTERNAL, set())
  )
  x86_external_boards = (
      _x86_external_boards |
      ge_arch_board_dict.get(config_lib.CONFIG_X86_EXTERNAL, set())
  )

  # Every board should be in only 1 of the above sets.
  boards_dict['distinct_board_sets'] = [
      arm_internal_release_boards,
      arm_external_boards,
      x86_internal_release_boards,
      x86_external_boards,
  ]

  arm_full_boards = (
      arm_internal_release_boards |
      arm_external_boards)
  x86_full_boards = (
      x86_internal_release_boards |
      x86_external_boards)

  arm_boards = arm_full_boards
  x86_boards = x86_full_boards

  boards_dict['all_release_boards'] = (
      _arm_internal_release_boards |
      _x86_internal_release_boards
  )
  boards_dict['all_full_boards'] = (
      arm_full_boards |
      x86_full_boards
  )
  all_boards = x86_boards | arm_boards
  boards_dict['all_boards'] = (
      all_boards
  )

  boards_dict['internal_boards'] = boards_dict['all_release_boards']

  # This set controls the final vmtest override. It allows us to specify
  # vm_tests for each class of builders, but only execute on _vmtest_boards.
  boards_dict['no_vmtest_boards'] = (
      all_boards - _vmtest_boards
  )

  return boards_dict


def SiteParameters():
  """Create the site parameters for this site.

  Returns:
    dict containing SiteParameters for ChromeOS.
  """
  # Chrome OS site parameters.
  site_params = config_lib.DefaultSiteParameters()

  # Helpers for constructing Chrome OS site parameters.
  manifest_project = 'chromiumos/manifest'
  manifest_int_project = 'chromeos/manifest-internal'
  external_remote = 'cros'
  internal_remote = 'cros-internal'
  chromium_remote = 'chromium'
  chrome_remote = 'chrome'
  aosp_remote = 'aosp'
  weave_remote = 'weave'

  # Gerrit instance site parameters.
  site_params.update(
      config_lib.GerritInstanceParameters('EXTERNAL', 'chromium'))
  site_params.update(
      config_lib.GerritInstanceParameters('INTERNAL', 'chrome-internal'))
  site_params.update(
      config_lib.GerritInstanceParameters('AOSP', 'android'))
  site_params.update(
      config_lib.GerritInstanceParameters('WEAVE', 'weave'))

  site_params.update(
      # Parameters to define which manifests to use.
      MANIFEST_PROJECT=manifest_project,
      MANIFEST_INT_PROJECT=manifest_int_project,
      MANIFEST_PROJECTS=(manifest_project, manifest_int_project),
      MANIFEST_URL='%s/%s' % (
          site_params['EXTERNAL_GOB_URL'], manifest_project
      ),
      MANIFEST_INT_URL='%s/%s' % (
          site_params['INTERNAL_GERRIT_URL'], manifest_int_project
      ),

      # CrOS remotes specified in the manifests.
      EXTERNAL_REMOTE=external_remote,
      INTERNAL_REMOTE=internal_remote,
      GOB_REMOTES={
          site_params['EXTERNAL_GOB_INSTANCE']: external_remote,
          site_params['INTERNAL_GOB_INSTANCE']: internal_remote
      },
      CHROMIUM_REMOTE=chromium_remote,
      CHROME_REMOTE=chrome_remote,
      AOSP_REMOTE=aosp_remote,
      WEAVE_REMOTE=weave_remote,

      # Only remotes listed in CROS_REMOTES are considered branchable.
      # CROS_REMOTES and BRANCHABLE_PROJECTS must be kept in sync.
      GERRIT_HOSTS={
          external_remote: site_params['EXTERNAL_GERRIT_HOST'],
          internal_remote: site_params['INTERNAL_GERRIT_HOST'],
          aosp_remote: site_params['AOSP_GERRIT_HOST'],
          weave_remote: site_params['WEAVE_GERRIT_HOST']
      },
      CROS_REMOTES={
          external_remote: site_params['EXTERNAL_GOB_URL'],
          internal_remote: site_params['INTERNAL_GOB_URL'],
          aosp_remote: site_params['AOSP_GOB_URL'],
          weave_remote: site_params['WEAVE_GOB_URL']
      },
      GIT_REMOTES={
          chromium_remote: site_params['EXTERNAL_GOB_URL'],
          chrome_remote: site_params['INTERNAL_GOB_URL'],
          external_remote: site_params['EXTERNAL_GOB_URL'],
          internal_remote: site_params['INTERNAL_GOB_URL'],
          aosp_remote: site_params['AOSP_GOB_URL'],
          weave_remote: site_params['WEAVE_GOB_URL']
      },

      # Prefix to distinguish internal and external changes. This is used
      # when a user specifies a patch with "-g", when generating a key for
      # a patch to use in our PatchCache, and when displaying a custom
      # string for the patch.
      CHANGE_PREFIX={
          external_remote: site_params['EXTERNAL_CHANGE_PREFIX'],
          internal_remote: site_params['INTERNAL_CHANGE_PREFIX'],
      },

      # List of remotes that are okay to include in the external manifest.
      EXTERNAL_REMOTES=(
          external_remote, chromium_remote, aosp_remote, weave_remote,
      ),

      # Mapping 'remote name' -> regexp that matches names of repositories on
      # that remote that can be branched when creating CrOS branch.
      # Branching script will actually create a new git ref when branching
      # these projects. It won't attempt to create a git ref for other projects
      # that may be mentioned in a manifest. If a remote is missing from this
      # dictionary, all projects on that remote are considered to not be
      # branchable.
      BRANCHABLE_PROJECTS={
          external_remote: r'(chromiumos|aosp)/(.+)',
          internal_remote: r'chromeos/(.+)',
      },

      # Additional parameters used to filter manifests, create modified
      # manifests, and to branch manifests.
      MANIFEST_VERSIONS_GOB_URL=(
          '%s/chromiumos/manifest-versions' % site_params['EXTERNAL_GOB_URL']
      ),
      MANIFEST_VERSIONS_INT_GOB_URL=(
          '%s/chromeos/manifest-versions' % site_params['INTERNAL_GOB_URL']
      ),
      MANIFEST_VERSIONS_GOB_URL_TEST=(
          '%s/chromiumos/manifest-versions-test' % (
              site_params['EXTERNAL_GOB_URL']
          )
      ),
      MANIFEST_VERSIONS_INT_GOB_URL_TEST=(
          '%s/chromeos/manifest-versions-test' % site_params['INTERNAL_GOB_URL']
      ),
      MANIFEST_VERSIONS_GS_URL='gs://chromeos-manifest-versions',

      # Standard directories under buildroot for cloning these repos.
      EXTERNAL_MANIFEST_VERSIONS_PATH='manifest-versions',
      INTERNAL_MANIFEST_VERSIONS_PATH='manifest-versions-internal',

      # URL of the repo project.
      REPO_URL='%s/external/repo' % site_params['EXTERNAL_GOB_URL']
  )

  return site_params


def DefaultSettings(site_params):
  """Create the default build config values for this site.

  Args:
    site_params: A populated config_lib.SiteParameters instance.

  Returns:
    dict: of default config_lib.BuildConfig values to use for this site.
  """
  # Site specific adjustments for default BuildConfig values.
  defaults = config_lib.DefaultSettings()

  # Git repository URL for our manifests.
  #  https://chromium.googlesource.com/chromiumos/manifest
  #  https://chrome-internal.googlesource.com/chromeos/manifest-internal
  defaults['manifest_repo_url'] = site_params['MANIFEST_URL']

  return defaults


def GeneralTemplates(site_config, ge_build_config):
  """Defines templates that are shared between categories of builders.

  Args:
    site_config: A SiteConfig object to add the templates too.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  is_release_branch = ge_build_config[config_lib.CONFIG_TEMPLATE_RELEASE_BRANCH]
  hw_test_list = HWTestList(ge_build_config)

  site_config.AddTemplate(
      'default_hw_tests_override',
      hw_tests_override=hw_test_list.DefaultList(
          pool=constants.HWTEST_TRYBOT_POOL,
          file_bugs=False),
  )

  # Config parameters for builders that do not run tests on the builder.
  site_config.AddTemplate(
      'no_unittest_builder',
      unittests=False,
  )

  # Notice all builders except for _vmtest_boards should not run vmtest.
  site_config.AddTemplate(
      'no_vmtest_builder',
      vm_tests=[],
      vm_tests_override=None,
      gce_tests=[],
  )

  site_config.AddTemplate(
      'no_hwtest_builder',
      hw_tests=[],
      hw_tests_override=[],
  )

  # Builder type templates.

  site_config.AddTemplate(
      'full',
      site_config.templates.default_hw_tests_override,
      # Full builds are test builds to show that we can build from scratch,
      # so use settings to build from scratch, and archive the results.
      usepkg_build_packages=False,
      chrome_sdk=True,
      display_label=config_lib.DISPLAY_LABEL_FULL,
      build_type=constants.BUILD_FROM_SOURCE_TYPE,
      archive_build_debug=True,
      images=['base', 'recovery', 'test', 'factory_install'],
      git_sync=True,
      description='Full Builds',
      image_test=True,
      doc='http://www.chromium.org/chromium-os/build/builder-overview#'
          'TOC-Continuous',
  )

  site_config.AddTemplate(
      'paladin',
      hw_tests_override=hw_test_list.DefaultListNonCanary(
          pool=constants.HWTEST_TRYBOT_POOL,
          file_bugs=False),
      chroot_replace=False,
      important=True,
      display_label=config_lib.DISPLAY_LABEL_CQ,
      build_type=constants.PALADIN_TYPE,
      overlays=constants.PUBLIC_OVERLAYS,
      prebuilts=constants.PUBLIC,
      manifest_version=True,
      description='Commit Queue',
      upload_standalone_images=False,
      images=['base', 'test'],
      image_test=True,
      chrome_sdk=True,
      chrome_sdk_build_chrome=False,
      doc='http://www.chromium.org/chromium-os/build/builder-overview#TOC-CQ',
      # This only applies to vmtest enabled boards like betty and novato.
      vm_tests=[config_lib.VMTestConfig(
          constants.VM_SUITE_TEST_TYPE,
          test_suite='smoke')],
      vm_tests_override=TRADITIONAL_VM_TESTS_SUPPORTED,
  )

  # Incremental builders are intended to test the developer workflow.
  # For that reason, they don't uprev.
  site_config.AddTemplate(
      'incremental',
      site_config.templates.default_hw_tests_override,
      display_label=config_lib.DISPLAY_LABEL_INCREMENATAL,
      build_type=constants.INCREMENTAL_TYPE,
      chroot_replace=False,
      uprev=False,
      overlays=constants.PUBLIC_OVERLAYS,
      description='Incremental Builds',
      doc='http://www.chromium.org/chromium-os/build/builder-overview#'
          'TOC-Continuous',
  )

  site_config.AddTemplate(
      'external',
      internal=False,
      overlays=constants.PUBLIC_OVERLAYS,
      manifest_repo_url=site_config.params['MANIFEST_URL'],
      manifest=constants.DEFAULT_MANIFEST,
  )

  # This builds with more source available.
  site_config.AddTemplate(
      'internal',
      internal=True,
      overlays=constants.BOTH_OVERLAYS,
      manifest_repo_url=site_config.params['MANIFEST_INT_URL'],
  )

  site_config.AddTemplate(
      'accelerator',
      sync_chrome=False,
      chrome_sdk=False,
  )

  site_config.AddTemplate(
      'brillo',
      sync_chrome=False,
      chrome_sdk=False,
      afdo_use=False,
      dev_installer_prebuilts=False,
  )

  site_config.AddTemplate(
      'lakitu',
      site_config.templates.no_hwtest_builder,
      sync_chrome=False,
      chrome_sdk=False,
      afdo_use=False,
      dev_installer_prebuilts=False,
  )

  site_config.AddTemplate(
      'lassen',
      sync_chrome=False,
      chrome_sdk=False,
      image_test=False,
  )

  site_config.AddTemplate(
      'x30evb',
      site_config.templates.no_hwtest_builder,
      sync_chrome=False,
      chrome_sdk=False,
      signer_tests=False,
      paygen=False,
      upload_hw_test_artifacts=False,
      image_test=False,
      images=['base', 'test'],
      packages=['virtual/target-os',
                'virtual/target-os-dev',
                'virtual/target-os-test'],
  )

  site_config.AddTemplate(
      'termina',
      site_config.templates.no_vmtest_builder,
      site_config.templates.no_hwtest_builder,
      sync_chrome=False,
      chrome_sdk=False,
      afdo_use=False,
      dev_installer_prebuilts=False,
      signer_tests=False,
      sign_types=None,
      paygen=False,
      upload_hw_test_artifacts=False,
      upload_stripped_packages=['sys-kernel/*kernel*'],
      image_test=False,
      images=['base', 'test'],
      packages=['virtual/target-os',
                'virtual/target-os-dev',
                'virtual/target-os-test'],
  )

  site_config.AddTemplate(
      'loonix',
      site_config.templates.no_vmtest_builder,
      site_config.templates.no_hwtest_builder,
      sync_chrome=False,
      chrome_sdk=False,
      afdo_use=False,
      dev_installer_prebuilts=False,
      # TODO(harshmodi): Re-enable this when we start using vboot
      signer_tests=False,
      paygen=False,
      upload_hw_test_artifacts=False,
      image_test=False,
      images=remove_images(['recovery', 'factory_install'])
  )

  # An anchor of Laktiu' test customizations.
  # TODO: renable SIMPLE_AU_TEST_TYPE once b/67510964 is fixed.
  site_config.AddTemplate(
      'lakitu_test_customizations',
      vm_tests=[config_lib.VMTestConfig(constants.VM_SUITE_TEST_TYPE,
                                        test_suite='smoke')],
      vm_tests_override=None,
      gce_tests=[config_lib.GCETestConfig(constants.GCE_SUITE_TEST_TYPE,
                                          test_suite='gce-sanity'),
                 config_lib.GCETestConfig(constants.GCE_SUITE_TEST_TYPE,
                                          test_suite='gce-smoke')],
  )

  # No GCE tests for lakitu-nc; Enable 'hsm' profile by default.
  site_config.AddTemplate(
      'lakitu_nc_customizations',
      profile='hsm',
      vm_tests=[config_lib.VMTestConfig(constants.VM_SUITE_TEST_TYPE,
                                        test_suite='smoke')],
      vm_tests_override=None,
  )

  # Test customizations for lakitu boards' paladin builders.
  site_config.AddTemplate(
      'lakitu_paladin_test_customizations',
      vm_tests=[config_lib.VMTestConfig(constants.VM_SUITE_TEST_TYPE,
                                        test_suite='smoke')],
      vm_tests_override=None,
      gce_tests=[config_lib.GCETestConfig(constants.GCE_SUITE_TEST_TYPE,
                                          test_suite='gce-sanity')],
  )

  # An anchor of Laktiu' notification email settings.
  site_config.AddTemplate(
      'lakitu_notification_emails',
      # Send an email on build failures.
      health_threshold=1,
      health_alert_recipients=['gci-alerts+buildbots@google.com'],
  )

  site_config.AddTemplate(
      'moblab',
      site_config.templates.no_vmtest_builder,
      image_test=False,
  )

  site_config.AddTemplate(
      'beaglebone',
      site_config.templates.brillo,
      image_test=False,
      rootfs_verification=False,
      paygen=False,
      signer_tests=False,
      images=remove_images(['dev', 'test', 'recovery', 'factory_install']),
  )

  # This adds Chrome branding.
  site_config.AddTemplate(
      'official_chrome',
      useflags=append_useflags([constants.USE_CHROME_INTERNAL]),
  )

  # This sets chromeos_official.
  site_config.AddTemplate(
      'official',
      site_config.templates.official_chrome,
      chromeos_official=True,
  )

  site_config.AddTemplate(
      'asan',
      site_config.templates.default_hw_tests_override,
      profile='asan',
      disk_layout='2gb-rootfs',
      # TODO(deymo): ASan builders generate bigger files, in particular a bigger
      # Chrome binary, that update_engine can't handle in delta payloads due to
      # memory limits. Remove the following lines once crbug.com/329248 is
      # fixed.
      vm_tests=[config_lib.VMTestConfig(constants.VM_SUITE_TEST_TYPE,
                                        test_suite='smoke')],
      vm_tests_override=None,
      doc='http://www.chromium.org/chromium-os/build/builder-overview#'
          'TOC-ASAN',
  )

  site_config.AddTemplate(
      'fuzzer',
      site_config.templates.default_hw_tests_override,
      site_config.templates.full,
      site_config.templates.no_hwtest_builder,
      display_label=config_lib.DISPLAY_LABEL_INFORMATIONAL,
      profile='fuzzer',
      chrome_sdk=False,
      # Run fuzzer builder specific stages.
      builder_class_name='fuzzer_builders.FuzzerBuilder',
      # Need larger rootfs since fuzzing also enables asan.
      disk_layout='2gb-rootfs',
      gs_path='gs://chromeos-fuzzing-artifacts/libfuzzer-asan',
      images=['base'],
      packages=['virtual/target-fuzzers'],
  )

  site_config.AddTemplate(
      'telemetry',
      site_config.templates.default_hw_tests_override,
      display_label=config_lib.DISPLAY_LABEL_INFORMATIONAL,
      build_type=constants.INCREMENTAL_TYPE,
      uprev=False,
      overlays=constants.PUBLIC_OVERLAYS,
      vm_tests=[config_lib.VMTestConfig(constants.VM_SUITE_TEST_TYPE,
                                        test_suite='telemetry_unit_server',
                                        # Add an extra 60 minutes.
                                        timeout=120 * 60)],
      description='Telemetry Builds',
  )

  site_config.AddTemplate(
      'external_chromium_pfq',
      site_config.templates.default_hw_tests_override,
      build_type=constants.CHROME_PFQ_TYPE,
      important=True,
      uprev=False,
      # Increase the master timeout to 6 hours crbug.com/611139.
      build_timeout=6 * 60 * 60,
      overlays=constants.PUBLIC_OVERLAYS,
      manifest_version=True,
      chrome_rev=constants.CHROME_REV_LATEST,
      chrome_sdk=True,
      unittests=False,
      description='Preflight Chromium Uprev & Build (public)',
      # Add betty smoke VMTest crbug.com/710629.
      vm_tests=[config_lib.VMTestConfig(constants.SIMPLE_AU_TEST_TYPE)],
      vm_tests_override=None,
  )

  # TODO(davidjames): Convert this to an external config once the unified master
  # logic is ready.
  site_config.AddTemplate(
      'chromium_pfq',
      site_config.templates.internal,
      site_config.templates.external_chromium_pfq,
      display_label=config_lib.DISPLAY_LABEL_CHROME_PFQ,
      description='Preflight Chromium Uprev & Build (internal)',
      overlays=constants.BOTH_OVERLAYS,
      prebuilts=constants.PUBLIC,
      doc='http://www.chromium.org/chromium-os/build/builder-overview#'
          'TOC-Chrome-PFQ',
  )

  site_config.AddTemplate(
      'chrome_pfq',
      site_config.templates.chromium_pfq,
      site_config.templates.official,
      important=True,
      overlays=constants.BOTH_OVERLAYS,
      description='Preflight Chrome Uprev & Build (internal)',
      prebuilts=constants.PRIVATE,
  )

  site_config.AddTemplate(
      'chrome_try',
      build_type=constants.CHROME_PFQ_TYPE,
      chrome_rev=constants.CHROME_REV_TOT,
      important=False,
      manifest_version=False,
  )

  site_config.AddTemplate(
      'chromium_pfq_informational',
      site_config.templates.external_chromium_pfq,
      site_config.templates.chrome_try,
      display_label=config_lib.DISPLAY_LABEL_INFORMATIONAL,
      chrome_sdk=False,
      unittests=False,
      description='Informational Chromium Uprev & Build (public)',
  )

  site_config.AddTemplate(
      'chrome_pfq_informational',
      site_config.templates.chromium_pfq_informational,
      site_config.templates.internal,
      site_config.templates.official,
      unittests=False,
      description='Informational Chrome Uprev & Build (internal)',
  )

  site_config.AddTemplate(
      'chrome_pfq_cheets_informational',
      site_config.templates.chrome_pfq_informational,
      hw_tests=hw_test_list.SharedPoolAndroidPFQ(),
      hw_tests_override=hw_test_list.SharedPoolAndroidPFQ()
  )

  site_config.AddTemplate(
      'tot_asan_informational',
      site_config.templates.chromium_pfq_informational,
      site_config.templates.asan,
      display_label=config_lib.DISPLAY_LABEL_INFORMATIONAL,
      unittests=True,
      description='Build TOT Chrome with Address Sanitizer (Clang)',
  )

  site_config.AddTemplate(
      'chrome_perf',
      site_config.templates.chrome_pfq_informational,
      site_config.templates.no_unittest_builder,
      site_config.templates.no_vmtest_builder,
      description='Chrome Performance test bot',
      hw_tests=[config_lib.HWTestConfig(
          'perf_v2', pool=constants.HWTEST_CHROME_PERF_POOL,
          timeout=90 * 60, critical=True)],
      use_chrome_lkgm=True,
      useflags=append_useflags(['-cros-debug']),
  )

  site_config.AddTemplate(
      'pre_flight_branch',
      site_config.templates.internal,
      site_config.templates.official_chrome,
      build_type=constants.PFQ_TYPE,
      build_timeout=20 * 60,
      important=True,
      manifest_version=True,
      branch=True,
      master=True,
      slave_configs=[],
      vm_tests=[],
      vm_tests_override=TRADITIONAL_VM_TESTS_SUPPORTED,
      hw_tests=[],
      hw_tests_override=[],
      uprev=True,
      overlays=constants.BOTH_OVERLAYS,
      push_overlays=constants.BOTH_OVERLAYS,
      doc='http://www.chromium.org/chromium-os/build/builder-overview#'
          'TOC-Chrome-PFQ',
      active_waterfall=waterfall.WATERFALL_RELEASE)

  site_config.AddTemplate(
      'internal_paladin',
      site_config.templates.paladin,
      site_config.templates.internal,
      site_config.templates.official_chrome,
      manifest=constants.OFFICIAL_MANIFEST,
      overlays=constants.BOTH_OVERLAYS,
      prebuilts=constants.PRIVATE,
      vm_tests=[],
      description=site_config.templates.paladin.description + ' (internal)',
  )

  # Used for paladin builders with nowithdebug flag (a.k.a -cros-debug)
  site_config.AddTemplate(
      'internal_nowithdebug_paladin',
      site_config.templates.internal_paladin,
      useflags=append_useflags(['-cros-debug']),
      description=(site_config.templates.paladin.description +
                   ' (internal, nowithdebug)'),
      prebuilts=False,
  )

  # Internal incremental builders don't use official chrome because we want
  # to test the developer workflow.
  site_config.AddTemplate(
      'internal_incremental',
      site_config.templates.internal,
      site_config.templates.incremental,
      overlays=constants.BOTH_OVERLAYS,
      description='Incremental Builds (internal)',
  )

  # A test-ap image is just a test image with a special profile enabled.
  # Note that each board enabled for test-ap use has to have the testbed-ap
  # profile linked to from its private overlay.
  site_config.AddTemplate(
      'test_ap',
      site_config.templates.internal,
      site_config.templates.no_vmtest_builder,
      site_config.templates.default_hw_tests_override,
      display_label=config_lib.DISPLAY_LABEL_UTILITY,
      build_type=constants.INCREMENTAL_TYPE,
      description='WiFi AP images used in testing',
      profile='testbed-ap',
  )

  # Create tryjob build configs to help with stress testing.
  site_config.AddTemplate(
      'unittest_stress',
      site_config.templates.no_vmtest_builder,
      site_config.templates.no_hwtest_builder,
      display_label=config_lib.DISPLAY_LABEL_TRYJOB,
      build_type=constants.TRYJOB_TYPE,
      description='Run Unittests repeatedly to look for flake.',

      builder_class_name='test_builders.UnittestStressBuilder',
      active_waterfall=waterfall.WATERFALL_SWARMING,

      # Make this available, so we can stress a previous build.
      manifest_version=True,
  )

  site_config.AddTemplate(
      'release',
      site_config.templates.full,
      site_config.templates.official,
      site_config.templates.internal,
      site_config.templates.default_hw_tests_override,
      display_label=config_lib.DISPLAY_LABEL_RELEASE,
      build_type=constants.CANARY_TYPE,
      chroot_use_image=False,
      suite_scheduling=True,
      build_timeout=12 * 60 * 60 if is_release_branch else (7 * 60 + 50) * 60,
      useflags=append_useflags(['-cros-debug']),
      afdo_use=True,
      manifest=constants.OFFICIAL_MANIFEST,
      manifest_version=True,
      images=['base', 'recovery', 'test', 'factory_install'],
      sign_types=['recovery'],
      push_image=True,
      upload_symbols=True,
      binhost_bucket='gs://chromeos-dev-installer',
      binhost_key='RELEASE_BINHOST',
      binhost_base_url='https://commondatastorage.googleapis.com/'
                       'chromeos-dev-installer',
      dev_installer_prebuilts=True,
      git_sync=False,
      vm_tests=[
          config_lib.VMTestConfig(constants.VM_SUITE_TEST_TYPE,
                                  test_suite='smoke'),
          config_lib.VMTestConfig(constants.DEV_MODE_TEST_TYPE),
          config_lib.VMTestConfig(constants.CROS_VM_TEST_TYPE)],
      # Some release builders disable VMTests to be able to build on GCE, but
      # still want VMTests enabled on trybot builders.
      vm_tests_override=[
          config_lib.VMTestConfig(constants.VM_SUITE_TEST_TYPE,
                                  test_suite='smoke'),
          config_lib.VMTestConfig(constants.DEV_MODE_TEST_TYPE),
          config_lib.VMTestConfig(constants.CROS_VM_TEST_TYPE)],
      hw_tests=(hw_test_list.CtsGtsQualTests() +
                hw_test_list.SharedPoolCanary()),
      paygen=True,
      signer_tests=True,
      hwqual=True,
      description="Release Builds (canary) (internal)",
      chrome_sdk=True,
      image_test=True,
      doc='http://www.chromium.org/chromium-os/build/builder-overview#'
          'TOC-Canaries',
  )

  ### Release AFDO configs.

  site_config.AddTemplate(
      'release_afdo',
      site_config.templates.release,
      suite_scheduling=False,
      hw_tests=(
          hw_test_list.DefaultList(pool=constants.HWTEST_SUITES_POOL) +
          hw_test_list.AFDOList()
      ),
      push_image=False,
      paygen=False,
      dev_installer_prebuilts=False,
  )

  site_config.AddTemplate(
      'release_afdo_generate',
      site_config.templates.release_afdo,
      afdo_generate_min=True,
      afdo_use=False,
      afdo_update_ebuild=True,

      hw_tests=[hw_test_list.AFDORecordTest()],
      hw_tests_override=[hw_test_list.AFDORecordTest(
          pool=constants.HWTEST_TRYBOT_POOL,
          file_bugs=False,
          priority=constants.HWTEST_DEFAULT_PRIORITY,
      )],
  )

  site_config.AddTemplate(
      'release_afdo_use',
      site_config.templates.release_afdo,
      afdo_use=True,
  )

  site_config.AddTemplate(
      'moblab_release',
      site_config.templates.release,
      description='Moblab release builders',
      images=['base', 'recovery', 'test'],
      important=False,
      afdo_use=False,
      signer_tests=False,
      hw_tests=[
          config_lib.HWTestConfig(constants.HWTEST_MOBLAB_SUITE,
                                  timeout=120*60),
          config_lib.HWTestConfig(constants.HWTEST_BVT_SUITE,
                                  warn_only=True),
          config_lib.HWTestConfig(constants.HWTEST_INSTALLER_SUITE,
                                  warn_only=True)],
  )

  site_config.AddTemplate(
      'cheets_release',
      site_config.templates.release,
      description='Cheets release builders',
      hw_tests=[
          config_lib.HWTestConfig(constants.HWTEST_ARC_COMMIT_SUITE),
          config_lib.HWTestConfig(constants.HWTEST_INSTALLER_SUITE,
                                  warn_only=True)],
  )

  # Factory and Firmware releases much inherit from these classes.
  # Modifications for these release builders should go here.

  # Naming conventions also must be followed. Factory and firmware branches
  # must end in -factory or -firmware suffixes.

  site_config.AddTemplate(
      'factory',
      site_config.templates.release,
      site_config.templates.no_hwtest_builder,
      site_config.templates.no_vmtest_builder,
      display_label=config_lib.DISPLAY_LABEL_FACTORY,
      afdo_use=False,
      chrome_sdk=False,
      chrome_sdk_build_chrome=False,
      description='Factory Builds',
      dev_installer_prebuilts=False,
      factory_toolkit=True,
      hwqual=False,
      images=['test', 'factory_install'],
      image_test=False,
      paygen=False,
      signer_tests=False,
      sign_types=['factory'],
      upload_hw_test_artifacts=False,
      upload_symbols=False,
  )

  site_config.AddTemplate(
      'firmware_base',
      site_config.templates.no_vmtest_builder,
      display_label=config_lib.DISPLAY_LABEL_FIRMWARE,
      images=[],
      hwqual=False,
      factory_toolkit=False,
      packages=['virtual/chromeos-firmware'],
      usepkg_build_packages=True,
      sync_chrome=False,
      chrome_sdk=False,
      unittests=False,
      hw_tests=[],
      hw_tests_override=None,
      dev_installer_prebuilts=False,
      upload_hw_test_artifacts=True,
      upload_symbols=False,
      useflags=append_useflags(['chromeless_tty']),
      signer_tests=False,
      paygen=False,
      image_test=False,
      sign_types=['firmware'],
  )

  site_config.AddTemplate(
      'firmware',
      site_config.templates.release,
      site_config.templates.firmware_base,
      description='Firmware Canary',
      manifest=constants.DEFAULT_MANIFEST,
      afdo_use=False,
  )

  site_config.AddTemplate(
      'depthcharge_firmware',
      site_config.templates.firmware,
      useflags=append_useflags(['depthcharge']))

  site_config.AddTemplate(
      'depthcharge_full_firmware',
      site_config.templates.full,
      site_config.templates.internal,
      site_config.templates.firmware_base,
      useflags=append_useflags(['depthcharge']),
      description='Firmware Informational',
  )

  site_config.AddTemplate(
      'payloads',
      site_config.templates.internal,
      site_config.templates.no_vmtest_builder,
      site_config.templates.no_unittest_builder,
      site_config.templates.no_hwtest_builder,
      display_label=config_lib.DISPLAY_LABEL_TRYJOB,
      build_type=constants.PAYLOADS_TYPE,
      builder_class_name='release_builders.GeneratePayloadsBuilder',
      description='Regenerate release payloads.',
      # Sync to the code used to do the build the first time.
      manifest_version=True,
      # This is the actual work we want to do.
      paygen=True,
      upload_hw_test_artifacts=False,
      active_waterfall=waterfall.WATERFALL_SWARMING,
  )

  site_config.AddTemplate(
      'build_external_chrome',
      useflags=append_useflags(['-%s' % constants.USE_CHROME_INTERNAL]),
  )

  # Tast is an experimental system for running integration tests.
  site_config.AddTemplate(
      'tast_vm_canary_tests',
      tast_vm_tests=[
          config_lib.TastVMTestConfig('tast_vm_canary', ['(bvt || canary)'])],
  )

  site_config.AddTemplate(
      'moblab_vm_tests',
      moblab_vm_tests=[
          config_lib.MoblabVMTestConfig(constants.MOBLAB_VM_SMOKE_TEST_TYPE)],
  )


def CreateBoardConfigs(site_config, boards_dict, ge_build_config):
  """Create mixin templates for each board."""
  # Extract the full list of board names from GE data.
  separate_board_names = set(config_lib.GeBuildConfigAllBoards(ge_build_config))
  unified_builds = config_lib.GetUnifiedBuildConfigAllBuilds(ge_build_config)
  unified_board_names = set([b[config_lib.CONFIG_TEMPLATE_REFERENCE_BOARD_NAME]
                             for b in unified_builds])
  board_names = separate_board_names | unified_board_names

  # TODO(crbug.com/648473): Remove these, after GE adds them to their data set.
  board_names = board_names.union(boards_dict['all_boards'])

  result = dict()
  for board in board_names:
    board_config = config_lib.BuildConfig(boards=[board])

    if board in _brillo_boards:
      board_config.apply(site_config.templates.brillo)
    if board in _lakitu_boards:
      board_config.apply(site_config.templates.lakitu)
    if board in _lassen_boards:
      board_config.apply(site_config.templates.lassen)
    if board in ['x30evb']:
      board_config.apply(site_config.templates.x30evb)
    if board in _loonix_boards:
      board_config.apply(site_config.templates.loonix)
    if board in _moblab_boards:
      board_config.apply(site_config.templates.moblab)
    if board in _accelerator_boards:
      board_config.apply(site_config.templates.accelerator)
    if board in _termina_boards:
      board_config.apply(site_config.templates.termina)
    if board in _nofactory_boards:
      board_config.apply(factory=False,
                         factory_toolkit=False,
                         factory_install_netboot=False,
                         images=remove_images(['factory_install']))
    if board in _toolchains_from_source:
      board_config.apply(usepkg_toolchain=False)
    if board in _noimagetest_boards:
      board_config.apply(image_test=False)
    if board in _nohwqual_boards:
      board_config.apply(hwqual=False)
    if board in _norootfs_verification_boards:
      board_config.apply(rootfs_verification=False)
    if board in _base_layout_boards:
      board_config.apply(disk_layout='base')
    if board in _no_unittest_boards:
      board_config.apply(site_config.templates.no_unittest_builder)
    if board in _beaglebone_boards:
      board_config.apply(site_config.templates.beaglebone)
    if board == 'moblab-generic-vm':
      board_config.apply(site_config.templates.moblab_vm_tests)

    result[board] = board_config

  return result


def CreateInternalBoardConfigs(site_config, boards_dict, ge_build_config):
  """Create mixin templates for each board."""
  result = CreateBoardConfigs(site_config, boards_dict, ge_build_config)

  for board in boards_dict['internal_boards']:
    if board in result:
      result[board].apply(site_config.templates.internal,
                          site_config.templates.official_chrome,
                          manifest=constants.OFFICIAL_MANIFEST)

  return result


def UpdateBoardConfigs(board_configs, boards, *args, **kwargs):
  """Update "board_configs" for selected boards.

  Args:
    board_configs: Dict in CreateBoardConfigs format to filter from.
    boards: Iterable of boards to update in the dict.
    args: List of templates to apply.
    kwargs: Individual keys to update.

  Returns:
    Copy of board_configs dict with boards boards update with templates
    and values applied.
  """
  result = board_configs.copy()
  for b in boards:
    result[b] = result[b].derive(*args, **kwargs)

  return result


def ToolchainBuilders(site_config, boards_dict, ge_build_config):
  """Define templates used for toolchain builders.

  Args:
    site_config: config_lib.SiteConfig to be modified by adding templates
                 and configs.
    boards_dict: A dict mapping board types to board name collections.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  board_configs = CreateInternalBoardConfigs(
      site_config, boards_dict, ge_build_config)
  hw_test_list = HWTestList(ge_build_config)

  site_config.AddTemplate(
      'toolchain',
      # Full build, AFDO, latest-toolchain, -cros-debug, and simple-chrome.
      site_config.templates.full,
      site_config.templates.internal,
      site_config.templates.official_chrome,
      site_config.templates.no_vmtest_builder,
      display_label=config_lib.DISPLAY_LABEL_TOOLCHAIN,
      build_type=constants.TOOLCHAIN_TYPE,
      buildslave_type=constants.GCE_BEEFY_BUILD_SLAVE_TYPE,
      images=['base', 'test', 'recovery'],
      build_timeout=(15 * 60 + 50) * 60,
      # Need to re-enable platform_SyncCrash after issue crosbug/658864 is
      # fixed. Need to re-enable network_VPNConnect.* tests after issue
      # crosbug/585936 is fixed. Need to re-enable
      # power_DarkResumeShutdownServer after issue crosbug/689598 is fixed.
      # According to crosbug/653496 security_OpenFDs will not work for
      # non-official builds, so we need to leave it permanently disabled.
      # Need to reenable power_DarkResumeDisplay after crosbug/703250 is fixed.
      # Need to reenable cheets_SELinuxTest after crosbug/693308 is fixed.
      # Need to reenable security_SMMLocked when crosbug/654610 is fixed.
      useflags=append_useflags(['-cros-debug',
                                '-tests_security_OpenFDs',
                                '-tests_platform_SyncCrash',
                                '-tests_network_VPNConnect.l2tpipsec_xauth',
                                '-tests_network_VPNConnect.l2tpipsec_psk',
                                '-tests_power_DarkResumeShutdownServer',
                                '-tests_power_DarkResumeDisplay',
                                '-tests_security_SMMLocked',
                                '-tests_cheets_SELinuxTest',
                                'thinlto']),
      afdo_use=True,
      latest_toolchain=True,
      manifest=constants.OFFICIAL_MANIFEST,
      manifest_version=True,
      git_sync=False,
      description="Toolchain Builds (internal)",
  )
  site_config.AddTemplate(
      'gcc_toolchain',
      site_config.templates.toolchain,
      description='Full release build with next minor GCC toolchain revision',
      useflags=append_useflags(['next_gcc']),
      hw_tests=hw_test_list.ToolchainTestFull(constants.HWTEST_SUITES_POOL),
      hw_tests_override=hw_test_list.ToolchainTestFull(
          constants.HWTEST_SUITES_POOL),
  )
  site_config.AddTemplate(
      'llvm_toolchain',
      site_config.templates.toolchain,
      description='Full release build with LLVM toolchain',
      hw_tests=hw_test_list.ToolchainTestMedium(constants.HWTEST_SUITES_POOL),
      hw_tests_override=hw_test_list.ToolchainTestMedium(
          constants.HWTEST_SUITES_POOL),
  )
  site_config.AddTemplate(
      'llvm_next_toolchain',
      site_config.templates.llvm_toolchain,
      description='Full release build with LLVM (next) toolchain',
      useflags=append_useflags(['llvm-next']),
  )

  ### Toolchain waterfall entries.
  ### Toolchain builder configs: 3 architectures {amd64,arm,arm64}
  ###                          x 1 toolchains {llvm-next}
  ### All of these builders should be slaves of 'master-toolchain'.

  ### Master toolchain config.
  master = site_config.Add(
      'master-toolchain',
      site_config.templates.toolchain,
      boards=[],
      description='Toolchain master (all others are slaves).',
      master=True,
      sync_chrome=True,
      health_alert_recipients=['c-compiler-chrome@google.com'],
      health_threshold=1,
      afdo_use=False,
      important=True,
      active_waterfall=waterfall.WATERFALL_INTERNAL,
      buildslave_type=constants.GCE_BEEFY_BUILD_SLAVE_TYPE,
      slave_configs=[],
  )

  def toolchainSlaveHelper(name, board, *args, **kwargs):
    master.AddSlaves([
        site_config.Add(
            name + '-llvm-next-toolchain',
            site_config.templates.llvm_next_toolchain,
            *args,
            boards=[board],
            important=True,
            active_waterfall=waterfall.WATERFALL_INTERNAL,
            hw_tests=hw_test_list.ToolchainTestMedium(
                constants.HWTEST_MACH_POOL),
            hw_tests_override=hw_test_list.ToolchainTestMedium(
                constants.HWTEST_MACH_POOL),
            **kwargs
        )
    ])

  # Create all waterfall slave builders.
  toolchainSlaveHelper('amd64', 'samus')
  toolchainSlaveHelper('arm', 'veyron_jaq')
  toolchainSlaveHelper('arm64', 'elm')

  #
  # Create toolchain tryjob builders.
  #
  builder_to_boards_dict = config_lib.GroupBoardsByBuilder(
      ge_build_config[config_lib.CONFIG_TEMPLATE_BOARDS])

  toolchain_tryjob_boards = builder_to_boards_dict[
      config_lib.CONFIG_TEMPLATE_RELEASE] | boards_dict['all_boards']

  site_config.AddForBoards(
      'llvm-toolchain',
      toolchain_tryjob_boards,
      board_configs,
      site_config.templates.llvm_toolchain,
  )
  site_config.AddForBoards(
      'llvm-next-toolchain',
      toolchain_tryjob_boards,
      board_configs,
      site_config.templates.llvm_next_toolchain,
  )
  site_config.Add(
      'llvm-clang-tidy-toolchain',
      site_config.templates.toolchain,
      site_config.templates.no_hwtest_builder,
      description='Full release build with LLVM toolchain, with clang-tidy)',
      chrome_sdk=False,
      # Run clang-tidy specific stages.
      builder_class_name='clang_tidy_builders.ClangTidyBuilder',
      useflags=append_useflags(['clang_tidy']),
      boards=['grunt'],
  )


def PreCqBuilders(site_config, boards_dict, ge_build_config):
  """Create all build configs associated with the PreCQ.

  Args:
    site_config: config_lib.SiteConfig to be modified by adding templates
                 and configs.
    boards_dict: A dict mapping board types to board name collections.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  board_configs = CreateInternalBoardConfigs(
      site_config, boards_dict, ge_build_config)
  hw_test_list = HWTestList(ge_build_config)

  site_config.AddTemplate(
      'pre_cq',
      site_config.templates.paladin,
      display_label=config_lib.DISPLAY_LABEL_PRECQ,
      luci_builder=config_lib.LUCI_BUILDER_PRECQ,
      active_waterfall=waterfall.WATERFALL_SWARMING,
      build_type=constants.PRE_CQ_TYPE,
      pre_cq=True,
      archive=False,
      chrome_sdk=False,
      chroot_replace=True,
      debug_symbols=False,
      prebuilts=False,
      cpe_export=False,
      vm_tests=[config_lib.VMTestConfig(constants.VM_SUITE_TEST_TYPE,
                                        test_suite='smoke')],
      vm_tests_override=None,
      description='Verifies compilation, building an image, and vm/unit tests '
                  'if supported.',
      doc='http://www.chromium.org/chromium-os/build/builder-overview#'
          'TOC-Pre-CQ',
      sanity_check_threshold=3,
  )

  # Pre-CQ targets that only check compilation and unit tests.
  site_config.AddTemplate(
      'unittest_only_pre_cq',
      site_config.templates.pre_cq,
      site_config.templates.no_vmtest_builder,
      description='Verifies compilation and unit tests only',
      compilecheck=True,
  )

  # Pre-CQ targets that don't run VMTests.
  site_config.AddTemplate(
      'no_vmtest_pre_cq',
      site_config.templates.pre_cq,
      site_config.templates.no_vmtest_builder,
      description='Verifies compilation, building an image, and unit tests '
                  'if supported.',
  )

  # Pre-CQ targets that only check compilation.
  site_config.AddTemplate(
      'compile_only_pre_cq',
      site_config.templates.unittest_only_pre_cq,
      description='Verifies compilation only',
      unittests=False,
  )

  site_config.AddWithoutTemplate(
      'pre-cq-launcher',
      site_config.templates.paladin,
      site_config.templates.internal_paladin,
      site_config.templates.no_vmtest_builder,
      site_config.templates.no_hwtest_builder,
      boards=[],
      display_label=config_lib.DISPLAY_LABEL_UTILITY,
      build_type=constants.PRE_CQ_LAUNCHER_TYPE,
      active_waterfall=waterfall.WATERFALL_SWARMING,
      description='Launcher for Pre-CQ builders',
      manifest_version=False,
      doc='http://www.chromium.org/chromium-os/build/builder-overview#'
          'TOC-Pre-CQ',
  )


  # Add a pre-cq config for every board.
  site_config.AddForBoards(
      'pre-cq',
      boards_dict['all_boards'],
      board_configs,
      site_config.templates.pre_cq,
  )
  site_config.AddForBoards(
      'no-vmtest-pre-cq',
      boards_dict['all_boards'],
      board_configs,
      site_config.templates.no_vmtest_pre_cq,
  )
  site_config.AddForBoards(
      'compile-only-pre-cq',
      boards_dict['all_boards'],
      board_configs,
      site_config.templates.compile_only_pre_cq,
  )
  site_config.Add(
      constants.BINHOST_PRE_CQ,
      site_config.templates.pre_cq,
      site_config.templates.no_vmtest_pre_cq,
      site_config.templates.internal,
      boards=[],
      binhost_test=True,
  )

  # Wifi specific PreCQ.
  site_config.AddTemplate(
      'wificell_pre_cq',
      site_config.templates.pre_cq,
      unittests=False,
      hw_tests=hw_test_list.WiFiCellPoolPreCQ(),
      hw_tests_override=hw_test_list.WiFiCellPoolPreCQ(),
      archive=True,
      image_test=False,
      description='WiFi tests acting as pre-cq for WiFi related changes',
  )

  site_config.AddGroup(
      'mixed-wificell-pre-cq',
      site_config.Add(
          'winky-wificell-pre-cq',
          site_config.templates.wificell_pre_cq,
          board_configs['winky']),
      site_config.Add(
          'veyron_speedy-wificell-pre-cq',
          site_config.templates.wificell_pre_cq,
          board_configs['veyron_speedy']),
      site_config.Add(
          'veyron_jerry-wificell-pre-cq',
          site_config.templates.wificell_pre_cq,
          board_configs['veyron_jerry']),
      site_config.Add(
          'daisy-wificell-pre-cq',
          site_config.templates.wificell_pre_cq,
          board_configs['daisy']),
      site_config.Add(
          'lulu-wificell-pre-cq',
          site_config.templates.wificell_pre_cq,
          board_configs['lulu']),
      site_config.Add(
          'cyan-wificell-pre-cq',
          site_config.templates.wificell_pre_cq,
          board_configs['cyan']),
      site_config.Add(
          'elm-wificell-pre-cq',
          site_config.templates.wificell_pre_cq,
          board_configs['elm']),
  )

  # Bluestreak specific PreCQ.
  site_config.Add(
      'bluestreak-pre-cq',
      board_configs['guado'],
      site_config.templates.pre_cq,
      hw_tests=hw_test_list.BluestreakPoolPreCQ(),
      hw_tests_override=hw_test_list.BluestreakPoolPreCQ(),
      archive=True,
      description='Bluestreak tests as pre-cq for CFM related changes',
  )

  site_config.Add(
      'signer-pre-cq',
      site_config.templates.pre_cq,
      site_config.templates.internal,
      site_config.templates.no_hwtest_builder,
      site_config.templates.no_vmtest_builder,
      boards=[],
      builder_class_name='test_builders.SignerTestsBuilder',
      description='Run the signer unittests.',
  )

  site_config.Add(
      'chromite-pre-cq',
      site_config.templates.pre_cq,
      site_config.templates.internal,
      site_config.templates.no_hwtest_builder,
      site_config.templates.no_vmtest_builder,
      boards=[],
      builder_class_name='test_builders.ChromiteTestsBuilder',
      description='Run the chromite network unittests.',
  )

  # Pre-cq for lakitu's public overlay.
  site_config.Add(
      'lakitu-external-pre-cq',
      site_config.templates.pre_cq,
      board_configs['lakitu'],
      site_config.templates.lakitu,
      site_config.templates.external,
      useflags=append_useflags(['-chrome_internal']),
  )

  site_config.AddWithoutTemplate(
      'chromeos-infra-puppet-pre-cq',
      site_config.templates.pre_cq,
      site_config.templates.internal,
      site_config.templates.no_hwtest_builder,
      site_config.templates.no_unittest_builder,
      site_config.templates.no_vmtest_builder,
      boards=[],
      builder_class_name='infra_builders.PuppetPreCqBuilder',
      use_sdk=True,
      build_timeout=60 * 60,
      description='Test Puppet specs',
      doc='https://chrome-internal.googlesource.com/'
          'chromeos/chromeos-admin/+/HEAD/puppet/README.md',
  )

  site_config.AddWithoutTemplate(
      'chromeos-infra-unittests-pre-cq',
      site_config.templates.pre_cq,
      site_config.templates.internal,
      site_config.templates.no_hwtest_builder,
      site_config.templates.no_unittest_builder,
      site_config.templates.no_vmtest_builder,
      boards=[],
      builder_class_name='infra_builders.InfraUnittestsPreCqBuilder',
      use_sdk=True,
      build_timeout=60 * 60,
      description='Run unittests for infra repositories',
  )


def AndroidPfqBuilders(site_config, boards_dict, ge_build_config):
  """Create all build configs associated with the Android PFQ.

  Args:
    site_config: config_lib.SiteConfig to be modified by adding templates
                 and configs.
    boards_dict: A dict mapping board types to board name collections.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  board_configs = CreateInternalBoardConfigs(
      site_config, boards_dict, ge_build_config)
  hw_test_list = HWTestList(ge_build_config)

  # Generic template shared by all Android versions.
  site_config.AddTemplate(
      'generic_android_pfq',
      site_config.templates.no_vmtest_builder,
      site_config.templates.default_hw_tests_override,
      build_type=constants.ANDROID_PFQ_TYPE,
      important=True,
      uprev=False,
      overlays=constants.BOTH_OVERLAYS,
      manifest_version=True,
      android_rev=constants.ANDROID_REV_LATEST,
      description='Preflight Android Uprev & Build (internal)',
  )

  # Template for Android NYC.
  site_config.AddTemplate(
      'nyc_android_pfq',
      site_config.templates.generic_android_pfq,
      display_label=config_lib.DISPLAY_LABEL_NYC_ANDROID_PFQ,
      android_package='android-container-nyc',
      android_import_branch=constants.ANDROID_NYC_BUILD_BRANCH,
      android_gts_build_branch='git_nyc-mr2-dev',
  )

  # Template for Android Master.
  site_config.AddTemplate(
      'pi_android_pfq',
      site_config.templates.generic_android_pfq,
      site_config.templates.internal,
      display_label=config_lib.DISPLAY_LABEL_PI_ANDROID_PFQ,
      android_package='android-container-pi',
      android_import_branch=constants.ANDROID_PI_BUILD_BRANCH,
  )

  # Template for Android Master.
  site_config.AddTemplate(
      'mst_android_pfq',
      site_config.templates.generic_android_pfq,
      site_config.templates.internal,
      display_label=config_lib.DISPLAY_LABEL_MST_ANDROID_PFQ,
      android_package='android-container-master-arc-dev',
      android_import_branch=constants.ANDROID_MST_BUILD_BRANCH,
  )

  # Mixin for masters.
  site_config.AddTemplate(
      'master_android_pfq_mixin',
      site_config.templates.internal,
      site_config.templates.no_vmtest_builder,
      boards=[],
      master=True,
      push_overlays=constants.BOTH_OVERLAYS,
  )

  # Android PFQ masters.
  # Any additions of Android PFQ masters should be reflected by a
  # change in lib/constants.py SOM_BUILDS to add Sheriff-o-Matic coverage.

  # Android MST master.
  mst_master_config = site_config.Add(
      constants.MST_ANDROID_PFQ_MASTER,
      site_config.templates.mst_android_pfq,
      site_config.templates.master_android_pfq_mixin,
  )

  _mst_hwtest_boards = frozenset([])
  _mst_no_hwtest_boards = frozenset([])
  _mst_no_hwtest_experimental_boards = frozenset([])
  _mst_no_hwtest_experimental_boards = frozenset([])
  _mst_vmtest_boards = frozenset([])

  # Android PI master.
  pi_master_config = site_config.Add(
      constants.PI_ANDROID_PFQ_MASTER,
      site_config.templates.pi_android_pfq,
      site_config.templates.master_android_pfq_mixin,
      buildslave_type=constants.GCE_BEEFY_BUILD_SLAVE_TYPE,
  )

  _pi_no_hwtest_boards = frozenset([])
  _pi_no_hwtest_experimental_boards = frozenset([
      'eve-arcnext',
  ])
  _pi_hwtest_boards = frozenset([
      'caroline-arcnext',
      'kevin-arcnext',
  ])
  _pi_hwtest_experimental_boards = frozenset([])
  _pi_vmtest_boards = frozenset([])
  _pi_vmtest_experimental_boards = frozenset([
      'betty-arcnext',
  ])

  # Android NYC master.
  nyc_master_config = site_config.Add(
      constants.NYC_ANDROID_PFQ_MASTER,
      site_config.templates.nyc_android_pfq,
      site_config.templates.master_android_pfq_mixin,
  )

  _nyc_hwtest_boards = frozenset([
      'cyan',
      'samus',
      'veyron_jaq',
      'veyron_minnie',
  ])
  _nyc_no_hwtest_boards = frozenset([
      'bob',
      'caroline',
      'coral',
      'hana',
      'reef',
  ])
  _nyc_no_hwtest_experimental_boards = frozenset([])
  _nyc_vmtest_boards = frozenset([
      'betty',
      'betty-arc64',
  ])

  # Android MST slaves.
  mst_master_config.AddSlaves(
      site_config.AddForBoards(
          'mst-android-pfq',
          _mst_hwtest_boards,
          board_configs,
          site_config.templates.mst_android_pfq,
          hw_tests=hw_test_list.SharedPoolAndroidPFQ(),
      ) +
      site_config.AddForBoards(
          'mst-android-pfq',
          _mst_no_hwtest_boards,
          board_configs,
          site_config.templates.mst_android_pfq,
      ) +
      site_config.AddForBoards(
          'mst-android-pfq',
          _mst_no_hwtest_experimental_boards,
          board_configs,
          site_config.templates.mst_android_pfq,
          important=False,
          active_waterfall=waterfall.WATERFALL_INTERNAL,
      ) +
      site_config.AddForBoards(
          'mst-android-pfq',
          _mst_vmtest_boards,
          board_configs,
          site_config.templates.mst_android_pfq,
          vm_tests=[config_lib.VMTestConfig(constants.VM_SUITE_TEST_TYPE,
                                            test_suite='smoke'),
                    config_lib.VMTestConfig(constants.SIMPLE_AU_TEST_TYPE)],
      )
  )

  # Android PI slaves.
  pi_master_config.AddSlaves(
      site_config.AddForBoards(
          'pi-android-pfq',
          _pi_hwtest_boards,
          board_configs,
          site_config.templates.pi_android_pfq,
          hw_tests=hw_test_list.SharedPoolAndroidPFQ(),
      ) +
      site_config.AddForBoards(
          'pi-android-pfq',
          _pi_no_hwtest_boards,
          board_configs,
          site_config.templates.pi_android_pfq,
      ) +
      site_config.AddForBoards(
          'pi-android-pfq',
          _pi_no_hwtest_experimental_boards,
          board_configs,
          site_config.templates.pi_android_pfq,
          important=False,
          active_waterfall=waterfall.WATERFALL_INTERNAL,
      ) +
      site_config.AddForBoards(
          'pi-android-pfq',
          _pi_hwtest_experimental_boards,
          board_configs,
          site_config.templates.pi_android_pfq,
          important=False,
          active_waterfall=waterfall.WATERFALL_INTERNAL,
          hw_tests=hw_test_list.SharedPoolAndroidPFQ(),
      ) +
      site_config.AddForBoards(
          'pi-android-pfq',
          _pi_vmtest_boards,
          board_configs,
          site_config.templates.pi_android_pfq,
          active_waterfall=waterfall.WATERFALL_INTERNAL,
          vm_tests=[config_lib.VMTestConfig(constants.VM_SUITE_TEST_TYPE,
                                            test_suite='smoke'),
                    config_lib.VMTestConfig(constants.SIMPLE_AU_TEST_TYPE)],
      ) +
      site_config.AddForBoards(
          'pi-android-pfq',
          _pi_vmtest_experimental_boards,
          board_configs,
          site_config.templates.pi_android_pfq,
          important=False,
          active_waterfall=waterfall.WATERFALL_INTERNAL,
          vm_tests=[config_lib.VMTestConfig(constants.VM_SUITE_TEST_TYPE,
                                            test_suite='smoke'),
                    config_lib.VMTestConfig(constants.SIMPLE_AU_TEST_TYPE)],
      )
  )



  # Android NYC slaves.
  nyc_master_config.AddSlaves(
      site_config.AddForBoards(
          'nyc-android-pfq',
          _nyc_hwtest_boards,
          board_configs,
          site_config.templates.nyc_android_pfq,
          hw_tests=hw_test_list.SharedPoolAndroidPFQ(),
      ) +
      site_config.AddForBoards(
          'nyc-android-pfq',
          _nyc_no_hwtest_boards,
          board_configs,
          site_config.templates.nyc_android_pfq,
      ) +
      site_config.AddForBoards(
          'nyc-android-pfq',
          _nyc_no_hwtest_experimental_boards,
          board_configs,
          site_config.templates.nyc_android_pfq,
          important=False,
          active_waterfall=waterfall.WATERFALL_INTERNAL,
      ) +
      site_config.AddForBoards(
          'nyc-android-pfq',
          _nyc_vmtest_boards,
          board_configs,
          site_config.templates.nyc_android_pfq,
          vm_tests=[config_lib.VMTestConfig(constants.VM_SUITE_TEST_TYPE,
                                            test_suite='smoke'),
                    config_lib.VMTestConfig(constants.SIMPLE_AU_TEST_TYPE)],
      )
  )


def FullBuilders(site_config, boards_dict, ge_build_config):
  """Create all full builders.

  Args:
    site_config: config_lib.SiteConfig to be modified by adding templates
                 and configs.
    boards_dict: A dict mapping board types to board name collections.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  external_board_configs = CreateBoardConfigs(
      site_config, boards_dict, ge_build_config)

  site_config.AddForBoards(
      config_lib.CONFIG_TYPE_FULL,
      boards_dict['all_full_boards'],
      external_board_configs,
      site_config.templates.full,
      site_config.templates.build_external_chrome,
      internal=False,
      manifest_repo_url=site_config.params['MANIFEST_URL'],
      overlays=constants.PUBLIC_OVERLAYS,
      prebuilts=constants.PUBLIC)


def CqBuilders(site_config, boards_dict, ge_build_config):
  """Create all CQ build configs.

  Args:
    site_config: config_lib.SiteConfig to be modified by adding templates
                 and configs.
    boards_dict: A dict mapping board types to board name collections.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  board_configs = CreateInternalBoardConfigs(
      site_config, boards_dict, ge_build_config)
  hw_test_list = HWTestList(ge_build_config)

  _separate_boards = boards_dict['all_boards']
  _unified_builds = config_lib.GetUnifiedBuildConfigAllBuilds(ge_build_config)
  _unified_board_names = set([b[config_lib.CONFIG_TEMPLATE_REFERENCE_BOARD_NAME]
                              for b in _unified_builds])

  _paladin_boards = _separate_boards | _unified_board_names
  # List of paladin boards where the regular paladin config is important.
  _paladin_important_boards = frozenset([
      'amd64-generic',
      'arm-generic',
      'auron_yuna',
      'beaglebone',
      'betty',
      'betty-arc64',
      'bob',
      'caroline',
      'caroline-arcnext',
      'cave',
      'chell',
      'coral',
      'cyan',
      'daisy',
      'daisy_skate',
      'daisy_spring',
      'edgar',
      'elm',
      'eve',
      'falco',
      'fizz',
      'gale',
      'glados',
      'grunt',
      'guado_moblab',
      'hana',
      'kahlee',
      'kevin',
      'kevin-arcnext',
      'kip',
      'lakitu',
      'lakitu-gpu',
      'lakitu-st',
      'leon',
      'link',
      'monroe',
      'moblab-generic-vm',
      'nyan_big',
      'nyan_kitty',
      'oak',
      'panther',
      'peach_pit',
      'peppy',
      'poppy',
      'quawks',
      'rainier',
      'reef',
      'samus',
      'scarlet',
      'sentry',
      'stout',
      'tatl',
      'terra',
      'tidus',
      'tricky',
      'veyron_jaq',
      'veyron_jerry',
      'veyron_mighty',
      'veyron_minnie',
      'veyron_rialto',
      'veyron_speedy',
      'veyron_tiger',
      'winky',
      'wizpig',
      'wolf',
      'zoombini',
  ])

  # Paladin configs that exist and should be important as soon as they are
  # shown to be green. All new paladins should start in this group and get
  # promoted to _paladin_important_boards.
  # A paladin is generally considered good enough for important if it can
  # pass the last ~20 build attempts, e.g. the builder page shows mostly green.
  # Note that paladins are expected to fail occasionally as they block bad CLs
  # from landing, a red paladin from a bad CL in the CQ is a working paladin.
  _paladin_new_boards = frozenset([
      'auron',
      'auron_paine',
      'betty-arcnext',
      'cheza', #contact: philipchen@
      'eve-campfire', # contact:yueherngl@
      'nami',
      'nocturne', # contact:moragues@
      'octopus',
      'tael',
      'atlas',
  ])

  # Paladin configs that exist and should stay as experimental until further
  # notice, preferably with a comment indicating why and a bug.
  _paladin_experimental_boards = _paladin_new_boards | frozenset([
      'capri', # contact:ghines@
      'capri-zfpga', # contact:victoryang@
      'cobblepot', # contact:jkoleszar@
      'eve-arcnext', # contact: ihf@ (crbug.com/826755)
      'fizz-accelerator', # contact:perley@
      'gonzo', # contact:icoolidge@
      'guado', # contact:egemih@
      'guado-accelerator', # contact:perley@ (crbug.com/748635)
      'lasilla-ground', # contact:jemele@
      'lasilla-sky', # contact:jemele@
      'macchiato-ground', # contact:jemele@
      'octavius', # contact:dpjacques@
      'romer', # contact:michaelho@
      'whirlwind', # contact:mxt@ (crbug.com/794242, crbug.com/819287)
      'wooten', # contact:icoolidge@
      'nyan_blaze', # contact: xixuan@
  ])

  assert not (_paladin_experimental_boards & _paladin_important_boards), (
      'Experimental and important paladin board sets must be disjoint.')

  _paladin_active = _paladin_important_boards | _paladin_experimental_boards

  _paladin_simple_vmtest_boards = frozenset([
      'betty',
  ])

  _paladin_devmode_vmtest_boards = frozenset([
      'betty',
  ])

  _paladin_cros_vmtest_boards = frozenset([
      'betty',
  ])

  _paladin_smoke_vmtest_boards = frozenset([
      'betty',
      'betty-arc64',
      'betty-arcnext',
  ])

  _paladin_default_vmtest_boards = frozenset([
      'betty',
  ])

  # Jetstream devices run unique hw tests
  _paladin_jetstream_hwtest_boards = frozenset([
      'whirlwind',
      'gale',
  ])

  _paladin_moblab_hwtest_boards = frozenset([
      'guado_moblab',
  ])

  _paladin_chroot_replace_boards = frozenset([
      'daisy_spring',
  ])

  _paladin_separate_symbols = frozenset([
      'amd64-generic',
  ])

  _paladin_bluestreak_hwtest_boards = frozenset([
      'guado',
  ])

  ### Master paladin (CQ builder).
  master_config = site_config.Add(
      'master-paladin',
      site_config.templates.paladin,
      site_config.templates.internal_paladin,
      boards=[],
      master=True,
      slave_configs=[],
      binhost_test=True,
      push_overlays=constants.BOTH_OVERLAYS,
      description='Commit Queue master (all others are slaves)',

      # This name should remain synced with with the name used in
      # build_internals/masters/master.chromeos/board_config.py.
      # TODO(mtennant): Fix this.  There should be some amount of auto-
      # configuration in the board_config.py code.
      auto_reboot=True,  # TODO(dgarrett): Disable chroot.img stable.
  )

  ### Other paladins (CQ builders).
  # These are slaves of the master paladin by virtue of matching
  # in a few config values (e.g. 'build_type', 'branch', etc).  If
  # they are not 'important' then they are ignored slaves.
  # TODO(mtennant): This master-slave relationship should be specified
  # here in the configuration, rather than GetSlavesForMaster().
  # Something like the following:
  # master_paladin = site_config.AddConfig(internal_paladin, ...)
  # master_paladin.AddSlave(site_config.AddConfig(internal_paladin, ...))

  for board in _paladin_boards:
    assert board in board_configs, '%s not in board_configs' % board
    config_name = '%s-%s' % (board, constants.PALADIN_TYPE)
    customizations = config_lib.BuildConfig()
    base_config = board_configs[board]
    if board in _unified_board_names:
      for unibuild in _unified_builds:
        if board == unibuild[config_lib.CONFIG_TEMPLATE_REFERENCE_BOARD_NAME]:
          models = []
          for model in unibuild[config_lib.CONFIG_TEMPLATE_MODELS]:
            name = model[config_lib.CONFIG_TEMPLATE_MODEL_NAME]
            lab_board_name = model[config_lib.CONFIG_TEMPLATE_MODEL_BOARD_NAME]
            if (config_lib.CONFIG_TEMPLATE_MODEL_CQ_TEST_ENABLED in model
                and model[config_lib.CONFIG_TEMPLATE_MODEL_CQ_TEST_ENABLED]):
              models.append(config_lib.ModelTestConfig(name, lab_board_name))

          customizations.update(models=models)

    if board in _paladin_moblab_hwtest_boards:
      customizations.update(
          hw_tests=[
              config_lib.HWTestConfig(
                  constants.HWTEST_MOBLAB_QUICK_SUITE,
                  timeout=90*60,
                  pool=constants.HWTEST_PALADIN_POOL)
          ],
          hw_tests_override=None)
    if board in _paladin_jetstream_hwtest_boards:
      customizations.update(
          hw_tests=[
              config_lib.HWTestConfig(
                  constants.HWTEST_JETSTREAM_COMMIT_SUITE,
                  pool=constants.HWTEST_PALADIN_POOL)
          ],
          hw_tests_override=None)
    if board in _paladin_bluestreak_hwtest_boards:
      customizations.update(
          hw_tests=hw_test_list.BluestreakPoolPreCQ(),
          hw_tests_override=hw_test_list.BluestreakPoolPreCQ())
    if board not in _paladin_important_boards:
      customizations.update(important=False)
    if board in _paladin_chroot_replace_boards:
      customizations.update(chroot_replace=True)
    if (board in boards_dict['internal_boards']
        or board in _unified_board_names):
      customizations = customizations.derive(
          site_config.templates.internal,
          site_config.templates.official_chrome,
          manifest=constants.OFFICIAL_MANIFEST)
    if board in _paladin_separate_symbols:
      customizations.update(separate_debug_symbols=True)

    if board not in _paladin_default_vmtest_boards:
      vm_tests = []
      if board in _paladin_simple_vmtest_boards:
        vm_tests.append(
            config_lib.VMTestConfig(constants.SIMPLE_AU_TEST_TYPE))
      if board in _paladin_cros_vmtest_boards:
        vm_tests.append(config_lib.VMTestConfig(constants.CROS_VM_TEST_TYPE))
      if board in _paladin_devmode_vmtest_boards:
        vm_tests.append(config_lib.VMTestConfig(constants.DEV_MODE_TEST_TYPE))
      if board in _paladin_smoke_vmtest_boards:
        vm_tests.append(
            config_lib.VMTestConfig(constants.VM_SUITE_TEST_TYPE,
                                    test_suite='smoke'))

      customizations.update(vm_tests=vm_tests)

      if site_config.templates.paladin.vm_tests_override is not None:
        # Make sure any new tests are also in override.
        override = site_config.templates.paladin.vm_tests_override[:]
        for test in vm_tests:
          if test not in override:
            override.append(test)

        customizations.update(vm_tests_override=override)

    if base_config.get('internal'):
      customizations.update(
          prebuilts=constants.PRIVATE,
          description=(site_config.templates.paladin.description +
                       ' (internal)'))
    else:
      customizations.update(prebuilts=constants.PUBLIC)

    if board in _paladin_active:
      if base_config.get('internal'):
        customizations.update(active_waterfall=waterfall.WATERFALL_INTERNAL)
      else:
        customizations.update(active_waterfall=waterfall.WATERFALL_EXTERNAL)

    if board in _lakitu_boards:
      customizations.update(
          site_config.templates.lakitu_paladin_test_customizations)

    config = site_config.Add(
        config_name,
        site_config.templates.paladin,
        customizations,
        base_config,
    )

    if config.active_waterfall:
      master_config.AddSlave(config)

  # N.B. The ordering of columns here is coupled to the ordering of
  # suites returned by DefaultListNonCanary().  If you change the
  # ordering here, you must also change the ordering there.
  #
  # TODO: Fill in any notable gaps in this table. crbug.com/730076
  # pylint: disable=bad-continuation
  # pylint: disable=bad-whitespace
  _paladin_hwtest_assignments = frozenset([
    # bvt-inline      bvt-cq           bvt-arc           family
    ('link',          None,            None),            # ivybridge
    (None,            None,            None),            # daisy (Exynos5250)
    ('wolf',          'peppy',         None),            # slippy (HSW)
    ('peach_pit',     None,            None),            # peach (Exynos5420)
    ('winky',         'kip',           'quawks'),        # rambi (BYT)
    ('nyan_big',      'nyan_kitty',    None),            # nyan (K1)
    ('auron_paine',   'tidus',         'auron_yuna'),    # auron (BDW)
    ('veyron_mighty', 'veyron_speedy', 'veyron_minnie'), # pinky (RK3288)
    ('wizpig',        'edgar',         'cyan'),          # strago (BSW)
    ('cave',          'sentry',        None),            # glados (SKL)
    ('elm',           None,            'hana'),          # oak (MTK8173)
    ('bob',           None,            'kevin'),         # gru (RK3399)
    ('reef',          None,            None),            # reef (APL)
    ('coral',         None,            None),            # coral (APL)
    (None,            'eve',           'soraka'),        # poppy (KBL)
    (None,            None,            'kevin-arcnext'), # gru + arcnext
    (None,            None,            'caroline-arcnext'), # arcnext
  ])

  sharded_hw_tests = hw_test_list.DefaultListCQ()
  # Run provision suite first everywhere.
  default_tests = [sharded_hw_tests.pop(0)]
  for board_assignments in _paladin_hwtest_assignments:
    assert len(board_assignments) == len(sharded_hw_tests)
    for board, suite in zip(board_assignments, sharded_hw_tests):
      if board is None:
        continue
      config_name = '%s-%s' % (board, constants.PALADIN_TYPE)
      # Only configurate hw_tests for unified builds if they have specified
      # models they want to test against (based on lab provisioning)
      if board in _unified_board_names:
        if site_config[config_name]['models']:
          site_config[config_name]['hw_tests'] = default_tests + [suite]
      else:
        site_config[config_name]['hw_tests'] = default_tests + [suite]

  #
  # Paladins with alternative configs.
  #
  site_config.AddForBoards(
      'nowithdebug-paladin',
      ['x86-generic', 'amd64-generic'],
      board_configs,
      site_config.templates.paladin,
      site_config.templates.internal_nowithdebug_paladin,
      important=False,
  )

  master_config.AddSlaves([
      site_config.Add(
          'chell-nowithdebug-paladin',
          site_config.templates.paladin,
          site_config.templates.internal_nowithdebug_paladin,
          boards=['chell'],
          active_waterfall=waterfall.WATERFALL_INTERNAL,
      ),

      site_config.Add(
          'incremental-paladin',
          site_config.templates.paladin,
          site_config.templates.internal_paladin,
          site_config.templates.no_hwtest_builder,
          boards=['reef'],
          build_before_patching=True,
          prebuilts=False,
          compilecheck=True,
          unittests=False,
          important=False,
          active_waterfall=waterfall.WATERFALL_INTERNAL,
      ),
  ])

  # Used for builders which build completely from source except Chrome.
  # These boards pass with -clang-clean CFLAG, so ensure they stay that way.
  master_config.AddSlaves(
      site_config.AddForBoards(
          'full-compile-paladin',
          ['falco', 'nyan'],
          board_configs,
          site_config.templates.paladin,
          site_config.templates.no_hwtest_builder,
          board_replace=True,
          chrome_binhost_only=True,
          chrome_sdk=False,
          compilecheck=True,
          cpe_export=False,
          debug_symbols=False,
          prebuilts=False,
          unittests=False,
          upload_hw_test_artifacts=False,
          vm_tests=[],
          active_waterfall=waterfall.WATERFALL_INTERNAL,
      )
  )

  # TODO(ihf): remove this as obsolete.
  site_config.Add(
      'x86-generic-asan-paladin',
      site_config.templates.paladin,
      board_configs['x86-generic'],
      site_config.templates.asan,
      description='Paladin build with Address Sanitizer (Clang)',
      important=False,
  )

  site_config.Add(
      'amd64-generic-asan-paladin',
      site_config.templates.paladin,
      site_config.templates.no_hwtest_builder,
      board_configs['amd64-generic'],
      site_config.templates.asan,
      description='Paladin build with Address Sanitizer (Clang)',
      # THESE IMAGES CAN DAMAGE THE LAB and cannot be used for hardware testing.
      disk_layout='4gb-rootfs',
      important=False,
  )

  site_config.Add(
      'betty-asan-paladin',
      site_config.templates.paladin,
      site_config.templates.no_hwtest_builder,
      board_configs['betty'],
      site_config.templates.asan,
      description='Paladin build with Address Sanitizer (Clang)',
      # THESE IMAGES CAN DAMAGE THE LAB and cannot be used for hardware testing.
      disk_layout='4gb-rootfs',
      important=False,
  )


def IncrementalBuilders(site_config, boards_dict, ge_build_config):
  """Create all incremental build configs.

  Args:
    site_config: config_lib.SiteConfig to be modified by adding templates
                 and configs.
    boards_dict: A dict mapping board types to board name collections.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  board_configs = CreateInternalBoardConfigs(
      site_config, boards_dict, ge_build_config)

  site_config.Add(
      'beaglebone-incremental',
      site_config.templates.incremental,
      site_config.templates.beaglebone,
      boards=['beaglebone'],
      description='Incremental Beaglebone Builder',
  )

  # Build external source, for an internal board.
  site_config.Add(
      'daisy-incremental',
      site_config.templates.incremental,
      board_configs['daisy'],
      site_config.templates.external,
      useflags=append_useflags(['-chrome_internal']),
      active_waterfall=waterfall.WATERFALL_EXTERNAL,
  )

  site_config.Add(
      'amd64-generic-incremental',
      site_config.templates.incremental,
      # This builder runs on a VM, so it can't run VM tests.
      site_config.templates.no_vmtest_builder,
      board_configs['amd64-generic'],
      active_waterfall=waterfall.WATERFALL_EXTERNAL,
  )

  # TODO(ihf): Delete this builder.
  site_config.Add(
      'x32-generic-incremental',
      site_config.templates.incremental,
      # This builder runs on a VM, so it can't run VM tests.
      site_config.templates.no_vmtest_builder,
      board_configs['x32-generic'],
  )

  site_config.Add(
      'betty-incremental',
      site_config.templates.incremental,
      site_config.templates.internal_incremental,
      boards=['betty'],
      active_waterfall=waterfall.WATERFALL_INTERNAL,
      vm_tests=getInfoVMTest(),
      vm_tests_override=getInfoVMTest(),
  )

  site_config.Add(
      'chell-incremental',
      site_config.templates.incremental,
      site_config.templates.internal_incremental,
      boards=['chell'],
      active_waterfall=waterfall.WATERFALL_INTERNAL,
  )

  site_config.Add(
      'lakitu-incremental',
      site_config.templates.incremental,
      site_config.templates.internal_incremental,
      site_config.templates.lakitu_notification_emails,
      board_configs['lakitu'],
      site_config.templates.lakitu_test_customizations,
      active_waterfall=waterfall.WATERFALL_INTERNAL,
  )

  site_config.Add(
      'lakitu-gpu-incremental',
      site_config.templates.incremental,
      site_config.templates.internal_incremental,
      site_config.templates.lakitu_notification_emails,
      board_configs['lakitu-gpu'],
      site_config.templates.lakitu_test_customizations,
  )

  site_config.Add(
      'lakitu-st-incremental',
      site_config.templates.incremental,
      site_config.templates.internal_incremental,
      site_config.templates.lakitu_notification_emails,
      board_configs['lakitu-st'],
      site_config.templates.lakitu_test_customizations,
  )

  site_config.Add(
      'lakitu_next-incremental',
      site_config.templates.incremental,
      site_config.templates.internal_incremental,
      site_config.templates.lakitu_notification_emails,
      board_configs['lakitu_next'],
      site_config.templates.lakitu_test_customizations,
  )


def ReleaseAfdoBuilders(site_config, boards_dict, ge_build_config):
  """Create AFDO Performance tryjobs.

  Args:
    site_config: config_lib.SiteConfig to be modified by adding templates
                 and configs.
    boards_dict: A dict mapping board types to board name collections.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  board_configs = CreateInternalBoardConfigs(
      site_config, boards_dict, ge_build_config)

  # Now generate generic release-afdo configs if we haven't created anything
  # more specific above already. release-afdo configs are builders that do AFDO
  # profile collection and optimization in the same builder. Used by developers
  # that want to measure performance changes caused by their changes.
  for board in boards_dict['all_release_boards']:
    base = board_configs[board]

    config_name = '%s-%s' % (board, config_lib.CONFIG_TYPE_RELEASE_AFDO)
    if config_name in site_config:
      continue

    generate_config_name = (
        '%s-%s-%s' % (board,
                      config_lib.CONFIG_TYPE_RELEASE_AFDO,
                      'generate'))
    use_config_name = '%s-%s-%s' % (board,
                                    config_lib.CONFIG_TYPE_RELEASE_AFDO,
                                    'use')

    # We can't use AFDO data if afdo_use is disabled for this board.
    if not base.get('afdo_use', True):
      continue

    site_config.AddGroup(
        config_name,
        site_config.Add(
            generate_config_name,
            site_config.templates.release_afdo_generate,
            base
        ),
        site_config.Add(
            use_config_name,
            site_config.templates.release_afdo_use,
            base
        ),
    )


def InformationalBuilders(site_config, boards_dict, ge_build_config):
  """Create all informational builders.

  We have a number of informational builders that are built, but whose output is
  not directly used for anything other than reporting success or failure.

  Args:
    site_config: config_lib.SiteConfig to be modified by adding templates
                 and configs.
    boards_dict: A dict mapping board types to board name collections.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  external_board_configs = CreateBoardConfigs(
      site_config, boards_dict, ge_build_config)
  internal_board_configs = CreateInternalBoardConfigs(
      site_config, boards_dict, ge_build_config)

  _chrome_boards = frozenset(
      board for board, config in internal_board_configs.iteritems()
      if config.get('sync_chrome', True))


  hw_test_list = HWTestList(ge_build_config)

  _chrome_informational_hwtest_boards = frozenset([
      'caroline',
      'eve',
      'peach_pit',
      'reks',
      'tricky',
      'veyron_minnie',
  ])

  _chrome_informational_swarming_boards = frozenset([
      'eve',
  ])

  # We have to mark all autogenerated PFQs as not important so the master
  # does not wait for them.  http://crbug.com/386214
  # If you want an important PFQ, you'll have to declare it yourself.

  informational_boards = (
      (boards_dict['all_release_boards'] & _chrome_boards))

  _tot_chrome_pfq_informational_board_configs = UpdateBoardConfigs(
      internal_board_configs,
      _chrome_informational_hwtest_boards,
      hw_tests=hw_test_list.DefaultListChromePFQInformational(
          pool=constants.HWTEST_CONTINUOUS_POOL))

  _tot_chrome_pfq_informational_board_configs = UpdateBoardConfigs(
      _tot_chrome_pfq_informational_board_configs,
      _chrome_informational_swarming_boards,
      active_waterfall=waterfall.WATERFALL_SWARMING)

  site_config.AddForBoards(
      'tot-chrome-pfq-informational',
      informational_boards,
      _tot_chrome_pfq_informational_board_configs,
      site_config.templates.chrome_pfq_informational,
      important=False)

  # TODO(ihf): Remove as obsolete.
  site_config.Add(
      'x86-generic-tot-asan-informational',
      site_config.templates.tot_asan_informational,
      boards=['x86-generic'],
  )

  site_config.Add(
      'amd64-generic-asan',
      site_config.templates.asan,
      site_config.templates.incremental,
      site_config.templates.no_hwtest_builder,
      display_label=config_lib.DISPLAY_LABEL_INFORMATIONAL,
      boards=['amd64-generic'],
      description='Build with Address Sanitizer (Clang)',
      # THESE IMAGES CAN DAMAGE THE LAB and cannot be used for hardware testing.
      disk_layout='4gb-rootfs',
  )

  site_config.Add(
      'amd64-generic-tot-asan-informational',
      site_config.templates.tot_asan_informational,
      site_config.templates.no_hwtest_builder,
      # THESE IMAGES CAN DAMAGE THE LAB and cannot be used for hardware testing.
      disk_layout='4gb-rootfs',
      boards=['amd64-generic'],
  )

  site_config.Add(
      'betty-asan',
      site_config.templates.asan,
      site_config.templates.incremental,
      site_config.templates.no_hwtest_builder,
      site_config.templates.internal,
      display_label=config_lib.DISPLAY_LABEL_INFORMATIONAL,
      boards=['betty'],
      description='Build with Address Sanitizer (Clang)',
      # THESE IMAGES CAN DAMAGE THE LAB and cannot be used for hardware testing.
      disk_layout='4gb-rootfs',
  )

  site_config.Add(
      'betty-tot-asan-informational',
      site_config.templates.tot_asan_informational,
      site_config.templates.no_hwtest_builder,
      # THESE IMAGES CAN DAMAGE THE LAB and cannot be used for hardware testing.
      disk_layout='4gb-rootfs',
      boards=['betty'],
  )

  site_config.Add(
      'amd64-generic-fuzzer',
      site_config.templates.fuzzer,
      boards=['amd64-generic'],
      description='Build for fuzzing testing',
      # THESE IMAGES CAN DAMAGE THE LAB and cannot be used for hardware testing.
      disk_layout='4gb-rootfs',
  )

  site_config.Add(
      'amd64-generic-goma-canary-chromium-pfq-informational',
      site_config.templates.chromium_pfq_informational,
      site_config.templates.no_hwtest_builder,
      site_config.templates.no_vmtest_builder,
      important=False,
      description='Test canary versions of goma.',
      boards=[
          'amd64-generic',
      ],
  )

  _chrome_perf_boards = frozenset([
      'daisy',
      'reef',
  ])

  site_config.AddForBoards(
      'chrome-perf',
      _chrome_perf_boards,
      internal_board_configs,
      site_config.templates.chrome_perf,
  )

  site_config.AddForBoards(
      'telem-chromium-pfq-informational',
      ['x86-generic', 'amd64-generic'],
      internal_board_configs,
      site_config.templates.chromium_pfq_informational,
      site_config.templates.telemetry,
      site_config.templates.chrome_try,
  )

  _tot_chromium_pfq_informational_swarming_boards = frozenset([
      'amd64-generic',
      'daisy',
  ])

  _tot_chromium_pfq_informational_board_configs = UpdateBoardConfigs(
      external_board_configs,
      _tot_chromium_pfq_informational_swarming_boards,
      active_waterfall=waterfall.WATERFALL_SWARMING)

  site_config.AddForBoards(
      'tot-chromium-pfq-informational',
      (boards_dict['all_full_boards'] & _chrome_boards),
      _tot_chromium_pfq_informational_board_configs,
      site_config.templates.chromium_pfq_informational,
      site_config.templates.build_external_chrome,
      important=False,
      internal=False,
      manifest_repo_url=site_config.params['MANIFEST_URL'],
      overlays=constants.PUBLIC_OVERLAYS)

  _telemetry_boards = frozenset([
      'amd64-generic',
      'arm-generic',
      'betty',
  ])

  site_config.AddForBoards(
      'telemetry',
      _telemetry_boards,
      internal_board_configs,
      site_config.templates.telemetry,
  )


def ChromePfqBuilders(site_config, boards_dict, ge_build_config):
  """Create all Chrome PFQ build configs.

  Args:
    site_config: config_lib.SiteConfig to be modified by adding templates
                 and configs.
    boards_dict: A dict mapping board types to board name collections.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  external_board_configs = CreateBoardConfigs(
      site_config, boards_dict, ge_build_config)
  internal_board_configs = CreateInternalBoardConfigs(
      site_config, boards_dict, ge_build_config)

  _chrome_boards = frozenset(
      board for board, config in internal_board_configs.iteritems()
      if config.get('sync_chrome', True))

  _chromium_pfq_important_boards = frozenset([
      'arm-generic',
      'daisy',
      'veyron_jerry',
      'amd64-generic',
  ])

  master_config = site_config.Add(
      'master-chromium-pfq',
      site_config.templates.chromium_pfq,
      boards=[],
      master=True,
      slave_configs=[],
      binhost_test=True,
      push_overlays=constants.BOTH_OVERLAYS,
      afdo_update_ebuild=True,
      chrome_sdk=False,
      health_alert_recipients=['chromeos-infra-eng@grotations.appspotmail.com',
                               'chrome'],
  )

  # Create important configs, then non-important configs.
  master_config.AddSlaves(
      site_config.AddForBoards(
          'chromium-pfq',
          _chromium_pfq_important_boards,
          external_board_configs,
          site_config.templates.chromium_pfq,
          site_config.templates.build_external_chrome)
  )
  site_config.AddForBoards(
      'chromium-pfq',
      ((boards_dict['all_full_boards'] & _chrome_boards) -
       _chromium_pfq_important_boards),
      external_board_configs,
      site_config.templates.chromium_pfq,
      site_config.templates.build_external_chrome,
      important=False)

  _chrome_pfq_important_boards = frozenset([
      'betty',
      'caroline',
      'caroline-arcnext',
      'chell',
      'coral',
      'cyan',
      'daisy_skate',
      'eve-arcnext',
      'kevin-arcnext',
      'peach_pit',
      'peppy',
      'reef',
      'terra',
      'tricky',
      'veyron_minnie',
      'veyron_rialto',
  ])

  _chrome_pfq_experimental_boards = frozenset([
      'betty-arcnext',
      'bob',
      'hana',
      'nyan_big',
      'scarlet',
  ])

  _chrome_pfq_tryjob_boards = (
      (boards_dict['all_release_boards'] & _chrome_boards) -
      (_chrome_pfq_important_boards | _chrome_pfq_experimental_boards)
  )

  master_config.AddSlaves(
      site_config.AddForBoards(
          'chrome-pfq',
          _chrome_pfq_important_boards,
          internal_board_configs,
          site_config.templates.chrome_pfq,
          important=True,
          active_waterfall=waterfall.WATERFALL_INTERNAL,
      )
  )
  master_config.AddSlaves(
      site_config.AddForBoards(
          'chrome-pfq',
          _chrome_pfq_experimental_boards,
          internal_board_configs,
          site_config.templates.chrome_pfq,
          important=False,
          active_waterfall=waterfall.WATERFALL_INTERNAL,
      )
  )
  # Define the result of the build configs for tryjob purposes.
  site_config.AddForBoards(
      'chrome-pfq',
      _chrome_pfq_tryjob_boards,
      internal_board_configs,
      site_config.templates.chrome_pfq,
      important=False,
  )


def FirmwareBuilders(site_config, boards_dict, ge_build_config):
  """Create all firmware build configs.

  Args:
    site_config: config_lib.SiteConfig to be modified by adding templates
                 and configs.
    boards_dict: A dict mapping board types to board name collections.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  board_configs = CreateInternalBoardConfigs(
      site_config, boards_dict, ge_build_config)

  _firmware_boards = _arm_internal_release_boards | _x86_internal_release_boards

  _x86_depthcharge_firmware_boards = frozenset([
      'link',
  ])

  # Add x86 and arm firmware configs.
  for board in _firmware_boards:
    site_config.Add(
        '%s-%s' % (board, config_lib.CONFIG_TYPE_FIRMWARE),
        site_config.templates.firmware,
        board_configs[board],
        site_config.templates.no_vmtest_builder,
    )

  for board in _x86_depthcharge_firmware_boards:
    site_config.Add(
        '%s-%s-%s' % (board, 'depthcharge', config_lib.CONFIG_TYPE_FIRMWARE),
        site_config.templates.depthcharge_firmware,
        board_configs[board],
        site_config.templates.no_vmtest_builder,
    )
    site_config.Add(
        '%s-%s-%s-%s' % (board, 'depthcharge', config_lib.CONFIG_TYPE_FULL,
                         config_lib.CONFIG_TYPE_FIRMWARE),
        site_config.templates.depthcharge_full_firmware,
        board_configs[board],
        site_config.templates.no_vmtest_builder,
    )


def FactoryBuilders(site_config, boards_dict, ge_build_config):
  """Create all factory build configs.

  Args:
    site_config: config_lib.SiteConfig to be modified by adding templates
                 and configs.
    boards_dict: A dict mapping board types to board name collections.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  board_configs = CreateInternalBoardConfigs(
      site_config, boards_dict, ge_build_config)

  _factory_boards = _arm_internal_release_boards | _x86_internal_release_boards
  _factory_boards -= _nofactory_boards

  for board in _factory_boards:
    site_config.Add(
        '%s-%s' % (board, config_lib.CONFIG_TYPE_FACTORY),
        site_config.templates.factory,
        board_configs[board],
        site_config.templates.no_vmtest_builder,
    )


def ReleaseBuilders(site_config, boards_dict, ge_build_config):
  """Create all release builders.

  Args:
    site_config: config_lib.SiteConfig to be modified by adding templates
                 and configs.
    boards_dict: A dict mapping board types to board name collections.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  # TODO: Use this to stop generating unnecessary configs for release branches.
  is_release_branch = ge_build_config[config_lib.CONFIG_TEMPLATE_RELEASE_BRANCH]

  board_configs = CreateInternalBoardConfigs(
      site_config, boards_dict, ge_build_config)

  unified_builds = config_lib.GetUnifiedBuildConfigAllBuilds(ge_build_config)
  unified_board_names = set([b[config_lib.CONFIG_TEMPLATE_REFERENCE_BOARD_NAME]
                             for b in unified_builds])

  ### Master release config.
  master_config = site_config.Add(
      'master-release',
      site_config.templates.release,
      boards=[],
      master=True,
      slave_configs=[],
      sync_chrome=False,
      chrome_sdk=False,
      afdo_use=False,
      branch_util_test=True,
      important=True,
  )

  ### Release configs.

  # These currently do not support unified build boards.
  _critical_for_chrome_boards = frozenset([
      'daisy',
      'eve',
  ])

  site_config.AddForBoards(
      config_lib.CONFIG_TYPE_RELEASE,
      _critical_for_chrome_boards,
      board_configs,
      site_config.templates.release,
      critical_for_chrome=True,
  )

  builder_to_boards_dict = config_lib.GroupBoardsByBuilder(
      ge_build_config[config_lib.CONFIG_TEMPLATE_BOARDS])

  _all_release_builder_boards = builder_to_boards_dict[
      config_lib.CONFIG_TEMPLATE_RELEASE]

  site_config.AddForBoards(
      config_lib.CONFIG_TYPE_RELEASE,
      (((boards_dict['all_release_boards'] | _all_release_builder_boards)
        - _critical_for_chrome_boards) - unified_board_names),
      board_configs,
      site_config.templates.release,
  )

  def _GetConfigWaterfall(builder):
    if builder == config_lib.CONFIG_TEMPLATE_RELEASE:
      if is_release_branch:
        return waterfall.WATERFALL_RELEASE
      else:
        return waterfall.WATERFALL_INTERNAL
    else:
      # Currently just support RELEASE builders
      raise ValueError('Do not support builder %s.' % builder)

  hw_test_list = HWTestList(ge_build_config)

  for unibuild in config_lib.GetUnifiedBuildConfigAllBuilds(ge_build_config):
    active_waterfall = _GetConfigWaterfall(
        unibuild[config_lib.CONFIG_TEMPLATE_BUILDER])
    models = []
    for model in unibuild[config_lib.CONFIG_TEMPLATE_MODELS]:
      name = model[config_lib.CONFIG_TEMPLATE_MODEL_NAME]
      lab_board_name = model[config_lib.CONFIG_TEMPLATE_MODEL_BOARD_NAME]
      if config_lib.CONFIG_TEMPLATE_MODEL_TEST_SUITES in model:
        test_suites = model[config_lib.CONFIG_TEMPLATE_MODEL_TEST_SUITES]
        if 'bvt-arc' in test_suites:
          # TODO(crbug.com/814793)
          # We're tying these test suites to bvt-arc because it's not worth
          # plumbing this all the way through the GE UI since that architecture
          # was never fully implemented and we shouldn't have tied to it in
          # the first place.
          # Once test planning is properly implemented, this will fall out.
          test_suites.append('arc-cts-follower')
          test_suites.append('arc-cts-qual')
          test_suites.append('arc-gts-qual')
        models.append(config_lib.ModelTestConfig(
            name,
            lab_board_name,
            test_suites))
      else:
        no_model_test_suites = []
        models.append(config_lib.ModelTestConfig(
            name, lab_board_name, no_model_test_suites))

    reference_board_name = unibuild[
        config_lib.CONFIG_TEMPLATE_REFERENCE_BOARD_NAME]

    pool = constants.HWTEST_MACH_POOL
    config_name = '%s-release' % reference_board_name
    site_config.AddForBoards(
        config_lib.CONFIG_TYPE_RELEASE,
        [reference_board_name],
        board_configs,
        site_config.templates.release,
        models=models,
        important=not unibuild[config_lib.CONFIG_TEMPLATE_EXPERIMENTAL],
        active_waterfall=active_waterfall,
        hw_tests=(hw_test_list.SharedPoolCanary(pool=pool) +
                  hw_test_list.CtsGtsQualTests()),
    )

    master_config.AddSlave(site_config[config_name])

  def GetReleaseConfigName(board):
    """Convert a board name into a release config name."""
    return '%s-release' % board

  def GetConfigName(builder, board):
    """Convert a board name into a config name."""
    if builder == config_lib.CONFIG_TEMPLATE_RELEASE:
      return GetReleaseConfigName(board)
    else:
      # Currently just support RELEASE builders
      raise Exception('Do not support other builders.')

  def _GetConfigValues(builder, board):
    """Get and return config values from template"""

    config_values = {
        'important': not board[config_lib.CONFIG_TEMPLATE_EXPERIMENTAL],
        'active_waterfall': _GetConfigWaterfall(builder)
    }

    return config_values

  def _AdjustUngroupedReleaseConfigs(builder_ungrouped_dict):
    """Adjust for ungrouped release boards"""
    for builder in builder_ungrouped_dict:
      for board in builder_ungrouped_dict[builder]:
        config_name = GetConfigName(builder,
                                    board[config_lib.CONFIG_TEMPLATE_NAME])
        site_config[config_name].apply(
            _GetConfigValues(builder, board),
        )
        master_config.AddSlave(site_config[config_name])

  def _AdjustGroupedReleaseConfigs(builder_group_dict):
    """Adjust leader and follower configs for grouped boards"""
    for builder in builder_group_dict:
      for group in builder_group_dict[builder]:
        board_group = builder_group_dict[builder][group]

        # Leaders are built on baremetal builders and run all tests needed by
        # the related boards.
        for board in board_group.leader_boards:
          config_name = GetConfigName(builder,
                                      board[config_lib.CONFIG_TEMPLATE_NAME])
          site_config[config_name].apply(
              _GetConfigValues(builder, board),
          )
          master_config.AddSlave(site_config[config_name])

        # Followers are built on GCE instances, and turn off testing that breaks
        # on GCE. The missing tests run on the leader board.
        for board in board_group.follower_boards:
          config_name = GetConfigName(builder,
                                      board[config_lib.CONFIG_TEMPLATE_NAME])
          site_config[config_name].apply(
              _GetConfigValues(builder, board),
              buildslave_type=constants.GCE_BEEFY_BUILD_SLAVE_TYPE,
              chrome_sdk_build_chrome=False,
              vm_tests=[],
          )
          master_config.AddSlave(site_config[config_name])

  def _AdjustReleaseConfigs():
    """Adjust ungrouped and grouped release configs"""
    (builder_group_dict, builder_ungrouped_dict) = (
        config_lib.GroupBoardsByBuilderAndBoardGroup(
            ge_build_config[config_lib.CONFIG_TEMPLATE_BOARDS]))
    _AdjustUngroupedReleaseConfigs(builder_ungrouped_dict)
    _AdjustGroupedReleaseConfigs(builder_group_dict)

    for board in _moblab_boards:
      config_name = GetReleaseConfigName(board)
      if config_name not in site_config:
        continue
      # If the board is in _moblab_boards, use moblab_release template
      site_config[config_name].apply(
          site_config.templates.moblab_release,
          board_configs[board],
      )

  _AdjustReleaseConfigs()


def PayloadBuilders(site_config, boards_dict):
  """Create <board>-payloads configs for all payload generating boards.

  We create a config named 'board-payloads' for every board which has a
  config with 'paygen' True. The idea is that we have a build that generates
  payloads, we need to have a tryjob to re-attempt them on failure.
  """
  for board in boards_dict['all_release_boards']:
    if site_config['%s-release' % board].paygen:
      site_config.Add(
          '%s-payloads' % board,
          site_config.templates.payloads,
          boards=[board],
      )


def InsertHwTestsOverrideDefaults(build):
  """Insert default hw_tests values for a given build.

  Also updates child builds.

  Args:
    build: BuildConfig instance to modify in place.
  """
  for child in build['child_configs']:
    InsertHwTestsOverrideDefaults(child)

  if build['hw_tests_override'] is not None:
    # Explicitly set, no need to insert defaults.
    return

  if build['hw_tests']:
    # Copy over base tests.
    build['hw_tests_override'] = [copy.copy(x) for x in build['hw_tests']]

    # Adjust for manual test environment.
    for hw_config in build['hw_tests_override']:
      hw_config.pool = constants.HWTEST_TRYBOT_POOL
      hw_config.file_bugs = False
      hw_config.priority = constants.HWTEST_DEFAULT_PRIORITY


def InsertWaterfallDefaults(site_config, ge_build_config):
  """Method with un-refactored build configs/templates.

  Args:
    site_config: config_lib.SiteConfig containing builds to have their
                 waterfall values updated.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  is_release_branch = ge_build_config[config_lib.CONFIG_TEMPLATE_RELEASE_BRANCH]

  for name, c in site_config.iteritems():
    if not c.get('active_waterfall'):
      c['active_waterfall'] = GetDefaultWaterfall(c, is_release_branch)

  # Apply manual configs, not used for release branches.
  if not is_release_branch:
    for wfall, names in _waterfall_config_map.iteritems():
      for name in names:
        site_config[name]['active_waterfall'] = wfall


def ApplyCustomOverrides(site_config, ge_build_config):
  """Method with to override specific flags for specific builders.

  Generally try really hard to avoid putting anything here that isn't
  a really special case for a single specific builder. This is performed
  after every other bit of processing, so it always has the final say.

  Args:
    site_config: config_lib.SiteConfig containing builds to have their
                 waterfall values updated.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  hw_test_list = HWTestList(ge_build_config)

  overwritten_configs = {
      'amd64-generic-chromium-pfq': {
          'useflags': [],
      },

      'lakitu-pre-cq':
          site_config.templates.lakitu_test_customizations,

      'lakitu-gpu-pre-cq':
          site_config.templates.lakitu_test_customizations,

      'lakitu-nc-pre-cq':
          site_config.templates.lakitu_nc_customizations,

      'lakitu-st-pre-cq':
          site_config.templates.lakitu_test_customizations,

      'lakitu_next-pre-cq':
          site_config.templates.lakitu_test_customizations,

      ### Arm release configs
      'smaug-release' : {
          'paygen': False,
          'sign_types':['nv_lp0_firmware'],
      },

      'whirlwind-release': {
          'dev_installer_prebuilts':True,
      },

      'gale-release': {
          'dev_installer_prebuilts':True,
      },

      'lakitu-release': config_lib.BuildConfig().apply(
          site_config.templates.lakitu_test_customizations,
          site_config.templates.lakitu_notification_emails,
          sign_types=['base'],
      ),

      # This is the full build of open-source overlay.
      'lakitu-full': config_lib.BuildConfig().apply(
          site_config.templates.lakitu_notification_emails,
      ),

      'lakitu-gpu-release': config_lib.BuildConfig().apply(
          site_config.templates.lakitu_test_customizations,
          site_config.templates.lakitu_notification_emails,
          sign_types=['base'],
          paygen=False,
      ),

      'lakitu-nc-release': config_lib.BuildConfig().apply(
          site_config.templates.lakitu_nc_customizations,
          site_config.templates.lakitu_notification_emails,
          signer_tests=False,
          paygen=False,
      ),

      'lakitu-st-release': config_lib.BuildConfig().apply(
          site_config.templates.lakitu_test_customizations,
          site_config.templates.lakitu_notification_emails,
          sign_types=['base'],
          paygen=False,
      ),

      'lakitu_next-release': config_lib.BuildConfig().apply(
          site_config.templates.lakitu_test_customizations,
          site_config.templates.lakitu_notification_emails,
          signer_tests=False,
      ),

      'guado_labstation-release': {
          'hw_tests': [],
          'hwqual':False,
          'image_test':False,
          'images':['base', 'test'],
          'signer_tests':False,
          'paygen':False,
          'vm_tests':[],
      },

      'terra-release': {
          'useflags': append_useflags(['cfi']),
      },

      'caroline-release': {
          'useflags': append_useflags(['cfi']),
      },

      'elm-release': {
          'useflags': append_useflags(['thinlto']),
      },

      'peach_pit-release': {
          'useflags': append_useflags(['thinlto']),
      },

      'chell-chrome-pfq': {
          'afdo_generate': True,
          'archive_build_debug': True,
          # Disable hugepages before collecting AFDO profile.
          # Disable debug fission before collecting AFDO profile.
          # Disable thinlto before collecting AFDO profile.
          'useflags': append_useflags(['-transparent_hugepage',
                                       '-debug_fission',
                                       '-thinlto']),
          'hw_tests': [hw_test_list.AFDORecordTest()]
      },

      'cyan-chrome-pfq': {
          'hw_tests': hw_test_list.SharedPoolAndroidPFQ(),
      },

      'caroline-arcnext-chrome-pfq': {
          'hw_tests': hw_test_list.SharedPoolAndroidPFQ(),
      },

      'kevin-arcnext-chrome-pfq': {
          'hw_tests': hw_test_list.SharedPoolAndroidPFQ(),
      },

      'peppy-chrome-pfq': {
          'hw_tests': hw_test_list.SharedPoolPFQ(),
      },

      'reef-chrome-pfq': {
          'hw_tests': hw_test_list.SharedPoolAndroidPFQ(),
      },

      'veyron_minnie-chrome-pfq': {
          'hw_tests': hw_test_list.SharedPoolAndroidPFQ(),
      },

      'peach_pit-chrome-pfq': {
          'hw_tests': hw_test_list.SharedPoolPFQ(),
      },

      'tricky-chrome-pfq': {
          'hw_tests': hw_test_list.SharedPoolPFQ(),
      },

      # Currently factory and firmware branches will be created after DVT stage
      # therefore we need signed factory shim or accessory_rwsig firmware from
      # ToT temporarily.
      #
      # After factory and firmware branches are created, the configuation of
      # this project should be removed.
      # --- start from here ---
      'cheza-release': {
          'sign_types': ['recovery', 'factory'],
      },

      'poppy-release': {
          'sign_types': ['recovery', 'accessory_rwsig', 'factory'],
      },

      'nautilus-release': {
          'sign_types': ['recovery', 'accessory_rwsig', 'factory'],
      },

      'nami-release': {
          'sign_types': ['recovery', 'factory'],
      },

      'meowth-release': {
          'sign_types': ['recovery', 'accessory_rwsig', 'factory'],
      },
      # --- end from here ---

      'betty-release':
          site_config.templates.tast_vm_canary_tests,

  }

  for config_name, overrides  in overwritten_configs.iteritems():
    # TODO: Turn this assert into a unittest.
    # config = site_config[config_name]
    # for k, v in overrides.iteritems():
    #   assert config[k] != v, ('Unnecessary override: %s: %s' %
    #                           (config_name, k))
    site_config[config_name].apply(**overrides)


def SpecialtyBuilders(site_config, boards_dict, ge_build_config):
  """Add a variety of specialized builders or tryjobs.

  Args:
    site_config: config_lib.SiteConfig to be modified by adding templates
                 and configs.
    boards_dict: A dict mapping board types to board name collections.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  board_configs = CreateInternalBoardConfigs(
      site_config, boards_dict, ge_build_config)
  hw_test_list = HWTestList(ge_build_config)

  site_config.AddWithoutTemplate(
      'success-build',
      site_config.templates.external,
      site_config.templates.no_hwtest_builder,
      site_config.templates.no_vmtest_builder,
      boards=[],
      display_label=config_lib.DISPLAY_LABEL_TRYJOB,
      builder_class_name='test_builders.SucessBuilder',
      description='Builder always passes as quickly as possible.',
  )

  site_config.AddWithoutTemplate(
      'fail-build',
      site_config.templates.external,
      site_config.templates.no_hwtest_builder,
      site_config.templates.no_vmtest_builder,
      boards=[],
      display_label=config_lib.DISPLAY_LABEL_TRYJOB,
      builder_class_name='test_builders.FailBuilder',
      description='Builder always fails as quickly as possible.',
  )

  site_config.AddWithoutTemplate(
      'chromiumos-sdk',
      site_config.templates.full,
      site_config.templates.no_hwtest_builder,
      # The amd64-host has to be last as that is when the toolchains
      # are bundled up for inclusion in the sdk.
      boards=[
          'arm-generic', 'amd64-generic'
      ],
      display_label=config_lib.DISPLAY_LABEL_UTILITY,
      build_type=constants.CHROOT_BUILDER_TYPE,
      active_waterfall=waterfall.WATERFALL_EXTERNAL,
      buildslave_type=constants.GCE_BEEFY_BUILD_SLAVE_TYPE,
      builder_class_name='sdk_builders.ChrootSdkBuilder',
      use_sdk=False,
      prebuilts=constants.PUBLIC,
      build_timeout=18 * 60 * 60,
      description='Build the SDK and all the cross-compilers',
      doc='http://www.chromium.org/chromium-os/build/builder-overview#'
          'TOC-Continuous',
  )

  site_config.AddWithoutTemplate(
      'som-dispatcher',
      site_config.templates.internal,
      site_config.templates.no_hwtest_builder,
      site_config.templates.no_vmtest_builder,
      important=True,
      display_label=config_lib.DISPLAY_LABEL_UTILITY,
      description=('Updates alerts displayed on SoM website. go/som/chromeos'),
      build_type=constants.GENERIC_TYPE,
      boards=[],
      builder_class_name='alert_builders.SomDispatcherBuilder',
      active_waterfall=constants.WATERFALL_SWARMING,
      buildslave_type=constants.GCE_BEEFY_BUILD_SLAVE_TYPE,
  )

  site_config.AddWithoutTemplate(
      'config-updater',
      site_config.templates.internal,
      site_config.templates.no_hwtest_builder,
      site_config.templates.no_vmtest_builder,
      important=True,
      display_label=config_lib.DISPLAY_LABEL_UTILITY,
      description=('Build Config Updater reads updated GE config files from'
                   ' GS, and commits them to chromite after running tests.'),
      build_type=constants.GENERIC_TYPE,
      boards=[],
      builder_class_name='config_builders.UpdateConfigBuilder',
      active_waterfall=constants.WATERFALL_SWARMING,
      binhost_test=True,
  )

  site_config.AddWithoutTemplate(
      constants.BRANCH_UTIL_CONFIG,
      site_config.templates.paladin,
      site_config.templates.internal_paladin,
      site_config.templates.no_vmtest_builder,
      site_config.templates.no_hwtest_builder,
      display_label=config_lib.DISPLAY_LABEL_UTILITY,
      boards=[],
      # Disable postsync_patch to prevent conflicting patches from being applied
      # - e.g., patches from 'master' branch being applied to a branch.
      postsync_patch=False,
      # Disable postsync_reexec to continue running the 'master' branch chromite
      # for all stages, rather than the chromite in the branch buildroot.
      postsync_reexec=False,
      # Need to reset the paladin build_type we inherited.
      build_type=None,
      builder_class_name='release_builders.CreateBranchBuilder',
      description='Used for creating/deleting branches (TPMs only)',
      active_waterfall=waterfall.WATERFALL_SWARMING,
      # This very weird tryjob is only run locally. It should never upload
      # build artifacts.
      archive=False,
      gs_path=os.path.join(constants.TRASH_BUCKET, 'branch-util-noise/')
  )

  site_config.AddWithoutTemplate(
      'sync-test-cbuildbot',
      site_config.templates.no_hwtest_builder,
      boards=[],
      display_label=config_lib.DISPLAY_LABEL_TRYJOB,
      build_type=constants.INCREMENTAL_TYPE,
      builder_class_name='test_builders.ManifestVersionedSyncBuilder',
      chroot_replace=True,
      description='Sync tryjob to help with cbuildbot development',
      active_waterfall=waterfall.WATERFALL_SWARMING,
  )



  site_config.AddWithoutTemplate(
      'betty-vmtest-informational',
      site_config.templates.internal,
      site_config.templates.no_hwtest_builder,
      display_label=config_lib.DISPLAY_LABEL_INFORMATIONAL,
      description='VMTest Informational Builder for running long run tests.',
      build_type=constants.GENERIC_TYPE,
      boards=['betty'],
      builder_class_name='test_builders.VMInformationalBuilder',
      vm_tests=getInfoVMTest(),
      vm_tests_override=getInfoVMTest(),
      active_waterfall=constants.WATERFALL_INTERNAL,
      vm_test_report_to_dashboards=True,
  )

  # Create our unittest stress build configs (used for tryjobs only)
  site_config.AddForBoards(
      'unittest-stress',
      boards_dict['all_boards'],
      board_configs,
      site_config.templates.unittest_stress,
  )

  site_config.AddGroup(
      'test-ap-group',
      site_config.Add('panther-test-ap',
                      site_config.templates.test_ap,
                      boards=['panther']),
      site_config.Add('whirlwind-test-ap',
                      site_config.templates.test_ap,
                      boards=['whirlwind']),
      site_config.Add('gale-test-ap',
                      site_config.templates.test_ap,
                      boards=['gale']),
      description='Create images used to power access points in WiFi lab.',
  )

  # *-pre-flight-branch builders are in chromeos_release waterfall.
  site_config.Add(
      'samus-chrome-pre-flight-branch',
      site_config.templates.pre_flight_branch,
      display_label=config_lib.DISPLAY_LABEL_CHROME_PFQ,
      boards=['samus'],
      afdo_generate=True,
      afdo_update_ebuild=True,
      sync_chrome=True,
      chrome_rev=constants.CHROME_REV_STICKY,
      hw_tests=[hw_test_list.AFDORecordTest()],
      useflags=append_useflags(['-transparent_hugepage',
                                '-debug_fission',
                                '-thinlto']),
      prebuilts=constants.PRIVATE,
      archive_build_debug=True,
  )

  site_config.Add(
      'reef-android-nyc-pre-flight-branch',
      site_config.templates.pre_flight_branch,
      display_label=config_lib.DISPLAY_LABEL_NYC_ANDROID_PFQ,
      boards=['reef'],
      sync_chrome=True,
      android_rev=constants.ANDROID_REV_LATEST,
      android_package='android-container-nyc',
      android_import_branch=constants.ANDROID_NYC_BUILD_BRANCH,
      prebuilts=False,
  )

  site_config.AddWithoutTemplate(
      'chromeos-infra-go',
      site_config.templates.no_hwtest_builder,
      site_config.templates.no_unittest_builder,
      site_config.templates.no_vmtest_builder,
      boards=[],
      display_label=config_lib.DISPLAY_LABEL_UTILITY,
      build_type=constants.GENERIC_TYPE,
      buildslave_type=constants.GCE_BEEFY_BUILD_SLAVE_TYPE,
      builder_class_name='infra_builders.InfraGoBuilder',
      use_sdk=True,
      prebuilts=constants.PUBLIC,
      build_timeout=60 * 60,
      description='Build Chromium OS infra Go binaries',
      doc='https://goto.google.com/cros-infra-go-packaging',
      active_waterfall=waterfall.WATERFALL_EXTERNAL,
  )


def EnsureVmTestsOnVmTestBoards(site_config, boards_dict, _gs_build_config):
  """Make sure VMTests are only enabled on boards that support them.

  Args:
    site_config: config_lib.SiteConfig containing builds to have their
                 waterfall values updated.
    boards_dict: A dict mapping board types to board name collections.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  for c in site_config.itervalues():
    if set(c['boards']).intersection(set(boards_dict['no_vmtest_boards'])):
      c.apply(site_config.templates.no_vmtest_builder)
      if c.child_configs:
        for cc in c.child_configs:
          cc.apply(site_config.templates.no_vmtest_builder)


def EnsureVmTestsOnBaremetal(site_config, _gs_build_config):
  """Make sure VMTests have a builder than can run them.

  Args:
    site_config: config_lib.SiteConfig containing builds to have their
                 waterfall values updated.
    ge_build_config: Dictionary containing the decoded GE configuration file.
  """
  for c in site_config.itervalues():
    # We can only run vmtests on baremetal, so ensure we have it.
    if (c.vm_tests or c.moblab_vm_tests) and c.build_type != 'pre_cq':
      c['buildslave_type'] = constants.BAREMETAL_BUILD_SLAVE_TYPE


def TryjobMirrors(site_config):
  """Create tryjob specialized variants of every build config.

  This creates a new 'tryjob' config for every existing config, unless the
  existing config has a display_label that is already in TRYJOB_DISPLAY_LABEL.

  Args:
    site_config: config_lib.SiteConfig to be modified by adding templates
                 and configs.
  """
  # Tryjobs which are unsafe to talk to Prod CIDB.
  cidb_unsafe = frozenset(['pre-cq-launcher'])

  tryjob_configs = {}

  for build_name, config in site_config.iteritems():
    # Don't mirror builds that are already tryjob safe.
    if config.display_label in config_lib.TRYJOB_DISPLAY_LABEL:
      config.apply(hw_tests_override=None, vm_tests_override=None)
      continue

    tryjob_name = build_name + '-tryjob'

    # Don't overwrite mirrored versions that were explicitly created earlier.
    if tryjob_name in site_config:
      continue

    tryjob_config = copy.deepcopy(config)
    tryjob_config.apply(
        name=tryjob_name,
        display_label=config_lib.DISPLAY_LABEL_TRYJOB,
        luci_builder=config_lib.LUCI_BUILDER_TRY,
        # Generally make tryjobs safer.
        chroot_replace=True,
        debug=True,
        paygen=False,
        push_image=False,
        push_overlays=None,
        # Force uprev. This is so patched in changes are always built.
        uprev=True,
        active_waterfall=waterfall.WATERFALL_SWARMING,
        gs_path=config_lib.GS_PATH_DEFAULT,
    )

    # Force uprev. This is so patched in changes are always built.
    if tryjob_config.internal:
      tryjob_config.apply(overlays=constants.BOTH_OVERLAYS)

    # In trybots, we want to always run VM tests and all unit tests, so that
    # developers will get better testing for their changes.
    if tryjob_config.hw_tests_override is not None:
      tryjob_config.apply(hw_tests=tryjob_config.hw_tests_override,
                          hw_tests_override=None)

    if tryjob_config.vm_tests_override is not None:
      tryjob_config.apply(vm_tests=tryjob_config.vm_tests_override,
                          vm_tests_override=None)

    if build_name in cidb_unsafe or tryjob_config.master:
      tryjob_config.apply(debug_cidb=True)

    # Save off the new config so we can insert into site_config.
    tryjob_configs[tryjob_name] = tryjob_config

  for tryjob_name, tryjob_config in tryjob_configs.iteritems():
    site_config[tryjob_name] = tryjob_config


@factory.CachedFunctionCall
def GetConfig():
  """Create the Site configuration for all ChromeOS builds.

  Returns:
    A config_lib.SiteConfig.
  """
  site_params = SiteParameters()
  defaults = DefaultSettings(site_params)

  ge_build_config = config_lib.LoadGEBuildConfigFromFile()

  # site_config with no templates or build configurations.
  site_config = config_lib.SiteConfig(defaults=defaults,
                                      site_params=site_params)
  boards_dict = GetBoardTypeToBoardsDict(ge_build_config)

  GeneralTemplates(site_config, ge_build_config)

  ToolchainBuilders(site_config, boards_dict, ge_build_config)

  ReleaseBuilders(site_config, boards_dict, ge_build_config)

  PayloadBuilders(site_config, boards_dict)

  SpecialtyBuilders(site_config, boards_dict, ge_build_config)

  PreCqBuilders(site_config, boards_dict, ge_build_config)

  CqBuilders(site_config, boards_dict, ge_build_config)

  IncrementalBuilders(site_config, boards_dict, ge_build_config)

  ReleaseAfdoBuilders(site_config, boards_dict, ge_build_config)

  InformationalBuilders(site_config, boards_dict, ge_build_config)

  FirmwareBuilders(site_config, boards_dict, ge_build_config)

  FactoryBuilders(site_config, boards_dict, ge_build_config)

  AndroidPfqBuilders(site_config, boards_dict, ge_build_config)

  ChromePfqBuilders(site_config, boards_dict, ge_build_config)

  FullBuilders(site_config, boards_dict, ge_build_config)

  # Insert default HwTests for tryjobs.
  for build in site_config.itervalues():
    InsertHwTestsOverrideDefaults(build)

  # Assign waterfalls to builders that don't have them yet.
  InsertWaterfallDefaults(site_config, ge_build_config)

  ApplyCustomOverrides(site_config, ge_build_config)

  EnsureVmTestsOnVmTestBoards(site_config, boards_dict, ge_build_config)

  EnsureVmTestsOnBaremetal(site_config, ge_build_config)

  TryjobMirrors(site_config)

  return site_config
