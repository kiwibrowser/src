#!/usr/bin/python
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

#
# CoverageHelper
#
# The CoverageHelper is an object which defines lists used by the coverage
# scripts for grouping or filtering source by path, as well as filtering sources
# by name.
class CoverageHelper(object):
  def __init__(self):
    self.ignore_set = self.Ignore()
    self.path_filter = self.Filter()
    self.groups = self.Groups()

  #
  # Return a list of fully qualified paths for the directories availible
  # at the input path specified.
  #
  def GetDirList(self, startpath):
    paths = []
    realpath = os.path.realpath(startpath)
    for name in os.listdir(realpath):
      path = os.path.join(realpath, name)
      if os.path.isdir(path):
        paths.append(path)
    return paths

  #
  # Set of results to ignore by path.
  #
  def Filter(self):
    filters = ['src/third_party/valgrind', 'src/tools']
    return set([os.path.realpath(path) for path in filters])

  #
  # Set of files to ignore because they are in the TCB but used by the
  # validator.
  #
  def Ignore(self):
    return set([
      'cpu_x86_test.c',
      'defsize64.c',
      'lock_insts.c',
      'lock_insts.h',
      'long_mode.c',
      'long_mode.h',
      'nacl_illegal.c',
      'nacl_illegal.h',
      'nc_read_segment.c',
      'nc_read_segment.h',
      'nc_rep_prefix.c',
      'nc_rep_prefix.h',
      'ncdecodeX87.c',
      'ncdecode_OF.c',
      'ncdecode_forms.c',
      'ncdecode_forms.h',
      'ncdecode_onebyte.c',
      'ncdecode_sse.c',
      'ncdecode_st.c',
      'ncdecode_st.h',
      'ncdecode_table.c',
      'ncdecode_tablegen.c',
      'ncdecode_tablegen.h',
      'ncdecode_tests.c',
      'ncdis.c',
      'ncdis_segments.c',
      'ncdis_segments.h',
      'ncdis_util.c',
      'ncdis_util.h',
      'ncenuminsts.c',
      'ncenuminsts.h',
      'ncval.c',
      'ncval_driver.c',
      'ncval_driver.h',
      'ncval_tests.c',
      'ze64.h',
      'zero_extends.c',
      'zero_extends.h',
     ])

  #
  # Set of results to group by path.
  #
  def Groups(self):
    groups = []
    groups.append(os.path.realpath('scons-out'))
    groups.append(os.path.realpath('src/include'))
    groups.append(os.path.realpath('src/tools'))
    groups.extend(self.GetDirList('src/trusted'))
    groups.extend(self.GetDirList('src/shared'))
    groups.extend(self.GetDirList('src/third_party'))
    groups.extend(self.GetDirList('..'))
    return groups
