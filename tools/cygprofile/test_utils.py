# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test utilities for cygprofile scripts."""

import collections

import process_profiles

SimpleTestSymbol = collections.namedtuple(
    'SimpleTestSymbol', ['name', 'offset', 'size'])


class TestSymbolOffsetProcessor(process_profiles.SymbolOffsetProcessor):
  def __init__(self, symbol_infos):
    super(TestSymbolOffsetProcessor, self).__init__(None)
    self._symbol_infos = symbol_infos


class TestProfileManager(process_profiles.ProfileManager):
  def __init__(self, filecontents_mapping):
    super(TestProfileManager, self).__init__(filecontents_mapping.keys())
    self._filecontents_mapping = filecontents_mapping

  def _ReadOffsets(self, filename):
    return self._filecontents_mapping[filename]
