# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from recipe_engine import recipe_api


class RevisionResolver(object):
  """Resolves the revision based on build properties."""

  def resolve(self, properties):  # pragma: no cover
    raise NotImplementedError()


class RevisionFallbackChain(RevisionResolver):
  """Specify that a given project's sync revision follows the fallback chain."""
  def __init__(self, default=None):
    self._default = default

  def resolve(self, properties):
    """Resolve the revision via the revision fallback chain.

    If the given revision was set using the revision_fallback_chain() function,
    this function will follow the chain, looking at relevant build properties
    until it finds one set or reaches the end of the chain and returns the
    default. If the given revision was not set using revision_fallback_chain(),
    this function just returns it as-is.
    """
    return (properties.get('parent_got_revision') or
            properties.get('orig_revision') or
            properties.get('revision') or
            self._default)


def jsonish_to_python(spec, is_top=False):
  """Turn a json spec into a python parsable object.

  This exists because Gclient specs, while resembling json, is actually
  ingested using a python "eval()".  Therefore a bit of plumming is required
  to turn our newly constructed Gclient spec into a gclient-readable spec.
  """
  ret = ''
  if is_top:  # We're the 'top' level, so treat this dict as a suite.
    ret = '\n'.join(
      '%s = %s' % (k, jsonish_to_python(spec[k])) for k in sorted(spec)
    )
  else:
    if isinstance(spec, dict):
      ret += '{'
      ret += ', '.join(
        "%s: %s" % (repr(str(k)), jsonish_to_python(spec[k]))
        for k in sorted(spec)
      )
      ret += '}'
    elif isinstance(spec, list):
      ret += '['
      ret += ', '.join(jsonish_to_python(x) for x in spec)
      ret += ']'
    elif isinstance(spec, basestring):
      ret = repr(str(spec))
    else:
      ret = repr(spec)
  return ret

