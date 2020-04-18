#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for generate_buildbot_json.py."""

import argparse
import os
import unittest

import generate_buildbot_json


class FakeBBGen(generate_buildbot_json.BBJSONGenerator):
  def __init__(self, waterfalls, test_suites, exceptions, luci_milo_cfg):
    super(FakeBBGen, self).__init__()
    self.files = {
      'waterfalls.pyl': waterfalls,
      'test_suites.pyl': test_suites,
      'test_suite_exceptions.pyl': exceptions,
      os.path.join( '..', '..', 'infra', 'config', 'global', 'luci-milo.cfg'):
          luci_milo_cfg,
    }

  def read_file(self, relative_path):
    return self.files[relative_path]

  def write_file(self, relative_path, contents):
    self.files[relative_path] = contents


FOO_GTESTS_WATERFALL = """\
[
  {
    'name': 'chromium.test',
    'machines': {
      'Fake Tester': {
        'swarming': {
          'dimension_sets': [
            {
              'kvm': '1',
            },
          ],
        },
        'test_suites': {
          'gtest_tests': 'foo_tests',
        },
      },
    },
  },
]
"""

FOO_GTESTS_WITH_ENABLE_FEATURES_WATERFALL = """\
[
  {
    'name': 'chromium.test',
    'machines': {
      'Fake Tester': {
        'test_suites': {
          'gtest_tests': 'foo_tests',
        },
        'args': [
          '--enable-features=Baz',
        ],
      },
    },
  },
]
"""

FOO_GTESTS_MULTI_DIMENSION_WATERFALL = """\
[
  {
    'name': 'chromium.test',
    'machines': {
      'Fake Tester': {
        'swarming': {
          'dimension_sets': [
            {
              "gpu": "none",
              "os": "1",
            },
          ],
        },
        'use_multi_dimension_trigger_script': True,
        'alternate_swarming_dimensions': [
          {
            "gpu": "none",
            "os": "2",
          },
        ],
        'test_suites': {
          'gtest_tests': 'foo_tests',
        },
      },
    },
  },
]
"""

COMPOSITION_GTEST_SUITE_WATERFALL = """\
[
  {
    'name': 'chromium.test',
    'machines': {
      'Fake Tester': {
        'test_suites': {
          'gtest_tests': 'composition_tests',
        },
      },
    },
  },
]
"""

COMPOSITION_GTEST_SUITE_WITH_ARGS_WATERFALL = """\
[
  {
    'name': 'chromium.test',
    'machines': {
      'Fake Tester': {
        'test_suites': {
          'gtest_tests': 'composition_tests',
        },
        'args': [
          '--this-is-an-argument',
        ],
      },
    },
  },
]
"""

FOO_ISOLATED_SCRIPTS_WATERFALL = """\
[
  {
    'name': 'chromium.test',
    'machines': {
      'Fake Tester': {
        'test_suites': {
          'isolated_scripts': 'composition_tests',
        },
      },
    },
  },
]
"""

FOO_SCRIPT_WATERFALL = """\
[
  {
    'name': 'chromium.test',
    'machines': {
      'Fake Tester': {
        'test_suites': {
          'scripts': 'foo_scripts',
        },
      },
    },
  },
]
"""

FOO_JUNIT_WATERFALL = """\
[
  {
    'name': 'chromium.test',
    'machines': {
      'Fake Tester': {
        'test_suites': {
          'junit_tests': 'composition_tests',
        },
      },
    },
  },
]
"""

FOO_CTS_WATERFALL = """\
[
  {
    'name': 'chromium.test',
    'machines': {
      'Fake Tester': {
        'test_suites': {
          'cts_tests': 'foo_cts_tests',
        },
      },
    },
  },
]
"""

FOO_INSTRUMENTATION_TEST_WATERFALL = """\
[
  {
    'name': 'chromium.test',
    'machines': {
      'Fake Tester': {
        'test_suites': {
          'instrumentation_tests': 'composition_tests',
        },
      },
    },
  },
]
"""

UNKNOWN_TEST_SUITE_WATERFALL = """\
[
  {
    'name': 'chromium.test',
    'machines': {
      'Fake Tester': {
        'test_suites': {
          'gtest_tests': 'baz_tests',
        },
      },
    },
  },
]
"""

