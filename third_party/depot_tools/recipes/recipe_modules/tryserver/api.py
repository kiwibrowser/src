# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import contextlib
import hashlib

from recipe_engine import recipe_api


class TryserverApi(recipe_api.RecipeApi):
  def __init__(self, *args, **kwargs):
    super(TryserverApi, self).__init__(*args, **kwargs)
    self._failure_reasons = []

  @property
  def is_tryserver(self):
    """Returns true iff we have a change to check out."""
    return (self.is_patch_in_git or self.is_gerrit_issue)

  @property
  def is_gerrit_issue(self):
    """Returns true iff the properties exist to match a Gerrit issue."""
    if self.m.properties.get('patch_storage') == 'gerrit':
      return True
    # TODO(tandrii): remove this, once nobody is using buildbot Gerrit Poller.
    return ('event.patchSet.ref' in self.m.properties and
            'event.change.url' in self.m.properties and
            'event.change.id' in self.m.properties)

  @property
  def is_patch_in_git(self):
    return (self.m.properties.get('patch_storage') == 'git' and
            self.m.properties.get('patch_repo_url') and
            self.m.properties.get('patch_ref'))

  def get_files_affected_by_patch(self, patch_root, **kwargs):
    """Returns list of paths to files affected by the patch.

    Argument:
      patch_root: path relative to api.path['root'], usually obtained from
        api.gclient.calculate_patch_root(patch_project)

    Returned paths will be relative to to patch_root.
    """
    cwd = self.m.context.cwd or self.m.path['start_dir'].join(patch_root)
    with self.m.context(cwd=cwd):
      step_result = self.m.git(
          '-c', 'core.quotePath=false', 'diff', '--cached', '--name-only',
          name='git diff to analyze patch',
          stdout=self.m.raw_io.output(),
          step_test_data=lambda:
            self.m.raw_io.test_api.stream_output('foo.cc'),
          **kwargs)
    paths = [self.m.path.join(patch_root, p) for p in
             step_result.stdout.split()]
    if self.m.platform.is_win:
      # Looks like "analyze" wants POSIX slashes even on Windows (since git
      # uses that format even on Windows).
      paths = [path.replace('\\', '/') for path in paths]
    step_result.presentation.logs['files'] = paths
    return paths

  def set_subproject_tag(self, subproject_tag):
    """Adds a subproject tag to the build.

    This can be used to distinguish between builds that execute different steps
    depending on what was patched, e.g. blink vs. pure chromium patches.
    """
    assert self.is_tryserver

    step_result = self.m.step.active_result
    step_result.presentation.properties['subproject_tag'] = subproject_tag

  def _set_failure_type(self, failure_type):
    if not self.is_tryserver:
      return

    step_result = self.m.step.active_result
    step_result.presentation.properties['failure_type'] = failure_type

  def set_patch_failure_tryjob_result(self):
    """Mark the tryjob result as failure to apply the patch."""
    self._set_failure_type('PATCH_FAILURE')

  def set_compile_failure_tryjob_result(self):
    """Mark the tryjob result as a compile failure."""
    self._set_failure_type('COMPILE_FAILURE')

  def set_test_failure_tryjob_result(self):
    """Mark the tryjob result as a test failure.

    This means we started running actual tests (not prerequisite steps
    like checkout or compile), and some of these tests have failed.
    """
    self._set_failure_type('TEST_FAILURE')

  def set_invalid_test_results_tryjob_result(self):
    """Mark the tryjob result as having invalid test results.

    This means we run some tests, but the results were not valid
    (e.g. no list of specific test cases that failed, or too many
    tests failing, etc).
    """
    self._set_failure_type('INVALID_TEST_RESULTS')

  def add_failure_reason(self, reason):
    """
    Records a more detailed reason why build is failing.

    The reason can be any JSON-serializable object.
    """
    assert self.m.json.is_serializable(reason)
    self._failure_reasons.append(reason)

  @contextlib.contextmanager
  def set_failure_hash(self):
    """
    Context manager that sets a failure_hash build property on StepFailure.

    This can be used to easily compare whether two builds have failed
    for the same reason. For example, if a patch is bad (breaks something),
    we'd expect it to always break in the same way. Different failures
    for the same patch are usually a sign of flakiness.
    """
    try:
      yield
    except self.m.step.StepFailure as e:
      self.add_failure_reason(e.reason)

      try:
        step_result = self.m.step.active_result
      except ValueError:
        step_result = None
      if step_result:
        failure_hash = hashlib.sha1()
        failure_hash.update(self.m.json.dumps(self._failure_reasons))
        step_result.presentation.properties['failure_hash'] = (
            failure_hash.hexdigest())

      raise e

  def get_footers(self, patch_text=None):
    """Retrieves footers from the patch description.

    footers are machine readable tags embedded in commit messages. See
    git-footers documentation for more information.
    """
    if patch_text is None:
      patch_text = self.m.gerrit.get_change_description(
          self.m.properties['patch_gerrit_url'],
          self.m.properties['patch_issue'],
          self.m.properties['patch_set'])

    result = self.m.python(
        'parse description', self.package_repo_resource('git_footers.py'),
        args=['--json', self.m.json.output()],
        stdin=self.m.raw_io.input(data=patch_text))
    return result.json.output

  def get_footer(self, tag, patch_text=None):
    """Gets a specific tag from a CL description"""
    return self.get_footers(patch_text).get(tag, [])

  def normalize_footer_name(self, footer):
    return '-'.join([ word.title() for word in footer.strip().split('-') ])
