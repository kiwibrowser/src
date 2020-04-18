# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from recipe_engine import recipe_api

import string

class GitClApi(recipe_api.RecipeApi):
  def __call__(self, subcmd, args, name=None, **kwargs):
    if not name:
      name = 'git_cl ' + subcmd

    if kwargs.get('suffix'):
      name = name + ' (%s)' % kwargs.pop('suffix')

    my_loc = self.c.repo_location if self.c else None
    with self.m.context(cwd=self.m.context.cwd or my_loc):
      return self.m.step(
          name, [self.package_repo_resource('git_cl.py'), subcmd] + args,
          **kwargs)

  def get_description(self, patch_url=None, codereview=None, **kwargs):
    """DEPRECATED. Consider using gerrit.get_change_description instead."""
    args = ['-d']
    if patch_url or codereview:
      assert patch_url and codereview, (
          'Both patch_url and codereview must be provided')
      args.append('--%s' % codereview)
      args.append(patch_url)

    return self('description', args, stdout=self.m.raw_io.output(), **kwargs)

  def set_description(self, description, patch_url=None, codereview=None, **kwargs):
    args = ['-n', '-']
    if patch_url or codereview:
      assert patch_url and codereview, (
          'Both patch_url and codereview must be provided')
      args.append(patch_url)
      args.append('--%s' % codereview)

    return self(
        'description', args, stdout=self.m.raw_io.output(),
        stdin=self.m.raw_io.input_text(description),
        name='git_cl set description', **kwargs)

  def upload(self, message, upload_args=None, **kwargs):
    upload_args = upload_args or []

    upload_args.extend(['--message-file', self.m.raw_io.input_text(message)])

    return self('upload', upload_args, **kwargs)

  def issue(self, **kwargs):
    return self('issue', [], stdout=self.m.raw_io.output(), **kwargs)

