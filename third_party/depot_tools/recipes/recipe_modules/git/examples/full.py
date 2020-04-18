# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

DEPS = [
  'git',
  'recipe_engine/context',
  'recipe_engine/path',
  'recipe_engine/platform',
  'recipe_engine/properties',
  'recipe_engine/raw_io',
  'recipe_engine/step',
]


def RunSteps(api):
  url = 'https://chromium.googlesource.com/chromium/src.git'

  # git.checkout can optionally dump GIT_CURL_VERBOSE traces to a log file,
  # useful for debugging git access issues that are reproducible only on bots.
  curl_trace_file = None
  if api.properties.get('use_curl_trace'):
    curl_trace_file = api.path['start_dir'].join('curl_trace.log')

  submodule_update_force = api.properties.get('submodule_update_force', False)
  submodule_update_recursive = api.properties.get('submodule_update_recursive',
          True)

  # You can use api.git.checkout to perform all the steps of a safe checkout.
  retVal = api.git.checkout(
      url,
      ref=api.properties.get('revision'),
      recursive=True,
      submodule_update_force=submodule_update_force,
      set_got_revision=api.properties.get('set_got_revision'),
      curl_trace_file=curl_trace_file,
      remote_name=api.properties.get('remote_name'),
      display_fetch_size=api.properties.get('display_fetch_size'),
      file_name=api.properties.get('checkout_file_name'),
      submodule_update_recursive=submodule_update_recursive,
      use_git_cache=api.properties.get('use_git_cache'))

  assert retVal == "deadbeef", (
    "expected retVal to be %r but was %r" % ("deadbeef", retVal))

  # count_objects shows number and size of objects in .git dir.
  api.git.count_objects(
      name='count-objects',
      can_fail_build=api.properties.get('count_objects_can_fail_build'),
      git_config_options={'foo': 'bar'})

  # Get the remote URL.
  api.git.get_remote_url(
      step_test_data=lambda: api.raw_io.test_api.stream_output('foo'))

  api.git.get_timestamp(test_data='foo')

  # You can use api.git.fetch_tags to fetch all tags from the remote
  api.git.fetch_tags(api.properties.get('remote_name'))

  # If you need to run more arbitrary git commands, you can use api.git itself,
  # which behaves like api.step(), but automatically sets the name of the step.
  with api.context(cwd=api.path['checkout']):
    api.git('status')

  api.git('status', name='git status can_fail_build',
          can_fail_build=True)

  api.git('status', name='git status cannot_fail_build',
          can_fail_build=False)

  # You should run git new-branch before you upload something with git cl.
  api.git.new_branch('refactor')  # Upstream is origin/master by default.
  # And use upstream kwarg to set up different upstream for tracking.
  api.git.new_branch('feature', upstream='refactor')

  # You can use api.git.rebase to rebase the current branch onto another one
  api.git.rebase(name_prefix='my repo', branch='origin/master',
                 dir_path=api.path['checkout'],
                 remote_name=api.properties.get('remote_name'))

  if api.properties.get('cat_file', None):
    step_result = api.git.cat_file_at_commit(api.properties['cat_file'],
                                             api.properties['revision'],
                                             stdout=api.raw_io.output())
    if 'TestOutput' in step_result.stdout:
      pass  # Success!

  # Bundle the repository.
  api.git.bundle_create(
        api.path['start_dir'].join('all.bundle'))


def GenTests(api):
  yield api.test('basic')
  yield api.test('basic_ref') + api.properties(revision='refs/foo/bar')
  yield api.test('basic_branch') + api.properties(revision='refs/heads/testing')
  yield api.test('basic_hash') + api.properties(
      revision='abcdef0123456789abcdef0123456789abcdef01')
  yield api.test('basic_file_name') + api.properties(checkout_file_name='DEPS')
  yield api.test('basic_submodule_update_force') + api.properties(
      submodule_update_force=True)

  yield api.test('platform_win') + api.platform.name('win')

  yield api.test('curl_trace_file') + api.properties(
      revision='refs/foo/bar', use_curl_trace=True)

  yield (
    api.test('can_fail_build') +
    api.step_data('git status can_fail_build', retcode=1)
  )

  yield (
    api.test('cannot_fail_build') +
    api.step_data('git status cannot_fail_build', retcode=1)
  )

  yield (
    api.test('set_got_revision') +
    api.properties(set_got_revision=True)
  )

  yield (
    api.test('rebase_failed') +
    api.step_data('my repo rebase', retcode=1)
  )

  yield api.test('remote_not_origin') + api.properties(remote_name='not_origin')

  yield (
      api.test('count-objects_delta') +
      api.properties(display_fetch_size=True))

  yield (
      api.test('count-objects_failed') +
      api.step_data('count-objects', retcode=1))

  yield (
      api.test('count-objects_with_bad_output') +
      api.step_data(
          'count-objects',
          stdout=api.raw_io.output(api.git.count_objects_output('xxx'))))

  yield (
      api.test('count-objects_with_bad_output_fails_build') +
      api.step_data(
          'count-objects',
          stdout=api.raw_io.output(api.git.count_objects_output('xxx'))) +
      api.properties(count_objects_can_fail_build=True))
  yield (
      api.test('cat-file_test') +
      api.step_data('git cat-file abcdef12345:TestFile',
                    stdout=api.raw_io.output('TestOutput')) +
      api.properties(revision='abcdef12345', cat_file='TestFile'))

  yield (
      api.test('git-cache-checkout') +
      api.properties(use_git_cache=True))
