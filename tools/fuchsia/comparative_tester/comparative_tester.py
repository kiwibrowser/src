#!/usr/bin/env python3
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script takes in a list of test targets to be run on both Linux and
# Fuchsia devices and then compares their output to each other, extracting the
# relevant performance data from the output of gtest.

import os
import re
import subprocess
import sys
from typing import *

import target_spec


def RunCommand(command: List[str], msg: str,
               ignore_errors: bool = False) -> str:
  "One-shot start and complete command with useful default kwargs"
  proc = subprocess.run(
      command,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      stdin=subprocess.DEVNULL)
  out = proc.stdout.decode("utf-8", errors="ignore")
  err = proc.stderr.decode("utf-8", errors="ignore")
  if proc.returncode != 0:
    sys.stderr.write("{}\nreturn code: {}\nstdout: {}\nstderr: {}".format(
        msg, proc.returncode, out, err))
    if not ignore_errors:
      raise subprocess.SubprocessError(
          "Command failed to complete successfully. {}".format(command))
  return out


class TestTarget(object):
  """TestTarget encapsulates a single BUILD.gn target, extracts a name from the
  target string, and manages the building and running of the target for both
  Linux and Fuchsia.
  """

  def __init__(self, target: str, filters: str = "") -> None:
    self.Target = target
    self.Name = target.split(":")[-1]
    if filters != "":
      self.FilterFlag = "--gtest_filter='" + filters + "'"
    else:
      self.FilterFlag = ""

  def ExecFuchsia(self, out_dir: str, run_locally: bool) -> str:
    runner_name = "{}/bin/run_{}".format(out_dir, self.Name)
    command = [runner_name, self.FilterFlag]
    if not run_locally:
      command.append("-d")

    # TODO(stephanstross): Remove this when fuchsia logging fix lands
    command.extend([
        "--test-launcher-summary-output", "/tmp/fuchsia.json", "--",
        "--gtest_output=json:/data/test_summary.json"
    ])
    return RunCommand(
        command,
        "Test {} failed on fuchsia!".format(self.Target),
        ignore_errors=True)

  def ExecLinux(self, out_dir: str, run_locally: bool) -> str:
    local_path = "{}/{}".format(out_dir, self.Name)
    command = []
    if not run_locally:
      user = target_spec.linux_device_user
      ip = target_spec.linux_device_hostname
      host_machine = "{0}@{1}".format(user, ip)
      # Next is the transfer of all the directories to the destination device.
      self.TransferDependencies(out_dir, host_machine)
      command = [
          "ssh", "{}@{}".format(user, ip), "xvfb-run -a {1}/{0}/{1} {2}".format(
              out_dir, self.Name, self.FilterFlag)
      ]
    else:
      command = [local_path, self.FilterFlag]
    result = RunCommand(
        command,
        "Test {} failed on linux!".format(self.Target),
        ignore_errors=True)
    # Clean up the copy of the test files on the host after execution
    RunCommand(["rm", "-rf", self.Name],
               "Failed to remove host directory for {}".format(self.Target))
    return result

  def TransferDependencies(self, out_dir: str, host: str):
    gn_desc = ["gn", "desc", out_dir, self.Target, "runtime_deps"]
    out = RunCommand(
        gn_desc, "Failed to get dependencies of target {}".format(self.Target))

    paths = []
    for line in out.split("\n"):
      if line == "":
        continue
      line = out_dir + "/" + line.strip()
      line = os.path.abspath(line)
      paths.append(line)
    common = os.path.commonpath(paths)
    paths = [os.path.relpath(path, common) for path in paths]

    archive_name = self.Name + ".tar.gz"
    # Compress the dependencies of the test.
    RunCommand(
        ["tar", "-czf", archive_name] + paths,
        "{} dependency compression failed".format(self.Target),
    )
    # Make sure the containing directory exists on the host, for easy cleanup.
    RunCommand(["ssh", host, "mkdir -p {}".format(self.Name)],
               "Failed to create directory on host for {}".format(self.Target))
    # Transfer the test deps to the host.
    RunCommand(
        ["scp", archive_name, "{}:{}/{}".format(host, self.Name, archive_name)],
        "{} dependency transfer failed".format(self.Target),
    )
    # Decompress the dependencies once they're on the host.
    RunCommand(
        [
            "ssh", host, "tar -xzf {0}/{1} -C {0}".format(
                self.Name, archive_name)
        ],
        "{} dependency decompression failed".format(self.Target),
    )
    # Clean up the local copy of the archive that is no longer needed.
    RunCommand(
        ["rm", archive_name],
        "{} dependency archive cleanup failed".format(self.Target),
    )


def ExtractParts(string: str) -> (str, float, str):
  """This function accepts lines like the one that follow this sentence, and
  attempts to extract all of the relevant pieces of information from it.

   task: 1_threads_scheduling_to_io_pump= .47606626678091973 us/task

  The above line would be split into chunks as follows:

  info=data units

  info and units can be any string, and data must be a valid float. data and
  units must be separated by a space, and info and data must be separated by
  at least an '='
  """
  pieces = string.split("=")
  info = pieces[0].strip()
  measure = pieces[1].strip().split(" ")
  data = float(measure[0])
  units = measure[1].strip()
  return info, data, units


