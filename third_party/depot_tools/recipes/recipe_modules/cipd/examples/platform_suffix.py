# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from recipe_engine.config import List, Single, ConfigList, ConfigGroup
from recipe_engine.recipe_api import Property

DEPS = [
  'recipe_engine/path',
  'recipe_engine/platform',
  'recipe_engine/properties',
  'recipe_engine/step',
  'cipd',
]

PROPERTIES = {
  'arch_override': Property(kind=str, default=None),
  'bits_override': Property(kind=int, default=None),
  'expect_error': Property(kind=bool, default=False),
}

def RunSteps(api, arch_override, bits_override, expect_error):
  was_error = False
  try:
    api.step('platform_suffix', ['echo', api.cipd.platform_suffix(
      arch=arch_override,
      bits=bits_override,
    )])
  except KeyError:
    was_error = True

  assert was_error == expect_error


def GenTests(api):
  for name, arch, bits in (
      ('linux', 'intel', 32),
      ('linux', 'intel', 64),
      ('linux', 'mips', 64),
      ('linux', 'arm', 32),
      ('linux', 'arm', 64),
      ('mac', 'intel', 64),
      ('win', 'intel', 32),
      ('win', 'intel', 64)):
    test = (
      api.test('%s_%s_%d' % (name, arch, bits)) +
      api.platform(name, bits)
    )
    if arch != 'intel':
      test += api.properties(arch_override=arch)
    yield test

  yield (
      api.test('junk arch') +
      api.properties(arch_override='pants', expect_error=True))

  yield (
      api.test('junk bits') +
      api.properties(bits_override=42, expect_error=True))
