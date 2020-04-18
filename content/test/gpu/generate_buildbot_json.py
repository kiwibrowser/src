#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to generate chromium.gpu.json and chromium.gpu.fyi.json in
the src/testing/buildbot directory. Maintaining these files by hand is
too unwieldy.
"""

import copy
import json
import os
import string
import sys

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.dirname(os.path.dirname(os.path.dirname(THIS_DIR)))

# Current stable Windows NVIDIA Quadro P400 device/driver/os identifiers.
WIN7_NVIDIA_QUADRO_P400_STABLE_DRIVER = '10de:1cb3-23.21.13.8792'
WIN7_NVIDIA_QUADRO_P400_STABLE_OS = 'Windows-2008ServerR2-SP1'
WIN10_NVIDIA_QUADRO_P400_STABLE_DRIVER = '10de:1cb3-23.21.13.8816'
WIN10_NVIDIA_QUADRO_P400_STABLE_OS = 'Windows-10'

# Current experimental Windows NVIDIA Quadro P400 device/driver/os
# identifiers.
WIN10_NVIDIA_QUADRO_P400_EXPERIMENTAL_DRIVER = '10de:1cb3-23.21.13.8816'
WIN10_NVIDIA_QUADRO_P400_EXPERIMENTAL_OS = 'Windows-10'

# Use this to match all drivers for the NVIDIA Quadro P400.
NVIDIA_QUADRO_P400_ALL_DRIVERS = '10de:1cb3-*'

# Linux NVIDIA Quadro P400.
LINUX_QUADRO_P400_STABLE_DRIVER = '10de:1cb3-384.90'

# Intel HD 630 (both Windows and Linux).
INTEL_HD_630 = '8086:5912'
WIN10_INTEL_HD_630_STABLE_DRIVER = '8086:5912-23.20.16.4877'
WIN10_INTEL_HD_630_EXPERIMENTAL_DRIVER = '8086:5912-24.20.100.6025'

# "Types" of waterfalls and bots. A bot's type is the union of its own
# type and the type of its waterfall. Predicates can apply to these
# sets in order to run tests only on a certain subset of the bots.
class Types(object):
  GPU = 'gpu'
  GPU_FYI = 'gpu_fyi'
  OPTIONAL = 'optional'
  V8_FYI = 'v8_fyi'
  # The Win ANGLE AMD tryserver is split off because there isn't
  # enough capacity to run all the tests from chromium.gpu.fyi's Win
  # AMD bot on a tryserver. It represents some of the tests on
  # win_angle_rel_ng and is not a real machine on the waterfall.
  WIN_ANGLE_AMD_TRYSERVER = 'win_angle_amd_tryserver'
  # The dEQP tests use a different compiler configuration than the
  # rest of the bots; they're the only target which use exceptions and
  # RTTI. They're split off so that these specialized compiler options
  # apply only to these targets.
  DEQP = 'deqp'
  # The experimental try servers are meant to test new driver versions
  # and OS flavors.
  EXPERIMENTAL = 'experimental'
  # This deliberately doesn't match any predicate, and is used to
  # completely disable a particular trybot.
  NOOP = 'noop'

# The predicate functions receive a list of types as input and
# determine whether the test should run on the given bot.
class Predicates(object):
  @staticmethod
  def DEFAULT(x):
    # By default, tests run on the chromium.gpu and chromium.gpu.fyi
    # waterfalls, but not on the DEQP bots, not on the optional
    # tryservers, not on the client.v8.fyi waterfall, nor on the Win
    # ANGLE AMD tryserver.
    return Types.DEQP not in x and Types.OPTIONAL not in x and \
      Types.V8_FYI not in x and Types.WIN_ANGLE_AMD_TRYSERVER not in x and \
      Types.NOOP not in x

  @staticmethod
  def FYI_ONLY(x):
    # This predicate is more complex than desired because the optional
    # tryservers and the Win ANGLE AMD tryserver are considered to be
    # on the chromium.gpu.fyi waterfall.
    return Types.GPU_FYI in x and Types.DEQP not in x and \
      Types.OPTIONAL not in x and Types.WIN_ANGLE_AMD_TRYSERVER not in x and \
      Types.NOOP not in x

  @staticmethod
  def FYI_AND_OPTIONAL(x):
    return Predicates.FYI_ONLY(x) or Types.OPTIONAL in x

  @staticmethod
  def FYI_AND_OPTIONAL_AND_WIN_ANGLE_AMD(x):
    return Predicates.FYI_ONLY(x) or Types.OPTIONAL in x or \
      Types.WIN_ANGLE_AMD_TRYSERVER in x

  @staticmethod
  def FYI_OPTIONAL_AND_V8(x):
    return Predicates.FYI_AND_OPTIONAL(x) or Types.V8_FYI in x

  @staticmethod
  def FYI_OPTIONAL_V8_AND_WIN_ANGLE_AMD(x):
    return Predicates.FYI_OPTIONAL_AND_V8(x) or \
      Types.WIN_ANGLE_AMD_TRYSERVER in x

  @staticmethod
  def DEFAULT_PLUS_V8(x):
    return Predicates.DEFAULT(x) or Types.V8_FYI in x

  @staticmethod
  def DEFAULT_AND_OPTIONAL(x):
    return Predicates.DEFAULT(x) or Types.OPTIONAL in x

  @staticmethod
  def DEQP(x):
    return Types.DEQP in x

  @staticmethod
  def EXPERIMENTAL_CONDITIONALLY(x):
    return Types.EXPERIMENTAL in x

# Most of the bots live in the Chrome-GPU pool as defined here (Google
# employees only, sorry):
# https://chrome-internal.googlesource.com/infradata/config/+/master/configs/
#   chromium-swarm/bots.cfg
#
# Some of them, like the Mac Minis and Nexus 5X devices, are shared
# resources and live in the regular Chrome pool.

WATERFALL = {
  'name': 'chromium.gpu',
  'type': Types.GPU,

  'builders': {
    'GPU Win Builder' : {},
    'GPU Win Builder (dbg)' : {},
    'GPU Mac Builder' : {},
    'GPU Mac Builder (dbg)' : {},
    'GPU Linux Builder' : {},
    'GPU Linux Builder (dbg)' : {},
   },

  'testers': {
    'Win10 Release (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': WIN10_NVIDIA_QUADRO_P400_STABLE_DRIVER,
          'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'win',
      'use_gpu_trigger_script': True,
    },
    'Win10 Debug (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': WIN10_NVIDIA_QUADRO_P400_STABLE_DRIVER,
          'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Debug',
      'swarming': True,
      'os_type': 'win',
    },
    'Mac Release (Intel)': {
      'swarming_dimensions': [
        {
          'gpu': '8086:0a2e',
          'os': 'Mac-10.12.6',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'mac',
    },
    'Mac Debug (Intel)': {
      'swarming_dimensions': [
        {
          'gpu': '8086:0a2e',
          'os': 'Mac-10.12.6',
        },
      ],
      'build_config': 'Debug',
      'swarming': True,
      'os_type': 'mac',
    },
    'Mac Retina Release (AMD)': {
      'swarming_dimensions': [
        {
          'gpu': '1002:6821',
          'hidpi': '1',
          'os': 'Mac-10.12.6',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'mac',
    },
    'Mac Retina Debug (AMD)': {
      'swarming_dimensions': [
        {
          'gpu': '1002:6821',
          'hidpi': '1',
          'os': 'Mac-10.12.6',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Debug',
      'swarming': True,
      'os_type': 'mac',
    },
    'Linux Release (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': LINUX_QUADRO_P400_STABLE_DRIVER,
          'os': 'Ubuntu',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'linux',
    },
    'Linux Debug (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': LINUX_QUADRO_P400_STABLE_DRIVER,
          'os': 'Ubuntu',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Debug',
      'swarming': True,
      'os_type': 'linux',
    },
    'Android Release (Nexus 5X)': {
      'swarming_dimensions': [
        {
          'device_type': 'bullhead',
          'device_os': 'MMB29Q',
          'os': 'Android'
        },
      ],
      'build_config': 'android-chromium',
      'swarming': True,
      'os_type': 'android',
    },
  }
}

FYI_WATERFALL = {
  'name': 'chromium.gpu.fyi',
  'type': Types.GPU_FYI,

  'builders': {
    'GPU FYI Win Builder' : {},
    'GPU FYI Win Builder (dbg)' : {},
    'GPU FYI Win dEQP Builder': {},
    'GPU FYI Win x64 Builder' : {},
    'GPU FYI Win x64 Builder (dbg)' : {},
    'GPU FYI Win x64 dEQP Builder' : {},
    'GPU FYI Mac Builder' : {},
    'GPU FYI Mac Builder (dbg)' : {},
    'GPU FYI Mac dEQP Builder' : {},
    'GPU FYI Linux Builder' : {},
    'GPU FYI Linux Builder (dbg)' : {},
    'GPU FYI Linux Ozone Builder' : {},
    'GPU FYI Linux dEQP Builder' : {},
  },

  'testers': {
    'Win7 FYI Release (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': WIN7_NVIDIA_QUADRO_P400_STABLE_DRIVER,
          'os': 'Windows-2008ServerR2-SP1',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'win',
      'use_gpu_trigger_script': True,
    },
    'Win10 FYI dEQP Release (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': WIN10_NVIDIA_QUADRO_P400_STABLE_DRIVER,
          'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'win',
      'type': Types.DEQP,
    },
    # TODO(kbr): "Experimental" caused too-long path names pre-LUCI.
    # crbug.com/812000
    'Win10 FYI Exp Release (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': WIN10_NVIDIA_QUADRO_P400_EXPERIMENTAL_DRIVER,
          'os': WIN10_NVIDIA_QUADRO_P400_EXPERIMENTAL_OS,
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'win',
      'type': Types.EXPERIMENTAL,
      # This should match another config name specified in this file.
      'stable_tester_name': 'Win10 FYI Release (NVIDIA)',
    },
    'Win10 FYI Release (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': WIN10_NVIDIA_QUADRO_P400_STABLE_DRIVER,
          'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'win',
      'use_gpu_trigger_script': True,
    },
    'Win10 FYI Debug (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': WIN10_NVIDIA_QUADRO_P400_STABLE_DRIVER,
          'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Debug',
      'swarming': True,
      'os_type': 'win',
    },
    'Win7 FYI Release (AMD)': {
      'swarming_dimensions': [
        {
          'gpu': '1002:6613',
          'os': 'Windows-2008ServerR2-SP1',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'win',
    },
    'Win7 FYI Debug (AMD)': {
      'swarming_dimensions': [
        {
          'gpu': '1002:6613',
          'os': 'Windows-2008ServerR2-SP1',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Debug',
      'swarming': True,
      'os_type': 'win',
    },
    'Win7 FYI dEQP Release (AMD)': {
      'swarming_dimensions': [
        {
          'gpu': '1002:6613',
          'os': 'Windows-2008ServerR2-SP1',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'win',
      'type': Types.DEQP,
    },
    'Win10 FYI Exp Release (Intel HD 630)': {
      'swarming_dimensions': [
        {
          'gpu': WIN10_INTEL_HD_630_EXPERIMENTAL_DRIVER,
          'os': 'Windows-10',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'win',
    },
    'Win10 FYI Release (Intel HD 630)': {
      'swarming_dimensions': [
        {
          'gpu': WIN10_INTEL_HD_630_STABLE_DRIVER,
          'os': 'Windows-10',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'win',
    },
    'Win10 FYI dEQP Release (Intel HD 630)': {
      'swarming_dimensions': [
        {
          'gpu': WIN10_INTEL_HD_630_STABLE_DRIVER,
          'os': 'Windows-10',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'win',
      'type': Types.DEQP,
    },
    'Win7 FYI x64 Release (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': WIN7_NVIDIA_QUADRO_P400_STABLE_DRIVER,
          'os': 'Windows-2008ServerR2-SP1',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release_x64',
      'swarming': True,
      'os_type': 'win',
    },
    'Win7 FYI x64 dEQP Release (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': WIN7_NVIDIA_QUADRO_P400_STABLE_DRIVER,
          'os': 'Windows-2008ServerR2-SP1',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release_x64',
      'swarming': True,
      'os_type': 'win',
      'type': Types.DEQP,
    },
    'Mac FYI Release (Intel)': {
      'swarming_dimensions': [
        {
          'gpu': '8086:0a2e',
          'os': 'Mac-10.12.6',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'mac',
    },
    'Mac FYI Debug (Intel)': {
      'swarming_dimensions': [
        {
          'gpu': '8086:0a2e',
          'os': 'Mac-10.12.6',
        },
      ],
      'build_config': 'Debug',
      'swarming': True,
      'os_type': 'mac',
    },
    'Mac Pro FYI Release (AMD)': {
      'swarming_dimensions': [
        {
          'gpu': '1002:679e',
          'os': 'Mac-10.12',
          'pool': 'Chrome-GPU',
        },
      ],
      # TODO(kbr): this separate dictionary is a hack to avoid generalizing the
      # "swarming_dimensions" handling in this generator script. When merging
      # this script with the one in src/testing/buildbot/, this will no longer
      # be necessary.
      'swarming_settings': {
        # There are only two bots of this type in the Swarming pool right now,
        # so we have to increase the default expiration time of 1 hour (3600
        # seconds) to prevent webgl2_conformance_tests' shards from timing out.
        'expiration': 10800,
      },
      'build_config': 'Release',
      # Even though this bot is a one-off, it's still in the Swarming pool.
      'swarming': True,
      'os_type': 'mac',
    },
    'Mac FYI Retina Release (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': '10de:0fe9',
          'hidpi': '1',
          'os': 'Mac-10.12.6',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'mac',
    },
    'Mac FYI Retina Debug (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': '10de:0fe9',
          'hidpi': '1',
          'os': 'Mac-10.12.6',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Debug',
      'swarming': True,
      'os_type': 'mac',
    },
    'Mac FYI Retina Release (AMD)': {
      'swarming_dimensions': [
        {
          'gpu': '1002:6821',
          'hidpi': '1',
          'os': 'Mac-10.12.6',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'mac',
    },
    'Mac FYI Retina Debug (AMD)': {
      'swarming_dimensions': [
        {
          'gpu': '1002:6821',
          'hidpi': '1',
          'os': 'Mac-10.12.6',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Debug',
      'swarming': True,
      'os_type': 'mac',
    },
    'Mac FYI Experimental Release (Intel)': {
      'swarming_dimensions': [
        {
          'gpu': '8086:0a2e',
          'os': 'Mac-10.13.4',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'mac',
    },
    'Mac FYI Experimental Retina Release (AMD)': {
      'swarming_dimensions': [
        {
          'gpu': '1002:6821',
          'hidpi': '1',
          'os': 'Mac-10.13.4',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'mac',
    },
    'Mac FYI Experimental Retina Release (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': '10de:0fe9',
          'hidpi': '1',
          'os': 'Mac-10.13.4',
          'pool': 'Chrome-GPU',
        },
      ],
      # TODO(kbr): this separate dictionary is a hack to avoid generalizing the
      # "swarming_dimensions" handling in this generator script. When merging
      # this script with the one in src/testing/buildbot/, this will no longer
      # be necessary.
      'swarming_settings': {
        # There's only one bot of this type in the Swarming pool right now, so
        # we have to increase the default expiration time of 1 hour (3600
        # seconds) to prevent webgl2_conformance_tests' shards from timing out.
        'expiration': 10800,
      },
      'build_config': 'Release',
      # Even though this bot is a one-off, it's still in the Swarming pool.
      'swarming': True,
      'os_type': 'mac',
    },
    'Mac FYI GPU ASAN Release': {
      # This bot spawns jobs on multiple GPU types.
      'swarming_dimensions': [
        {
          'gpu': '8086:0a2e',
          'os': 'Mac-10.12.6',
        },
        {
          'gpu': '1002:6821',
          'hidpi': '1',
          'os': 'Mac-10.12.6',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'mac',
      'is_asan': True,
    },
    'Mac FYI dEQP Release AMD': {
      # This bot spawns jobs on multiple GPU types.
      'swarming_dimensions': [
        {
          'gpu': '1002:6821',
          'hidpi': '1',
          'os': 'Mac-10.12.6',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'mac',
      'type': Types.DEQP,
    },
    'Mac FYI dEQP Release Intel': {
      # This bot spawns jobs on multiple GPU types.
      'swarming_dimensions': [
        {
          'gpu': '8086:0a2e',
          'os': 'Mac-10.12.6',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'mac',
      'type': Types.DEQP,
    },
    'Linux FYI Release (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': LINUX_QUADRO_P400_STABLE_DRIVER,
          'os': 'Ubuntu',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'linux',
    },
    'Linux FYI Debug (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': LINUX_QUADRO_P400_STABLE_DRIVER,
          'os': 'Ubuntu',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Debug',
      'swarming': True,
      'os_type': 'linux',
    },
    'Linux FYI dEQP Release (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': LINUX_QUADRO_P400_STABLE_DRIVER,
          'os': 'Ubuntu',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'linux',
      'type': Types.DEQP,
    },
    'Linux FYI Release (Intel HD 630)': {
      'swarming_dimensions': [
        {
          'gpu': INTEL_HD_630,
          'os': 'Ubuntu',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'linux',
    },
    'Linux FYI dEQP Release (Intel HD 630)': {
      'swarming_dimensions': [
        {
          'gpu': INTEL_HD_630,
          'os': 'Ubuntu',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'linux',
      'type': Types.DEQP,
    },
    'Linux FYI Release (AMD R7 240)': {
      'swarming_dimensions': [
        {
          'gpu': '1002:6613',
          'os': 'Ubuntu',
          'pool': 'Chrome-GPU',
        },
      ],
      # TODO(kbr): this separate dictionary is a hack to avoid generalizing the
      # "swarming_dimensions" handling in this generator script. When merging
      # this script with the one in src/testing/buildbot/, this will no longer
      # be necessary.
      'swarming_settings': {
        # There's only one bot of this type in the Swarming pool right now, so
        # we have to increase the default expiration time of 1 hour (3600
        # seconds) to prevent webgl2_conformance_tests' shards from timing out.
        'expiration': 10800,
      },
      'build_config': 'Release',
      # Even though this bot is a one-off, it's still in the Swarming pool.
      'swarming': True,
      'os_type': 'linux',
    },
    'Linux FYI GPU TSAN Release': {
      'swarming_dimensions': [
        {
          'gpu': LINUX_QUADRO_P400_STABLE_DRIVER,
          'os': 'Ubuntu',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'linux',
      'instrumentation_type': 'tsan',
    },
    'Linux FYI Ozone (Intel)': {
      'swarming_dimensions': [
        {
          'gpu': '8086:1912',
          'os': 'Ubuntu',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      # Even though this bot is a one-off, it's still in the Swarming pool.
      'swarming': True,
      'os_type': 'linux',
    },
    'Android FYI Release (Nexus 5)': {
      'swarming_dimensions': [
        {
          'device_type': 'hammerhead',
          'device_os': 'L',
          'os': 'Android',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'android-chromium',
      'swarming': True,
      'os_type': 'android',
    },
    'Android FYI Release (Nexus 5X)': {
      'swarming_dimensions': [
        {
          'device_type': 'bullhead',
          'device_os': 'MMB29Q',
          'os': 'Android'
        },
      ],
      'build_config': 'android-chromium',
      'swarming': True,
      'os_type': 'android',
    },
    'Android FYI Release (Nexus 6)': {
      'swarming_dimensions': [
        {
          'device_type': 'shamu',
          'device_os': 'L',
          'os': 'Android',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'android-chromium',
      'swarming': True,
      'os_type': 'android',
    },
    'Android FYI Release (Nexus 6P)': {
      'swarming_dimensions': [
        {
          'device_type': 'angler',
          'device_os': 'M',
          'os': 'Android',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'android-chromium',
      'swarming': True,
      'os_type': 'android',
    },
    'Android FYI Release (Nexus 9)': {
      'swarming_dimensions': [
        {
          'device_type': 'flounder',
          'device_os': 'M',
          'os': 'Android',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'android-chromium',
      'swarming': True,
      'os_type': 'android',
    },
    'Android FYI Release (NVIDIA Shield TV)': {
      'swarming_dimensions': [
        {
          'device_type': 'foster',
          'device_os': 'N',
          'os': 'Android',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'android-chromium',
      'swarming': True,
      'os_type': 'android',
    },
    'Android FYI dEQP Release (Nexus 5X)': {
      'swarming_dimensions': [
        {
          'device_type': 'bullhead',
          'device_os': 'MMB29Q',
          'os': 'Android'
        },
      ],
      'build_config': 'android-chromium',
      'swarming': True,
      'os_type': 'android',
      'type': Types.DEQP,
    },
    'Android FYI 32 Vk Release (Nexus 5X)': {
      'swarming_dimensions': [
        {
          'device_type': 'bullhead',
          'device_os': 'O',
          'os': 'Android',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'android-chromium',
      'swarming': True,
      'os_type': 'android',
    },
    'Android FYI 64 Vk Release (Nexus 5X)': {
      'swarming_dimensions': [
        {
          'device_type': 'bullhead',
          'device_os': 'O',
          'os': 'Android',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'android-chromium',
      'swarming': True,
      'os_type': 'android',
    },
    'Android FYI 32 dEQP Vk Release (Nexus 5X)': {
      'swarming_dimensions': [
        {
          'device_type': 'bullhead',
          'device_os': 'O',
          'os': 'Android',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'android-chromium',
      'swarming': True,
      'os_type': 'android',
      'type': Types.DEQP,
    },
    'Android FYI 64 dEQP Vk Release (Nexus 5X)': {
      'swarming_dimensions': [
        {
          'device_type': 'bullhead',
          'device_os': 'O',
          'os': 'Android',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'android-chromium',
      'swarming': True,
      'os_type': 'android',
      'type': Types.DEQP,
    },

    # The following "optional" testers don't actually exist on the
    # waterfall. They are present here merely to specify additional
    # tests which aren't on the main tryservers. Unfortunately we need
    # a completely different (redundant) bot specification to handle
    # this.

    'Optional Win10 Release (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': WIN10_NVIDIA_QUADRO_P400_STABLE_DRIVER,
          'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'win',
      'type': Types.OPTIONAL,
      'use_gpu_trigger_script': True,
    },
    'Optional Win10 Release (Intel HD 630)': {
      'swarming_dimensions': [
        {
          'gpu': WIN10_INTEL_HD_630_STABLE_DRIVER,
          'os': 'Windows-10',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'win',
      'type': Types.OPTIONAL,
    },
    'Optional Mac Release (Intel)': {
      'swarming_dimensions': [
        {
          'gpu': '8086:0a2e',
          'os': 'Mac-10.12.6',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'mac',
      'type': Types.OPTIONAL,
    },
    'Optional Mac Retina Release (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': '10de:0fe9',
          'hidpi': '1',
          'os': 'Mac-10.12.6',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'mac',
      'type': Types.OPTIONAL,
    },
    'Optional Mac Retina Release (AMD)': {
      'swarming_dimensions': [
        {
          'gpu': '1002:6821',
          'hidpi': '1',
          'os': 'Mac-10.12.6',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'mac',
      'type': Types.OPTIONAL,
    },
    'Optional Linux Release (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': LINUX_QUADRO_P400_STABLE_DRIVER,
          'os': 'Ubuntu',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'linux',
      'type': Types.OPTIONAL,
    },
    'Optional Linux Release (Intel HD 630)': {
      'swarming_dimensions': [
        {
          'gpu': INTEL_HD_630,
          'os': 'Ubuntu',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'linux',
      'type': Types.OPTIONAL,
    },
    'Optional Android Release (Nexus 5X)': {
      'swarming_dimensions': [
        {
          'device_type': 'bullhead',
          'device_os': 'MMB29Q',
          'os': 'Android'
        },
      ],
      'build_config': 'android-chromium',
      'swarming': True,
      'os_type': 'android',
      'type': Types.NOOP,
    },
    # This tryserver doesn't actually exist; it's a separate
    # configuration from the Win AMD bot on this waterfall because we
    # don't have enough tryserver capacity to run all the tests from
    # that bot on win_angle_rel_ng.
    'Win7 ANGLE Tryserver (AMD)': {
      'swarming_dimensions': [
        {
          'gpu': '1002:6613',
          'os': 'Windows-2008ServerR2-SP1',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'win',
      'type': Types.WIN_ANGLE_AMD_TRYSERVER,
    },
  }
}

V8_FYI_WATERFALL = {
  'name': 'client.v8.fyi',
  'type': Types.V8_FYI,

  'prologue': {
    "V8 Android GN (dbg)": {
      "additional_compile_targets": [
        "chrome_public_apk"
      ],
      "gtest_tests": []
    },
    "V8 Linux GN": {
      "additional_compile_targets": [
        "accessibility_unittests",
        "aura_unittests",
        "browser_tests",
        "cacheinvalidation_unittests",
        "capture_unittests",
        "cast_unittests",
        "cc_unittests",
        "chromedriver_unittests",
        "components_browsertests",
        "components_unittests",
        "content_browsertests",
        "content_unittests",
        "crypto_unittests",
        "dbus_unittests",
        "device_unittests",
        "display_unittests",
        "events_unittests",
        "extensions_browsertests",
        "extensions_unittests",
        "gcm_unit_tests",
        "gfx_unittests",
        "gn_unittests",
        "google_apis_unittests",
        "gpu_unittests",
        "interactive_ui_tests",
        "ipc_tests",
        "jingle_unittests",
        "media_unittests",
        "media_blink_unittests",
        "mojo_unittests",
        "nacl_loader_unittests",
        "net_unittests",
        "pdf_unittests",
        "ppapi_unittests",
        "printing_unittests",
        "remoting_unittests",
        "sandbox_linux_unittests",
        "skia_unittests",
        "sql_unittests",
        "storage_unittests",
        "sync_integration_tests",
        "ui_base_unittests",
        "ui_touch_selection_unittests",
        "unit_tests",
        "url_unittests",
        "views_unittests",
        "wm_unittests"
      ]
    }
  },
  'testers': {
    'Win V8 FYI Release (NVIDIA)': {
      'swarming_dimensions': [
        {
          # TODO(kbr): cut this bot over to Win10, coordinating with
          # V8 team.
          'gpu': WIN7_NVIDIA_QUADRO_P400_STABLE_DRIVER,
          'os': 'Windows-2008ServerR2-SP1',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'win',
    },
    'Mac V8 FYI Release (Intel)': {
      'swarming_dimensions': [
        {
          'gpu': '8086:0a2e',
          'os': 'Mac-10.12.6',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'mac',
    },
    'Linux V8 FYI Release (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': LINUX_QUADRO_P400_STABLE_DRIVER,
          'os': 'Ubuntu',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'linux',
    },
    'Linux V8 FYI Release - concurrent marking (NVIDIA)': {
      'swarming_dimensions': [
        {
          'gpu': LINUX_QUADRO_P400_STABLE_DRIVER,
          'os': 'Ubuntu',
          'pool': 'Chrome-GPU',
        },
      ],
      'build_config': 'Release',
      'swarming': True,
      'os_type': 'linux',
    },
    'Android V8 FYI Release (Nexus 5X)': {
      'swarming_dimensions': [
        {
          'device_type': 'bullhead',
          'device_os': 'MMB29Q',
          'os': 'Android'
        },
      ],
      'build_config': 'android-chromium',
      'swarming': True,
      'os_type': 'android',
    },
  }
}

COMMON_GTESTS = {
  'angle_deqp_egl_tests': {
    'tester_configs': [
      {
        'predicate': Predicates.DEQP,
        # Run only on the Win10 Release NVIDIA 32- and 64-bit bots
        # (and trybots) for the time being, at least until more capacity is
        # added.
        # TODO(jmadill): Run on the Linux Release NVIDIA bots.
        'build_configs': ['Release', 'Release_x64'],
        'swarming_dimension_sets': [
          {
            'gpu': NVIDIA_QUADRO_P400_ALL_DRIVERS,
            'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          }
        ],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
    ],
    'swarming': {
      'shards': 4,
    },
    'args': [
      '--test-launcher-batch-limit=400'
    ]
  },

  'angle_deqp_gles2_d3d11_tests': {
    'tester_configs': [
      {
        'predicate': Predicates.DEQP,
        'swarming_dimension_sets': [
          # NVIDIA Win 10
          {
            'gpu': NVIDIA_QUADRO_P400_ALL_DRIVERS,
            'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          },
          # AMD Win 7
          {
            'gpu': '1002:6613',
            'os': 'Windows-2008ServerR2-SP1'
          },
          # Intel Win 10
          {
            'gpu': WIN10_INTEL_HD_630_STABLE_DRIVER,
            'os': 'Windows-10',
          },
        ],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
    ],
    'desktop_swarming': {
      'shards': 4,
    },
    'test': 'angle_deqp_gles2_tests',
    'args': [
      '--test-launcher-batch-limit=400',
      '--deqp-egl-display-type=angle-d3d11'
    ]
  },

  'angle_deqp_gles2_gl_tests': {
    'tester_configs': [
      {
        'predicate': Predicates.DEQP,
        'swarming_dimension_sets': [
          # Linux NVIDIA Quadro P400
          {
            'gpu': LINUX_QUADRO_P400_STABLE_DRIVER,
            'os': 'Ubuntu'
          },
          # Linux Intel HD 630
          {
            'gpu': INTEL_HD_630,
            'os': 'Ubuntu'
          },
          # Mac Intel
          {
            'gpu': '8086:0a2e',
            'os': 'Mac-10.12.6'
          },
          # Mac AMD
          {
            'gpu': '1002:6821',
            'hidpi': '1',
            'os': 'Mac-10.12.6'
          },
        ],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
    ],
    'desktop_swarming': {
      'shards': 4,
    },
    'test': 'angle_deqp_gles2_tests',
    'args': [
      '--test-launcher-batch-limit=400',
      '--deqp-egl-display-type=angle-gl'
    ]
  },

  'angle_deqp_gles2_gles_tests': {
    'tester_configs': [
      {
        'predicate': Predicates.DEQP,
        # Run on Nexus 5X swarmed bots.
        'build_configs': ['android-chromium'],
        'swarming_dimension_sets': [
          # Nexus 5X
          {
            'device_type': 'bullhead',
            'device_os': 'MMB29Q',
            'os': 'Android'
          }
        ],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
    ],
    'swarming': {
      'shards': 4,
    },
    'test': 'angle_deqp_gles2_tests',
    'args': [
      '--deqp-egl-display-type=angle-gles'
    ],
    'desktop_args': [
      '--test-launcher-batch-limit=400',
    ],
    'android_args': [
      '--enable-xml-result-parsing',
      '--shard-timeout=500'
    ],
  },

  'angle_deqp_gles2_vulkan_tests': {
    'tester_configs': [
      {
        'predicate': Predicates.DEQP,
        'swarming_dimension_sets': [
          # NVIDIA Win 10
          {
            'gpu': NVIDIA_QUADRO_P400_ALL_DRIVERS,
            'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          },
          # AMD Win 7
          {
            'gpu': '1002:6613',
            'os': 'Windows-2008ServerR2-SP1'
          },
          # NVIDIA Linux Quadro P400
          {
            'gpu': LINUX_QUADRO_P400_STABLE_DRIVER,
            'os': 'Ubuntu'
          },
          # Qualcomm Android Adreno 418
          {
            'device_type': 'bullhead',
            'device_os': 'O',
            'os': 'Android',
            'pool': 'Chrome-GPU',
          },
        ],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
    ],
    'swarming': {
      'shards': 4,
    },
    'test': 'angle_deqp_gles2_tests',
    'args': [
      '--deqp-egl-display-type=angle-vulkan'
    ],
    'desktop_args': [
      '--test-launcher-batch-limit=400',
    ],
    'android_args': [
      '--enable-xml-result-parsing',
      '--shard-timeout=500'
    ],
  },

  'angle_deqp_gles3_gles_tests': {
    'tester_configs': [
      {
        'predicate': Predicates.DEQP,
        # Run on Nexus 5X swarmed bots.
        'build_configs': ['android-chromium'],
        'swarming_dimension_sets': [
          # Nexus 5X
          {
            'device_type': 'bullhead',
            'device_os': 'MMB29Q',
            'os': 'Android'
          }
        ],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
    ],
    'swarming': {
      'shards': 12,
    },
    'test': 'angle_deqp_gles3_tests',
    # Only pass the display type to desktop. The Android runner doesn't support
    # passing args to the executable but only one display type is supported on
    # Android anyways.
    'desktop_args': [
      '--test-launcher-batch-limit=400',
      '--deqp-egl-display-type=angle-gles'
    ],
    'android_args': [
      '--enable-xml-result-parsing',
      '--shard-timeout=500'
    ],
  },

  'angle_deqp_gles3_d3d11_tests': {
    'tester_configs': [
      {
        # TODO(jmadill): Run this on ANGLE roll tryservers.
        'predicate': Predicates.DEQP,
        'swarming_dimension_sets': [
          # NVIDIA Win 10
          {
            'gpu': NVIDIA_QUADRO_P400_ALL_DRIVERS,
            'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          },
          # AMD Win 7
          # Temporarily disabled to prevent a recipe engine crash.
          # TODO(jmadill): Re-enable when http://crbug.com713196 is fixed.
          # {
          #   'gpu': '1002:6613',
          #   'os': 'Windows-2008ServerR2-SP1'
          # },
        ],
      }
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
    ],
    'swarming': {
      'shards': 12,
    },
    'test': 'angle_deqp_gles3_tests',
    'args': [
      '--test-launcher-batch-limit=400',
      '--deqp-egl-display-type=angle-d3d11'
    ]
  },

  'angle_deqp_gles3_gl_tests': {
    'tester_configs': [
      {
        # TODO(jmadill): Run this on ANGLE roll tryservers.
        'predicate': Predicates.DEQP,
        'swarming_dimension_sets': [
          # NVIDIA Linux Quadro P400
          {
            'gpu': LINUX_QUADRO_P400_STABLE_DRIVER,
            'os': 'Ubuntu'
          },
          # Mac Intel
          {
            'gpu': '8086:0a2e',
            'os': 'Mac-10.12.6'
          },
          # Mac AMD
          {
            'gpu': '1002:6821',
            'hidpi': '1',
            'os': 'Mac-10.12.6'
          },
        ],
      }
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
    ],
    'swarming': {
      'shards': 12,
    },
    'test': 'angle_deqp_gles3_tests',
    'args': [
      '--test-launcher-batch-limit=400',
      '--deqp-egl-display-type=angle-gl'
    ]
  },

  'angle_deqp_gles31_d3d11_tests': {
    'tester_configs': [
      {
        'predicate': Predicates.DEQP,
        'swarming_dimension_sets': [
          {
            'gpu': NVIDIA_QUADRO_P400_ALL_DRIVERS,
            'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          }
        ],
      }
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
    ],
    'swarming': {
      'shards': 6,
    },
    'test': 'angle_deqp_gles31_tests',
    'args': [
      '--test-launcher-batch-limit=400',
      '--deqp-egl-display-type=angle-d3d11'
    ]
  },

  'angle_deqp_gles31_gl_tests': {
    'tester_configs': [
      {
        'predicate': Predicates.DEQP,
        'swarming_dimension_sets': [
          {
            'gpu': NVIDIA_QUADRO_P400_ALL_DRIVERS,
            'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          },
          {
            'gpu': LINUX_QUADRO_P400_STABLE_DRIVER,
            'os': 'Ubuntu'
          }
        ],
      }
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
    ],
    'swarming': {
      'shards': 6,
    },
    'test': 'angle_deqp_gles31_tests',
    'args': [
      '--test-launcher-batch-limit=400',
      '--deqp-egl-display-type=angle-gl'
    ]
  },

  # Until we have more capacity, run angle_end2end_tests only on the
  # FYI waterfall, the ANGLE trybots (which mirror the FYI waterfall),
  # and the optional trybots (mainly used during ANGLE rolls).
  'angle_end2end_tests': {
    'tester_configs': [
      {
        'predicate': Predicates.FYI_AND_OPTIONAL_AND_WIN_ANGLE_AMD,
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          # TODO(ynovikov) Investigate why the test breaks on older devices.
          'Android FYI Release (Nexus 5)',
          'Android FYI Release (Nexus 6)',
          'Android FYI Release (Nexus 9)',
          # Temporarily disabled due to AMDGPU-PRO issues crbug.com/786219
          'Linux FYI Release (AMD R7 240)',
        ],
      },
    ],
    'desktop_args': [
      '--use-gpu-in-tests',
      # ANGLE test retries deliberately disabled to prevent flakiness.
      # http://crbug.com/669196
      '--test-launcher-retry-limit=0'
    ]
  },
  # white_box tests should run where end2end tests run
  'angle_white_box_tests': {
    'tester_configs': [
      {
        'predicate': Predicates.FYI_AND_OPTIONAL_AND_WIN_ANGLE_AMD,
        'os_types': ['win', 'linux'],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
    ],
    'desktop_args': [
      # ANGLE test retries deliberately disabled to prevent flakiness.
      # http://crbug.com/669196
      '--test-launcher-retry-limit=0'
    ]
  },
  'angle_unittests': {
    'disabled_tester_configs': [
      {
        'names': [
          # On Android, these are already run on the main waterfall.
          # Run them on the one-off Android FYI bots, though.
          'Android Release (Nexus 5X)',
          'Android FYI Release (Nexus 5X)',
        ],
      },
    ],
    'desktop_args': [
      '--use-gpu-in-tests',
      # ANGLE test retries deliberately disabled to prevent flakiness.
      # http://crbug.com/669196
      '--test-launcher-retry-limit=0'
    ],
    'linux_args': [ '--no-xvfb' ]
  },
  'gl_tests': {
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          'Android FYI 32 Vk Release (Nexus 5X)',
          'Android FYI 64 Vk Release (Nexus 5X)',
          # On Android, these are already run on the main waterfall.
          # Run them on the one-off Android FYI bots, though.
          'Android Release (Nexus 5X)',
          'Android FYI Release (Nexus 5X)',
        ],
      },
    ],
    'desktop_args': [
      '--use-gpu-in-tests',
      '--use-cmd-decoder=validating',
    ]
  },
  'gl_tests_passthrough': {
    'tester_configs': [
      {
        'os_types': ['win'],
      }
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
    ],
    'test': 'gl_tests',
    'desktop_args': [
      '--use-gpu-in-tests',
      '--use-cmd-decoder=passthrough',
     ]
  },
  'gl_unittests': {
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          'Android FYI 32 Vk Release (Nexus 5X)',
          'Android FYI 64 Vk Release (Nexus 5X)',
          # On Android, these are already run on the main waterfall.
          # Run them on the one-off Android FYI bots, though.
          'Android Release (Nexus 5X)',
          'Android FYI Release (Nexus 5X)',
          # Temporarily disabled due to AMDGPU-PRO issues crbug.com/786219
          'Linux FYI Release (AMD R7 240)',
        ],
      },
    ],
    'desktop_args': ['--use-gpu-in-tests'],
    'linux_args': [ '--no-xvfb' ]
  },
  'gpu_unittests': {
    'tester_configs': [
      {
        # Run this on the FYI waterfall and optional tryservers.
        'predicate': Predicates.FYI_AND_OPTIONAL,
        # gpu_unittests is killing the Swarmed Linux GPU bots
        # similarly to how content_unittests was:
        # http://crbug.com/763498 .
        'os_types': ['win', 'mac', 'android'],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          'Android FYI 32 Vk Release (Nexus 5X)',
          'Android FYI 64 Vk Release (Nexus 5X)',
        ],
      },
    ],
  },
  # The gles1_conform_tests are closed-source and deliberately only run
  # on the FYI waterfall.
  'angle_gles1_conformance_tests': {
    'tester_configs': [
      {
        # Run this on the FYI waterfall and ANGLE tryservers.
        'predicate': Predicates.FYI_ONLY,
        'os_types': ['win'],
      }
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
    ],
    'args': ['--use-gpu-in-tests', ]
  },
  # The gles2_conform_tests are closed-source and deliberately only run
  # on the FYI waterfall and the optional tryservers.
  'gles2_conform_test': {
    'tester_configs': [
      {
        # Run this on the FYI waterfall and optional tryservers.
        'predicate': Predicates.FYI_AND_OPTIONAL,
      }
    ],
    # Don't run these tests on Android.
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
      {
        'os_types': ['android'],
      },
    ],
    'args': ['--use-gpu-in-tests']
  },
  'gles2_conform_d3d9_test': {
    'tester_configs': [
      {
        # Run this on the FYI waterfall and optional tryservers.
        'predicate': Predicates.FYI_AND_OPTIONAL,
        'os_types': ['win'],
      }
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
    ],
    'args': [
      '--use-gpu-in-tests',
      '--use-angle=d3d9',
    ],
    'test': 'gles2_conform_test',
  },
  'gles2_conform_gl_test': {
    'tester_configs': [
      {
        # Run this on the FYI waterfall and optional tryservers.
        'predicate': Predicates.FYI_AND_OPTIONAL,
        'os_types': ['win'],
      }
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
    ],
    'args': [
      '--use-gpu-in-tests',
      '--use-angle=gl',
      '--disable-gpu-sandbox',
    ],
    'test': 'gles2_conform_test',
  },
  # Face and barcode detection unit tests, which currently only run on
  # Mac OS, and require physical hardware.
  'services_unittests': {
    'tester_configs': [
      {
        # Run this on the FYI waterfall and optional tryservers.
        'predicate': Predicates.FYI_AND_OPTIONAL,
        'os_types': ['mac'],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
      {
        'swarming_dimension_sets': [
          # These tests fail on the Mac Pros.
          {
            'gpu': '1002:679e',
          },
        ],
      },
    ],
    'args': [
      '--gtest_filter=*Detection*',
      '--use-gpu-in-tests'
    ]
  },
  'swiftshader_unittests': {
    'tester_configs': [
      {
        # Run this on the FYI waterfall and optional tryservers.
        'predicate': Predicates.FYI_AND_OPTIONAL,
        'os_types': ['win', 'linux', 'mac'],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          'Mac FYI Experimental Release (Intel)',
          'Mac FYI Experimental Retina Release (AMD)',
          'Mac FYI Experimental Retina Release (NVIDIA)',
          'Mac Pro FYI Release (AMD)',
        ],
      },
    ],
  },
  'tab_capture_end2end_tests': {
    'tester_configs': [
      {
        'build_configs': ['Release', 'Release_x64'],
        'disabled_instrumentation_types': ['tsan'],
      }
    ],
    # Don't run these tests on Android.
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
        'os_types': ['android'],
      },
    ],
    'args': [
      '--enable-gpu',
      '--test-launcher-bot-mode',
      '--test-launcher-jobs=1',
      '--gtest_filter=CastStreamingApiTestWithPixelOutput.EndToEnd*:' + \
          'TabCaptureApiPixelTest.EndToEnd*'
    ],
    'linux_args': [ '--no-xvfb' ],
    'test': 'browser_tests',
  },
  'video_decode_accelerator_d3d11_unittest': {
    'tester_configs': [
      {
        'os_types': ['win']
      },
    ],
    'args': [
      '--use-angle=d3d11',
      '--use-test-data-path',
      '--test_video_data=test-25fps.h264:320:240:250:258:::1',
    ],
    'test': 'video_decode_accelerator_unittest',
  },
  'video_decode_accelerator_d3d9_unittest': {
    'tester_configs': [
      {
        # Run this on the FYI waterfall and optional tryservers.
        'predicate': Predicates.FYI_ONLY,
        'os_types': ['win']
      },
    ],
    'args': [
      '--use-angle=d3d9',
      '--use-test-data-path',
      '--test_video_data=test-25fps.h264:320:240:250:258:::1',
    ],
    'test': 'video_decode_accelerator_unittest',
  },
  'video_decode_accelerator_gl_unittest': {
    'tester_configs': [
      {
        # Run this on the FYI waterfall and optional tryservers.
        'predicate': Predicates.FYI_ONLY,
        'os_types': ['win']
      },
    ],
    # Windows Intel doesn't have the GL extensions to support this test
    'disabled_tester_configs': [
      {
        'names': [
          'Win10 FYI Release (Intel HD 630)',
          'Win10 FYI Exp Release (Intel HD 630)',
        ],
      },
    ],
    'args': [
      '--use-angle=gl',
      '--use-test-data-path',
      '--test_video_data=test-25fps.h264:320:240:250:258:::1',
    ],
    'test': 'video_decode_accelerator_unittest',
  },
  'vr_browser_tests': {
    'tester_configs': [
      {
        'predicate': Predicates.DEFAULT_AND_OPTIONAL,
        'os_types': ['win'],
        'swarming_dimension_sets': [
          {
            'gpu': NVIDIA_QUADRO_P400_ALL_DRIVERS,
            'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          },
          {
            'gpu': WIN10_INTEL_HD_630_STABLE_DRIVER,
            'os': 'Windows-10',
          }
        ],
      },
    ],
    'args': [
      '--enable-gpu',
      '--test-launcher-bot-mode',
      '--test-launcher-jobs=1',
      '--gtest_filter=VrBrowserTest*:XrBrowserTest*',
      '--enable-pixel-output-in-tests',
      '--gtest_also_run_disabled_tests',
    ],
    'test': 'browser_tests',
  }
}

# This requires a hack because the isolate's name is different than
# the executable's name. On the few non-swarmed testers, this causes
# the executable to not be found. It would be better if the Chromium
# recipe supported running isolates locally. crbug.com/581953

NON_SWARMED_GTESTS = {
  'tab_capture_end2end_tests': {
    'swarming': {
      'can_use_on_swarming_builders': False
    },
    # Don't run these tests on Android.
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
        'os_types': ['android'],
      },
    ],
    'test': 'browser_tests',
    'args': [
      '--enable-gpu',
      '--no-xvfb',
      '--test-launcher-jobs=1',
      '--gtest_filter=CastStreamingApiTestWithPixelOutput.EndToEnd*:' + \
          'TabCaptureApiPixelTest.EndToEnd*'
    ],
    'swarming': {
      'can_use_on_swarming_builders': False,
    },
  }
}

# These tests use Telemetry's new browser_test_runner, which is a much
# simpler harness for correctness testing.
TELEMETRY_GPU_INTEGRATION_TESTS = {
  'context_lost': {
    'tester_configs': [
      {
        'predicate': Predicates.DEFAULT_PLUS_V8,
        'disabled_instrumentation_types': ['tsan'],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          'Android FYI 32 Vk Release (Nexus 5X)',
          'Android FYI 64 Vk Release (Nexus 5X)',
        ],
      },
    ],
  },
  'depth_capture': {
    'tester_configs': [
      {
        'predicate': Predicates.DEFAULT_PLUS_V8,
        'disabled_instrumentation_types': ['tsan'],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          'Android FYI 32 Vk Release (Nexus 5X)',
          'Android FYI 64 Vk Release (Nexus 5X)',
        ],
      },
    ],
  },
  'gpu_process_launch_tests': {
    'target_name': 'gpu_process',
    'tester_configs': [
      {
        'predicate': Predicates.DEFAULT_PLUS_V8,
        'disabled_instrumentation_types': ['tsan'],
      }
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          'Android FYI 32 Vk Release (Nexus 5X)',
          'Android FYI 64 Vk Release (Nexus 5X)',
        ],
      },
    ],
  },
  'hardware_accelerated_feature': {
    'tester_configs': [
      {
        'predicate': Predicates.DEFAULT_PLUS_V8,
        'disabled_instrumentation_types': ['tsan'],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          'Android FYI 32 Vk Release (Nexus 5X)',
          'Android FYI 64 Vk Release (Nexus 5X)',
        ],
      },
    ],
  },
  'info_collection_tests': {
    'target_name': 'info_collection',
    'args': [
      '--expected-vendor-id',
      '${gpu_vendor_id}',
      '--expected-device-id',
      '${gpu_device_id}',
    ],
    'tester_configs': [
      {
        'predicate': Predicates.DEFAULT_AND_OPTIONAL,
        'disabled_instrumentation_types': ['tsan'],
      }
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          'Android FYI 32 Vk Release (Nexus 5X)',
          'Android FYI 64 Vk Release (Nexus 5X)',

          # The Mac ASAN swarming runs on two different GPU types so we can't
          # have one expected vendor ID / device ID
          'Mac FYI GPU ASAN Release',
        ],
      },
    ],
  },
  'maps_pixel_test': {
    'target_name': 'maps',
    'args': [
      '--dont-restore-color-profile-after-test',
      '--os-type',
      '${os_type}',
      '--build-revision',
      '${got_revision}',
      '--test-machine-name',
      '${buildername}',
    ],
    'tester_configs': [
      {
        'predicate': Predicates.DEFAULT_PLUS_V8,
        'disabled_instrumentation_types': ['tsan'],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          'Android FYI 32 Vk Release (Nexus 5X)',
          'Android FYI 64 Vk Release (Nexus 5X)',
        ],
      },
    ],
  },
  'noop_sleep': {
    'tester_configs': [
      {
        # Only run the noop sleep test on experimental configs.
        'predicate': Predicates.EXPERIMENTAL_CONDITIONALLY,
        'disabled_instrumentation_types': ['tsan'],
      },
    ],
  },
  'pixel_test': {
    'target_name': 'pixel',
    'args': [
      '--dont-restore-color-profile-after-test',
      '--refimg-cloud-storage-bucket',
      'chromium-gpu-archive/reference-images',
      '--os-type',
      '${os_type}',
      '--build-revision',
      '${got_revision}',
      '--test-machine-name',
      '${buildername}',
    ],
    'non_precommit_args': [
      '--upload-refimg-to-cloud-storage',
    ],
    'precommit_args': [
      '--download-refimg-from-cloud-storage',
    ],
    'tester_configs': [
      {
        'predicate': Predicates.DEFAULT_PLUS_V8,
        'disabled_instrumentation_types': ['tsan'],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          'Android FYI 32 Vk Release (Nexus 5X)',
          'Android FYI 64 Vk Release (Nexus 5X)',
        ],
      },
    ],
  },
  'screenshot_sync': {
    'args': [
      '--dont-restore-color-profile-after-test',
    ],
    'tester_configs': [
      {
        'predicate': Predicates.DEFAULT_PLUS_V8,
        'disabled_instrumentation_types': ['tsan'],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          'Android FYI 32 Vk Release (Nexus 5X)',
          'Android FYI 64 Vk Release (Nexus 5X)',
        ],
      },
    ],
  },
  'trace_test': {
    'tester_configs': [
      {
        'predicate': Predicates.DEFAULT_PLUS_V8,
        'disabled_instrumentation_types': ['tsan'],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          'Android FYI 32 Vk Release (Nexus 5X)',
          'Android FYI 64 Vk Release (Nexus 5X)',
        ],
      },
    ],
  },
  'webgl_conformance': {
    'tester_configs': [
      {
        'predicate': Predicates.DEFAULT_PLUS_V8,
        'disabled_instrumentation_types': ['tsan'],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          'Android FYI 32 Vk Release (Nexus 5X)',
          'Android FYI 64 Vk Release (Nexus 5X)',
        ],
      },
    ],
    'extra_browser_args': [
      '--use-cmd-decoder=validating',
    ],
    'asan_args': ['--is-asan'],
    'android_swarming': {
      # On desktop platforms these don't take very long (~7 minutes),
      # but on Android they take ~30 minutes and we want to shard them
      # when sharding is available -- specifically on the Nexus 5X
      # bots, which are currently the only Android configuration on
      # the waterfalls where these tests are swarmed. If we had to
      # restrict the sharding to certain Android devices, then we'd
      # need some way to apply these Swarming parameters only to a
      # subset of machines, like the way the tester_configs work.
      'shards': 6,
    },
    'android_args': [
      # The current working directory when run via isolate is
      # out/Debug or out/Release. Reference this file relatively to
      # it.
      '--read-abbreviated-json-results-from=' + \
      '../../content/test/data/gpu/webgl_conformance_tests_output.json',
    ],
    'swarming': {
      'shards': 2,
    },
  },
  'webgl_conformance_d3d9_tests': {
    'tester_configs': [
      {
        # Run this on the FYI waterfall and optional tryservers.
        'predicate': Predicates.FYI_AND_OPTIONAL,
        'os_types': ['win'],
        'disabled_instrumentation_types': ['tsan'],
      }
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          # TODO(jdarpinian): Re-enable when http://crbug.com/841789 is fixed.
          'Win10 FYI Exp Release (Intel HD 630)',
        ],
      },
    ],
    'target_name': 'webgl_conformance',
    'extra_browser_args': [
      '--use-angle=d3d9',
      '--use-cmd-decoder=validating',
    ],
    'asan_args': ['--is-asan'],
    'swarming': {
      'shards': 2,
    },
  },
  'webgl_conformance_d3d9_passthrough': {
    'tester_configs': [
      {
        # Run this on the FYI waterfall, optional tryservers, and Win
        # ANGLE AMD tryserver.
        'predicate': Predicates.FYI_AND_OPTIONAL_AND_WIN_ANGLE_AMD,
        'os_types': ['win'],
        'disabled_instrumentation_types': ['tsan'],
      }
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          # TODO(jdarpinian): Re-enable when http://crbug.com/841789 is fixed.
          'Win10 FYI Exp Release (Intel HD 630)',
        ],
      },
    ],
    'target_name': 'webgl_conformance',
    'extra_browser_args': [
      '--use-angle=d3d9',
      '--use-cmd-decoder=passthrough',
    ],
    'asan_args': ['--is-asan'],
    'swarming': {
      'shards': 2,
    },
  },
  'webgl_conformance_d3d11_passthrough': {
    'tester_configs': [
      {
        # Run this on the FYI waterfall, optional tryservers, and Win
        # ANGLE AMD tryserver.
        'predicate': Predicates.FYI_AND_OPTIONAL_AND_WIN_ANGLE_AMD,
        'os_types': ['win'],
        'disabled_instrumentation_types': ['tsan'],
      }
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
    ],
    'target_name': 'webgl_conformance',
    'extra_browser_args': [
      '--use-angle=d3d11',
      '--use-cmd-decoder=passthrough',
    ],
    'asan_args': ['--is-asan'],
    'swarming': {
      'shards': 2,
    },
  },
  'webgl_conformance_gl_passthrough': {
    'tester_configs': [
      {
        # Run this on the FYI waterfall and optional tryservers.
        'predicate': Predicates.FYI_AND_OPTIONAL,
        'os_types': ['linux', 'win'],
        'disabled_instrumentation_types': ['tsan'],
      }
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
      {
        'swarming_dimension_sets': [
          # crbug.com/555545 and crbug.com/649824:
          # Disable webgl_conformance_gl_tests on some Win/AMD cards.
          # Always fails on older cards, flaky on newer cards.
          # Note that these must match the GPUs exactly; wildcard
          # matches (i.e., only device ID) aren't supported!
          {
            'gpu': '1002:6779',
            'os': 'Windows-2008ServerR2-SP1'
          },
          {
            'gpu': '1002:6613',
            'os': 'Windows-2008ServerR2-SP1'
          },
          # BUG 590951: Disable webgl_conformance_gl_tests on Win/Intel
          {
            'gpu': '8086:041a',
            'os': 'Windows-2008ServerR2-SP1'
          },
          {
            'gpu': '8086:0412',
            'os': 'Windows-2008ServerR2-SP1'
          },
        ],
      },
    ],
    'target_name': 'webgl_conformance',
    'extra_browser_args': [
      '--use-gl=angle',
      '--use-angle=gl',
      '--use-cmd-decoder=passthrough',
    ],
    'asan_args': ['--is-asan'],
    'swarming': {
      'shards': 2,
    },
  },
  'webgl2_conformance_tests': {
    'tester_configs': [
      {
         # The WebGL 2.0 conformance tests take over an hour to run on
         # the Debug bots, which is too long.
        'build_configs': ['Release', 'Release_x64'],
        'predicate': Predicates.FYI_OPTIONAL_V8_AND_WIN_ANGLE_AMD,
        'disabled_instrumentation_types': ['tsan'],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',

          # The Mac NVIDIA Retina bots don't have the capacity to run
          # this test suite on mac_optional_gpu_tests_rel.
          'Optional Mac Retina Release (NVIDIA)',
        ],
        # Don't run these tests on Android yet.
        'os_types': ['android'],
      },
    ],
    'target_name': 'webgl_conformance',
    'extra_browser_args': [
      '--use-cmd-decoder=validating',
    ],
    'args': [
      '--webgl-conformance-version=2.0.1',
      # The current working directory when run via isolate is
      # out/Debug or out/Release. Reference this file relatively to
      # it.
      '--read-abbreviated-json-results-from=' + \
      '../../content/test/data/gpu/webgl2_conformance_tests_output.json',
    ],
    'asan_args': ['--is-asan'],
    'swarming': {
      # These tests currently take about an hour and fifteen minutes
      # to run. Split them into roughly 5-minute shards.
      'shards': 20,
    },
  },
  'webgl2_conformance_gl_passthrough_tests': {
    'tester_configs': [
      {
         # The WebGL 2.0 conformance tests take over an hour to run on
         # the Debug bots, which is too long.
        'build_configs': ['Release'],
        'predicate': Predicates.FYI_ONLY,
        # Run on the NVIDIA Release Windows/Linux and Intel Release Linux bots
        'swarming_dimension_sets': [
          {
            'gpu': LINUX_QUADRO_P400_STABLE_DRIVER,
            'os': 'Ubuntu'
          },
          {
            'gpu': '8086:0412',
            'os': 'Ubuntu'
          },
          {
            'gpu': '8086:1912',
            'os': 'Ubuntu'
          },
          {
            'gpu': NVIDIA_QUADRO_P400_ALL_DRIVERS,
            'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          },
        ],
        'disabled_instrumentation_types': ['tsan'],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
        ],
      },
    ],
    'target_name': 'webgl_conformance',
    'extra_browser_args': [
      '--use-gl=angle',
      '--use-angle=gl',
      '--use-cmd-decoder=passthrough',
    ],
    'args': [
      '--webgl-conformance-version=2.0.1',
      # The current working directory when run via isolate is
      # out/Debug or out/Release. Reference this file relatively to
      # it.
      '--read-abbreviated-json-results-from=' + \
      '../../content/test/data/gpu/webgl2_conformance_tests_output.json',
    ],
    'asan_args': ['--is-asan'],
    'swarming': {
      # These tests currently take about an hour and fifteen minutes
      # to run serially.
      'shards': 20,
    },
  },
  'webgl2_conformance_d3d11_passthrough_tests': {
    'tester_configs': [
      {
         # The WebGL 2.0 conformance tests take over an hour to run on
         # the Debug bots, which is too long.
        'build_configs': ['Release'],
        'predicate': Predicates.FYI_ONLY,
        # Only run on the NVIDIA Release Windows bots.
        'swarming_dimension_sets': [
          {
            'gpu': NVIDIA_QUADRO_P400_ALL_DRIVERS,
            'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          },
        ],
        'disabled_instrumentation_types': ['tsan'],
      },
    ],
    'target_name': 'webgl_conformance',
    'extra_browser_args': [
      '--use-angle=d3d11',
      '--use-cmd-decoder=passthrough',
    ],
    'args': [
      '--webgl-conformance-version=2.0.1',
      # The current working directory when run via isolate is
      # out/Debug or out/Release. Reference this file relatively to
      # it.
      '--read-abbreviated-json-results-from=' + \
      '../../content/test/data/gpu/webgl2_conformance_tests_output.json',
    ],
    'asan_args': ['--is-asan'],
    'swarming': {
      # These tests currently take about an hour and fifteen minutes
      # to run. Split them into roughly 5-minute shards.
      'shards': 20,
    },
  },
  'viz_screenshot_sync': {
    'target_name': 'screenshot_sync',
    'args': [
      '--dont-restore-color-profile-after-test',
    ],
    'extra_browser_args': [
      # This test confirms that GPU compositing is working with OOP-D.
      '--enable-features=VizDisplayCompositor',
    ],
    'tester_configs': [
      {
        'predicate': Predicates.DEFAULT,
        'disabled_instrumentation_types': ['tsan'],
        'os_types': ['win', 'linux'],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          # TODO(crbug.com/825906): Enable test on Win7 FYI bots when OOP-D +
          # passthrough command decoder bug is fixed.
          'Win7 FYI Release (NVIDIA)',
          'Win7 FYI Release (AMD)',
          'Win7 FYI Debug (AMD)',
          'Win7 FYI x64 Release (NVIDIA)',
        ],
      },
    ],
  },
}

# These isolated tests don't use telemetry. They need to be placed in the
# isolated_scripts section of the generated json.
NON_TELEMETRY_ISOLATED_SCRIPT_TESTS = {
  # We run perf tests on the CQ to ensure the tests don't crash.
  'angle_perftests': {
    'tester_configs': [
      {
        'predicate': Predicates.FYI_AND_OPTIONAL,
        # Run on the Win/Linux Release NVIDIA bots and Android
        'build_configs': ['Release', 'android-chromium'],
        'swarming_dimension_sets': [
          {
            'gpu': NVIDIA_QUADRO_P400_ALL_DRIVERS,
            'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          },
          {
            'gpu': LINUX_QUADRO_P400_STABLE_DRIVER,
            'os': 'Ubuntu'
          },
          {
            'os': 'Android'
          },
        ],
      },
    ],
    'disabled_tester_configs': [
      {
        'names': [
          'Linux FYI Ozone (Intel)',
          # anglebug.com/2433
          'Android FYI Release (Nexus 6)',
        ],
      },
    ],
    'args': [
      # Tell the tests to exit after one frame for faster iteration.
      '--one-frame-only',
    ],
  },

  'validating_command_buffer_perftests': {
    'tester_configs': [
      {
        'predicate': Predicates.FYI_AND_OPTIONAL,
        # Run on the Win Release NVIDIA bots.
        # TODO(jmadill): Run on Linux bots when possible.
        'build_configs': ['Release'],
        'swarming_dimension_sets': [
          {
            'gpu': NVIDIA_QUADRO_P400_ALL_DRIVERS,
            'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          },
        ],
      },
    ],
    'isolate_name': 'command_buffer_perftests',
    'args': [
      '--use-cmd-decoder=validating',
      '--use-stub',
    ],
  },

  'passthrough_command_buffer_perftests': {
    'tester_configs': [
      {
        'predicate': Predicates.FYI_AND_OPTIONAL,
        # Run on the Win Release NVIDIA bots.
        # TODO(jmadill): Run on Linux bots when possible.
        'build_configs': ['Release'],
        'swarming_dimension_sets': [
          {
            'gpu': NVIDIA_QUADRO_P400_ALL_DRIVERS,
            'os': WIN10_NVIDIA_QUADRO_P400_STABLE_OS,
          },
        ],
      },
    ],
    'isolate_name': 'command_buffer_perftests',
    'args': [
      '--use-cmd-decoder=passthrough',
      '--use-angle=gl-null',
    ],
  },
}

def substitute_args(tester_config, args):
  """Substitutes the variables in |args| from tester_config properties."""
  substitutions = {
    'os_type': tester_config['os_type'],
    'gpu_vendor_id': '0',
    'gpu_device_id': '0',
  }

  if 'gpu' in tester_config['swarming_dimensions'][0]:
    # First remove the driver version, then split into vendor and device.
    gpu = tester_config['swarming_dimensions'][0]['gpu']
    gpu = gpu.split('-')[0].split(':')
    substitutions['gpu_vendor_id'] = gpu[0]
    substitutions['gpu_device_id'] = gpu[1]

  return [string.Template(arg).safe_substitute(substitutions) for arg in args]

def matches_swarming_dimensions(tester_config, dimension_sets):
  for dimensions in dimension_sets:
    for cur_dims in tester_config['swarming_dimensions']:
      match = True
      for key, value in dimensions.iteritems():
        if key not in cur_dims:
          match = False
        elif value.endswith('*'):
          if not cur_dims[key].startswith(value[0:-1]):
            match = False
        elif cur_dims[key] != value:
          match = False
      if match:
        return True
  return False

def is_android(tester_config):
  return tester_config['os_type'] == 'android'

def is_linux(tester_config):
  return tester_config['os_type'] == 'linux'

def is_asan(tester_config):
  return tester_config.get('is_asan', False)


# Returns a list describing the type of this tester. It may include
# both the type of the bot as well as the waterfall.
def get_tester_type(tester_config):
  result = []
  if 'type' in tester_config:
    result.append(tester_config['type'])
  result.append(tester_config['parent']['type'])
  return result

def tester_config_matches_tester(tester_name, tester_config, tc,
                                 check_waterfall):
  if check_waterfall:
    if not tc.get('predicate', Predicates.DEFAULT)(
        get_tester_type(tester_config)):
      return False

  if 'names' in tc:
    # Give priority to matching the tester_name.
    if tester_name in tc['names']:
      return True
    if not tester_name in tc['names']:
      return False
  if 'os_types' in tc:
    if not tester_config['os_type'] in tc['os_types']:
      return False
  if 'instrumentation_type' in tester_config:
    if 'disabled_instrumentation_types' in tc:
      if tester_config['instrumentation_type'] in \
            tc['disabled_instrumentation_types']:
        return False
  if 'build_configs' in tc:
    if not tester_config['build_config'] in tc['build_configs']:
      return False
  if 'swarming_dimension_sets' in tc:
    if not matches_swarming_dimensions(tester_config,
                                       tc['swarming_dimension_sets']):
      return False
  return True


def experimental_config_matches_stable(waterfall, tester_config):
  stable_tester_name = tester_config['stable_tester_name']
  if not stable_tester_name in waterfall['testers']:
    raise Exception('Stable config "' + stable_tester_name + '" not found')

  stable_tester = waterfall['testers'][stable_tester_name]

  stable_config = stable_tester['swarming_dimensions'][0]
  experimental_config = tester_config['swarming_dimensions'][0]

  return experimental_config == stable_config


def is_test_config_experimental_conditionally(test_config):
  if 'tester_configs' in test_config:
    for tc in test_config['tester_configs']:
      pred = tc.get('predicate', Predicates.DEFAULT)
      if pred == Predicates.EXPERIMENTAL_CONDITIONALLY:
        return True
  return False

def should_run_on_tester(waterfall, tester_name, tester_config, test_config):
  # Special case for experimental tester configs. Don't run tests by default
  # if the experimental config matches the stable config.
  if Types.EXPERIMENTAL in get_tester_type(tester_config):
    # TODO(kbr): there's a bug here where if the experimental bot
    # doesn't match the stable bot, it runs too many tests because it
    # doesn't obey the test's predicate.
    is_conditional = is_test_config_experimental_conditionally(test_config)
    if experimental_config_matches_stable(waterfall, tester_config):
      return is_conditional
    else:
      return not is_conditional

  # Check if this config is disabled on this tester
  if 'disabled_tester_configs' in test_config:
    for dtc in test_config['disabled_tester_configs']:
      if tester_config_matches_tester(tester_name, tester_config, dtc, False):
        return False
  if 'tester_configs' in test_config:
    for tc in test_config['tester_configs']:
      if tester_config_matches_tester(tester_name, tester_config, tc, True):
        return True
    return False
  else:
    # If tester_configs is unspecified, run nearly all tests by default,
    # but let tester_config_matches_tester filter out any undesired
    # tests, such as ones that should only run on the Optional bots.
    return tester_config_matches_tester(tester_name, tester_config, {}, True)

def remove_tester_configs_from_result(result):
  if 'tester_configs' in result:
    # Don't print the tester_configs in the JSON.
    result.pop('tester_configs')
  if 'disabled_tester_configs' in result:
    # Don't print the disabled_tester_configs in the JSON.
    result.pop('disabled_tester_configs')

def add_common_test_properties(test, tester_config):
  if tester_config.get('use_gpu_trigger_script'):
    test['trigger_script'] = {
      'script': '//testing/trigger_scripts/trigger_multiple_dimensions.py',
      'args': [
        '--multiple-trigger-configs',
        json.dumps(tester_config['swarming_dimensions'] +
                   tester_config.get('alternate_swarming_dimensions', [])),
        '--multiple-dimension-script-verbose',
        'True'
      ],
    }

def generate_gtest(waterfall, tester_name, tester_config, test, test_config):
  if not should_run_on_tester(
      waterfall, tester_name, tester_config, test_config):
    return None
  result = copy.deepcopy(test_config)
  if 'test' in result:
    result['name'] = test
  else:
    result['test'] = test
  if (not tester_config['swarming']) and test in NON_SWARMED_GTESTS:
    # Need to override this result.
    result = copy.deepcopy(NON_SWARMED_GTESTS[test])
    result['name'] = test
  else:
    if not 'swarming' in result:
      result['swarming'] = {}
    result['swarming'].update({
      'can_use_on_swarming_builders': tester_config['swarming'],
      'dimension_sets': tester_config['swarming_dimensions']
    })
    if is_android(tester_config):
      # Integrate with the unified logcat system.
      result['swarming'].update({
        'cipd_packages': [
          {
            'cipd_package': 'infra/tools/luci/logdog/butler/${platform}',
            'location': 'bin',
            'revision': 'git_revision:ff387eadf445b24c935f1cf7d6ddd279f8a6b04c'
          }
        ],
        'output_links': [
          {
            'link': [
              'https://luci-logdog.appspot.com/v/?s',
              '=android%2Fswarming%2Flogcats%2F',
              '${TASK_ID}%2F%2B%2Funified_logcats'
            ],
            'name': 'shard #${SHARD_INDEX} logcats'
          }
        ]
      })

  def add_conditional_args(key, fn):
    if key in result:
      if fn(tester_config):
        if not 'args' in result:
          result['args'] = []
        result['args'] += result[key]
      # Don't put the conditional args in the JSON.
      result.pop(key)

  add_conditional_args('desktop_args', lambda cfg: not is_android(cfg))
  add_conditional_args('linux_args', is_linux)
  add_conditional_args('android_args', is_android)

  if 'desktop_swarming' in result:
    if not is_android(tester_config):
      result['swarming'].update(result['desktop_swarming'])
    # Don't put the desktop_swarming in the JSON.
    result.pop('desktop_swarming')
  # Remove the tester_configs and disabled_tester_configs, if present,
  # from the result.
  remove_tester_configs_from_result(result)

  add_common_test_properties(result, tester_config)
  return result

def generate_gtests(waterfall, tester_name, tester_config, test_dictionary):
  # The relative ordering of some of the tests is important to
  # minimize differences compared to the handwritten JSON files, since
  # Python's sorts are stable and there are some tests with the same
  # key (see gles2_conform_d3d9_test and similar variants). Avoid
  # losing the order by avoiding coalescing the dictionaries into one.
  gtests = []
  for test_name, test_config in sorted(test_dictionary.iteritems()):
    test = generate_gtest(waterfall, tester_name, tester_config,
                          test_name, test_config)
    if test:
      # generate_gtest may veto the test generation on this platform.
      gtests.append(test)
  return gtests

def generate_isolated_test(waterfall, tester_name, tester_config, test,
                           test_config, extra_browser_args, isolate_name,
                           prefix_args):
  if not should_run_on_tester(waterfall, tester_name, tester_config,
                              test_config):
    return None
  test_args = ['-v']
  extra_browser_args_string = ""
  if extra_browser_args != None:
    extra_browser_args_string += ' '.join(extra_browser_args)
  if 'extra_browser_args' in test_config:
    extra_browser_args_string += ' ' + ' '.join(
        test_config['extra_browser_args'])
  if extra_browser_args_string != "":
    test_args.append('--extra-browser-args=' + extra_browser_args_string)
  if 'args' in test_config:
    test_args.extend(substitute_args(tester_config, test_config['args']))
  if 'desktop_args' in test_config and not is_android(tester_config):
    test_args.extend(substitute_args(tester_config,
                                     test_config['desktop_args']))
  if 'android_args' in test_config and is_android(tester_config):
    test_args.extend(substitute_args(tester_config,
                                     test_config['android_args']))
  if 'asan_args' in test_config and is_asan(tester_config):
    test_args.extend(substitute_args(tester_config,
                                     test_config['asan_args']))
  # The step name must end in 'test' or 'tests' in order for the
  # results to automatically show up on the flakiness dashboard.
  # (At least, this was true some time ago.) Continue to use this
  # naming convention for the time being to minimize changes.
  step_name = test
  if not (step_name.endswith('test') or step_name.endswith('tests')):
    step_name = '%s_tests' % step_name
  # Prepend GPU-specific flags.
  swarming = {
    'can_use_on_swarming_builders': tester_config['swarming'],
    'dimension_sets': tester_config['swarming_dimensions']
  }
  if 'swarming_settings' in tester_config:
    swarming.update(tester_config['swarming_settings'])
  if 'swarming' in test_config:
    swarming.update(test_config['swarming'])
  if 'android_swarming' in test_config and is_android(tester_config):
    swarming.update(test_config['android_swarming'])
  result = {
    'args': prefix_args + test_args,
    'isolate_name': isolate_name,
    'name': step_name,
    'swarming': swarming,
  }
  if 'non_precommit_args' in test_config:
    result['non_precommit_args'] = test_config['non_precommit_args']
  if 'precommit_args' in test_config:
    result['precommit_args'] = test_config['precommit_args']
  add_common_test_properties(result, tester_config)
  return result

def generate_telemetry_test(waterfall, tester_name, tester_config,
                            test, test_config):
  extra_browser_args = ['--enable-logging=stderr', '--js-flags=--expose-gc']
  benchmark_name = test_config.get('target_name') or test
  prefix_args = [
    benchmark_name,
    '--show-stdout',
    '--browser=%s' % tester_config['build_config'].lower(),
    # --passthrough displays more of the logging in Telemetry when run
    # --via typ, in particular some of the warnings about tests being
    # --expected to fail, but passing.
    '--passthrough',
  ]
  return generate_isolated_test(waterfall, tester_name, tester_config, test,
                                test_config, extra_browser_args,
                                'telemetry_gpu_integration_test',
                                prefix_args)

def generate_telemetry_tests(waterfall, tester_name, tester_config,
                             test_dictionary):
  isolated_scripts = []
  for test_name, test_config in sorted(test_dictionary.iteritems()):
    test = generate_telemetry_test(waterfall,
      tester_name, tester_config, test_name, test_config)
    if test:
      isolated_scripts.append(test)
  return isolated_scripts

def generate_non_telemetry_isolated_test(waterfall, tester_name, tester_config,
                                         test, test_config):
  isolate_name = test
  if 'isolate_name' in test_config:
    isolate_name = test_config['isolate_name']
  return generate_isolated_test(waterfall, tester_name, tester_config, test,
                                test_config, None, isolate_name, [])

def generate_non_telemetry_isolated_tests(waterfall, tester_name, tester_config,
                                          test_dictionary):
  isolated_scripts = []
  for test_name, test_config in sorted(test_dictionary.iteritems()):
    test = generate_non_telemetry_isolated_test(waterfall,
      tester_name, tester_config, test_name, test_config)
    if test:
      isolated_scripts.append(test)
  return isolated_scripts

def install_parent_links(waterfall):
  # Make the testers point back to the top-level waterfall so that we
  # can ask about its properties when determining whether a given test
  # should run on a given waterfall.
  for name, config in waterfall.get('testers', {}).iteritems():
    config['parent'] = waterfall

def generate_all_tests(waterfall, filename):
  tests = {}
  for builder, config in waterfall.get('prologue', {}).iteritems():
    tests[builder] = config
  for builder, config in waterfall.get('builders', {}).iteritems():
    tests[builder] = config
  for name, config in waterfall['testers'].iteritems():
    gtests = generate_gtests(waterfall, name, config, COMMON_GTESTS)
    isolated_scripts = \
      generate_telemetry_tests(waterfall,
        name, config, TELEMETRY_GPU_INTEGRATION_TESTS) + \
      generate_non_telemetry_isolated_tests(waterfall, name, config,
        NON_TELEMETRY_ISOLATED_SCRIPT_TESTS)
    tests[name] = {
      'gtest_tests': sorted(gtests, key=lambda x: x['test']),
      'isolated_scripts': sorted(isolated_scripts, key=lambda x: x['name'])
    }
  tests['AAAAA1 AUTOGENERATED FILE DO NOT EDIT'] = {}
  tests['AAAAA2 See gpu/generate_buildbot_json.py to make changes'] = {}
  with open(os.path.join(SRC_DIR, 'testing', 'buildbot', filename), 'wb') as fp:
    json.dump(tests, fp, indent=2, separators=(',', ': '), sort_keys=True)
    fp.write('\n')

def main():
  install_parent_links(FYI_WATERFALL)
  install_parent_links(WATERFALL)
  install_parent_links(V8_FYI_WATERFALL)

  generate_all_tests(FYI_WATERFALL, 'chromium.gpu.fyi.json')
  generate_all_tests(WATERFALL, 'chromium.gpu.json')
  generate_all_tests(V8_FYI_WATERFALL, 'client.v8.fyi.json')
  return 0

if __name__ == "__main__":
  sys.exit(main())
