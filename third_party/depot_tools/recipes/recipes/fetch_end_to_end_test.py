# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

DEPS = [
  'bot_update',
  'gclient',
  'recipe_engine/context',
  'recipe_engine/file',
  'recipe_engine/path',
  'recipe_engine/python',
  'recipe_engine/step',
]


def RunSteps(api):
  # Create a test depot_tools checkout, possibly applying user patches.
  api.gclient.set_config('depot_tools')
  api.bot_update.ensure_checkout()

  # List fetch configs available in the test depot_tools checkout.
  fetch_configs = set(
      api.path.basename(f)[:-3] for f in api.file.listdir(
          'listdir fetch_configs', api.path['checkout'].join('fetch_configs'))
      if str(f).endswith('.py'))

  # config_util is a helper library, not a config.
  fetch_configs.discard('config_util')

  # Try to run "fetch" for each of the configs. It's important to use
  # the checkout under test.
  with api.context(
      env={'DEPOT_TOOLS_UPDATE': '0', 'CHROME_HEADLESS': '1'},
      env_prefixes={'PATH': [api.path['checkout']]}):
    with api.step.defer_results():
      for config_name in sorted(fetch_configs):
        # Create a dedicated temp directory. We want to test checking out
        # from scratch.
        temp_dir = api.path.mkdtemp('fetch_end_to_end_test_%s' % config_name)

        try:
          with api.context(cwd=temp_dir):
            api.python(
                'fetch %s' % config_name,
                api.path['checkout'].join('fetch.py'),
                [config_name])
        finally:
          api.file.rmtree('cleanup', temp_dir)


def GenTests(api):
  yield (
      api.test('basic') +
      api.step_data('listdir fetch_configs', api.file.listdir(
          ['depot_tools.py', 'infra.py']))
  )
