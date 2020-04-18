# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from collections import OrderedDict

from blinkpy.common.host_mock import MockHost
from blinkpy.common.system.filesystem_mock import MockFileSystem
from blinkpy.common.system.log_testing import LoggingTestCase
from blinkpy.web_tests.update_flaky_expectations import RemoveFlakesOMatic
from blinkpy.web_tests.builder_list import BuilderList
from blinkpy.web_tests.port.factory import PortFactory
from blinkpy.web_tests.port.test import LAYOUT_TEST_DIR
from blinkpy.web_tests.update_flaky_expectations import main
from blinkpy.tool.commands.flaky_tests import FlakyTests


class FakeBotTestExpectations(object):

    def __init__(self, results_by_path):
        self._results = {}

        # Make the results distinct like the real BotTestExpectations.
        for path, results in results_by_path.iteritems():
            self._results[path] = list(set(results))

    def all_results_by_path(self):
        return self._results


class FakeBotTestExpectationsFactory(object):

    def __init__(self):
        """The distinct results seen in at least one run of the test.

        For example, if the bot results for mytest.html are:
            PASS PASS FAIL PASS TIMEOUT
        then |all_results_by_builder| would be:
            {
                'WebKit Linux Trusty' : {
                    'mytest.html': ['FAIL', 'PASS', 'TIMEOUT']
                }
            }
        """
        self.all_results_by_builder = {}

    def expectations_for_builder(self, builder):
        if builder not in self.all_results_by_builder:
            return None

        return FakeBotTestExpectations(self.all_results_by_builder[builder])


class FakePortFactory(PortFactory):

    def __init__(self, host, all_build_types=None, all_systems=None):
        super(FakePortFactory, self).__init__(host)
        self._all_build_types = all_build_types or ()
        self._all_systems = all_systems or ()
        self._configuration_specifier_macros = {
            'mac': ['mac10.10'],
            'win': ['win7'],
            'linux': ['trusty']
        }

    def get(self, port_name=None, options=None, **kwargs):
        """Returns an object implementing the Port interface.

        This fake object will always return the 'test' port.
        """
        port = super(FakePortFactory, self).get('test', None)
        port.all_build_types = self._all_build_types
        port.all_systems = self._all_systems
        port.configuration_specifier_macros_dict = self._configuration_specifier_macros
        return port


class MockWebBrowser(object):

    def __init__(self):
        self.opened_url = None

    def open(self, url):
        self.opened_url = url


