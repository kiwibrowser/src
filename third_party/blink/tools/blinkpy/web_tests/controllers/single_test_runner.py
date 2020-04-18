# Copyright (C) 2011 Google Inc. All rights reserved.
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


import hashlib
import logging
import re

from blinkpy.web_tests.controllers import repaint_overlay
from blinkpy.web_tests.controllers import test_result_writer
from blinkpy.web_tests.port.driver import DeviceFailure, DriverInput, DriverOutput
from blinkpy.web_tests.models import test_expectations
from blinkpy.web_tests.models import test_failures
from blinkpy.web_tests.models.test_results import TestResult
from blinkpy.web_tests.models import testharness_results


_log = logging.getLogger(__name__)


def run_single_test(
        port, options, results_directory, worker_name, primary_driver,
        secondary_driver, test_input, stop_when_done):
    runner = SingleTestRunner(
        port, options, results_directory, worker_name, primary_driver,
        secondary_driver, test_input, stop_when_done)
    try:
        return runner.run()
    except DeviceFailure as error:
        _log.error('device failed: %s', error)
        return TestResult(test_input.test_name, device_failed=True)


class SingleTestRunner(object):
    def __init__(self, port, options, results_directory, worker_name,
                 primary_driver, secondary_driver, test_input, stop_when_done):
        self._port = port
        self._filesystem = port.host.filesystem
        self._options = options
        self._results_directory = results_directory
        self._driver = primary_driver
        self._reference_driver = primary_driver
        self._timeout_ms = test_input.timeout_ms
        self._worker_name = worker_name
        self._test_name = test_input.test_name
        self._should_run_pixel_test = test_input.should_run_pixel_test
        self._should_run_pixel_test_first = test_input.should_run_pixel_test_first
        self._reference_files = test_input.reference_files
        self._stop_when_done = stop_when_done

        # If this is a virtual test that uses the default flags instead of the
        # virtual flags for it's references, run it on the secondary driver so
        # that the primary driver does not need to be restarted.
        if (secondary_driver and
                self._port.is_virtual_test(self._test_name) and
                not self._port.lookup_virtual_reference_args(self._test_name)):
            self._reference_driver = secondary_driver

        if self._reference_files:
            # Detect and report a test which has a wrong combination of expectation files.
            # For example, if 'foo.html' has two expectation files, 'foo-expected.html' and
            # 'foo-expected.png', we should warn users. One test file must be used exclusively
            # in either layout tests or reftests, but not in both. Text expectation is an
            # exception.
            for suffix in self._port.BASELINE_EXTENSIONS:
                if suffix == '.txt':
                    continue
                expected_filename = self._port.expected_filename(self._test_name, suffix)
                if self._filesystem.exists(expected_filename):
                    _log.error('%s is a reftest, but has an unused expectation file. Please remove %s.',
                               self._test_name, expected_filename)

    def _expected_driver_output(self):
        return DriverOutput(self._port.expected_text(self._test_name),
                            self._port.expected_image(self._test_name),
                            self._port.expected_checksum(self._test_name),
                            self._port.expected_audio(self._test_name))

    def _should_fetch_expected_checksum(self):
        return self._should_run_pixel_test and not self._options.reset_results

    def _driver_input(self):
        # The image hash is used to avoid doing an image dump if the
        # checksums match, so it should be set to a blank value if we
        # are generating a new baseline.  (Otherwise, an image from a
        # previous run will be copied into the baseline.)
        image_hash = None
        if self._should_fetch_expected_checksum():
            image_hash = self._port.expected_checksum(self._test_name)

        test_base = self._port.lookup_virtual_test_base(self._test_name)
        if test_base:
            # If the file actually exists under the virtual dir, we want to use it (largely for virtual references),
            # but we want to use the extra command line args either way.
            if self._filesystem.exists(self._port.abspath_for_test(self._test_name)):
                test_name = self._test_name
            else:
                test_name = test_base
            args = self._port.lookup_virtual_test_args(self._test_name)
        else:
            test_name = self._test_name
            args = self._port.lookup_physical_test_args(self._test_name)
        return DriverInput(test_name, self._timeout_ms, image_hash, self._should_run_pixel_test, args)

    def run(self):
        if self._options.enable_sanitizer:
            return self._run_sanitized_test()
        if self._options.reset_results or self._options.copy_baselines:
            if self._reference_files:
                expected_txt_filename = self._port.expected_filename(self._test_name, '.txt')
                if not self._filesystem.exists(expected_txt_filename):
                    reftest_type = set([reference_file[0] for reference_file in self._reference_files])
                    result = TestResult(self._test_name, reftest_type=reftest_type)
                    result.type = test_expectations.SKIP
                    return result
                self._should_run_pixel_test = False
            return self._run_rebaseline()
        if self._reference_files:
            return self._run_reftest()
        return self._run_compare_test()

    def _run_sanitized_test(self):
        # running a sanitized test means that we ignore the actual test output and just look
        # for timeouts and crashes (real or forced by the driver). Most crashes should
        # indicate problems found by a sanitizer (ASAN, LSAN, etc.), but we will report
        # on other crashes and timeouts as well in order to detect at least *some* basic failures.
        driver_output = self._driver.run_test(self._driver_input(), self._stop_when_done)
        expected_driver_output = self._expected_driver_output()
        failures = self._handle_error(driver_output)
        test_result = TestResult(self._test_name, failures, driver_output.test_time, driver_output.has_stderr(),
                                 pid=driver_output.pid, crash_site=driver_output.crash_site)
        test_result_writer.write_test_result(self._filesystem, self._port, self._results_directory,
                                             self._test_name, driver_output, expected_driver_output, test_result.failures)
        return test_result

    def _run_compare_test(self):
        """Runs the signle test and returns test result."""
        driver_output = self._driver.run_test(self._driver_input(), self._stop_when_done)
        expected_driver_output = self._expected_driver_output()

        test_result = self._compare_output(expected_driver_output, driver_output)
        test_result_writer.write_test_result(self._filesystem, self._port, self._results_directory,
                                             self._test_name, driver_output, expected_driver_output, test_result.failures)
        return test_result

    def _run_rebaseline(self):
        """Similar to _run_compare_test(), but has the side effect of updating or adding baselines.
        This is called when --reset-results and/or --copy-baselines are specified in the command line.
        If --reset-results, in the returned result we treat baseline mismatch as success."""
        driver_output = self._driver.run_test(self._driver_input(), self._stop_when_done)
        expected_driver_output = self._expected_driver_output()
        actual_failures = self._compare_output(expected_driver_output, driver_output).failures
        failures = self._handle_error(driver_output) if self._options.reset_results else actual_failures
        test_result_writer.write_test_result(self._filesystem, self._port, self._results_directory,
                                             self._test_name, driver_output, expected_driver_output, failures)
        self._update_or_add_new_baselines(driver_output, actual_failures)
        return TestResult(self._test_name, failures, driver_output.test_time, driver_output.has_stderr(),
                          pid=driver_output.pid, crash_site=driver_output.crash_site)

    _render_tree_dump_pattern = re.compile(r"^layer at \(\d+,\d+\) size \d+x\d+\n")

    def _update_or_add_new_baselines(self, driver_output, failures):
        """Updates or adds new baselines for the test if necessary."""
        if (test_failures.has_failure_type(test_failures.FailureTimeout, failures) or
                test_failures.has_failure_type(test_failures.FailureCrash, failures)):
            return
        self._save_baseline_data(driver_output.text, '.txt',
                                 test_failures.has_failure_type(test_failures.FailureMissingResult, failures))
        self._save_baseline_data(driver_output.audio, '.wav',
                                 test_failures.has_failure_type(test_failures.FailureMissingAudio, failures))
        self._save_baseline_data(driver_output.image, '.png',
                                 test_failures.has_failure_type(test_failures.FailureMissingImage, failures))

    def _save_baseline_data(self, data, extension, is_missing):
        if data is None:
            return
        port = self._port
        fs = self._filesystem

        flag_specific_dir = port.baseline_flag_specific_dir()
        if flag_specific_dir:
            output_dir = fs.join(flag_specific_dir, fs.dirname(self._test_name))
        elif self._options.copy_baselines:
            output_dir = fs.join(port.baseline_version_dir(), fs.dirname(self._test_name))
        else:
            output_dir = fs.dirname(port.expected_filename(self._test_name, extension, fallback_base_for_virtual=False))

        fs.maybe_make_directory(output_dir)
        output_basename = fs.basename(fs.splitext(self._test_name)[0] + '-expected' + extension)
        output_path = fs.join(output_dir, output_basename)

        # Remove |output_path| if it exists and is not the generic expectation to
        # avoid extra baseline if the new baseline is the same as the fallback baseline.
        generic_dir = fs.join(port.layout_tests_dir(),
                              fs.dirname(port.lookup_virtual_test_base(self._test_name) or self._test_name))
        if output_dir != generic_dir and fs.exists(output_path):
            _log.info('Removing the current baseline "%s"', port.relative_test_filename(output_path))
            fs.remove(output_path)

        current_expected_path = port.expected_filename(self._test_name, extension)
        if not fs.exists(current_expected_path):
            if not is_missing or not self._options.reset_results:
                return
        elif fs.sha1(current_expected_path) == hashlib.sha1(data).hexdigest():
            if self._options.reset_results:
                _log.info('Not writing new expected result "%s" because it is the same as the current expected result',
                          port.relative_test_filename(output_path))
            else:
                _log.info('Not copying baseline to "%s" because the actual result is the same as the current expected result',
                          port.relative_test_filename(output_path))
            return

        if self._options.reset_results:
            _log.info('Writing new expected result "%s"', port.relative_test_filename(output_path))
            port.update_baseline(output_path, data)
        else:
            _log.info('Copying baseline to "%s"', port.relative_test_filename(output_path))
            fs.copyfile(current_expected_path, output_path)

    def _handle_error(self, driver_output, reference_filename=None):
        """Returns test failures if some unusual errors happen in driver's run.

        Args:
          driver_output: The output from the driver.
          reference_filename: The full path to the reference file which produced the driver_output.
              This arg is optional and should be used only in reftests until we have a better way to know
              which html file is used for producing the driver_output.
        """
        failures = []
        if driver_output.timeout:
            failures.append(test_failures.FailureTimeout(bool(reference_filename)))

        if reference_filename:
            testname = self._port.relative_test_filename(reference_filename)
        else:
            testname = self._test_name

        if driver_output.crash:
            failures.append(test_failures.FailureCrash(bool(reference_filename),
                                                       driver_output.crashed_process_name,
                                                       driver_output.crashed_pid,
                                                       self._port.output_contains_sanitizer_messages(driver_output.crash_log)))
            if driver_output.error:
                _log.debug('%s %s crashed, (stderr lines):', self._worker_name, testname)
            else:
                _log.debug('%s %s crashed, (no stderr)', self._worker_name, testname)
        elif driver_output.leak:
            failures.append(test_failures.FailureLeak(bool(reference_filename),
                                                      driver_output.leak_log))
            _log.debug('%s %s leaked', self._worker_name, testname)
        elif driver_output.error:
            _log.debug('%s %s output stderr lines:', self._worker_name, testname)
        for line in driver_output.error.splitlines():
            _log.debug('  %s', line)
        return failures

    def _compare_output(self, expected_driver_output, driver_output):
        failures = []
        failures.extend(self._handle_error(driver_output))

        if driver_output.crash:
            # Don't continue any more if we already have a crash.
            # In case of timeouts, we continue since we still want to see the text and image output.
            return TestResult(self._test_name, failures, driver_output.test_time, driver_output.has_stderr(),
                              pid=driver_output.pid, crash_site=driver_output.crash_site)

        is_testharness_test, testharness_failures = self._compare_testharness_test(driver_output, expected_driver_output)
        if is_testharness_test:
            failures.extend(testharness_failures)

        compare_functions = []
        compare_image_fn = (self._compare_image, (expected_driver_output, driver_output))
        compare_txt_fn = (self._compare_text, (expected_driver_output.text, driver_output.text))
        compare_audio_fn = (self._compare_audio, (expected_driver_output.audio, driver_output.audio))

        if self._should_run_pixel_test_first:
            if driver_output.image_hash and self._should_run_pixel_test:
                compare_functions.append(compare_image_fn)
            elif not is_testharness_test:
                compare_functions.append(compare_txt_fn)
        else:
            if not is_testharness_test:
                compare_functions.append(compare_txt_fn)
            if self._should_run_pixel_test:
                compare_functions.append(compare_image_fn)
        compare_functions.append(compare_audio_fn)

        for func, args in compare_functions:
            failures.extend(func(*args))

        has_repaint_overlay = (repaint_overlay.result_contains_repaint_rects(expected_driver_output.text) or
                               repaint_overlay.result_contains_repaint_rects(driver_output.text))
        return TestResult(self._test_name, failures, driver_output.test_time, driver_output.has_stderr(),
                          pid=driver_output.pid, has_repaint_overlay=has_repaint_overlay)

    def _compare_testharness_test(self, driver_output, expected_driver_output):
        if expected_driver_output.text:
            return False, []

        if self._is_render_tree(driver_output.text):
            return False, []

        text = driver_output.text or ''

        if not testharness_results.is_testharness_output(text):
            return False, []
        if not testharness_results.is_testharness_output_passing(text):
            return True, [test_failures.FailureTestHarnessAssertion()]
        return True, []

    def _is_render_tree(self, text):
        return text and 'layer at (0,0) size' in text

    def _is_layer_tree(self, text):
        return text and '{\n  "layers": [' in text

    def _compare_text(self, expected_text, actual_text):
        if not actual_text:
            return []
        if not expected_text:
            return [test_failures.FailureMissingResult()]

        normalized_actual_text = self._get_normalized_output_text(actual_text)
        # Assuming expected_text is already normalized.
        if not self._port.do_text_results_differ(expected_text, normalized_actual_text):
            return []

        # Determine the text mismatch type

        def remove_chars(text, chars):
            for char in chars:
                text = text.replace(char, '')
            return text

        def remove_ng_text(results):
          processed = re.sub(r'LayoutNG(BlockFlow|ListItem|TableCell)', r'Layout\1', results)
          # LayoutTableCaption doesn't override LayoutBlockFlow::GetName, so
          # render tree dumps have "LayoutBlockFlow" for captions.
          processed = re.sub('LayoutNGTableCaption', 'LayoutBlockFlow', processed)
          return processed

        def is_ng_name_mismatch(expected, actual):
            if 'LayoutNGBlockFlow' not in actual:
                return False
            if not self._is_render_tree(actual) and not self._is_layer_tree(actual):
                return False
            # There's a mix of NG and legacy names in both expected and actual,
            # so just remove NG from both.
            return not self._port.do_text_results_differ(remove_ng_text(expected), remove_ng_text(actual))

        # LayoutNG name mismatch (e.g., LayoutBlockFlow vs. LayoutNGBlockFlow)
        # is treated as pass
        if is_ng_name_mismatch(expected_text, normalized_actual_text):
            return []

        # General text mismatch
        if self._port.do_text_results_differ(
                remove_chars(expected_text, ' \t\n'),
                remove_chars(normalized_actual_text, ' \t\n')):
            return [test_failures.FailureTextMismatch()]

        # Space-only mismatch
        if not self._port.do_text_results_differ(
                remove_chars(expected_text, ' \t'),
                remove_chars(normalized_actual_text, ' \t')):
            return [test_failures.FailureSpacesAndTabsTextMismatch()]

        # Newline-only mismatch
        if not self._port.do_text_results_differ(
                remove_chars(expected_text, '\n'),
                remove_chars(normalized_actual_text, '\n')):
            return [test_failures.FailureLineBreaksTextMismatch()]

        # Spaces and newlines
        return [test_failures.FailureSpaceTabLineBreakTextMismatch()]

    def _compare_audio(self, expected_audio, actual_audio):
        failures = []
        if (expected_audio and actual_audio and
                self._port.do_audio_results_differ(expected_audio, actual_audio)):
            failures.append(test_failures.FailureAudioMismatch())
        elif actual_audio and not expected_audio:
            failures.append(test_failures.FailureMissingAudio())
        return failures

    def _get_normalized_output_text(self, output):
        """Returns the normalized text output, i.e. the output in which
        the end-of-line characters are normalized to "\n".
        """
        # Running tests on Windows produces "\r\n".  The "\n" part is helpfully
        # changed to "\r\n" by our system (Python/Cygwin), resulting in
        # "\r\r\n", when, in fact, we wanted to compare the text output with
        # the normalized text expectation files.
        return output.replace('\r\r\n', '\r\n').replace('\r\n', '\n')

    # FIXME: This function also creates the image diff. Maybe that work should
    # be handled elsewhere?
    def _compare_image(self, expected_driver_output, driver_output):
        failures = []
        # If we didn't produce a hash file, this test must be text-only.
        if driver_output.image_hash is None:
            return failures
        if not expected_driver_output.image:
            failures.append(test_failures.FailureMissingImage())
        elif not expected_driver_output.image_hash:
            failures.append(test_failures.FailureMissingImageHash())
        elif driver_output.image_hash != expected_driver_output.image_hash:
            diff, err_str = self._port.diff_image(expected_driver_output.image, driver_output.image)
            if err_str:
                _log.warning('  %s : %s', self._test_name, err_str)
                failures.append(test_failures.FailureImageHashMismatch())
                driver_output.error = (driver_output.error or '') + err_str
            else:
                driver_output.image_diff = diff
                if driver_output.image_diff:
                    failures.append(test_failures.FailureImageHashMismatch())
                else:
                    # See https://bugs.webkit.org/show_bug.cgi?id=69444 for why this isn't a full failure.
                    _log.warning('  %s -> pixel hash failed (but diff passed)', self._test_name)
        return failures

    def _run_reftest(self):
        test_output = self._driver.run_test(self._driver_input(), self._stop_when_done)
        total_test_time = test_output.test_time
        expected_output = None
        test_result = None

        expected_text = self._port.expected_text(self._test_name)
        expected_text_output = DriverOutput(text=expected_text, image=None, image_hash=None, audio=None)

        # If the test crashed, or timed out, there's no point in running the reference at all.
        # This can save a lot of execution time if we have a lot of crashes or timeouts.
        if test_output.crash or test_output.timeout:
            test_result = self._compare_output(expected_text_output, test_output)

            if test_output.crash:
                test_result_writer.write_test_result(self._filesystem, self._port, self._results_directory,
                                                     self._test_name, test_output, expected_text_output, test_result.failures)
            return test_result

        # A reftest can have multiple match references and multiple mismatch references;
        # the test fails if any mismatch matches and all of the matches don't match.
        # To minimize the number of references we have to check, we run all of the mismatches first,
        # then the matches, and short-circuit out as soon as we can.
        # Note that sorting by the expectation sorts "!=" before "==" so this is easy to do.

        putAllMismatchBeforeMatch = sorted
        reference_test_names = []
        for expectation, reference_filename in putAllMismatchBeforeMatch(self._reference_files):
            if self._port.lookup_virtual_test_base(self._test_name):
                args = self._port.lookup_virtual_reference_args(self._test_name)
            else:
                args = self._port.lookup_physical_reference_args(self._test_name)
            reference_test_name = self._port.relative_test_filename(reference_filename)
            reference_test_names.append(reference_test_name)
            driver_input = DriverInput(reference_test_name, self._timeout_ms,
                                       image_hash=test_output.image_hash, should_run_pixel_test=True, args=args)
            expected_output = self._reference_driver.run_test(driver_input, self._stop_when_done)
            total_test_time += expected_output.test_time
            test_result = self._compare_output_with_reference(
                expected_output, test_output, reference_filename, expectation == '!=')

            if (expectation == '!=' and test_result.failures) or (expectation == '==' and not test_result.failures):
                break

        assert expected_output

        if expected_text:
            text_output = DriverOutput(text=test_output.text, image=None, image_hash=None, audio=None)
            text_compare_result = self._compare_output(expected_text_output, text_output)
            test_result.failures.extend(text_compare_result.failures)
            test_result.has_repaint_overlay = text_compare_result.has_repaint_overlay
            expected_output.text = expected_text_output.text

        test_result_writer.write_test_result(self._filesystem, self._port, self._results_directory,
                                             self._test_name, test_output, expected_output, test_result.failures)

        # FIXME: We don't really deal with a mix of reftest types properly. We pass in a set() to reftest_type
        # and only really handle the first of the references in the result.
        reftest_type = list(set([reference_file[0] for reference_file in self._reference_files]))
        return TestResult(self._test_name, test_result.failures, total_test_time,
                          test_result.has_stderr, reftest_type=reftest_type, pid=test_result.pid, crash_site=test_result.crash_site,
                          references=reference_test_names, has_repaint_overlay=test_result.has_repaint_overlay)

    # The returned TestResult always has 0 test_run_time. _run_reftest() calculates total_run_time from test outputs.
    def _compare_output_with_reference(self, reference_driver_output, actual_driver_output, reference_filename, mismatch):
        has_stderr = reference_driver_output.has_stderr() or actual_driver_output.has_stderr()
        failures = []
        failures.extend(self._handle_error(actual_driver_output))
        if failures:
            # Don't continue any more if we already have crash or timeout.
            return TestResult(self._test_name, failures, 0, has_stderr, crash_site=actual_driver_output.crash_site)
        failures.extend(self._handle_error(reference_driver_output, reference_filename=reference_filename))
        if failures:
            return TestResult(self._test_name, failures, 0, has_stderr, pid=actual_driver_output.pid,
                              crash_site=reference_driver_output.crash_site)

        if not actual_driver_output.image_hash:
            failures.append(test_failures.FailureReftestNoImageGenerated(reference_filename))
        elif not reference_driver_output.image_hash:
            failures.append(test_failures.FailureReftestNoReferenceImageGenerated(reference_filename))
        elif mismatch:
            if reference_driver_output.image_hash == actual_driver_output.image_hash:
                failures.append(test_failures.FailureReftestMismatchDidNotOccur(reference_filename))
        elif reference_driver_output.image_hash != actual_driver_output.image_hash:
            diff, err_str = self._port.diff_image(reference_driver_output.image, actual_driver_output.image)
            if diff:
                failures.append(test_failures.FailureReftestMismatch(reference_filename))
            elif err_str:
                _log.error(err_str)
            else:
                _log.warning("  %s -> ref test hashes didn't match but diff passed", self._test_name)

        return TestResult(self._test_name, failures, 0, has_stderr, pid=actual_driver_output.pid)
