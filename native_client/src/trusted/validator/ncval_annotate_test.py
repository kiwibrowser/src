
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
sys.path.append(os.path.join(os.path.dirname(__file__),
                             "..", "..", "..", "tests"))

import re
import subprocess
import unittest

from testutils import read_file, write_file
import testutils


ANNOTATE_DIR = os.path.join(os.path.dirname(__file__), "driver")

NACL_CFLAGS = os.environ.get("NACL_CFLAGS", "").split()


class ToolchainTests(testutils.TempDirTestCase):

  def test_ncval_returns_errors(self):
    # Check that ncval returns a non-zero return code when there is a
    # validation failure.
    source = """
void _start() {
#if defined(__i386__) || defined(__x86_64__)
  __asm__("ret"); /* This comment appears in output */
#else
#  error Update this test for other architectures!
#endif
  return 0;
}
"""
    temp_dir = self.make_temp_dir()
    source_file = os.path.join(temp_dir, "code.c")
    write_file(source_file, source)
    testutils.check_call(["x86_64-nacl-gcc", "-g", "-nostartfiles", "-nostdlib",
                          source_file,
                          "-o", os.path.join(temp_dir, "prog")] + NACL_CFLAGS)
    dest_file = os.path.join(self.make_temp_dir(), "file")
    rc = subprocess.call([sys.executable,
                          os.path.join(ANNOTATE_DIR, "ncval_annotate.py"),
                          os.path.join(temp_dir, "prog")],
                         stdout=open(dest_file, "w"))
    # ncval_annotate should propagate the exit code through so that it
    # can be used as a substitute for ncval.
    self.assertEquals(rc, 1)

    # Errors printed in two lines, with interspersed objdump output.
    # The first line starts with an ADDRESS and file/line.
    # The second is the error on the instruction, which ends with "<<<<".
    filter_pattern = "^[0-9a-f]+.*|.*<<<<$"
    actual_lines = [line for line in open(dest_file, "r")
                    if re.match(filter_pattern, line)]
    actual = "".join(actual_lines)
    # Strip windows' carriage return characters.
    actual = actual.replace("\r", "")

    expected_pattern = """
ADDRESS \(FILENAME:[0-9]+, function _start\): unrecognized instruction
\s+ADDRESS:\s+c3\s+retq?\s+<<<<
"""
    expected_pattern = expected_pattern.replace("ADDRESS", "[0-9a-f]+")
    # Cygwin mixes \ and / in filenames, so be liberal in what we accept.
    expected_pattern = expected_pattern.replace("FILENAME", "code.c")

    if re.match(expected_pattern, "\n" + actual) is None:
      raise AssertionError(
        "Output did not match.\n\nEXPECTED:\n%s\nACTUAL:\n%s"
        % (expected_pattern, actual))

  def test_ncval_handles_many_errors(self):
    # This tests for
    # http://code.google.com/p/nativeclient/issues/detail?id=915,
    # where ncval_annotate would truncate the number of results at 100.
    disallowed = """
#if defined(__i386__) || defined(__x86_64__)
  __asm__("int $0x80");
#else
#  error Update this test for other architectures!
#endif
"""
    source = "void _start() { %s }" % (disallowed * 150)
    temp_dir = self.make_temp_dir()
    source_file = os.path.join(temp_dir, "code.c")
    write_file(source_file, source)
    testutils.check_call(["x86_64-nacl-gcc", "-g", "-nostartfiles", "-nostdlib",
                          source_file,
                          "-o", os.path.join(temp_dir, "prog")] + NACL_CFLAGS)
    dest_file = os.path.join(self.make_temp_dir(), "file")
    subprocess.call([sys.executable,
                     os.path.join(ANNOTATE_DIR, "ncval_annotate.py"),
                     os.path.join(temp_dir, "prog")],
                    stdout=open(dest_file, "w"))
    # Filter unrecognized instructions that are printed, one per bundle.
    filter_pattern = ".*<<<<$"
    failures = len([line for line in open(dest_file, "r")
                    if re.match(filter_pattern, line)])
    self.assertEquals(failures, 10)


if __name__ == "__main__":
  unittest.main()
