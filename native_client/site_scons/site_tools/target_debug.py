#!/usr/bin/python2.4
# Copyright 2009 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Build tool setup for debug environments.

This module is a SCons tool which setups environments for debugging.
It is used as follows:
  debug_env = env.Clone(tools = ['target_debug'])
"""


def generate(env):
  # NOTE: SCons requires the use of this name, which fails gpylint.
  """SCons entry point for this tool."""

  # Set target platform bits
  env.SetBits('debug')

  env['TARGET_DEBUG'] = True

  env.Append(
      CPPDEFINES=['_DEBUG'] + env.get('CPPDEFINES_DEBUG', []),
      CCFLAGS=env.get('CCFLAGS_DEBUG', []),
      LINKFLAGS=env.get('LINKFLAGS_DEBUG', []),
  )
