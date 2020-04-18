# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
TargetTracker

The target tracker is responsible for tracking meta targets, and more complex
tragets such as NaCl test rules.
"""

class TargetTracker(object):
  def __init__(self):
    self.targets = {}
    self.always = set()
    self.alias = {}

  def Alias(self, alias, name):
    self.alias[alias] = name
    return alias

  def AlwaysBuild(self, name):
    self.always |= set([name])
    return name
