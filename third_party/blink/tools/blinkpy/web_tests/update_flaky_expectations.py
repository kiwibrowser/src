# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Updates TestExpectations based on results in builder bots.

Scans the TestExpectations file and uses results from actual builder bots runs
to remove tests that are marked as flaky but don't fail in the specified way.

E.g. If a test has this expectation:
    bug(test) fast/test.html [ Failure Pass ]

And all the runs on builders have passed the line will be removed.

Additionally, the runs don't all have to be Passing to remove the line;
as long as the non-Passing results are of a type not specified in the
expectation this line will be removed. For example, if this is the
expectation:

    bug(test) fast/test.html [ Crash Pass ]

But the results on the builders show only Passes and Timeouts, the line
will be removed since there's no Crash results.
"""

import argparse
import logging
import webbrowser

from blinkpy.tool.commands.flaky_tests import FlakyTests
from blinkpy.web_tests.models.test_expectations import CHROMIUM_BUG_PREFIX
from blinkpy.web_tests.models.test_expectations import TestExpectations

_log = logging.getLogger(__name__)


def main(host, bot_test_expectations_factory, argv):
    parser = argparse.ArgumentParser(epilog=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('--verbose', '-v', action='store_true', default=False, help='enable more verbose logging')
    parser.add_argument('--show-results',
                        '-s',
                        action='store_true',
                        default=False,
                        help='Open results dashboard for all removed lines')
    args = parser.parse_args(argv)

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO, format='%(levelname)s: %(message)s')

    port = host.port_factory.get()
    expectations_file = port.path_to_generic_test_expectations_file()
    if not host.filesystem.isfile(expectations_file):
        _log.warning("Didn't find generic expectations file at: " + expectations_file)
        return 1

    remove_flakes_o_matic = RemoveFlakesOMatic(
        host, port, bot_test_expectations_factory, webbrowser)

    test_expectations = remove_flakes_o_matic.get_updated_test_expectations()

    if args.show_results:
        remove_flakes_o_matic.show_removed_results()

    remove_flakes_o_matic.write_test_expectations(test_expectations, expectations_file)
    remove_flakes_o_matic.print_suggested_commit_description()
    return 0


class RemoveFlakesOMatic(object):

    def __init__(self, host, port, bot_test_expectations_factory, browser):
        self._host = host
        self._port = port
        self._expectations_factory = bot_test_expectations_factory
        self._builder_results_by_path = {}
        self._browser = browser
        self._expectations_to_remove_list = None

    def _can_delete_line(self, test_expectation_line):
        """Returns whether a given line in the expectations can be removed.

        Uses results from builder bots to determine if a given line is stale and
        can safely be removed from the TestExpectations file. (i.e. remove if
        the bots show that it's not flaky.) There are also some rules about when
        not to remove lines (e.g. never remove lines with Rebaseline
        expectations, don't remove non-flaky expectations, etc.)

        Args:
            test_expectation_line (TestExpectationLine): A line in the test
                expectation file to test for possible removal.

        Returns:
            True if the line can be removed, False otherwise.
        """
        expectations = test_expectation_line.expectations
        if len(expectations) < 2:
            return False

        # Don't check lines that have expectations like Skip.
        if self._has_unstrippable_expectations(expectations):
            return False

        # Don't check lines unless they're flaky. i.e. At least one expectation is a PASS.
        if not self._has_pass_expectation(expectations):
            return False

        # Don't check lines that have expectations for directories, since
        # the flakiness of all sub-tests isn't as easy to check.
        if self._port.test_isdir(test_expectation_line.name):
            return False

        # The line can be deleted if none of the expectations appear in the
        # actual results or only a PASS expectation appears in the actual
        # results.
        builders_checked = []
        for config in test_expectation_line.matching_configurations:
            builder_name = self._host.builders.builder_name_for_specifiers(config.version, config.build_type)

            if not builder_name:
                _log.debug('No builder with config %s', config)
                # For many configurations, there is no matching builder in
                # blinkpy/common/config/builders.json. We ignore these
                # configurations and make decisions based only on configurations
                # with actual builders.
                continue

            builders_checked.append(builder_name)

            if builder_name not in self._builder_results_by_path.keys():
                _log.error('Failed to find results for builder "%s"', builder_name)
                return False

            results_by_path = self._builder_results_by_path[builder_name]

            # No results means the tests were all skipped, or all results are passing.
            if test_expectation_line.path not in results_by_path.keys():
                continue

            results_for_single_test = results_by_path[test_expectation_line.path]

            expectations_met = self._expectations_that_were_met(test_expectation_line, results_for_single_test)
            if expectations_met != set(['PASS']) and expectations_met != set([]):
                return False

        if builders_checked:
            _log.debug('Checked builders:\n  %s', '\n  '.join(builders_checked))
        else:
            _log.warning('No matching builders for line, deleting line.')
        _log.info('Deleting line "%s"', test_expectation_line.original_string.strip())
        return True

    def _has_pass_expectation(self, expectations):
        return 'PASS' in expectations

    def _expectations_that_were_met(self, test_expectation_line, results_for_single_test):
        """Returns the set of expectations that appear in the given results.

        e.g. If the test expectations is "bug(test) fast/test.html [Crash Failure Pass]"
        and the results are ['TEXT', 'PASS', 'PASS', 'TIMEOUT'], then this method would
        return [Pass Failure] since the Failure expectation is satisfied by 'TEXT', Pass
        by 'PASS' but Crash doesn't appear in the results.

        Args:
            test_expectation_line: A TestExpectationLine object
            results_for_single_test: A list of result strings.
                e.g. ['IMAGE', 'IMAGE', 'PASS']

        Returns:
            A set containing expectations that occurred in the results.
        """
        # TODO(bokan): Does this not exist in a more central place?
        def replace_failing_with_fail(expectation):
            if expectation in ('TEXT', 'IMAGE', 'IMAGE+TEXT', 'AUDIO'):
                return 'FAIL'
            else:
                return expectation

        actual_results = {replace_failing_with_fail(r) for r in results_for_single_test}

        return set(test_expectation_line.expectations) & actual_results

    def _has_unstrippable_expectations(self, expectations):
        """Returns whether any of the given expectations are considered unstrippable.

        Unstrippable expectations are those which should stop a line from being
        removed regardless of builder bot results.

        Args:
            expectations: A list of string expectations.
                E.g. ['PASS', 'FAIL' 'CRASH']

        Returns:
            True if at least one of the expectations is unstrippable. False
            otherwise.
        """
        unstrippable_expectations = (
            'NEEDSMANUALREBASELINE',
            'REBASELINE',
            'SKIP',
            'SLOW',
        )
        return any(s in expectations for s in unstrippable_expectations)

    def _get_builder_results_by_path(self):
        """Returns a dictionary of results for each builder.

        Returns a dictionary where each key is a builder and value is a dictionary containing
        the distinct results for each test. E.g.

        {
            'WebKit Linux Precise': {
                  'test1.html': ['PASS', 'IMAGE'],
                  'test2.html': ['PASS'],
            },
            'WebKit Mac10.10': {
                  'test1.html': ['PASS', 'IMAGE'],
                  'test2.html': ['PASS', 'TEXT'],
            }
        }
        """
        builder_results_by_path = {}
        for builder_name in self._host.builders.all_continuous_builder_names():
            expectations_for_builder = (
                self._expectations_factory.expectations_for_builder(builder_name)
            )

            if not expectations_for_builder:
                # This is not fatal since we may not need to check these
                # results. If we do need these results we'll log an error later
                # when trying to check against them.
                _log.warning('Downloaded results are missing results for builder "%s"', builder_name)
                continue

            builder_results_by_path[builder_name] = (
                expectations_for_builder.all_results_by_path()
            )
        return builder_results_by_path

    def _remove_associated_comments_and_whitespace(self, expectations, removed_index):
        """Removes comments and whitespace from an empty expectation block.

        If the removed expectation was the last in a block of expectations, this method
        will remove any associated comments and whitespace.

        Args:
            expectations: A list of TestExpectationLine objects to be modified.
            removed_index: The index in the above list that was just removed.
        """
        was_last_expectation_in_block = (removed_index == len(expectations)
                                         or expectations[removed_index].is_whitespace()
                                         or expectations[removed_index].is_comment())

        # If the line immediately below isn't another expectation, then the block of
        # expectations definitely isn't empty so we shouldn't remove their associated comments.
        if not was_last_expectation_in_block:
            return

        did_remove_whitespace = False

        # We may have removed the last expectation in a block. Remove any whitespace above.
        while removed_index > 0 and expectations[removed_index - 1].is_whitespace():
            removed_index -= 1
            expectations.pop(removed_index)
            did_remove_whitespace = True

        # If we did remove some whitespace then we shouldn't remove any comments above it
        # since those won't have belonged to this expectation block. For example, commented
        # out expectations, or a section header.
        if did_remove_whitespace:
            return

        # Remove all comments above the removed line.
        while removed_index > 0 and expectations[removed_index - 1].is_comment():
            removed_index -= 1
            expectations.pop(removed_index)

        # Remove all whitespace above the comments.
        while removed_index > 0 and expectations[removed_index - 1].is_whitespace():
            removed_index -= 1
            expectations.pop(removed_index)

    def _expectations_to_remove(self):
        """Computes and returns the expectation lines that should be removed.

        Returns:
            A list of TestExpectationLine objects for lines that can be removed
            from the test expectations file. The result is memoized so that
            subsequent calls will not recompute the result.
        """
        if self._expectations_to_remove_list is not None:
            return self._expectations_to_remove_list

        self._builder_results_by_path = self._get_builder_results_by_path()
        self._expectations_to_remove_list = []
        test_expectations = TestExpectations(self._port, include_overrides=False).expectations()

        for expectation in test_expectations:
            if self._can_delete_line(expectation):
                self._expectations_to_remove_list.append(expectation)

        return self._expectations_to_remove_list

    def get_updated_test_expectations(self):
        """Filters out passing lines from TestExpectations file.

        Reads the current TestExpectations file and, using results from the
        build bots, removes lines that are passing. That is, removes lines that
        were not needed to keep the bots green.

        Returns:
            A TestExpectations object with the passing lines filtered out.
        """

        test_expectations = TestExpectations(self._port, include_overrides=False).expectations()
        for expectation in self._expectations_to_remove():
            index = test_expectations.index(expectation)
            test_expectations.remove(expectation)

            # Remove associated comments and whitespace if we've removed the last expectation under
            # a comment block. Only remove a comment block if it's not separated from the test
            # expectation line by whitespace.
            self._remove_associated_comments_and_whitespace(test_expectations, index)

        return test_expectations

    def show_removed_results(self):
        """Opens a browser showing the removed lines in the results dashboard.

        Opens the results dashboard in the browser, showing all the tests for
        lines removed from the TestExpectations file, allowing the user to
        manually confirm the results.
        """
        url = self._flakiness_dashboard_url()
        _log.info('Opening results dashboard: ' + url)
        self._browser.open(url)

    def write_test_expectations(self, test_expectations, test_expectations_file):
        """Writes the given TestExpectations object to the filesystem.

        Args:
            test_expectations: The TestExpectations object to write.
            test_expectations_file: The full file path of the Blink
                TestExpectations file. This file will be overwritten.
        """
        self._host.filesystem.write_text_file(
            test_expectations_file,
            TestExpectations.list_to_string(test_expectations, reconstitute_only_these=[]))

    def print_suggested_commit_description(self):
        """Prints the body of a suggested CL description after removing some lines."""
        dashboard_url = self._flakiness_dashboard_url()
        bugs = ', '.join(self._bug_numbers())
        message = ('Remove flaky TestExpectations for tests which appear non-flaky recently.\n\n'
                   'This change was made by the update-test-expectations script.\n\n'
                   'Recent test results history:\n%s\n\n'
                   'Bug: %s') % (dashboard_url, bugs)
        _log.info('Suggested commit description:\n' + message)

    def _flakiness_dashboard_url(self):
        removed_test_names = ','.join(x.name for x in self._expectations_to_remove())
        return FlakyTests.FLAKINESS_DASHBOARD_URL % removed_test_names

    def _bug_numbers(self):
        """Returns the list of all bug numbers affected by this change."""
        numbers = []
        for line in self._expectations_to_remove():
            for bug in line.bugs:
                if bug.startswith(CHROMIUM_BUG_PREFIX):
                    numbers.append(bug[len(CHROMIUM_BUG_PREFIX):])
        return sorted(numbers)