class UpdateTestExpectationsTest(LoggingTestCase):

    def setUp(self):
        super(UpdateTestExpectationsTest, self).setUp()
        self._mock_web_browser = MockWebBrowser()
        self._host = MockHost()
        self._port = self._host.port_factory.get('test', None)
        self._expectation_factory = FakeBotTestExpectationsFactory()
        self._flake_remover = RemoveFlakesOMatic(self._host,
                                                 self._port,
                                                 self._expectation_factory,
                                                 self._mock_web_browser)
        self._port.configuration_specifier_macros_dict = {
            'mac': ['mac10.10'],
            'win': ['win7'],
            'linux': ['trusty']
        }
        filesystem = self._host.filesystem
        self._write_tests_into_filesystem(filesystem)

    def tearDown(self):
        super(UpdateTestExpectationsTest, self).tearDown()

    def _write_tests_into_filesystem(self, filesystem):
        test_list = ['test/a.html',
                     'test/b.html',
                     'test/c.html',
                     'test/d.html',
                     'test/e.html',
                     'test/f.html',
                     'test/g.html']
        for test in test_list:
            path = filesystem.join(LAYOUT_TEST_DIR, test)
            filesystem.write_binary_file(path, '')

    def _assert_expectations_match(self, expectations, expected_string):
        self.assertIsNotNone(expectations)
        stringified_expectations = '\n'.join(
            x.to_string() for x in expectations)
        expected_string = '\n'.join(
            x.strip() for x in expected_string.split('\n'))
        self.assertEqual(stringified_expectations, expected_string)

    def _parse_expectations(self, expectations):
        """Parses a TestExpectation file given as string.

        This function takes a string representing the contents of the
        TestExpectations file and parses it, producing the TestExpectations
        object and sets it on the Port object where the script will read it
        from.

        Args:
            expectations: A string containing the contents of the
            TestExpectations file to use.
        """
        expectations_dict = OrderedDict()
        expectations_dict['expectations'] = expectations
        self._port.expectations_dict = lambda: expectations_dict

    def _define_builders(self, builders_dict):
        """Defines the available builders for the test.

        Args:
            builders_dict: A dictionary containing builder names to their
            attributes, see BuilderList.__init__ for the format.
        """
        self._host.builders = BuilderList(builders_dict)

    def test_dont_remove_non_flakes(self):
        """Tests that lines that aren't flaky are not touched.

        Lines are flaky if they contain a PASS as well as at least one other
        failing result.
        """
        test_expectations_before = """
            # Even though the results show all passing, none of the
            # expectations are flaky so we shouldn't remove any.
            Bug(test) test/a.html [ Pass ]
            Bug(test) test/b.html [ Timeout ]
            Bug(test) test/c.html [ Failure Timeout ]
            Bug(test) test/d.html [ Rebaseline ]
            Bug(test) test/e.html [ NeedsManualRebaseline ]"""

        self._define_builders({
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
        })
        self._port.all_build_types = ('release',)
        self._port.all_systems = (('trusty', 'x86_64'),)

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
                'test/a.html': ['PASS', 'PASS'],
                'test/b.html': ['PASS', 'PASS'],
                'test/c.html': ['PASS', 'PASS'],
                'test/d.html': ['PASS', 'PASS'],
                'test/e.html': ['PASS', 'PASS'],
                'test/f.html': ['PASS', 'PASS'],
            }
        }
        updated_expectations = (
            self._flake_remover.get_updated_test_expectations())
        self._assert_expectations_match(
            updated_expectations, test_expectations_before)

    def test_dont_remove_directory(self):
        """Tests that lines with directories are untouched."""
        test_expectations_before = """
            # This expectation is for a whole directory.
            Bug(test) test/ [ Failure Pass ]"""

        self._define_builders({
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
        })
        self._port.all_build_types = ('release',)
        self._port.all_systems = (('trusty', 'x86_64'),)

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
                'test/a.html': ['PASS', 'PASS'],
                'test/b.html': ['PASS', 'PASS'],
                'test/c.html': ['PASS', 'PASS'],
                'test/d.html': ['PASS', 'PASS'],
                'test/e.html': ['PASS', 'PASS'],
                'test/f.html': ['PASS', 'PASS'],
            }
        }
        updated_expectations = (
            self._flake_remover.get_updated_test_expectations())
        self._assert_expectations_match(
            updated_expectations, test_expectations_before)

    def test_dont_remove_skip(self):
        """Tests that lines with Skip are untouched.

        If a line is marked as Skip, it will eventually contain no results,
        which is indistinguishable from "All Passing" so don't remove since we
        don't know what the results actually are.
        """
        test_expectations_before = """
            # Skip expectations should never be removed.
            Bug(test) test/a.html [ Skip ]
            Bug(test) test/b.html [ Skip ]
            Bug(test) test/c.html [ Skip ]"""

        self._define_builders({
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
        })
        self._port.all_build_types = ('release',)
        self._port.all_systems = (('trusty', 'x86_64'),)

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
                'test/a.html': ['PASS', 'PASS'],
                'test/b.html': ['PASS', 'IMAGE'],
            }
        }
        updated_expectations = (
            self._flake_remover.get_updated_test_expectations())
        self._assert_expectations_match(
            updated_expectations, test_expectations_before)

    def test_dont_remove_rebaselines(self):
        """Tests that lines with rebaseline expectations are untouched."""
        test_expectations_before = """
            # Even though the results show all passing, none of the
            # expectations are flaky so we shouldn't remove any.
            Bug(test) test/a.html [ Failure Pass Rebaseline ]
            Bug(test) test/b.html [ Failure NeedsManualRebaseline Pass ]"""

        self._define_builders({
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
        })
        self._port.all_build_types = ('release',)
        self._port.all_systems = (('trusty', 'x86_64'),)

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
                'test/a.html': ['PASS', 'PASS'],
                'test/b.html': ['PASS', 'PASS'],
            }
        }
        updated_expectations = (
            self._flake_remover.get_updated_test_expectations())
        self._assert_expectations_match(
            updated_expectations, test_expectations_before)

    def test_all_failure_types(self):
        """Tests that all failure types are treated as failure."""
        test_expectations_before = (
            """Bug(test) test/a.html [ Failure Pass ]
            Bug(test) test/b.html [ Failure Pass ]
            Bug(test) test/c.html [ Failure Pass ]
            Bug(test) test/d.html [ Failure Pass ]
            # Remove these two since CRASH and TIMEOUT aren't considered
            # Failure.
            Bug(test) test/e.html [ Failure Pass ]
            Bug(test) test/f.html [ Failure Pass ]""")

        self._define_builders({
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
        })
        self._port.all_build_types = ('release',)
        self._port.all_systems = (('trusty', 'x86_64'),)

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
                'test/a.html': ['PASS', 'IMAGE'],
                'test/b.html': ['PASS', 'TEXT'],
                'test/c.html': ['PASS', 'IMAGE+TEXT'],
                'test/d.html': ['PASS', 'AUDIO'],
                'test/e.html': ['PASS', 'CRASH'],
                'test/f.html': ['PASS', 'TIMEOUT'],
            }
        }
        updated_expectations = (
            self._flake_remover.get_updated_test_expectations())
        self._assert_expectations_match(updated_expectations, (
            """Bug(test) test/a.html [ Failure Pass ]
            Bug(test) test/b.html [ Failure Pass ]
            Bug(test) test/c.html [ Failure Pass ]
            Bug(test) test/d.html [ Failure Pass ]"""))

    def test_basic_one_builder(self):
        """Tests basic functionality with a single builder.

        Test that flaky expectations with results from a single bot showing the
        expected failure isn't occurring should be removed. Results with failures
        of the expected type shouldn't be removed but other kinds of failures
        allow removal.
        """
        test_expectations_before = (
            """# Remove this since it's passing all runs.
            Bug(test) test/a.html [ Failure Pass ]
            # Remove this since, although there's a failure, it's not a timeout.
            Bug(test) test/b.html [ Pass Timeout ]
            # Keep since we have both crashes and passes.
            Bug(test) test/c.html [ Crash Pass ]""")

        self._define_builders({
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
        })
        self._port.all_build_types = ('release',)
        self._port.all_systems = (('trusty', 'x86_64'),)

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
                'test/b.html': ['PASS', 'IMAGE', 'PASS'],
                'test/c.html': ['PASS', 'CRASH', 'PASS'],
            }
        }
        updated_expectations = (
            self._flake_remover.get_updated_test_expectations())
        self._assert_expectations_match(updated_expectations, (
            """# Keep since we have both crashes and passes.
            Bug(test) test/c.html [ Crash Pass ]"""))

    def test_all_failure_case(self):
        """Tests that results with all failures are not treated as non-flaky."""
        test_expectations_before = (
            """# Keep since it's all failures.
            Bug(test) test/a.html [ Failure Pass ]""")

        self._define_builders({
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
        })
        self._port.all_build_types = ('release',)
        self._port.all_systems = (('trusty', 'x86_64'),)

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
                'test/a.html': ['IMAGE', 'IMAGE', 'IMAGE'],
            }
        }
        updated_expectations = (
            self._flake_remover.get_updated_test_expectations())
        self._assert_expectations_match(updated_expectations, (
            """# Keep since it's all failures.
            Bug(test) test/a.html [ Failure Pass ]"""))

    def test_remove_none_met(self):
        """Tests that expectations with no matching result are removed.

        Expectations that are failing in a different way than specified should
        be removed, even if there is no passing result.
        """
        test_expectations_before = (
            """# Remove these two since CRASH and TIMEOUT aren't considered
            # Failure.
            Bug(test) test/a.html [ Failure Pass ]
            Bug(test) test/b.html [ Failure Pass ]""")

        self._define_builders({
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
        })
        self._port.all_build_types = ('release',)
        self._port.all_systems = (('trusty', 'x86_64'),)

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
                'test/a.html': ['CRASH'],
                'test/b.html': ['TIMEOUT'],
            }
        }
        updated_expectations = (
            self._flake_remover.get_updated_test_expectations())
        self._assert_expectations_match(updated_expectations, (''))

    def test_empty_test_expectations(self):
        """Running on an empty TestExpectations file outputs an empty file."""
        test_expectations_before = ''

        self._define_builders({
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
        })
        self._port.all_build_types = ('release',)
        self._port.all_systems = (('trusty', 'x86_64'),)

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
            }
        }
        updated_expectations = (
            self._flake_remover.get_updated_test_expectations())
        self._assert_expectations_match(updated_expectations, '')

    def test_basic_multiple_builders(self):
        """Tests basic functionality with multiple builders."""
        test_expectations_before = (
            """# Remove since it's passing on both builders.
            Bug(test) test/a.html [ Failure Pass ]
            # Keep since it's failing on the Mac builder.
            Bug(test) test/b.html [ Failure Pass ]
            # Keep since it's failing on the Linux builder.
            Bug(test) test/c.html [ Failure Pass ]""")

        self._define_builders({
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
            'WebKit Mac10.10': {
                'port_name': 'mac-mac10.10',
                'specifiers': ['Mac10.10', 'Release']
            },
        })

        self._port.all_build_types = ('release',)
        self._port.all_systems = (('mac10.10', 'x86'),
                                  ('trusty', 'x86_64'))

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
                'test/b.html': ['PASS', 'PASS', 'PASS'],
                'test/c.html': ['AUDIO', 'AUDIO', 'AUDIO'],
            },
            'WebKit Mac10.10': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
                'test/b.html': ['PASS', 'PASS', 'IMAGE'],
                'test/c.html': ['PASS', 'PASS', 'PASS'],
            },
        }
        updated_expectations = (
            self._flake_remover.get_updated_test_expectations())
        self._assert_expectations_match(updated_expectations, (
            """# Keep since it's failing on the Mac builder.
            Bug(test) test/b.html [ Failure Pass ]
            # Keep since it's failing on the Linux builder.
            Bug(test) test/c.html [ Failure Pass ]"""))

    def test_multiple_builders_and_platform_specifiers(self):
        """Tests correct operation with platform specifiers."""
        test_expectations_before = (
            """# Keep since it's failing in the Mac10.10 results.
            Bug(test) [ Mac ] test/a.html [ Failure Pass ]
            # Keep since it's failing on the Windows builder.
            Bug(test) [ Linux Win ] test/b.html [ Failure Pass ]
            # Remove since it's passing on both Linux and Windows builders.
            Bug(test) [ Linux Win ] test/c.html [ Failure Pass ]
            # Remove since it's passing on Mac results
            Bug(test) [ Mac ] test/d.html [ Failure Pass ]""")

        self._define_builders({
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
            'WebKit Mac10.10': {
                'port_name': 'mac-mac10.10',
                'specifiers': ['Mac10.10', 'Release']
            },
            'WebKit Mac10.11': {
                'port_name': 'mac-mac10.11',
                'specifiers': ['Mac10.11', 'Release']
            },
            'WebKit Win7': {
                'port_name': 'win-win7',
                'specifiers': ['Win7', 'Release']
            },
        })
        self._port.all_build_types = ('release',)
        self._port.all_systems = (
            ('mac10.10', 'x86'),
            ('mac10.11', 'x86'),
            ('trusty', 'x86_64'),
            ('win7', 'x86'),
        )

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
                'test/b.html': ['PASS', 'PASS', 'PASS'],
                'test/c.html': ['PASS', 'PASS', 'PASS'],
                'test/d.html': ['IMAGE', 'PASS', 'PASS'],
            },
            'WebKit Mac10.10': {
                'test/a.html': ['PASS', 'PASS', 'IMAGE'],
                'test/b.html': ['PASS', 'IMAGE', 'PASS'],
                'test/c.html': ['PASS', 'IMAGE', 'PASS'],
                'test/d.html': ['PASS', 'PASS', 'PASS'],
            },
            'WebKit Mac10.11': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
                'test/b.html': ['PASS', 'PASS', 'PASS'],
                'test/c.html': ['PASS', 'PASS', 'PASS'],
                'test/d.html': ['PASS', 'PASS', 'PASS'],
            },
            'WebKit Win7': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
                'test/b.html': ['IMAGE', 'PASS', 'PASS'],
                'test/c.html': ['PASS', 'PASS', 'PASS'],
                'test/d.html': ['IMAGE', 'PASS', 'PASS'],
            },
        }
        updated_expectations = (
            self._flake_remover.get_updated_test_expectations())
        self._assert_expectations_match(updated_expectations, (
            """# Keep since it's failing in the Mac10.10 results.
            Bug(test) [ Mac ] test/a.html [ Failure Pass ]
            # Keep since it's failing on the Windows builder.
            Bug(test) [ Linux Win ] test/b.html [ Failure Pass ]"""))

    def test_debug_release_specifiers(self):
        """Tests correct operation of Debug/Release specifiers."""
        test_expectations_before = (
            """# Keep since it fails in debug.
            Bug(test) [ Linux ] test/a.html [ Failure Pass ]
            # Remove since the failure is in Release, Debug is all PASS.
            Bug(test) [ Debug ] test/b.html [ Failure Pass ]
            # Keep since there's a failure in Linux Release.
            Bug(test) [ Release ] test/c.html [ Failure Pass ]
            # Remove since the Release Linux builder is all passing.
            Bug(test) [ Release Linux ] test/d.html [ Failure Pass ]
            # Remove since all the Linux builders PASS.
            Bug(test) [ Linux ] test/e.html [ Failure Pass ]""")

        self._define_builders({
            'WebKit Win7': {
                'port_name': 'win-win7',
                'specifiers': ['Win7', 'Release']
            },
            'WebKit Win7 (dbg)': {
                'port_name': 'win-win7',
                'specifiers': ['Win7', 'Debug']
            },
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
            'WebKit Linux Trusty (dbg)': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Debug']
            },
        })
        self._port.all_build_types = ('release', 'debug')
        self._port.all_systems = (('win7', 'x86'),
                                  ('trusty', 'x86_64'))

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
                'test/b.html': ['PASS', 'IMAGE', 'PASS'],
                'test/c.html': ['PASS', 'IMAGE', 'PASS'],
                'test/d.html': ['PASS', 'PASS', 'PASS'],
                'test/e.html': ['PASS', 'PASS', 'PASS'],
            },
            'WebKit Linux Trusty (dbg)': {
                'test/a.html': ['PASS', 'IMAGE', 'PASS'],
                'test/b.html': ['PASS', 'PASS', 'PASS'],
                'test/c.html': ['PASS', 'PASS', 'PASS'],
                'test/d.html': ['IMAGE', 'PASS', 'PASS'],
                'test/e.html': ['PASS', 'PASS', 'PASS'],
            },
            'WebKit Win7 (dbg)': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
                'test/b.html': ['PASS', 'PASS', 'PASS'],
                'test/c.html': ['PASS', 'PASS', 'PASS'],
                'test/d.html': ['PASS', 'IMAGE', 'PASS'],
                'test/e.html': ['PASS', 'PASS', 'PASS'],
            },
            'WebKit Win7': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
                'test/b.html': ['PASS', 'PASS', 'IMAGE'],
                'test/c.html': ['PASS', 'PASS', 'PASS'],
                'test/d.html': ['PASS', 'IMAGE', 'PASS'],
                'test/e.html': ['PASS', 'PASS', 'PASS'],
            },
        }
        updated_expectations = (
            self._flake_remover.get_updated_test_expectations())
        self._assert_expectations_match(updated_expectations, (
            """# Keep since it fails in debug.
            Bug(test) [ Linux ] test/a.html [ Failure Pass ]
            # Keep since there's a failure in Linux Release.
            Bug(test) [ Release ] test/c.html [ Failure Pass ]"""))

    def test_preserve_comments_and_whitespace(self):
        """Tests that comments and whitespace are preserved appropriately.

        Comments and whitespace should be kept unless all the tests grouped
        below a comment are removed. In that case the comment block should also
        be removed.

        Ex:
            # This comment applies to the below tests.
            Bug(test) test/a.html [ Failure Pass ]
            Bug(test) test/b.html [ Failure Pass ]

            # <some prose>

            # This is another comment.
            Bug(test) test/c.html [ Failure Pass ]

        Assuming we removed a.html and c.html we get:
            # This comment applies to the below tests.
            Bug(test) test/b.html [ Failure Pass ]

            # <some prose>
        """
        test_expectations_before = """
            # Comment A - Keep since these aren't part of any test.
            # Comment B - Keep since these aren't part of any test.

            # Comment C - Remove since it's a block belonging to a
            # Comment D - and a is removed.
            Bug(test) test/a.html [ Failure Pass ]
            # Comment E - Keep since it's below a.


            # Comment F - Keep since only b is removed
            Bug(test) test/b.html [ Failure Pass ]
            Bug(test) test/c.html [ Failure Pass ]

            # Comment G - Should be removed since both d and e will be removed.
            Bug(test) test/d.html [ Failure Pass ]
            Bug(test) test/e.html [ Failure Pass ]"""

        self._define_builders({
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
        })
        self._port.all_build_types = ('release',)
        self._port.all_systems = (('trusty', 'x86_64'),)

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
                'test/b.html': ['PASS', 'PASS', 'PASS'],
                'test/c.html': ['PASS', 'IMAGE', 'PASS'],
                'test/d.html': ['PASS', 'PASS', 'PASS'],
                'test/e.html': ['PASS', 'PASS', 'PASS'],
            }
        }
        updated_expectations = (
            self._flake_remover.get_updated_test_expectations())
        self._assert_expectations_match(updated_expectations, (
            """
            # Comment A - Keep since these aren't part of any test.
            # Comment B - Keep since these aren't part of any test.
            # Comment E - Keep since it's below a.


            # Comment F - Keep since only b is removed
            Bug(test) test/c.html [ Failure Pass ]"""))

    def test_no_results_on_builders(self):
        """Tests that we remove a line that has no results on the builders.

        A test that has no results returned from the builders means that all
        runs passed or were skipped. A Skip expectation in TestExpectations
        shouldn't be removed but otherwise the test is passing.
        """
        test_expectations_before = """
            # A Skip expectation probably won't have any results but we
            # shouldn't consider those passing so this line should remain.
            Bug(test) test/a.html [ Skip ]
            # This line shouldn't be removed either since it's not flaky.
            Bug(test) test/b.html [ Failure Timeout ]
            # The lines below should be removed since they're flaky but all runs
            # are passing.
            Bug(test) test/c.html [ Failure Pass ]
            Bug(test) test/d.html [ Pass Timeout ]
            Bug(test) test/e.html [ Crash Pass ]"""

        self._define_builders({
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
        })
        self._port.all_build_types = ('release',)
        self._port.all_systems = (('trusty', 'x86_64'),)

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
            }
        }
        updated_expectations = (
            self._flake_remover.get_updated_test_expectations())
        self._assert_expectations_match(updated_expectations, """
            # A Skip expectation probably won't have any results but we
            # shouldn't consider those passing so this line should remain.
            Bug(test) test/a.html [ Skip ]
            # This line shouldn't be removed either since it's not flaky.
            Bug(test) test/b.html [ Failure Timeout ]""")

    def test_missing_builders_for_some_configurations(self):
        """Tests the behavior when there are no builders for some configurations.

        We don't necessarily expect to have builders for all configurations,
        so as long as a test appears to be non-flaky on all matching configurations
        that have builders, then it can be removed, even if there are extra
        configurations with no existing builders.
        """
        # Set the logging level used for assertLog to allow us to check
        # messages with a "debug" severity level.
        self.set_logging_level(logging.DEBUG)

        test_expectations_before = """
            # There are no builders that match this configuration at all.
            Bug(test) [ Win ] test/a.html [ Failure Pass ]

            # This matches the existing linux release builder and
            # also linux debug, which has no builder.
            Bug(test) [ Linux ] test/b.html [ Failure Pass ]

            # No message should be emitted for this one because it's not
            # marked as flaky, so we don't need to check builder results.
            Bug(test) test/c.html [ Failure ]

            # This one is marked as flaky and there are some matching
            # configurations with no builders, but for all configurations
            # with existing builders, it is non-flaky.
            Bug(test) test/d.html [ Failure Pass ]

            # This one only matches the existing linux release builder,
            # and it's still flaky, so it shouldn't be removed.
            Bug(test) [ Linux Release ] test/e.html [ Failure Pass ]"""

        self._define_builders({
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
        })

        self._port.all_build_types = ('release', 'debug')
        self._port.all_systems = (
            ('win7', 'x86'),
            ('trusty', 'x86_64'),
        )

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
                'test/b.html': ['PASS', 'PASS', 'PASS'],
                'test/c.html': ['PASS', 'IMAGE', 'PASS'],
                'test/d.html': ['PASS', 'PASS', 'PASS'],
                'test/e.html': ['PASS', 'IMAGE', 'PASS'],
            }
        }

        updated_expectations = (
            self._flake_remover.get_updated_test_expectations())

        self.assertLog([
            'DEBUG: No builder with config <win7, x86, release>\n',
            'DEBUG: No builder with config <win7, x86, debug>\n',
            'WARNING: No matching builders for line, deleting line.\n',
            'INFO: Deleting line "Bug(test) [ Win ] test/a.html [ Failure Pass ]"\n',
            'DEBUG: No builder with config <trusty, x86_64, debug>\n',
            'DEBUG: Checked builders:\n  WebKit Linux Trusty\n',
            'INFO: Deleting line "Bug(test) [ Linux ] test/b.html [ Failure Pass ]"\n',
            'DEBUG: No builder with config <trusty, x86_64, debug>\n',
            'DEBUG: No builder with config <win7, x86, release>\n',
            'DEBUG: No builder with config <win7, x86, debug>\n',
            'DEBUG: Checked builders:\n  WebKit Linux Trusty\n',
            'INFO: Deleting line "Bug(test) test/d.html [ Failure Pass ]"\n',
        ])
        self._assert_expectations_match(
            updated_expectations,
            """
            # No message should be emitted for this one because it's not
            # marked as flaky, so we don't need to check builder results.
            Bug(test) test/c.html [ Failure ]

            # This one only matches the existing linux release builder,
            # and it's still flaky, so it shouldn't be removed.
            Bug(test) [ Linux Release ] test/e.html [ Failure Pass ]""")

    def test_log_missing_results(self):
        """Tests that we emit the appropriate error for missing results.

        If the results dictionary we download from the builders is missing the
        results from one of the builders we matched we should have logged an
        error.
        """
        test_expectations_before = """
            Bug(test) [ Linux ] test/a.html [ Failure Pass ]
            # This line won't emit an error since the Linux Release results
            # exist.
            Bug(test) [ Linux Release ] test/b.html [ Failure Pass ]
            Bug(test) [ Release ] test/c.html [ Failure Pass ]
            # This line is not flaky so we shouldn't even check the results.
            Bug(test) [ Linux ] test/d.html [ Failure ]"""

        self._define_builders({
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
            'WebKit Linux Trusty (dbg)': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Debug']
            },
            'WebKit Win7': {
                'port_name': 'win-win7',
                'specifiers': ['Win7', 'Release']
            },
            'WebKit Win7 (dbg)': {
                'port_name': 'win-win7',
                'specifiers': ['Win7', 'Debug']
            },
        })

        # Two warnings and two errors should be emitted:
        # (1) A warning since the results don't contain anything for the Linux
        #     (dbg) builder
        # (2) A warning since the results don't contain anything for the Win
        #     release builder
        # (3) The first line needs and is missing results for Linux (dbg).
        # (4) The third line needs and is missing results for Win Release.
        self._port.all_build_types = ('release', 'debug')
        self._port.all_systems = (('win7', 'x86'),
                                  ('trusty', 'x86_64'))

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
                'test/b.html': ['PASS', 'IMAGE', 'PASS'],
                'test/c.html': ['PASS', 'PASS', 'PASS'],
                'test/d.html': ['PASS', 'PASS', 'PASS'],
            },
            'WebKit Win7 (dbg)': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
                'test/b.html': ['PASS', 'PASS', 'PASS'],
                'test/c.html': ['PASS', 'PASS', 'PASS'],
                'test/d.html': ['PASS', 'PASS', 'PASS'],
            },
        }

        updated_expectations = (
            self._flake_remover.get_updated_test_expectations())
        self.assertLog([
            'WARNING: Downloaded results are missing results for builder "WebKit Linux Trusty (dbg)"\n',
            'WARNING: Downloaded results are missing results for builder "WebKit Win7"\n',
            'ERROR: Failed to find results for builder "WebKit Linux Trusty (dbg)"\n',
            'ERROR: Failed to find results for builder "WebKit Win7"\n',
        ])

        # Also make sure we didn't remove any lines if some builders were
        # missing.
        self._assert_expectations_match(
            updated_expectations, test_expectations_before)

    def test_harness_updates_file(self):
        """Tests that the call harness updates the TestExpectations file."""

        self._define_builders({
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
            'WebKit Linux Trusty (dbg)': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Debug']
            },
        })

        # Setup the mock host and port.
        host = self._host
        host.port_factory = FakePortFactory(
            host,
            all_build_types=('release', 'debug'),
            all_systems=(('trusty', 'x86_64'),))

        # Write out a fake TestExpectations file.
        test_expectation_path = (
            host.port_factory.get().path_to_generic_test_expectations_file())
        test_expectations = """
            # Remove since passing on both bots.
            Bug(test) [ Linux ] test/a.html [ Failure Pass ]
            # Keep since there's a failure on release bot.
            Bug(test) [ Linux Release ] test/b.html [ Failure Pass ]
            # Remove since it's passing on both builders.
            Bug(test) test/c.html [ Failure Pass ]
            # Keep since there's a failure on debug bot.
            Bug(test) [ Linux ] test/d.html [ Failure ]"""
        files = {
            test_expectation_path: test_expectations
        }
        host.filesystem = MockFileSystem(files)
        self._write_tests_into_filesystem(host.filesystem)

        # Write out the fake builder bot results.
        expectation_factory = FakeBotTestExpectationsFactory()
        expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
                'test/b.html': ['PASS', 'IMAGE', 'PASS'],
                'test/c.html': ['PASS', 'PASS', 'PASS'],
                'test/d.html': ['PASS', 'PASS', 'PASS'],
            },
            'WebKit Linux Trusty (dbg)': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
                'test/b.html': ['PASS', 'PASS', 'PASS'],
                'test/c.html': ['PASS', 'PASS', 'PASS'],
                'test/d.html': ['IMAGE', 'PASS', 'PASS'],
            },
        }

        main(host, expectation_factory, [])

        self.assertEqual(host.filesystem.files[test_expectation_path], (
            """            # Keep since there's a failure on release bot.
            Bug(test) [ Linux Release ] test/b.html [ Failure Pass ]
            # Keep since there's a failure on debug bot.
            Bug(test) [ Linux ] test/d.html [ Failure ]"""))

    def test_harness_no_expectations(self):
        """Tests behavior when TestExpectations file doesn't exist.

        Tests that a warning is outputted if the TestExpectations file
        doesn't exist.
        """

        # Set up the mock host and port.
        host = MockHost()
        host.port_factory = FakePortFactory(host)

        # Write the test file but not the TestExpectations file.
        test_expectation_path = (
            host.port_factory.get().path_to_generic_test_expectations_file())
        host.filesystem = MockFileSystem()
        self._write_tests_into_filesystem(host.filesystem)

        # Write out the fake builder bot results.
        expectation_factory = FakeBotTestExpectationsFactory()
        expectation_factory.all_results_by_builder = {}

        self.assertFalse(host.filesystem.isfile(test_expectation_path))

        return_code = main(host, expectation_factory, [])

        self.assertEqual(return_code, 1)

        self.assertLog([
            "WARNING: Didn't find generic expectations file at: %s\n" % test_expectation_path
        ])
        self.assertFalse(host.filesystem.isfile(test_expectation_path))

    def test_harness_remove_all(self):
        """Tests that removing all expectations doesn't delete the file.

        Make sure we're prepared for the day when we exterminated flakes.
        """

        self._define_builders({
            'WebKit Linux Trusty': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
            'WebKit Linux Trusty (dbg)': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Debug']
            },
        })

        # Set up the mock host and port.
        host = self._host
        host.port_factory = FakePortFactory(
            host,
            all_build_types=('release', 'debug'),
            all_systems=(('trusty', 'x86_64'),))

        # Write out a fake TestExpectations file.
        test_expectation_path = (
            host.port_factory.get().path_to_generic_test_expectations_file())
        test_expectations = """
            # Remove since passing on both bots.
            Bug(test) [ Linux ] test/a.html [ Failure Pass ]"""

        files = {
            test_expectation_path: test_expectations
        }
        host.filesystem = MockFileSystem(files)
        self._write_tests_into_filesystem(host.filesystem)

        # Write out the fake builder bot results.
        expectation_factory = FakeBotTestExpectationsFactory()
        expectation_factory.all_results_by_builder = {
            'WebKit Linux Trusty': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
            },
            'WebKit Linux Trusty (dbg)': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
            },
        }

        main(host, expectation_factory, [])

        self.assertTrue(host.filesystem.isfile(test_expectation_path))
        self.assertEqual(host.filesystem.files[test_expectation_path], '')

    def test_show_results(self):
        """Tests that passing --show-results shows the removed results.

        --show-results opens the removed tests in the layout dashboard using
        the default browser. This tests mocks the webbrowser.open function and
        checks that it was called with the correct URL.
        """
        test_expectations_before = (
            """# Remove this since it's passing all runs.
            Bug(test) test/a.html [ Failure Pass ]
            # Remove this since, although there's a failure, it's not a timeout.
            Bug(test) test/b.html [ Pass Timeout ]
            # Keep since we have both crashes and passes.
            Bug(test) test/c.html [ Crash Pass ]""")

        self._define_builders({
            'WebKit Linux': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
        })
        self._port.all_build_types = ('release',)
        self._port.all_systems = (('trusty', 'x86_64'),)

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
                'test/b.html': ['PASS', 'IMAGE', 'PASS'],
                'test/c.html': ['PASS', 'CRASH', 'PASS'],
            }
        }
        self._flake_remover.show_removed_results()
        self.assertEqual(
            FlakyTests.FLAKINESS_DASHBOARD_URL % 'test/a.html,test/b.html',
            self._mock_web_browser.opened_url)

    def test_suggested_commit_description(self):
        """Tests display of the suggested commit message.
        """
        test_expectations_before = (
            """# Remove this since it's passing all runs.
            crbug.com/2222 test/a.html [ Failure Pass ]
            # Remove this since, although there's a failure, it's not a timeout.
            crbug.com/1111 test/b.html [ Pass Timeout ]
            # Keep since we have both crashes and passes.
            crbug.com/3333 test/c.html [ Crash Pass ]""")

        self._define_builders({
            'WebKit Linux': {
                'port_name': 'linux-trusty',
                'specifiers': ['Trusty', 'Release']
            },
        })
        self._port.all_build_types = ('release',)
        self._port.all_systems = (('trusty', 'x86_64'),)

        self._parse_expectations(test_expectations_before)
        self._expectation_factory.all_results_by_builder = {
            'WebKit Linux': {
                'test/a.html': ['PASS', 'PASS', 'PASS'],
                'test/b.html': ['PASS', 'IMAGE', 'PASS'],
                'test/c.html': ['PASS', 'CRASH', 'PASS'],
            }
        }
        self._flake_remover.print_suggested_commit_description()
        self.assertLog([
            'INFO: Deleting line "crbug.com/2222 test/a.html [ Failure Pass ]"\n',
            'INFO: Deleting line "crbug.com/1111 test/b.html [ Pass Timeout ]"\n',
            'INFO: Suggested commit description:\n'
            'Remove flaky TestExpectations for tests which appear non-flaky recently.\n\n'
            'This change was made by the update-test-expectations script.\n\n'
            'Recent test results history:\n'
            'https://test-results.appspot.com/dashboards/flakiness_dashboard.html'
            '#testType=webkit_layout_tests&tests=test/a.html,test/b.html\n\n'
            'Bug: 1111, 2222\n'
        ])
