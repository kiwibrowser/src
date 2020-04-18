# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

DEPS = [
  'gclient',
  'recipe_engine/context',
  'recipe_engine/path',
  'recipe_engine/properties',
  'recipe_engine/step',
]


TEST_CONFIGS = [
  'android',
  'angle',
  'boringssl',
  'build_internal',
  'build_internal_scripts_slave',
  'catapult',
  'crashpad',
  'custom_tabs_client',
  'dart',
  'disable_syntax_validation',
  'gerrit_test_cq_normal',
  'gyp',
  'infra',
  'infradata_master_manager',
  'internal_deps',
  'luci_gae',
  'luci_go',
  'luci_py',
  'master_deps',
  'mojo',
  'nacl',
  'pdfium',
  'recipes_py',
  'recipes_py_bare',
  'slave_deps',
  'wasm_llvm',
  'webports',
  'with_branch_heads',
  'with_tags',
]


def RunSteps(api):
  for config_name in TEST_CONFIGS:
    api.gclient.make_config(config_name)

  src_cfg = api.gclient.make_config(CACHE_DIR='[ROOT]/git_cache')
  soln = src_cfg.solutions.add()
  soln.name = 'src'
  soln.url = 'https://chromium.googlesource.com/chromium/src.git'
  soln.revision = api.properties.get('revision')
  soln.custom_vars = {'string_var': 'string_val', 'true_var': True}
  src_cfg.parent_got_revision_mapping['parent_got_revision'] = 'got_revision'
  api.gclient.c = src_cfg
  api.gclient.checkout()

  api.gclient.spec_alias = 'Angle'
  bl_cfg = api.gclient.make_config()
  soln = bl_cfg.solutions.add()
  soln.name = 'Angle'
  soln.url = 'https://chromium.googlesource.com/angle/angle.git'
  bl_cfg.revisions['src/third_party/angle'] = 'refs/heads/lkgr'

  bl_cfg.got_revision_mapping['src/blatley'] = 'got_blatley_revision'
  with api.context(cwd=api.path['start_dir'].join('src', 'third_party')):
    api.gclient.checkout(
        gclient_config=bl_cfg)

  api.gclient.got_revision_reverse_mapping(bl_cfg)

  api.gclient.break_locks()

  del api.gclient.spec_alias

  api.gclient.runhooks()

  assert not api.gclient.is_blink_mode


def GenTests(api):
  yield api.test('basic')

  yield api.test('buildbot') + api.properties(path_config='buildbot')

  yield api.test('revision') + api.properties(revision='abc')

  yield api.test('tryserver') + api.properties.tryserver()