UNKNOWN_TEST_SUITE_TYPE_WATERFALL = """\
[
  {
    'name': 'chromium.test',
    'machines': {
      'Fake Tester': {
        'test_suites': {
          'gtest_tests': 'foo_tests',
          'foo_test_type': 'foo_tests',
        },
      },
    },
  },
]
"""

ANDROID_WATERFALL = """\
[
  {
    'name': 'chromium.test',
    'machines': {
      'Android Builder': {
        'additional_compile_targets': [
          'bar_test',
        ],
      },
      'Fake Android K Tester': {
        'additional_compile_targets': [
          'bar_test',
        ],
        'swarming': {
          'dimension_sets': [
            {
              'device_os': 'KTU84P',
              'device_type': 'hammerhead',
            },
          ],
        },
        'os_type': 'android',
        'skip_device_recovery': True,
        'test_suites': {
          'gtest_tests': 'foo_tests',
        },
      },
      'Fake Android L Tester': {
        'swarming': {
          'dimension_sets': [
            {
              'device_os': 'KTU84P',
              'device_type': 'hammerhead',
            },
          ],
        },
        'os_type': 'android',
        'skip_merge_script': True,
        'skip_output_links': True,
        'test_suites': {
          'gtest_tests': 'foo_tests',
        },
      },
      'Fake Android M Tester': {
        'swarming': {
          'dimension_sets': [
            {
              'device_os': 'MMB29Q',
              'device_type': 'bullhead',
            },
          ],
        },
        'os_type': 'android',
        'use_swarming': False,
        'test_suites': {
          'gtest_tests': 'foo_tests',
        },
      },
    },
  },
]
"""

UNKNOWN_BOT_GTESTS_WATERFALL = """\
[
  {
    'name': 'chromium.test',
    'machines': {
      'Unknown Bot': {
        'test_suites': {
          'gtest_tests': 'foo_tests',
        },
      },
    },
  },
]
"""

FOO_TEST_SUITE = """\
{
  'foo_tests': {
    'foo_test': {
      'swarming': {
        'dimension_sets': [
          {
            'integrity': 'high',
          }
        ],
        'expiration': 120,
      },
    },
  },
}
"""

FOO_TEST_SUITE_WITH_ARGS = """\
{
  'foo_tests': {
    'foo_test': {
      'args': [
        '--c_arg',
      ],
    },
  },
}
"""

FOO_TEST_SUITE_WITH_ENABLE_FEATURES = """\
{
  'foo_tests': {
    'foo_test': {
      'args': [
        '--enable-features=Foo,Bar',
      ],
    },
  },
}
"""

FOO_SCRIPT_SUITE = """\
{
  'foo_scripts': {
    'foo_test': {
      'script': 'foo.py',
    },
    'bar_test': {
      'script': 'bar.py',
    },
  },
}
"""

FOO_CTS_SUITE = """\
{
  'foo_cts_tests': {
    'arch': 'arm64',
    'platform': 'L',
  },
}
"""

GOOD_COMPOSITION_TEST_SUITES = """\
{
  'foo_tests': {
    'foo_test': {},
  },
  'bar_tests': {
    'bar_test': {},
  },
  'composition_tests': [
    'foo_tests',
    'bar_tests',
  ],
}
"""

BAD_COMPOSITION_TEST_SUITES = """\
{
  'foo_tests': {},
  'bar_tests': {},
  'buggy_composition_tests': [
    'bar_tests',
  ],
  'composition_tests': [
    'foo_tests',
    'buggy_composition_tests',
  ],
}
"""

INSTRUMENTATION_TESTS_WITH_DIFFERENT_NAMES = """\
{
  'composition_tests': {
    'foo_tests': {
      'test': 'foo_test',
    },
    'bar_tests': {
      'test': 'foo_test',
    },
  },
}
"""

SCRIPT_SUITE = """\
{
  'foo_scripts': {
    'foo_test': {
      'script': 'foo.py',
    },
  },
}
"""

UNREFED_TEST_SUITE = """\
{
  'foo_tests': {},
  'bar_tests': {},
}
"""

REUSING_TEST_WITH_DIFFERENT_NAME = """\
{
  'foo_tests': {
    'foo_test': {},
    'variation_test': {
      'args': [
        '--variation',
      ],
      'test': 'foo_test',
    },
  },
}
"""

EMPTY_EXCEPTIONS = """\
{
}
"""

SCRIPT_WITH_ARGS_EXCEPTIONS = """\
{
  'foo_test': {
    'modifications': {
      'Fake Tester': {
        'args': ['--fake-arg'],
      },
    },
  },
}
"""

