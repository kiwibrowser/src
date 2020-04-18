"""Test format for the NDK tests."""
import os

import libcxx.android.test.format
import lit.util  # pylint: disable=import-error


def prune_xfails(test):
    """Removes most xfail handling from tests.

    We need to keep some xfail handling because some tests in libc++ that
    really should be using REQUIRES actually used XFAIL (i.e. `XFAIL: c++11`).
    """
    test.xfails = [x for x in test.xfails if x.startswith('c++')]


class TestFormat(libcxx.android.test.format.TestFormat):
    """Loose wrapper around the Android format that disables XFAIL handling."""

    # pylint: disable=too-many-arguments
    def __init__(self, cxx, libcxx_src_root, libcxx_obj_root, build_dir,
                 device_dir, timeout, exec_env=None, build_only=False):
        libcxx.android.test.format.TestFormat.__init__(
            self,
            cxx,
            libcxx_src_root,
            libcxx_obj_root,
            device_dir,
            timeout,
            exec_env,
            build_only)
        self.build_dir = build_dir

    def _evaluate_pass_test(self, test, tmp_base, lit_config, test_cxx,
                            parsers):
        """Clears the test's xfail list before delegating to the parent."""
        prune_xfails(test)
        tmp_base = os.path.join(self.build_dir, *test.path_in_suite)
        return super(TestFormat, self)._evaluate_pass_test(
            test, tmp_base, lit_config, test_cxx, parsers)

    def _evaluate_fail_test(self, test, test_cxx, parsers):
        """Clears the test's xfail list before delegating to the parent."""
        prune_xfails(test)
        return super(TestFormat, self)._evaluate_fail_test(
            test, test_cxx, parsers)

    def _clean(self, exec_path):
        exec_file = os.path.basename(exec_path)
        if not self.build_only:
            device_path = self._working_directory(exec_file)
            cmd = ['adb', 'shell', 'rm', '-r', device_path]
            lit.util.executeCommand(cmd)
