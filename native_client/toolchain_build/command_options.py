#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Class containing options for command filtering."""

class CommandOptions(object):
  def __init__(self, work_dir, clobber_working, clobber_source,
               trybot, buildbot):
    self._work_dir = work_dir
    self._clobber_working = clobber_working
    self._clobber_source = clobber_source
    self._trybot = trybot
    self._buildbot = buildbot

  def GetWorkDir(self):
    return self._work_dir

  def IsClobberWorking(self):
    return self._clobber_working

  def IsClobberSource(self):
    return self._clobber_source

  def IsTryBot(self):
    return self._trybot

  def IsBuildBot(self):
    return self._buildbot

  def IsBot(self):
    return self._trybot or self._buildbot
