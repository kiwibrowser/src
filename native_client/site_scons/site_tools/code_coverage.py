#!/usr/bin/python2.4
# Copyright 2009 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""SCons tool for generating code coverage.

This module enhances a debug environment to add code coverage.
It is used as follows:
  coverage_env = dbg_env.Clone(tools = ['code_coverage'])
"""


def AddCoverageSetup(env):
  """Add coverage related build steps and dependency links.

  Args:
    env: a leaf environment ready to have coverage steps added.
  """
  # Add a step to start coverage (for instance windows needs this).
  # This step should get run before and tests are run.
  if env.get('COVERAGE_START_CMD', None):
    start = env.Command('$COVERAGE_START_FILE', [], '$COVERAGE_START_CMD')
    env.AlwaysBuild(start)
  else:
    start = []

  # Add a step to end coverage (used on basically all platforms).
  # This step should get after all the tests have run.
  if env.get('COVERAGE_STOP_CMD', None):
    stop = env.Command('$COVERAGE_OUTPUT_FILE', [], '$COVERAGE_STOP_CMD')
    env.AlwaysBuild(stop)
  else:
    stop = []

  targets = env.SubstList2('$COVERAGE_TARGETS')
  targets = [env.Alias(t) for t in [start] + targets + [stop]]

  # Add an alias for coverage.
  env.Alias('coverage', targets)


def generate(env):
  # NOTE: SCons requires the use of this name, which fails gpylint.
  """SCons entry point for this tool."""

  env['COVERAGE_ENABLED'] = True
  env.SetBits('coverage_enabled')

  env.SetDefault(
      # Setup up coverage related tool paths.
      # These can be overridden elsewhere, if needed, to relocate the tools.
      COVERAGE_MCOV='mcov',
      COVERAGE_GENHTML='genhtml',
      COVERAGE_ANALYZER='coverage_analyzer.exe',
      COVERAGE_VSPERFCMD='VSPerfCmd.exe',
      COVERAGE_VSINSTR='vsinstr.exe',

      # Setup coverage related locations.
      COVERAGE_DIR='$TARGET_ROOT/coverage',
      COVERAGE_HTML_DIR='$COVERAGE_DIR/html',
      COVERAGE_START_FILE='$COVERAGE_DIR/start.junk',
      COVERAGE_OUTPUT_FILE='$COVERAGE_DIR/coverage.lcov',

      # The list of aliases containing test execution targets.
      COVERAGE_TARGETS=['run_all_tests'],
  )

  # Add in coverage flags. These come from target_platform_xxx.
  env.Append(
      CCFLAGS='$COVERAGE_CCFLAGS',
      LIBS='$COVERAGE_LIBS',
      LINKFLAGS='$COVERAGE_LINKFLAGS',
      SHLINKFLAGS='$COVERAGE_SHLINKFLAGS',
  )

  # Change the definition of Install if required by the platform.
  if env.get('COVERAGE_INSTALL'):
    env['PRECOVERAGE_INSTALL'] = env['INSTALL']
    env['INSTALL'] = env['COVERAGE_INSTALL']

  # Add any extra paths.
  env.AppendENVPath('PATH', env.SubstList2('$COVERAGE_EXTRA_PATHS'))

  # Add coverage start/stop and processing in deferred steps.
  env.Defer(AddCoverageSetup)