class ResultLine(object):
  """This class is a single line of the comparison between linux and fuchsia.
  GTests output several lines of important info per test, which are collected,
  and then the pertinent pieces of information are extracted and stored in a
  ResultLine for each line, containing a shared description and unit, as well as
  linux and fuchsia performance scores.
  """

  def __init__(self, linux_line: str, fuchsia_line: str) -> None:
    linux_info, linux_val, linux_unit = ExtractParts(linux_line)
    fuchsia_info, fuchsia_val, fuchsia_unit = ExtractParts(fuchsia_line)

    if linux_info != fuchsia_info:
      print("Info mismatch! fuchsia was: {}".format(fuchsia_info))
    if linux_unit != fuchsia_unit:
      print("Unit mismatch! fuchsia was: {}".format(fuchsia_unit))

    self.desc = linux_info
    self.linux = linux_val
    self.fuchsia = fuchsia_val
    self.unit = fuchsia_unit

  def comparison(self) -> float:
    return (self.fuchsia / self.linux) * 100.0

  def ToCsvFormat(self) -> str:
    return ",".join([
        self.desc.replace(",", ";"),
        str(self.linux),
        str(self.fuchsia),
        str(self.comparison()),
        self.unit,
    ])

  def __format__(self, format_spec: str) -> str:
    return "{} in {}: linux:{}, fuchsia:{}, ratio:{}".format(
        self.desc, self.unit, self.linux, self.fuchsia, self.comparison())


class TestComparison(object):
  """This class represents a single test target, and all of its informative
  lines of output for each test case, extracted into statistical comparisons of
  this run on linux v fuchsia.
  """

  def __init__(self, name: str, tests: Dict[str, List[ResultLine]]) -> None:
    self.suite_name = name
    self.tests = tests

  def MakeCsvFormat(self) -> str:
    lines = []
    for test_name, lines in self.tests.items():
      for line in lines:
        lines.append("{},{},{}".format(self.suite_name, test_name,
                                       line.MakeCsvFormat()))
    return "\n".join(lines)

  def __format__(self, format_spec: str) -> str:
    lines = [self.suite_name]
    for test_case, lines in self.tests.items():
      lines.append("  {}".format(test_case))
      for line in lines:
        lines.append("    {}".format(line))
    return "\n".join(lines)


def ExtractCases(out_lines: List[str]) -> Dict[str, List[str]]:
  """ExtractCases attempts to associate GTest names to the lines of output that
  they produce. Given a list of input like the following:

  [==========] Running 24 tests from 10 test cases.
  [----------] Global test environment set-up.
  [----------] 9 tests from ScheduleWorkTest
  [ RUN      ] ScheduleWorkTest.ThreadTimeToIOFromOneThread
  *RESULT task: 1_threads_scheduling_to_io_pump= .47606626678091973 us/task
  RESULT task_min_batch_time: 1_threads_scheduling_to_io_pump= .335 us/task
  RESULT task_max_batch_time: 1_threads_scheduling_to_io_pump= 5.071 us/task
  *RESULT task_thread_time: 1_threads_scheduling_to_io_pump= .3908787013 us/task
  [       OK ] ScheduleWorkTest.ThreadTimeToIOFromOneThread (5352 ms)
  [ RUN      ] ScheduleWorkTest.ThreadTimeToIOFromTwoThreads
  *RESULT task: 2_threads_scheduling_to_io_pump= 6.216794903666874 us/task
  RESULT task_min_batch_time: 2_threads_scheduling_to_io_pump= 2.523 us/task
  RESULT task_max_batch_time: 2_threads_scheduling_to_io_pump= 142.989 us/task
  *RESULT task_thread_time: 2_threads_scheduling_to_io_pump= 2.02621823 us/task
  [       OK ] ScheduleWorkTest.ThreadTimeToIOFromTwoThreads (5022 ms)
  [ RUN      ] ScheduleWorkTest.ThreadTimeToIOFromFourThreads

  It will first skip all lines which do not contain either RUN or RESULT.
  Then, each 'RUN' line is stripped of the bracketed portion, down to just the
  name of the test, and then placed into a dictionary that maps it to all the
  lines beneath it, up to the next RUN line. The RESULT lines all have their
  RESULT portions chopped out as well, and only the piece following RESULT is
  kept

  {'ScheduleWorkTest.ThreadTimeToIOFromOneThread':[
    'task: 1_threads_scheduling_to_io_pump= .47606626678091973 us/task',
    'task_min_batch_time: 1_threads_scheduling_to_io_pump= .335 us/task',
    'task_max_batch_time: 1_threads_scheduling_to_io_pump= 5.071 us/task',
    'task_thread_time: 1_threads_scheduling_to_io_pump= .390834314 us/task'],
  'ScheduleWorkTest.ThreadTimeToIOFromTwoThreads':[
    'task: 2_threads_scheduling_to_io_pump= 6.216794903666874 us/task',
    'task_min_batch_time: 2_threads_scheduling_to_io_pump= 2.523 us/task',
    'task_max_batch_time: 2_threads_scheduling_to_io_pump= 142.989 us/task',
    'task_thread_time: 2_threads_scheduling_to_io_pump= 2.02620013 us/task'],
  'ScheduleWorkTest.ThreadTimeToIOFromFourThreads':[]}
  """
  lines = []
  for line in out_lines:
    if "RUN" in line or "RESULT" in line:
      lines.append(line)
  cases = {}
  name = ""
  case_lines = []
  for line in lines:
    # We've hit a new test suite, write the old accumulators and start new
    # ones. The name variable is checked to make sure this isn't the first one
    # in the list of lines
    if "RUN" in line:
      if name != "":
        cases[name] = case_lines
        case_lines = []
      name = line.split("]")[-1]  # Get the actual name of the test case.

    else:
      if "RESULT" not in line:
        print("{} did not get filtered!".format(line))
      line_trimmed = line.split("RESULT")[-1].strip()
      case_lines.append(line_trimmed)
  return cases


