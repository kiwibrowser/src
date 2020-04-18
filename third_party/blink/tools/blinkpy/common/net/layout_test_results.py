# Copyright (c) 2010, Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import json

from blinkpy.common.memoized import memoized
from blinkpy.web_tests.layout_package import json_results_generator


class LayoutTestResult(object):

    def __init__(self, test_name, result_dict):
        self._test_name = test_name
        self._result_dict = result_dict

    def result_dict(self):
        return self._result_dict

    def test_name(self):
        return self._test_name

    def did_pass_or_run_as_expected(self):
        return self.did_pass() or self.did_run_as_expected()

    def did_pass(self):
        return 'PASS' in self.actual_results()

    def did_run_as_expected(self):
        return not self._result_dict.get('is_unexpected', False)

    def is_missing_image(self):
        return self._result_dict.get('is_missing_image', False)

    def is_missing_text(self):
        return self._result_dict.get('is_missing_text', False)

    def is_missing_audio(self):
        return self._result_dict.get('is_missing_audio', False)

    def actual_results(self):
        return self._result_dict['actual']

    def expected_results(self):
        return self._result_dict['expected']

    def has_mismatch_result(self):
        last_retry_result = self.actual_results().split()[-1]
        return last_retry_result in ('TEXT', 'IMAGE', 'IMAGE+TEXT', 'AUDIO')

    def is_missing_baseline(self):
        return self._result_dict['actual'] == 'MISSING'


# FIXME: This should be unified with ResultsSummary or other NRWT layout tests code
# in the layout_tests package.
# This doesn't belong in common.net, but we don't have a better place for it yet.
class LayoutTestResults(object):

    @classmethod
    def results_from_string(cls, string, chromium_revision=None):
        """Creates a LayoutTestResults object from a test result JSON string.

        Args:
            string: JSON string containing layout test result.
            chromium_revision: If given, it will override the chromium_revision
                field in json, to indicate the last revision that has completed
                uploading onto the storage server. chromium_revision can be a
                git hash or position number.
        """

        if not string:
            return None

        content_string = json_results_generator.strip_json_wrapper(string)
        json_dict = json.loads(content_string)
        if not json_dict:
            return None

        return cls(json_dict, chromium_revision)

    def __init__(self, parsed_json, chromium_revision=None):
        self._results = parsed_json
        self._chromium_revision = chromium_revision

    def run_was_interrupted(self):
        return self._results['interrupted']

    def builder_name(self):
        return self._results['builder_name']

    @memoized
    def chromium_revision(self, git=None):
        """Returns the revision of the results in commit position number format."""
        revision = self._chromium_revision or self._results['chromium_revision']
        if not revision.isdigit():
            assert git, 'git is required if the original revision is a git hash.'
            revision = git.commit_position_from_git_commit(revision)
        return int(revision)

    def result_for_test(self, test):
        parts = test.split('/')
        tree = self._test_result_tree()
        for part in parts:
            if part not in tree:
                return None
            tree = tree[part]
        return LayoutTestResult(test, tree)

    def for_each_test(self, handler):
        LayoutTestResults._for_each_test(self._test_result_tree(), handler, '')

    @staticmethod
    def _for_each_test(tree, handler, prefix=''):
        for key in tree:
            new_prefix = (prefix + '/' + key) if prefix else key
            if 'actual' not in tree[key]:
                LayoutTestResults._for_each_test(tree[key], handler, new_prefix)
            else:
                handler(LayoutTestResult(new_prefix, tree[key]))

    def _test_result_tree(self):
        return self._results['tests']

    def _filter_tests(self, result_filter):
        """Returns LayoutTestResult objects for tests which pass the given filter."""
        results = []

        def add_if_passes(result):
            if result_filter(result):
                results.append(result)

        LayoutTestResults._for_each_test(self._test_result_tree(), add_if_passes)
        return sorted(results, key=lambda r: r.test_name())

    def didnt_run_as_expected_results(self):
        # TODO(qyearsley): Rename this method.
        return self._filter_tests(lambda r: not r.did_run_as_expected())
