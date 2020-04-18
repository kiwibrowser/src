# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import hashlib
import struct
from recipe_engine import recipe_test_api


class BotUpdateTestApi(recipe_test_api.RecipeTestApi):
  def properties(self, apply_patch_on_gclient):
    ret = self.test(None)
    ret.properties = {
        '$depot_tools/bot_update': {
            'apply_patch_on_gclient': apply_patch_on_gclient,
        }
    }
    return ret

  def output_json(self, root, first_sln, revision_mapping, fail_patch=False,
                  fixed_revisions=None):
    """Deterministically synthesize json.output test data for gclient's
    --output-json option.
    """
    output = {
        'did_run': True,
        'patch_failure': False
    }

    properties = {
        property_name: self.gen_revision(project_name)
        for property_name, project_name in revision_mapping.iteritems()
    }
    properties.update({
        '%s_cp' % property_name: ('refs/heads/master@{#%s}' %
                                  self.gen_commit_position(project_name))
        for property_name, project_name in revision_mapping.iteritems()
    })

    output.update({
        'patch_root': root or first_sln,
        'root': first_sln,
        'properties': properties,
        'step_text': 'Some step text'
    })
    output.update({
      'manifest': {
				project_name: {
					'repository': 'https://fake.org/%s.git' % project_name,
					'revision': self.gen_revision(project_name),
				}
			for project_name in set(revision_mapping.values())}})

    output.update({
      'source_manifest': {
        'version': 0,
        'directories': {
          project_name: {
            'git_checkout': {
              'repo_url': 'https://fake.org/%s.git' % project_name,
              'revision': self.gen_revision(project_name),
            }
          } for project_name in set(revision_mapping.values())
        }
      }
    })

    if fixed_revisions:
      output['fixed_revisions'] = fixed_revisions

    if fail_patch:
      output['patch_failure'] = True
      output['failed_patch_body'] = '\n'.join([
        'Downloading patch...',
        'Applying the patch...',
        'Patch: foo/bar.py',
        'Index: foo/bar.py',
        'diff --git a/foo/bar.py b/foo/bar.py',
        'index HASH..HASH MODE',
        '--- a/foo/bar.py',
        '+++ b/foo/bar.py',
        'context',
        '+something',
        '-something',
        'more context',
      ])
      output['patch_apply_return_code'] = 1
      if fail_patch == 'download':
        output['patch_apply_return_code'] = 3
    return self.m.json.output(output)

  @staticmethod
  def gen_revision(project):
    """Hash project to bogus deterministic git hash values."""
    h = hashlib.sha1(project)
    return h.hexdigest()

  @staticmethod
  def gen_commit_position(project):
    """Hash project to bogus deterministic Cr-Commit-Position values."""
    h = hashlib.sha1(project)
    return struct.unpack('!I', h.digest()[:4])[0] % 300000
