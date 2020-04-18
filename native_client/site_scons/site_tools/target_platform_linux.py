#!/usr/bin/python2.4
# Copyright 2009 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Build tool setup for Linux.

This module is a SCons tool which should be include in the topmost windows
environment.
It is used as follows:
  env = base_env.Clone(tools = ['component_setup'])
  linux_env = base_env.Clone(tools = ['target_platform_linux'])
"""


def ComponentPlatformSetup(env, builder_name):
  """Hook to allow platform to modify environment inside a component builder.

  Args:
    env: Environment to modify
    builder_name: Name of the builder
  """
  if env.get('ENABLE_EXCEPTIONS'):
    env.FilterOut(CCFLAGS=['-fno-exceptions'])
    env.Append(CCFLAGS=['-fexceptions'])

#------------------------------------------------------------------------------


def generate(env):
  # NOTE: SCons requires the use of this name, which fails gpylint.
  """SCons entry point for this tool."""

  # Preserve some variables that get blown away by the tools.
  saved = dict()
  for k in ['ASFLAGS', 'CFLAGS', 'CCFLAGS', 'CXXFLAGS', 'LINKFLAGS', 'LIBS']:
    saved[k] = env.get(k, [])
    env[k] = []

  # Use g++
  env.Tool('g++')
  env.Tool('gcc')
  env.Tool('gnulink')
  env.Tool('ar')
  env.Tool('as')

  # Set target platform bits
  env.SetBits('linux', 'posix')

  env.Replace(
      TARGET_PLATFORM='LINUX',
      COMPONENT_PLATFORM_SETUP=ComponentPlatformSetup,
      CCFLAG_INCLUDE='-include',     # Command line option to include a header

      # Code coverage related.
      COVERAGE_CCFLAGS=['-ftest-coverage', '-fprofile-arcs', '-DCOVERAGE'],
      COVERAGE_LIBS='gcov',
      COVERAGE_STOP_CMD=[
          '$COVERAGE_MCOV --directory "$TARGET_ROOT" --output "$TARGET"',
          ('$COVERAGE_GENHTML --output-directory $COVERAGE_HTML_DIR '
           '$COVERAGE_OUTPUT_FILE'),
      ],
  )

  env.Append(
      # Settings for debug
      CCFLAGS_DEBUG=[
          '-O0',     # turn off optimizations
          '-g',      # turn on debugging info
      ],
      LINKFLAGS_DEBUG=['-g'],

      # Settings for optimized
      CCFLAGS_OPTIMIZED=['-O2'],

      # Settings for component_builders
      COMPONENT_LIBRARY_LINK_SUFFIXES=['.so', '.a'],
      COMPONENT_LIBRARY_DEBUG_SUFFIXES=[],
  )

  # Restore saved flags.
  env.Append(**saved)