NO_BAR_TEST_EXCEPTIONS = """\
{
  'bar_test': {
    'remove_from': [
      'Fake Tester',
    ]
  }
}
"""

EMPTY_BAR_TEST_EXCEPTIONS = """\
{
  'bar_test': {
  }
}
"""

FOO_TEST_MODIFICATIONS = """\
{
  'foo_test': {
    'modifications': {
      'Fake Tester': {
        'args': [
          '--bar',
        ],
        'swarming': {
          'hard_timeout': 600,
        },
      },
    },
  }
}
"""

ANDROID_TEST_EXCEPTIONS = """\
{
  'foo_test': {
    'key_removals': {
      'Fake Android K Tester': [
        'merge',
      ],
    },
  },
}
"""

NONEXISTENT_REMOVAL = """\
{
  'foo_test': {
    'remove_from': [
      'Nonexistent Tester',
    ]
  }
}
"""

NONEXISTENT_MODIFICATION = """\
{
  'foo_test': {
    'modifications': {
      'Nonexistent Tester': {
        'args': [],
      },
    },
  }
}
"""

NONEXISTENT_KEY_REMOVAL = """
{
  'foo_test': {
    'key_removals': {
      'Fake Tester': [
        'args',
      ],
    }
  },
}
"""

COMPOSITION_WATERFALL_OUTPUT = """\
{
  "AAAAA1 AUTOGENERATED FILE DO NOT EDIT": {},
  "AAAAA2 See generate_buildbot_json.py to make changes": {},
  "Fake Tester": {
    "gtest_tests": [
      {
        "swarming": {
          "can_use_on_swarming_builders": true
        },
        "test": "bar_test"
      },
      {
        "swarming": {
          "can_use_on_swarming_builders": true
        },
        "test": "foo_test"
      }
    ]
  }
}
"""

COMPOSITION_WATERFALL_WITH_ARGS_OUTPUT = """\
{
  "AAAAA1 AUTOGENERATED FILE DO NOT EDIT": {},
  "AAAAA2 See generate_buildbot_json.py to make changes": {},
  "Fake Tester": {
    "gtest_tests": [
      {
        "args": [
          "--this-is-an-argument"
        ],
        "swarming": {
          "can_use_on_swarming_builders": true
        },
        "test": "bar_test"
      },
      {
        "args": [
          "--this-is-an-argument"
        ],
        "swarming": {
          "can_use_on_swarming_builders": true
        },
        "test": "foo_test"
      }
    ]
  }
}
"""

VARIATION_GTEST_OUTPUT = """\
{
  "AAAAA1 AUTOGENERATED FILE DO NOT EDIT": {},
  "AAAAA2 See generate_buildbot_json.py to make changes": {},
  "Fake Tester": {
    "gtest_tests": [
      {
        "swarming": {
          "can_use_on_swarming_builders": true,
          "dimension_sets": [
            {
              "kvm": "1"
            }
          ]
        },
        "test": "foo_test"
      },
      {
        "args": [
          "--variation"
        ],
        "name": "variation_test",
        "swarming": {
          "can_use_on_swarming_builders": true,
          "dimension_sets": [
            {
              "kvm": "1"
            }
          ]
        },
        "test": "foo_test"
      }
    ]
  }
}
"""

COMPOSITION_WATERFALL_FILTERED_OUTPUT = """\
{
  "AAAAA1 AUTOGENERATED FILE DO NOT EDIT": {},
  "AAAAA2 See generate_buildbot_json.py to make changes": {},
  "Fake Tester": {
    "gtest_tests": [
      {
        "swarming": {
          "can_use_on_swarming_builders": true
        },
        "test": "foo_test"
      }
    ]
  }
}
"""

MERGED_ARGS_OUTPUT = """\
{
  "AAAAA1 AUTOGENERATED FILE DO NOT EDIT": {},
  "AAAAA2 See generate_buildbot_json.py to make changes": {},
  "Fake Tester": {
    "gtest_tests": [
      {
        "args": [
          "--c_arg",
          "--bar"
        ],
        "swarming": {
          "can_use_on_swarming_builders": true,
          "dimension_sets": [
            {
              "kvm": "1"
            }
          ],
          "hard_timeout": 600
        },
        "test": "foo_test"
      }
    ]
  }
}
"""