class GclientApi(recipe_api.RecipeApi):
  # Singleton object to indicate to checkout() that we should run a revert if
  # we detect that we're on the tryserver.
  RevertOnTryserver = object()

  def __init__(self, **kwargs):
    super(GclientApi, self).__init__(**kwargs)
    self.USE_MIRROR = None
    self._spec_alias = None

  def __call__(self, name, cmd, infra_step=True, **kwargs):
    """Wrapper for easy calling of gclient steps."""
    assert isinstance(cmd, (list, tuple))
    prefix = 'gclient '
    if self.spec_alias:
      prefix = ('[spec: %s] ' % self.spec_alias) + prefix

    # TODO(phajdan.jr): create a helper for adding to PATH.
    env = self.m.context.env
    env.setdefault('PATH', '%(PATH)s')
    env['PATH'] = self.m.path.pathsep.join([
        env['PATH'], str(self._module.PACKAGE_REPO_ROOT)])

    with self.m.context(env=env):
      return self.m.python(prefix + name,
                           self.package_repo_resource('gclient.py'),
                           cmd,
                           infra_step=infra_step,
                           **kwargs)

  @property
  def use_mirror(self):
    """Indicates if gclient will use mirrors in its configuration."""
    if self.USE_MIRROR is None:
      self.USE_MIRROR = self.m.properties.get('use_mirror', True)
    return self.USE_MIRROR

  @use_mirror.setter
  def use_mirror(self, val):  # pragma: no cover
    self.USE_MIRROR = val

  @property
  def spec_alias(self):
    """Optional name for the current spec for step naming."""
    return self._spec_alias

  @spec_alias.setter
  def spec_alias(self, name):
    self._spec_alias = name

  @spec_alias.deleter
  def spec_alias(self):
    self._spec_alias = None

  def get_config_defaults(self):
    return {
      'USE_MIRROR': self.use_mirror,
      'CACHE_DIR': self.m.infra_paths.default_git_cache_dir,
    }

  @staticmethod
  def config_to_pythonish(cfg):
    return jsonish_to_python(cfg.as_jsonish(), True)

  # TODO(machenbach): Remove this method when the old mapping is deprecated.
  @staticmethod
  def got_revision_reverse_mapping(cfg):
    """Returns the merged got_revision_reverse_mapping.

    Returns (dict): A mapping from property name -> project name. It merges the
        values of the deprecated got_revision_mapping and the new
        got_revision_reverse_mapping.
    """
    rev_map = cfg.got_revision_mapping.as_jsonish()
    reverse_rev_map = cfg.got_revision_reverse_mapping.as_jsonish()
    combined_length = len(rev_map) + len(reverse_rev_map)
    reverse_rev_map.update({v: k for k, v in rev_map.iteritems()})

    # Make sure we never have duplicate values in the old map.
    assert combined_length == len(reverse_rev_map)
    return reverse_rev_map

  def resolve_revision(self, revision):
    if hasattr(revision, 'resolve'):
      return revision.resolve(self.m.properties)
    return revision

  def sync(self, cfg, extra_sync_flags=None, **kwargs):
    revisions = []
    self.set_patch_project_revision(self.m.properties.get('patch_project'), cfg)
    for i, s in enumerate(cfg.solutions):
      if i == 0 and s.revision is None:
        s.revision = RevisionFallbackChain()

      if s.revision is not None and s.revision != '':
        fixed_revision = self.resolve_revision(s.revision)
        if fixed_revision:
          revisions.extend(['--revision', '%s@%s' % (s.name, fixed_revision)])

    for name, revision in sorted(cfg.revisions.items()):
      fixed_revision = self.resolve_revision(revision)
      if fixed_revision:
        revisions.extend(['--revision', '%s@%s' % (name, fixed_revision)])

    test_data_paths = set(self.got_revision_reverse_mapping(cfg).values() +
                          [s.name for s in cfg.solutions])
    step_test_data = lambda: (
      self.test_api.output_json(test_data_paths))
    try:
      # clean() isn't used because the gclient sync flags passed in checkout()
      # do much the same thing, and they're more correct than doing a separate
      # 'gclient revert' because it makes sure the other args are correct when
      # a repo was deleted and needs to be re-cloned (notably
      # --with_branch_heads), whereas 'revert' uses default args for clone
      # operations.
      #
      # TODO(mmoss): To be like current official builders, this step could
      # just delete the whole <slave_name>/build/ directory and start each
      # build from scratch. That might be the least bad solution, at least
      # until we have a reliable gclient method to produce a pristine working
      # dir for git-based builds (e.g. maybe some combination of 'git
      # reset/clean -fx' and removing the 'out' directory).
      j = '-j2' if self.m.platform.is_win else '-j8'
      args = ['sync', '--verbose', '--nohooks', j, '--reset', '--force',
              '--upstream', '--no-nag-max', '--with_branch_heads',
              '--with_tags']
      args.extend(extra_sync_flags or [])
      if cfg.delete_unversioned_trees:
        args.append('--delete_unversioned_trees')
      self('sync', args + revisions +
                 ['--output-json', self.m.json.output()],
                 step_test_data=step_test_data,
                 **kwargs)
    finally:
      result = self.m.step.active_result
      solutions = result.json.output['solutions']
      for propname, path in sorted(
          self.got_revision_reverse_mapping(cfg).iteritems()):
        # gclient json paths always end with a slash
        info = solutions.get(path + '/') or solutions.get(path)
        if info:
          result.presentation.properties[propname] = info['revision']

    return result

  def inject_parent_got_revision(self, gclient_config=None, override=False):
    """Match gclient config to build revisions obtained from build_properties.

    Args:
      gclient_config (gclient config object) - The config to manipulate. A value
        of None manipulates the module's built-in config (self.c).
      override (bool) - If True, will forcibly set revision and custom_vars
        even if the config already contains values for them.
    """
    cfg = gclient_config or self.c

    for prop, custom_var in cfg.parent_got_revision_mapping.iteritems():
      val = str(self.m.properties.get(prop, ''))
      # TODO(infra): Fix coverage.
      if val:  # pragma: no cover
        # Special case for 'src', inject into solutions[0]
        if custom_var is None:
          # This is not covered because we are deprecating this feature and
          # it is no longer used by the public recipes.
          if cfg.solutions[0].revision is None or override:  # pragma: no cover
            cfg.solutions[0].revision = val
        else:
          if custom_var not in cfg.solutions[0].custom_vars or override:
            cfg.solutions[0].custom_vars[custom_var] = val

  def checkout(self, gclient_config=None, revert=RevertOnTryserver,
               inject_parent_got_revision=True, extra_sync_flags=None,
               **kwargs):
    """Return a step generator function for gclient checkouts."""
    cfg = gclient_config or self.c
    assert cfg.complete()

    if revert is self.RevertOnTryserver:
      revert = self.m.tryserver.is_tryserver

    if inject_parent_got_revision:
      self.inject_parent_got_revision(cfg, override=True)

    self('setup', ['config', '--spec', self.config_to_pythonish(cfg)], **kwargs)

    sync_step = None
    try:
      sync_step = self.sync(cfg, extra_sync_flags=extra_sync_flags, **kwargs)

      cfg_cmds = [
        ('user.name', 'local_bot'),
        ('user.email', 'local_bot@example.com'),
      ]
      for var, val in cfg_cmds:
        name = 'recurse (git config %s)' % var
        self(name, ['recurse', 'git', 'config', var, val], **kwargs)
    finally:
      cwd = kwargs.get('cwd', self.m.path['start_dir'])
      if 'checkout' not in self.m.path:
        self.m.path['checkout'] = cwd.join(
          *cfg.solutions[0].name.split(self.m.path.sep))

    return sync_step

  def runhooks(self, args=None, name='runhooks', **kwargs):
    args = args or []
    assert isinstance(args, (list, tuple))
    with self.m.context(cwd=(self.m.context.cwd or self.m.path['checkout'])):
      return self(name, ['runhooks'] + list(args), infra_step=False, **kwargs)

  @property
  def is_blink_mode(self):
    """ Indicates wether the caller is to use the Blink config rather than the
    Chromium config. This may happen for one of two reasons:
    1. The builder is configured to always use TOT Blink. (factory property
       top_of_tree_blink=True)
    2. A try job comes in that applies to the Blink tree. (patch_project is
       blink)
    """
    return (
      self.m.properties.get('top_of_tree_blink') or
      self.m.properties.get('patch_project') == 'blink')

  def break_locks(self):
    """Remove all index.lock files. If a previous run of git crashed, bot was
    reset, etc... we might end up with leftover index.lock files.
    """
    self.m.python.inline(
      'cleanup index.lock',
      """
        import os, sys

        build_path = sys.argv[1]
        if os.path.exists(build_path):
          for (path, dir, files) in os.walk(build_path):
            for cur_file in files:
              if cur_file.endswith('index.lock'):
                path_to_file = os.path.join(path, cur_file)
                print 'deleting %s' % path_to_file
                os.remove(path_to_file)
      """,
      args=[self.m.path['start_dir']],
      infra_step=True,
    )

  def calculate_patch_root(self, patch_project, gclient_config=None,
                           patch_repo=None):
    """Returns path where a patch should be applied to based patch_project.

    Maps the patch's repo to a path of directories relative to checkout's root,
    which describe where to place the patch. If no mapping is found for the
    repo url, falls back to trying to find a mapping for the old-style
    "patch_project".

    For now, considers only first solution (c.solutions[0]), but in theory can
    be extended to all of them.

    See patch_projects and repo_path_map solution config property.

    Returns:
      Relative path, including solution's root.
      If patch_project is not given or not recognized, it'll be just first
      solution root.
    """
    cfg = gclient_config or self.c
    root, _ = cfg.repo_path_map.get(patch_repo, ('', ''))
    if not root:
      root, _ = cfg.patch_projects.get(patch_project, ('', ''))
    if not root:
      # Failure case - assume patch is for first solution, as this is what most
      # projects rely on.
      return cfg.solutions[0].name
    # Note, that c.patch_projects contains patch roots as
    # slash(/)-separated path, which are roots of the respective project repos
    # and include actual solution name in them.
    return self.m.path.join(*root.split('/'))

  def set_patch_project_revision(self, patch_project, gclient_config=None):
    """Updates config revision corresponding to patch_project.

    Useful for bot_update only, as this is the only consumer of gclient's config
    revision map. This doesn't overwrite the revision if it was already set.
    """
    assert patch_project is None or isinstance(patch_project, basestring)
    cfg = gclient_config or self.c
    path, revision = cfg.patch_projects.get(patch_project, (None, None))
    if path and revision and path not in cfg.revisions:
      cfg.revisions[path] = revision
