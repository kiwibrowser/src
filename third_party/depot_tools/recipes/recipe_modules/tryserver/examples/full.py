# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

DEPS = [
  'recipe_engine/json',
  'recipe_engine/raw_io',
  'recipe_engine/path',
  'recipe_engine/platform',
  'recipe_engine/properties',
  'recipe_engine/python',
  'recipe_engine/step',
  'tryserver',
]


def RunSteps(api):
  if api.properties.get('set_failure_hash_with_no_steps'):
    with api.tryserver.set_failure_hash():
      raise api.step.StepFailure('boom!')

  api.path['checkout'] = api.path['start_dir']
  if api.properties.get('patch_text'):
    api.step('patch_text test', [
        'echo', str(api.tryserver.get_footers(api.properties['patch_text']))])
    api.step('patch_text test', [
        'echo', str(api.tryserver.get_footer(
            'Foo', api.properties['patch_text']))])
    return

  if api.tryserver.is_gerrit_issue:
    api.tryserver.get_footers()
  api.tryserver.get_files_affected_by_patch(
      api.properties.get('test_patch_root'))

  if api.tryserver.is_tryserver:
    api.tryserver.set_subproject_tag('v8')

  api.tryserver.set_patch_failure_tryjob_result()
  api.tryserver.set_compile_failure_tryjob_result()
  api.tryserver.set_test_failure_tryjob_result()
  api.tryserver.set_invalid_test_results_tryjob_result()

  api.tryserver.normalize_footer_name('Cr-Commit-Position')

  with api.tryserver.set_failure_hash():
    api.python.failing_step('fail', 'foo')


def GenTests(api):
  description_step = api.override_step_data(
      'git_cl description', stdout=api.raw_io.output_text('foobar'))
  # The 'test_patch_root' property used below is just so that these
  # tests can avoid using the gclient module to calculate the
  # patch root. Normal users would use gclient.calculate_patch_root().
  yield (api.test('with_git_patch') +
         api.properties(
             path_config='buildbot',
             patch_storage='git',
             patch_project='v8',
             patch_repo_url='http://patch.url/',
             patch_ref='johndoe#123.diff',
             test_patch_root='v8'))

  yield (api.test('with_git_patch_luci') +
         api.properties(
             patch_storage='git',
             patch_project='v8',
             patch_repo_url='http://patch.url/',
             patch_ref='johndoe#123.diff',
             test_patch_root='v8'))

  yield (api.test('with_wrong_patch') +
         api.platform('win', 32) +
         api.properties(test_patch_root=''))

  yield (api.test('with_gerrit_patch') +
         api.properties.tryserver(gerrit_project='infra/infra'))

  yield (api.test('with_wrong_patch_new') + api.platform('win', 32) +
         api.properties(test_patch_root='sub\\project'))

  yield (api.test('basic_tags') +
         api.properties(
             patch_text='hihihi\nfoo:bar\nbam:baz',
             footer='foo'
         ) +
         api.step_data(
             'parse description',
             api.json.output({'Foo': ['bar']})) +
         api.step_data(
             'parse description (2)',
             api.json.output({'Foo': ['bar']}))
  )

  yield (api.test('set_failure_hash_with_no_steps') +
         api.properties(set_failure_hash_with_no_steps=True))
