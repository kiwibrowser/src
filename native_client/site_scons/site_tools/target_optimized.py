#!/usr/bin/python2.4
# Copyright 2009 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Build tool setup for optimized environments.

This module is a SCons tool which setups environments for optimized builds.
It is used as follows:
  optimized_env = env.Clone(tools = ['target_optimized'])
"""


def generate(env):
  # NOTE: SCons requires the use of this name, which fails gpylint.
  """SCons entry point for this tool."""

  # Add in general options.
  env['TARGET_DEBUG'] = False

  env.Append(
      CPPDEFINES=['NDEBUG'] + env.get('CPPDEFINES_OPTIMIZED', []),
      CCFLAGS=env.get('CCFLAGS_OPTIMIZED', []),
      LINKFLAGS=env.get('LINKFLAGS_OPTIMIZED', []),
  )