MERGED_ENABLE_FEATURES_OUTPUT = """\
{
  "AAAAA1 AUTOGENERATED FILE DO NOT EDIT": {},
  "AAAAA2 See generate_buildbot_json.py to make changes": {},
  "Fake Tester": {
    "gtest_tests": [
      {
        "args": [
          "--enable-features=Foo,Bar,Baz"
        ],
        "swarming": {
          "can_use_on_swarming_builders": true
        },
        "test": "foo_test"
      }
    ]
  }
}
"""

MODIFIED_OUTPUT = """\
{
  "AAAAA1 AUTOGENERATED FILE DO NOT EDIT": {},
  "AAAAA2 See generate_buildbot_json.py to make changes": {},
  "Fake Tester": {
    "gtest_tests": [
      {
        "args": [
          "--bar"
        ],
        "swarming": {
          "can_use_on_swarming_builders": true,
          "dimension_sets": [
            {
              "integrity": "high",
              "kvm": "1"
            }
          ],
          "expiration": 120,
          "hard_timeout": 600
        },
        "test": "foo_test"
      }
    ]
  }
}
"""

ISOLATED_SCRIPT_OUTPUT = """\
{
  "AAAAA1 AUTOGENERATED FILE DO NOT EDIT": {},
  "AAAAA2 See generate_buildbot_json.py to make changes": {},
  "Fake Tester": {
    "isolated_scripts": [
      {
        "isolate_name": "foo_test",
        "name": "foo_test",
        "swarming": {
          "can_use_on_swarming_builders": true
        }
      }
    ]
  }
}
"""

SCRIPT_OUTPUT = """\
{
  "AAAAA1 AUTOGENERATED FILE DO NOT EDIT": {},
  "AAAAA2 See generate_buildbot_json.py to make changes": {},
  "Fake Tester": {
    "scripts": [
      {
        "name": "foo_test",
        "script": "foo.py"
      }
    ]
  }
}
"""

SCRIPT_WITH_ARGS_OUTPUT = """\
{
  "AAAAA1 AUTOGENERATED FILE DO NOT EDIT": {},
  "AAAAA2 See generate_buildbot_json.py to make changes": {},
  "Fake Tester": {
    "scripts": [
      {
        "args": [
          "--fake-arg"
        ],
        "name": "foo_test",
        "script": "foo.py"
      }
    ]
  }
}
"""

JUNIT_OUTPUT = """\
{
  "AAAAA1 AUTOGENERATED FILE DO NOT EDIT": {},
  "AAAAA2 See generate_buildbot_json.py to make changes": {},
  "Fake Tester": {
    "junit_tests": [
      {
        "test": "foo_test"
      }
    ]
  }
}
"""

CTS_OUTPUT = """\
{
  "AAAAA1 AUTOGENERATED FILE DO NOT EDIT": {},
  "AAAAA2 See generate_buildbot_json.py to make changes": {},
  "Fake Tester": {
    "cts_tests": [
      {
        "arch": "arm64",
        "platform": "L"
      }
    ]
  }
}
"""

INSTRUMENTATION_TEST_OUTPUT = """\
{
  "AAAAA1 AUTOGENERATED FILE DO NOT EDIT": {},
  "AAAAA2 See generate_buildbot_json.py to make changes": {},
  "Fake Tester": {
    "instrumentation_tests": [
      {
        "test": "foo_test"
      }
    ]
  }
}
"""

INSTRUMENTATION_TEST_DIFFERENT_NAMES_OUTPUT = """\
{
  "AAAAA1 AUTOGENERATED FILE DO NOT EDIT": {},
  "AAAAA2 See generate_buildbot_json.py to make changes": {},
  "Fake Tester": {
    "instrumentation_tests": [
      {
        "name": "bar_tests",
        "test": "foo_test"
      },
      {
        "name": "foo_tests",
        "test": "foo_test"
      }
    ]
  }
}
"""