def CollateTests(linux_lines: List[str], fuchsia_lines: List[str],
                 test_target: str) -> TestComparison:
  """This function takes the GTest output of a single test target, and matches
  the informational sections of the outputs together, before collapsing them
  down into ResultLines attached to TestComparisons.
  """

  linux_cases = ExtractCases(linux_lines)
  fuchsia_cases = ExtractCases(fuchsia_lines)

  comparisons = {}
  for case_name, linux_case_lines in linux_cases.items():
    # If fuchsia didn't contain that test case, skip it, but alert the user.
    if not case_name in fuchsia_cases:
      print("Fuchsia is missing test case {}".format(case_name))
      continue

    fuchsia_case_lines = fuchsia_cases[case_name]

    # Each test case should output its informational lines in the same order, so
    # if tests only produce partial output, any tailing info should be dropped,
    # and only data that was produced by both tests will be compared.
    paired_case_lines = zip(linux_case_lines, fuchsia_case_lines)
    if len(linux_case_lines) != len(fuchsia_case_lines):
      print("Linux and Fuchsia have produced different output lengths for the "
            "test {}!".format(case_name))
    desc_lines = [ResultLine(*pair) for pair in paired_case_lines]
    comparisons[case_name] = desc_lines

  for case_name in fuchsia_cases.keys():
    if case_name not in comparisons.keys():
      print("Linux is missing test case {}".format(case_name))

  return TestComparison(test_target, comparisons)


def RunTest(target: TestTarget, run_locally: bool = False) -> TestComparison:

  linux_output = target.ExecLinux(target_spec.linux_out_dir, run_locally)
  fuchsia_output = target.ExecFuchsia(target_spec.fuchsia_out_dir, run_locally)
  return CollateTests(
      linux_output.split("\n"), fuchsia_output.split("\n"), target.Name)


def RunGnForDirectory(dir_name: str, target_os: str) -> None:
  if not os.path.exists(dir_name):
    os.makedirs(dir_name)
  with open("{}/{}".format(dir_name, "args.gn"), "w") as args_file:
    args_file.write("is_debug = false\n")
    args_file.write("dcheck_always_on = false\n")
    args_file.write("is_component_build = false\n")
    args_file.write("use_goma = true\n")
    args_file.write("target_os = \"{}\"\n".format(target_os))

  subprocess.run(["gn", "gen", dir_name]).check_returncode()


def GenerateTestData() -> List[List[TestComparison]]:
  DIR_SOURCE_ROOT = os.path.abspath(
      os.path.join(os.path.dirname(__file__), *([os.pardir] * 3)))
  os.chdir(DIR_SOURCE_ROOT)

  # Grab parameters from config file.
  linux_dir = target_spec.linux_out_dir
  fuchsia_dir = target_spec.fuchsia_out_dir
  test_input = []
  for (test, filters) in target_spec.test_targets.items():
    test_input.append(TestTarget(test, filters))
  print("Test targets collected:\n{}".format("\n".join(
      [test.Target for test in test_input])))

  RunGnForDirectory(linux_dir, "linux")
  RunGnForDirectory(fuchsia_dir, "fuchsia")

  # Build test targets in both output directories.
  for directory in [linux_dir, fuchsia_dir]:
    build_command = ["autoninja", "-C", directory] \
                  + [test.Target for test in test_input]
    RunCommand(
        build_command,
        "Unable to build targets in directory {}".format(directory),
    )
  print("Builds completed.")

  # Execute the tests, one at a time, per system, and collect their results.
  results = []
  print("Running Tests")
  for test in test_input:
    results.append(RunTest(test))

  print("Tests Completed")
  with open("comparison_results.csv", "w") as out_file:
    out_file.write(
        "target,test,description,linux,fuchsia,fuchsia/linux,units\n")
    for result in results:
      out_file.write(result.MakeCsvFormat())
      out_file.write("\n")
  return results


if __name__ == "__main__":
  sys.exit(GenerateTestData())
