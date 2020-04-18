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
  'use_pkg': Property(default=False, kind=bool),
  'pkg_files': Property(default=(), kind=List(str)),
  'pkg_dirs': Property(default=(), kind=ConfigList(lambda: ConfigGroup(
    path=Single(str),
    exclusions=List(str),
  ))),
  'ver_files': Property(default=(), kind=List(str)),
  'install_mode': Property(default=None),
}

def RunSteps(api, use_pkg, pkg_files, pkg_dirs, ver_files, install_mode):
  # Need to set service account credentials.
  api.cipd.set_service_account_credentials(
      api.cipd.default_bot_service_account_credentials)

  package_name = 'public/package/%s' % api.cipd.platform_suffix()
  package_instance_id = '7f751b2237df2fdf3c1405be00590fefffbaea2d'
  packages = {package_name: package_instance_id}

  cipd_root = api.path['start_dir'].join('packages')
  # Some packages don't require credentials to be installed or queried.
  api.cipd.ensure(cipd_root, packages)
  step = api.cipd.search(package_name, tag='git_revision:40-chars-long-hash')
  api.cipd.describe(package_name,
                    version=step.json.output['result'][0]['instance_id'])

  # Others do, so provide creds first.
  api.cipd.set_service_account_credentials('fake-credentials.json')
  private_package_name = 'private/package/%s' % api.cipd.platform_suffix()
  packages[private_package_name] = 'latest'
  api.cipd.ensure(cipd_root, packages)
  step = api.cipd.search(private_package_name, tag='key:value')
  api.cipd.describe(private_package_name,
                    version=step.json.output['result'][0]['instance_id'],
                    test_data_tags=['custom:tagged', 'key:value'],
                    test_data_refs=['latest'])

  # The rest of commands expect credentials to be set.

  # Build & register new package version.
  api.cipd.build('fake-input-dir', 'fake-package-path', 'infra/fake-package')
  api.cipd.build('fake-input-dir', 'fake-package-path', 'infra/fake-package',
                 install_mode='copy')
  api.cipd.register('infra/fake-package', 'fake-package-path',
                    refs=['fake-ref-1', 'fake-ref-2'],
                    tags={'fake_tag_1': 'fake_value_1',
                          'fake_tag_2': 'fake_value_2'})

  # Create (build & register).
  if use_pkg:
    root = api.path['start_dir'].join('some_subdir')
    pkg = api.cipd.PackageDefinition('infra/fake-package', root, install_mode)
    for fullpath in pkg_files:
      pkg.add_file(api.path.abs_to_path(fullpath))
    pkg.add_dir(root)
    for obj in pkg_dirs:
      pkg.add_dir(api.path.abs_to_path(obj.get('path', '')),
                  obj.get('exclusions'))
    for pth in ver_files:
      pkg.add_version_file(pth)

    api.cipd.create_from_pkg(pkg,
                             refs=['fake-ref-1', 'fake-ref-2'],
                             tags={'fake_tag_1': 'fake_value_1',
                                   'fake_tag_2': 'fake_value_2'})
  else:
    api.cipd.create_from_yaml(api.path['start_dir'].join('fake-package.yaml'),
                              refs=['fake-ref-1', 'fake-ref-2'],
                              tags={'fake_tag_1': 'fake_value_1',
                                    'fake_tag_2': 'fake_value_2'})


  # Set tag or ref of an already existing package.
  api.cipd.set_tag('fake-package',
                   version='long/weird/ref/which/doesn/not/fit/into/40chars',
                   tags={'dead': 'beaf', 'more': 'value'})
  api.cipd.set_ref('fake-package', version='latest', refs=['any', 'some'])
  # Search by the new tag.
  api.cipd.search('fake-package/%s' % api.cipd.platform_suffix(),
                  tag='dead:beaf')


def GenTests(api):
  yield (
    # This is very common dev workstation, but not all devs are on it.
    api.test('basic')
    + api.platform('linux', 64)
  )

  yield (
    api.test('mac64')
    + api.platform('mac', 64)
  )

  yield (
    api.test('win64')
    + api.platform('win', 64)
  )

  yield (
    api.test('describe-failed')
    + api.platform('linux', 64)
    + api.override_step_data(
      'cipd describe public/package/linux-amd64',
      api.cipd.example_error(
        'package "public/package/linux-amd64-ubuntu14_04" not registered',
      ))
  )

  yield (
    api.test('describe-many-instances')
    + api.platform('linux', 64)
    + api.override_step_data(
      'cipd search fake-package/linux-amd64 dead:beaf',
      api.cipd.example_search(
        'public/package/linux-amd64-ubuntu14_04',
        instances=3
      ))
  )

  yield (
    api.test('basic_pkg')
    + api.properties(
      use_pkg=True,
      pkg_files=[
        '[START_DIR]/some_subdir/a/path/to/file.py',
        '[START_DIR]/some_subdir/some_config.cfg',
      ],
      pkg_dirs=[
        {
          'path': '[START_DIR]/some_subdir/directory',
        },
        {
          'path': '[START_DIR]/some_subdir/other_dir',
          'exclusions': [
            r'.*\.pyc',
          ]
        },
      ],
      ver_file=['.versions/file.cipd_version'],
    )
  )

  yield (
    api.test('pkg_bad_verfile')
    + api.properties(
      use_pkg=True,
      ver_files=['a', 'b'],
    )
    + api.expect_exception('ValueError')
  )

  yield (
    api.test('pkg_bad_mode')
    + api.properties(
      use_pkg=True,
      install_mode='',
    )
    + api.expect_exception('ValueError')
  )

  yield (
    api.test('pkg_bad_file')
    + api.properties(
      use_pkg=True,
      pkg_files=[
        '[START_DIR]/a/path/to/file.py',
      ],
    )
    + api.expect_exception('ValueError')
  )