ANDROID_WATERFALL_OUTPUT = """\
{
  "AAAAA1 AUTOGENERATED FILE DO NOT EDIT": {},
  "AAAAA2 See generate_buildbot_json.py to make changes": {},
  "Android Builder": {
    "additional_compile_targets": [
      "bar_test"
    ]
  },
  "Fake Android K Tester": {
    "additional_compile_targets": [
      "bar_test"
    ],
    "gtest_tests": [
      {
        "args": [
          "--gs-results-bucket=chromium-result-details"
        ],
        "swarming": {
          "can_use_on_swarming_builders": true,
          "cipd_packages": [
            {
              "cipd_package": "infra/tools/luci/logdog/butler/${platform}",
              "location": "bin",
              "revision": \
"git_revision:ff387eadf445b24c935f1cf7d6ddd279f8a6b04c"
            }
          ],
          "dimension_sets": [
            {
              "device_os": "KTU84P",
              "device_type": "hammerhead",
              "integrity": "high"
            }
          ],
          "expiration": 120,
          "output_links": [
            {
              "link": [
                "https://luci-logdog.appspot.com/v/?s",
                "=android%2Fswarming%2Flogcats%2F",
                "${TASK_ID}%2F%2B%2Funified_logcats"
              ],
              "name": "shard #${SHARD_INDEX} logcats"
            }
          ]
        },
        "test": "foo_test"
      }
    ]
  },
  "Fake Android L Tester": {
    "gtest_tests": [
      {
        "args": [
          "--gs-results-bucket=chromium-result-details",
          "--recover-devices"
        ],
        "swarming": {
          "can_use_on_swarming_builders": true,
          "cipd_packages": [
            {
              "cipd_package": "infra/tools/luci/logdog/butler/${platform}",
              "location": "bin",
              "revision": \
"git_revision:ff387eadf445b24c935f1cf7d6ddd279f8a6b04c"
            }
          ],
          "dimension_sets": [
            {
              "device_os": "KTU84P",
              "device_type": "hammerhead",
              "integrity": "high"
            }
          ],
          "expiration": 120
        },
        "test": "foo_test"
      }
    ]
  },
  "Fake Android M Tester": {
    "gtest_tests": [
      {
        "swarming": {
          "can_use_on_swarming_builders": false
        },
        "test": "foo_test"
      }
    ]
  }
}
"""

MULTI_DIMENSION_OUTPUT = """\
{
  "AAAAA1 AUTOGENERATED FILE DO NOT EDIT": {},
  "AAAAA2 See generate_buildbot_json.py to make changes": {},
  "Fake Tester": {
    "gtest_tests": [
      {
        "swarming": {
          "can_use_on_swarming_builders": true,
          "dimension_sets": [
            {
              "gpu": "none",
              "integrity": "high",
              "os": "1"
            }
          ],
          "expiration": 120
        },
        "test": "foo_test",
        "trigger_script": {
          "args": [
            "--multiple-trigger-configs",
            "[{\\"gpu\\": \\"none\\", \\"os\\": \\"1\\"}, \
{\\"gpu\\": \\"none\\", \\"os\\": \\"2\\"}]",
            "--multiple-dimension-script-verbose",
            "True"
          ],
          "script": "//testing/trigger_scripts/trigger_multiple_dimensions.py"
        }
      }
    ]
  }
}
"""

LUCI_MILO_CFG = """\
consoles {
  builders {
    name: "buildbucket/luci.chromium.ci/Fake Tester"
  }
}
"""

