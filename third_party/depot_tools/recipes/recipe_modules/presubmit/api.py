# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from recipe_engine import recipe_api

class PresubmitApi(recipe_api.RecipeApi):
  @property
  def presubmit_support_path(self):
    return self.package_repo_resource('presubmit_support.py')

  def __call__(self, *args, **kwargs):
    """Return a presubmit step."""

    name = kwargs.pop('name', 'presubmit')

    env = self.m.context.env
    env.setdefault('PATH', '%(PATH)s')
    env['PATH'] = self.m.path.pathsep.join([
        env['PATH'], str(self._module.PACKAGE_REPO_ROOT)])

    with self.m.context(env=env):
      return self.m.python(
          name, self.presubmit_support_path, list(args), **kwargs)