class UnitTest(unittest.TestCase):
  def test_base_generator(self):
    # Only needed for complete code coverage.
    self.assertRaises(NotImplementedError,
                      generate_buildbot_json.BaseGenerator(None).generate,
                      None, None, None, None)
    self.assertRaises(NotImplementedError,
                      generate_buildbot_json.BaseGenerator(None).sort,
                      None)

  def test_good_test_suites_are_ok(self):
    fbb = FakeBBGen(FOO_GTESTS_WATERFALL,
                    FOO_TEST_SUITE,
                    EMPTY_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.check_input_file_consistency()

  def test_good_multi_dimension_test_suites_are_ok(self):
    fbb = FakeBBGen(FOO_GTESTS_MULTI_DIMENSION_WATERFALL,
                    FOO_TEST_SUITE,
                    EMPTY_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.check_input_file_consistency()

  def test_good_composition_test_suites_are_ok(self):
    fbb = FakeBBGen(COMPOSITION_GTEST_SUITE_WATERFALL,
                    GOOD_COMPOSITION_TEST_SUITES,
                    EMPTY_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.check_input_file_consistency()

  def test_bad_composition_test_suites_are_caught(self):
    fbb = FakeBBGen(COMPOSITION_GTEST_SUITE_WATERFALL,
                    BAD_COMPOSITION_TEST_SUITES,
                    EMPTY_EXCEPTIONS,
                    LUCI_MILO_CFG)
    self.assertRaisesRegexp(generate_buildbot_json.BBGenErr,
                            'Composition test suites may not refer to.*',
                            fbb.check_input_file_consistency)

  def test_unknown_test_suites_are_caught(self):
    fbb = FakeBBGen(UNKNOWN_TEST_SUITE_WATERFALL,
                    FOO_TEST_SUITE,
                    EMPTY_EXCEPTIONS,
                    LUCI_MILO_CFG)
    self.assertRaisesRegexp(generate_buildbot_json.BBGenErr,
                            'Test suite baz_tests from machine.*',
                            fbb.check_input_file_consistency)

  def test_unknown_test_suite_types_are_caught(self):
    fbb = FakeBBGen(UNKNOWN_TEST_SUITE_TYPE_WATERFALL,
                    FOO_TEST_SUITE,
                    EMPTY_EXCEPTIONS,
                    LUCI_MILO_CFG)
    self.assertRaisesRegexp(generate_buildbot_json.BBGenErr,
                            'Unknown test suite type foo_test_type.*',
                            fbb.check_input_file_consistency)

  def test_unrefed_test_suite_caught(self):
    fbb = FakeBBGen(FOO_GTESTS_WATERFALL,
                    UNREFED_TEST_SUITE,
                    EMPTY_EXCEPTIONS,
                    LUCI_MILO_CFG)
    self.assertRaisesRegexp(generate_buildbot_json.BBGenErr,
                            '.*unreferenced.*bar_tests.*',
                            fbb.check_input_file_consistency)

  def test_good_waterfall_output(self):
    fbb = FakeBBGen(COMPOSITION_GTEST_SUITE_WATERFALL,
                    GOOD_COMPOSITION_TEST_SUITES,
                    EMPTY_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = COMPOSITION_WATERFALL_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_reusing_gtest_targets(self):
    fbb = FakeBBGen(FOO_GTESTS_WATERFALL,
                    REUSING_TEST_WITH_DIFFERENT_NAME,
                    EMPTY_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = VARIATION_GTEST_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_noop_exception_does_nothing(self):
    fbb = FakeBBGen(COMPOSITION_GTEST_SUITE_WATERFALL,
                    GOOD_COMPOSITION_TEST_SUITES,
                    EMPTY_BAR_TEST_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = COMPOSITION_WATERFALL_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_test_arg_merges(self):
    fbb = FakeBBGen(FOO_GTESTS_WATERFALL,
                    FOO_TEST_SUITE_WITH_ARGS,
                    FOO_TEST_MODIFICATIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = MERGED_ARGS_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_enable_features_arg_merges(self):
    fbb = FakeBBGen(FOO_GTESTS_WITH_ENABLE_FEATURES_WATERFALL,
                    FOO_TEST_SUITE_WITH_ENABLE_FEATURES,
                    EMPTY_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = MERGED_ENABLE_FEATURES_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_test_filtering(self):
    fbb = FakeBBGen(COMPOSITION_GTEST_SUITE_WATERFALL,
                    GOOD_COMPOSITION_TEST_SUITES,
                    NO_BAR_TEST_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = COMPOSITION_WATERFALL_FILTERED_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_test_modifications(self):
    fbb = FakeBBGen(FOO_GTESTS_WATERFALL,
                    FOO_TEST_SUITE,
                    FOO_TEST_MODIFICATIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = MODIFIED_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_isolated_script_tests(self):
    fbb = FakeBBGen(FOO_ISOLATED_SCRIPTS_WATERFALL,
                    GOOD_COMPOSITION_TEST_SUITES,
                    NO_BAR_TEST_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = ISOLATED_SCRIPT_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_script_with_args(self):
    fbb = FakeBBGen(FOO_SCRIPT_WATERFALL,
                    SCRIPT_SUITE,
                    SCRIPT_WITH_ARGS_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = SCRIPT_WITH_ARGS_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_script(self):
    fbb = FakeBBGen(FOO_SCRIPT_WATERFALL,
                    FOO_SCRIPT_SUITE,
                    NO_BAR_TEST_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = SCRIPT_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_junit_tests(self):
    fbb = FakeBBGen(FOO_JUNIT_WATERFALL,
                    GOOD_COMPOSITION_TEST_SUITES,
                    NO_BAR_TEST_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = JUNIT_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_cts_tests(self):
    fbb = FakeBBGen(FOO_CTS_WATERFALL,
                    FOO_CTS_SUITE,
                    EMPTY_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = CTS_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_instrumentation_tests(self):
    fbb = FakeBBGen(FOO_INSTRUMENTATION_TEST_WATERFALL,
                    GOOD_COMPOSITION_TEST_SUITES,
                    NO_BAR_TEST_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = INSTRUMENTATION_TEST_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_instrumentation_tests_with_different_names(self):
    fbb = FakeBBGen(FOO_INSTRUMENTATION_TEST_WATERFALL,
                    INSTRUMENTATION_TESTS_WITH_DIFFERENT_NAMES,
                    EMPTY_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = \
        INSTRUMENTATION_TEST_DIFFERENT_NAMES_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_ungenerated_output_files_are_caught(self):
    fbb = FakeBBGen(COMPOSITION_GTEST_SUITE_WATERFALL,
                    GOOD_COMPOSITION_TEST_SUITES,
                    NO_BAR_TEST_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = (
      '\n' + COMPOSITION_WATERFALL_FILTERED_OUTPUT)
    self.assertRaises(generate_buildbot_json.BBGenErr,
                      fbb.check_output_file_consistency)

  def test_android_output_options(self):
    fbb = FakeBBGen(ANDROID_WATERFALL,
                    FOO_TEST_SUITE,
                    ANDROID_TEST_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = ANDROID_WATERFALL_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_nonexistent_removal_raises(self):
    fbb = FakeBBGen(FOO_GTESTS_WATERFALL,
                    FOO_TEST_SUITE,
                    NONEXISTENT_REMOVAL,
                    LUCI_MILO_CFG)
    self.assertRaisesRegexp(generate_buildbot_json.BBGenErr,
                            'The following nonexistent machines.*',
                            fbb.check_input_file_consistency)

  def test_nonexistent_modification_raises(self):
    fbb = FakeBBGen(FOO_GTESTS_WATERFALL,
                    FOO_TEST_SUITE,
                    NONEXISTENT_MODIFICATION,
                    LUCI_MILO_CFG)
    self.assertRaisesRegexp(generate_buildbot_json.BBGenErr,
                            'The following nonexistent machines.*',
                            fbb.check_input_file_consistency)

  def test_nonexistent_key_removal_raises(self):
    fbb = FakeBBGen(FOO_GTESTS_WATERFALL,
                    FOO_TEST_SUITE,
                    NONEXISTENT_KEY_REMOVAL,
                    LUCI_MILO_CFG)
    with self.assertRaises(generate_buildbot_json.BBGenErr):
      fbb.check_output_file_consistency(verbose=True)

  def test_waterfall_args(self):
    fbb = FakeBBGen(COMPOSITION_GTEST_SUITE_WITH_ARGS_WATERFALL,
                    GOOD_COMPOSITION_TEST_SUITES,
                    EMPTY_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = COMPOSITION_WATERFALL_WITH_ARGS_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_multi_dimension_output(self):
    fbb = FakeBBGen(FOO_GTESTS_MULTI_DIMENSION_WATERFALL,
                    FOO_TEST_SUITE,
                    EMPTY_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.files['chromium.test.json'] = MULTI_DIMENSION_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_relative_pyl_file_dir(self):
    fbb = FakeBBGen(FOO_GTESTS_WATERFALL,
                    REUSING_TEST_WITH_DIFFERENT_NAME,
                    EMPTY_EXCEPTIONS,
                    LUCI_MILO_CFG)
    fbb.args = argparse.Namespace(pyl_files_dir='relative/path/')
    for file_name in list(fbb.files):
      if not 'luci-milo.cfg' in file_name:
        fbb.files[os.path.join('relative/path/', file_name)] = (
            fbb.files.pop(file_name))
    fbb.check_input_file_consistency()
    fbb.files['relative/path/chromium.test.json'] = VARIATION_GTEST_OUTPUT
    fbb.check_output_file_consistency(verbose=True)

  def test_nonexistent_bot_raises(self):
    fbb = FakeBBGen(UNKNOWN_BOT_GTESTS_WATERFALL,
                    FOO_TEST_SUITE,
                    NONEXISTENT_KEY_REMOVAL,
                    LUCI_MILO_CFG)
    with self.assertRaises(generate_buildbot_json.BBGenErr):
      fbb.check_input_file_consistency()


if __name__ == '__main__':
  unittest.main()
