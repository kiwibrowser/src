#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Meta checkout dependency manager for Git."""
# Files
#   .gclient      : Current client configuration, written by 'config' command.
#                   Format is a Python script defining 'solutions', a list whose
#                   entries each are maps binding the strings "name" and "url"
#                   to strings specifying the name and location of the client
#                   module, as well as "custom_deps" to a map similar to the
#                   deps section of the DEPS file below, as well as
#                   "custom_hooks" to a list similar to the hooks sections of
#                   the DEPS file below.
#   .gclient_entries : A cache constructed by 'update' command.  Format is a
#                   Python script defining 'entries', a list of the names
#                   of all modules in the client
#   <module>/DEPS : Python script defining var 'deps' as a map from each
#                   requisite submodule name to a URL where it can be found (via
#                   one SCM)
#
# Hooks
#   .gclient and DEPS files may optionally contain a list named "hooks" to
#   allow custom actions to be performed based on files that have changed in the
#   working copy as a result of a "sync"/"update" or "revert" operation.  This
#   can be prevented by using --nohooks (hooks run by default). Hooks can also
#   be forced to run with the "runhooks" operation.  If "sync" is run with
#   --force, all known but not suppressed hooks will run regardless of the state
#   of the working copy.
#
#   Each item in a "hooks" list is a dict, containing these two keys:
#     "pattern"  The associated value is a string containing a regular
#                expression.  When a file whose pathname matches the expression
#                is checked out, updated, or reverted, the hook's "action" will
#                run.
#     "action"   A list describing a command to run along with its arguments, if
#                any.  An action command will run at most one time per gclient
#                invocation, regardless of how many files matched the pattern.
#                The action is executed in the same directory as the .gclient
#                file.  If the first item in the list is the string "python",
#                the current Python interpreter (sys.executable) will be used
#                to run the command. If the list contains string
#                "$matching_files" it will be removed from the list and the list
#                will be extended by the list of matching files.
#     "name"     An optional string specifying the group to which a hook belongs
#                for overriding and organizing.
#
#   Example:
#     hooks = [
#       { "pattern": "\\.(gif|jpe?g|pr0n|png)$",
#         "action":  ["python", "image_indexer.py", "--all"]},
#       { "pattern": ".",
#         "name": "gyp",
#         "action":  ["python", "src/build/gyp_chromium"]},
#     ]
#
# Pre-DEPS Hooks
#   DEPS files may optionally contain a list named "pre_deps_hooks".  These are
#   the same as normal hooks, except that they run before the DEPS are
#   processed. Pre-DEPS run with "sync" and "revert" unless the --noprehooks
#   flag is used.
#
# Specifying a target OS
#   An optional key named "target_os" may be added to a gclient file to specify
#   one or more additional operating systems that should be considered when
#   processing the deps_os/hooks_os dict of a DEPS file.
#
#   Example:
#     target_os = [ "android" ]
#
#   If the "target_os_only" key is also present and true, then *only* the
#   operating systems listed in "target_os" will be used.
#
#   Example:
#     target_os = [ "ios" ]
#     target_os_only = True
#
# Specifying a target CPU
#   To specify a target CPU, the variables target_cpu and target_cpu_only
#   are available and are analagous to target_os and target_os_only.

from __future__ import print_function

__version__ = '0.7'

import collections
import copy
import json
import logging
import optparse
import os
import platform
import posixpath
import pprint
import re
import sys
import time
import urlparse

import detect_host_arch
import fix_encoding
import gclient_eval
import gclient_scm
import gclient_utils
import git_cache
from third_party.repo.progress import Progress
import subcommand
import subprocess2
import setup_color


class GNException(Exception):
  pass


def ToGNString(value, allow_dicts = True):
  """Returns a stringified GN equivalent of the Python value.

  allow_dicts indicates if this function will allow converting dictionaries
  to GN scopes. This is only possible at the top level, you can't nest a
  GN scope in a list, so this should be set to False for recursive calls."""
  if isinstance(value, basestring):
    if value.find('\n') >= 0:
      raise GNException("Trying to print a string with a newline in it.")
    return '"' + \
        value.replace('\\', '\\\\').replace('"', '\\"').replace('$', '\\$') + \
        '"'

  if isinstance(value, unicode):
    return ToGNString(value.encode('utf-8'))

  if isinstance(value, bool):
    if value:
      return "true"
    return "false"

  # NOTE: some type handling removed compared to chromium/src copy.

  raise GNException("Unsupported type when printing to GN.")


class Hook(object):
  """Descriptor of command ran before/after sync or on demand."""

  def __init__(self, action, pattern=None, name=None, cwd=None, condition=None,
               variables=None, verbose=False):
    """Constructor.

    Arguments:
      action (list of basestring): argv of the command to run
      pattern (basestring regex): noop with git; deprecated
      name (basestring): optional name; no effect on operation
      cwd (basestring): working directory to use
      condition (basestring): condition when to run the hook
      variables (dict): variables for evaluating the condition
    """
    self._action = gclient_utils.freeze(action)
    self._pattern = pattern
    self._name = name
    self._cwd = cwd
    self._condition = condition
    self._variables = variables
    self._verbose = verbose

  @staticmethod
  def from_dict(d, variables=None, verbose=False, conditions=None):
    """Creates a Hook instance from a dict like in the DEPS file."""
    # Merge any local and inherited conditions.
    gclient_eval.UpdateCondition(d, 'and', conditions)
    return Hook(
        d['action'],
        d.get('pattern'),
        d.get('name'),
        d.get('cwd'),
        d.get('condition'),
        variables=variables,
        # Always print the header if not printing to a TTY.
        verbose=verbose or not setup_color.IS_TTY)

  @property
  def action(self):
    return self._action

  @property
  def pattern(self):
    return self._pattern

  @property
  def name(self):
    return self._name

  @property
  def condition(self):
    return self._condition

  def matches(self, file_list):
    """Returns true if the pattern matches any of files in the list."""
    if not self._pattern:
      return True
    pattern = re.compile(self._pattern)
    return bool([f for f in file_list if pattern.search(f)])

  def run(self, root):
    """Executes the hook's command (provided the condition is met)."""
    if (self._condition and
        not gclient_eval.EvaluateCondition(self._condition, self._variables)):
      return

    cmd = [arg.format(**self._variables) for arg in self._action]

    if cmd[0] == 'python':
      # If the hook specified "python" as the first item, the action is a
      # Python script.  Run it by starting a new copy of the same
      # interpreter.
      cmd[0] = sys.executable
    elif cmd[0] == 'vpython' and _detect_host_os() == 'win':
      cmd[0] += '.bat'

    cwd = root
    if self._cwd:
      cwd = os.path.join(cwd, self._cwd)
    try:
      start_time = time.time()
      gclient_utils.CheckCallAndFilterAndHeader(
          cmd, cwd=cwd, always=self._verbose)
    except (gclient_utils.Error, subprocess2.CalledProcessError) as e:
      # Use a discrete exit status code of 2 to indicate that a hook action
      # failed.  Users of this script may wish to treat hook action failures
      # differently from VC failures.
      print('Error: %s' % str(e), file=sys.stderr)
      sys.exit(2)
    finally:
      elapsed_time = time.time() - start_time
      if elapsed_time > 10:
        print("Hook '%s' took %.2f secs" % (
            gclient_utils.CommandToStr(cmd), elapsed_time))


class DependencySettings(object):
  """Immutable configuration settings."""
  def __init__(
      self, parent, raw_url, url, managed, custom_deps, custom_vars,
      custom_hooks, deps_file, should_process, relative, condition):
    # These are not mutable:
    self._parent = parent
    self._deps_file = deps_file
    self._raw_url = raw_url
    self._url = url
    # The condition as string (or None). Useful to keep e.g. for flatten.
    self._condition = condition
    # 'managed' determines whether or not this dependency is synced/updated by
    # gclient after gclient checks it out initially.  The difference between
    # 'managed' and 'should_process' is that the user specifies 'managed' via
    # the --unmanaged command-line flag or a .gclient config, where
    # 'should_process' is dynamically set by gclient if it goes over its
    # recursion limit and controls gclient's behavior so it does not misbehave.
    self._managed = managed
    self._should_process = should_process
    # If this is a recursed-upon sub-dependency, and the parent has
    # use_relative_paths set, then this dependency should check out its own
    # dependencies relative to that parent's path for this, rather than
    # relative to the .gclient file.
    self._relative = relative
    # This is a mutable value which has the list of 'target_os' OSes listed in
    # the current deps file.
    self.local_target_os = None

    # These are only set in .gclient and not in DEPS files.
    self._custom_vars = custom_vars or {}
    self._custom_deps = custom_deps or {}
    self._custom_hooks = custom_hooks or []

    # Post process the url to remove trailing slashes.
    if isinstance(self.url, basestring):
      # urls are sometime incorrectly written as proto://host/path/@rev. Replace
      # it to proto://host/path@rev.
      self.set_url(self.url.replace('/@', '@'))
    elif not isinstance(self.url, (None.__class__)):
      raise gclient_utils.Error(
          ('dependency url must be either string or None, '
           'instead of %s') % self.url.__class__.__name__)

    # Make any deps_file path platform-appropriate.
    if self._deps_file:
      for sep in ['/', '\\']:
        self._deps_file = self._deps_file.replace(sep, os.sep)

  @property
  def deps_file(self):
    return self._deps_file

  @property
  def managed(self):
    return self._managed

  @property
  def parent(self):
    return self._parent

  @property
  def root(self):
    """Returns the root node, a GClient object."""
    if not self.parent:
      # This line is to signal pylint that it could be a GClient instance.
      return self or GClient(None, None)
    return self.parent.root

  @property
  def should_process(self):
    """True if this dependency should be processed, i.e. checked out."""
    return self._should_process

  @property
  def custom_vars(self):
    return self._custom_vars.copy()

  @property
  def custom_deps(self):
    return self._custom_deps.copy()

  @property
  def custom_hooks(self):
    return self._custom_hooks[:]

  @property
  def raw_url(self):
    """URL before variable expansion."""
    return self._raw_url

  @property
  def url(self):
    """URL after variable expansion."""
    return self._url

  @property
  def condition(self):
    return self._condition

  @property
  def target_os(self):
    if self.local_target_os is not None:
      return tuple(set(self.local_target_os).union(self.parent.target_os))
    else:
      return self.parent.target_os

  @property
  def target_cpu(self):
    return self.parent.target_cpu

  def set_url(self, url):
    self._url = url

  def set_raw_url(self, url):
    self._raw_url = url

  def get_custom_deps(self, name, url):
    """Returns a custom deps if applicable."""
    if self.parent:
      url = self.parent.get_custom_deps(name, url)
    # None is a valid return value to disable a dependency.
    return self.custom_deps.get(name, url)


class Dependency(gclient_utils.WorkItem, DependencySettings):
  """Object that represents a dependency checkout."""

  def __init__(self, parent, name, raw_url, url, managed, custom_deps,
               custom_vars, custom_hooks, deps_file, should_process,
               relative, condition, print_outbuf=False):
    gclient_utils.WorkItem.__init__(self, name)
    DependencySettings.__init__(
        self, parent, raw_url, url, managed, custom_deps, custom_vars,
        custom_hooks, deps_file, should_process, relative, condition)

    # This is in both .gclient and DEPS files:
    self._deps_hooks = []

    self._pre_deps_hooks = []

    # Calculates properties:
    self._dependencies = []
    self._vars = {}

    # A cache of the files affected by the current operation, necessary for
    # hooks.
    self._file_list = []
    # List of host names from which dependencies are allowed.
    # Default is an empty set, meaning unspecified in DEPS file, and hence all
    # hosts will be allowed. Non-empty set means whitelist of hosts.
    # allowed_hosts var is scoped to its DEPS file, and so it isn't recursive.
    self._allowed_hosts = frozenset()
    self._gn_args_from = None
    # Spec for .gni output to write (if any).
    self._gn_args_file = None
    self._gn_args = []
    # If it is not set to True, the dependency wasn't processed for its child
    # dependency, i.e. its DEPS wasn't read.
    self._deps_parsed = False
    # This dependency has been processed, i.e. checked out
    self._processed = False
    # This dependency had its pre-DEPS hooks run
    self._pre_deps_hooks_ran = False
    # This dependency had its hook run
    self._hooks_ran = False
    # This is the scm used to checkout self.url. It may be used by dependencies
    # to get the datetime of the revision we checked out.
    self._used_scm = None
    self._used_revision = None
    # The actual revision we ended up getting, or None if that information is
    # unavailable
    self._got_revision = None

    # This is a mutable value that overrides the normal recursion limit for this
    # dependency.  It is read from the actual DEPS file so cannot be set on
    # class instantiation.
    self.recursion_override = None
    # recursedeps is a mutable value that selectively overrides the default
    # 'no recursion' setting on a dep-by-dep basis.  It will replace
    # recursion_override.
    #
    # It will be a dictionary of {deps_name: {"deps_file": depfile_name}} or
    # None.
    self.recursedeps = None

    self._OverrideUrl()
    # This is inherited from WorkItem.  We want the URL to be a resource.
    if self.url and isinstance(self.url, basestring):
      # The url is usually given to gclient either as https://blah@123
      # or just https://blah.  The @123 portion is irrelevant.
      self.resources.append(self.url.split('@')[0])

    # Controls whether we want to print git's output when we first clone the
    # dependency
    self.print_outbuf = print_outbuf

    if not self.name and self.parent:
      raise gclient_utils.Error('Dependency without name')

  def _OverrideUrl(self):
    """Resolves the parsed url from the parent hierarchy."""
    parsed_url = self.get_custom_deps(self._name, self.url)
    if parsed_url != self.url:
      logging.info('Dependency(%s)._OverrideUrl(%s) -> %s', self._name,
                   self.url, parsed_url)
      self.set_url(parsed_url)

    elif isinstance(self.url, basestring):
      parsed_url = urlparse.urlparse(self.url)
      if (not parsed_url[0] and
          not re.match(r'^\w+\@[\w\.-]+\:[\w\/]+', parsed_url[2])):
        path = parsed_url[2]
        if not path.startswith('/'):
          raise gclient_utils.Error(
              'relative DEPS entry \'%s\' must begin with a slash' % self.url)
        # A relative url. Get the parent url, strip from the last '/'
        # (equivalent to unix basename), and append the relative url.
        parent_url = self.parent.url
        parsed_url = parent_url[:parent_url.rfind('/')] + self.url
        logging.info('Dependency(%s)._OverrideUrl(%s) -> %s', self.name,
                     self.url, parsed_url)
        self.set_url(parsed_url)

    elif self.url is None:
      logging.info('Dependency(%s)._OverrideUrl(None) -> None', self._name)

    else:
      raise gclient_utils.Error('Unknown url type')

  def PinToActualRevision(self):
    """Updates self.url and self.raw_url to the revision checked out on disk."""
    if self.url is None:
      return
    url = raw_url = None
    scm = self.CreateSCM()
    if os.path.isdir(scm.checkout_path):
      revision = scm.revinfo(None, None, None)
      url = '%s@%s' % (gclient_utils.SplitUrlRevision(self.url)[0], revision)
      raw_url = '%s@%s' % (
          gclient_utils.SplitUrlRevision(self.raw_url)[0], revision)
    self.set_url(url)
    self.set_raw_url(raw_url)

  def ToLines(self):
    s = []
    condition_part = (['    "condition": %r,' % self.condition]
                      if self.condition else [])
    s.extend([
        '  # %s' % self.hierarchy(include_url=False),
        '  "%s": {' % (self.name,),
        '    "url": "%s",' % (self.raw_url,),
    ] + condition_part + [
        '  },',
        '',
    ])
    return s

  @property
  def requirements(self):
    """Calculate the list of requirements."""
    requirements = set()
    # self.parent is implicitly a requirement. This will be recursive by
    # definition.
    if self.parent and self.parent.name:
      requirements.add(self.parent.name)

    # For a tree with at least 2 levels*, the leaf node needs to depend
    # on the level higher up in an orderly way.
    # This becomes messy for >2 depth as the DEPS file format is a dictionary,
    # thus unsorted, while the .gclient format is a list thus sorted.
    #
    # * _recursion_limit is hard coded 2 and there is no hope to change this
    # value.
    #
    # Interestingly enough, the following condition only works in the case we
    # want: self is a 2nd level node. 3nd level node wouldn't need this since
    # they already have their parent as a requirement.
    if self.parent and self.parent.parent and not self.parent.parent.parent:
      requirements |= set(i.name for i in self.root.dependencies if i.name)

    if self.name:
      requirements |= set(
          obj.name for obj in self.root.subtree(False)
          if (obj is not self
              and obj.name and
              self.name.startswith(posixpath.join(obj.name, ''))))
    requirements = tuple(sorted(requirements))
    logging.info('Dependency(%s).requirements = %s' % (self.name, requirements))
    return requirements

  @property
  def try_recursedeps(self):
    """Returns False if recursion_override is ever specified."""
    if self.recursion_override is not None:
      return False
    return self.parent.try_recursedeps

  @property
  def recursion_limit(self):
    """Returns > 0 if this dependency is not too recursed to be processed."""
    # We continue to support the absence of recursedeps until tools and DEPS
    # using recursion_override are updated.
    if self.try_recursedeps and self.parent.recursedeps != None:
      if self.name in self.parent.recursedeps:
        return 1

    if self.recursion_override is not None:
      return self.recursion_override
    return max(self.parent.recursion_limit - 1, 0)

  def verify_validity(self):
    """Verifies that this Dependency is fine to add as a child of another one.

    Returns True if this entry should be added, False if it is a duplicate of
    another entry.
    """
    logging.info('Dependency(%s).verify_validity()' % self.name)
    if self.name in [s.name for s in self.parent.dependencies]:
      raise gclient_utils.Error(
          'The same name "%s" appears multiple times in the deps section' %
              self.name)
    if not self.should_process:
      # Return early, no need to set requirements.
      return True

    # This require a full tree traversal with locks.
    siblings = [d for d in self.root.subtree(False) if d.name == self.name]
    for sibling in siblings:
      # Allow to have only one to be None or ''.
      if self.url != sibling.url and bool(self.url) == bool(sibling.url):
        raise gclient_utils.Error(
            ('Dependency %s specified more than once:\n'
            '  %s [%s]\n'
            'vs\n'
            '  %s [%s]') % (
              self.name,
              sibling.hierarchy(),
              sibling.url,
              self.hierarchy(),
              self.url))
      # In theory we could keep it as a shadow of the other one. In
      # practice, simply ignore it.
      logging.warn('Won\'t process duplicate dependency %s' % sibling)
      return False
    return True

  def _postprocess_deps(self, deps, rel_prefix):
    """Performs post-processing of deps compared to what's in the DEPS file."""
    # Make sure the dict is mutable, e.g. in case it's frozen.
    deps = dict(deps)

    # If a line is in custom_deps, but not in the solution, we want to append
    # this line to the solution.
    for dep_name, dep_info in self.custom_deps.iteritems():
      if dep_name not in deps:
        deps[dep_name] = {'url': dep_info, 'dep_type': 'git'}

    # Make child deps conditional on any parent conditions. This ensures that,
    # when flattened, recursed entries have the correct restrictions, even if
    # not explicitly set in the recursed DEPS file. For instance, if
    # "src/ios_foo" is conditional on "checkout_ios=True", then anything
    # recursively included by "src/ios_foo/DEPS" should also require
    # "checkout_ios=True".
    if self.condition:
      for value in deps.itervalues():
        gclient_eval.UpdateCondition(value, 'and', self.condition)

    if rel_prefix:
      logging.warning('use_relative_paths enabled.')
      rel_deps = {}
      for d, url in deps.items():
        # normpath is required to allow DEPS to use .. in their
        # dependency local path.
        rel_deps[os.path.normpath(os.path.join(rel_prefix, d))] = url
      logging.warning('Updating deps by prepending %s.', rel_prefix)
      deps = rel_deps

    return deps

  def _deps_to_objects(self, deps, use_relative_paths):
    """Convert a deps dict to a dict of Dependency objects."""
    deps_to_add = []
    for name, dep_value in deps.iteritems():
      should_process = self.recursion_limit and self.should_process
      deps_file = self.deps_file
      if self.recursedeps is not None:
        ent = self.recursedeps.get(name)
        if ent is not None:
          deps_file = ent['deps_file']
      if dep_value is None:
        continue

      condition = dep_value.get('condition')
      dep_type = dep_value.get('dep_type')

      if condition and not self._get_option('process_all_deps', False):
        should_process = should_process and gclient_eval.EvaluateCondition(
            condition, self.get_vars())

      if dep_type == 'cipd':
        cipd_root = self.GetCipdRoot()
        for package in dep_value.get('packages', []):
          if 'version' in package:
            # Matches version to vars value.
            raw_version = package['version']
            version = raw_version.format(**self.get_vars())
            package['version'] = version
          deps_to_add.append(
              CipdDependency(
                  self, name, package, cipd_root, self.custom_vars,
                  should_process, use_relative_paths, condition))
      else:
        raw_url = dep_value.get('url')
        url = raw_url.format(**self.get_vars()) if raw_url else None
        deps_to_add.append(
            GitDependency(
                self, name, raw_url, url, None, None, self.custom_vars, None,
                deps_file, should_process, use_relative_paths, condition))

    deps_to_add.sort(key=lambda x: x.name)
    return deps_to_add

  def ParseDepsFile(self, expand_vars=True):
    """Parses the DEPS file for this dependency."""
    assert not self.deps_parsed
    assert not self.dependencies

    deps_content = None

    # First try to locate the configured deps file.  If it's missing, fallback
    # to DEPS.
    deps_files = [self.deps_file]
    if 'DEPS' not in deps_files:
      deps_files.append('DEPS')
    for deps_file in deps_files:
      filepath = os.path.join(self.root.root_dir, self.name, deps_file)
      if os.path.isfile(filepath):
        logging.info(
            'ParseDepsFile(%s): %s file found at %s', self.name, deps_file,
            filepath)
        break
      logging.info(
          'ParseDepsFile(%s): No %s file found at %s', self.name, deps_file,
          filepath)

    if os.path.isfile(filepath):
      deps_content = gclient_utils.FileRead(filepath)
      logging.debug('ParseDepsFile(%s) read:\n%s', self.name, deps_content)

    local_scope = {}
    if deps_content:
      try:
        local_scope = gclient_eval.Parse(
            deps_content, expand_vars,
            self._get_option('validate_syntax', False),
            filepath, self.get_vars())
      except SyntaxError as e:
        gclient_utils.SyntaxErrorToError(filepath, e)

    if 'allowed_hosts' in local_scope:
      try:
        self._allowed_hosts = frozenset(local_scope.get('allowed_hosts'))
      except TypeError:  # raised if non-iterable
        pass
      if not self._allowed_hosts:
        logging.warning("allowed_hosts is specified but empty %s",
                        self._allowed_hosts)
        raise gclient_utils.Error(
            'ParseDepsFile(%s): allowed_hosts must be absent '
            'or a non-empty iterable' % self.name)

    self._gn_args_from = local_scope.get('gclient_gn_args_from')
    self._gn_args_file = local_scope.get('gclient_gn_args_file')
    self._gn_args = local_scope.get('gclient_gn_args', [])
    # It doesn't make sense to set all of these, since setting gn_args_from to
    # another DEPS will make gclient ignore any other local gn_args* settings.
    assert not (self._gn_args_from and self._gn_args_file), \
        'Only specify one of "gclient_gn_args_from" or ' \
        '"gclient_gn_args_file + gclient_gn_args".'

    self._vars = local_scope.get('vars', {})
    if self.parent:
      for key, value in self.parent.get_vars().iteritems():
        if key in self._vars:
          self._vars[key] = value
    # Since we heavily post-process things, freeze ones which should
    # reflect original state of DEPS.
    self._vars = gclient_utils.freeze(self._vars)

    # If use_relative_paths is set in the DEPS file, regenerate
    # the dictionary using paths relative to the directory containing
    # the DEPS file.  Also update recursedeps if use_relative_paths is
    # enabled.
    # If the deps file doesn't set use_relative_paths, but the parent did
    # (and therefore set self.relative on this Dependency object), then we
    # want to modify the deps and recursedeps by prepending the parent
    # directory of this dependency.
    use_relative_paths = local_scope.get('use_relative_paths', False)
    rel_prefix = None
    if use_relative_paths:
      rel_prefix = self.name
    elif self._relative:
      rel_prefix = os.path.dirname(self.name)

    if 'recursion' in local_scope:
      self.recursion_override = local_scope.get('recursion')
      logging.warning(
          'Setting %s recursion to %d.', self.name, self.recursion_limit)
    self.recursedeps = None
    if 'recursedeps' in local_scope:
      self.recursedeps = {}
      for ent in local_scope['recursedeps']:
        if isinstance(ent, basestring):
          self.recursedeps[ent] = {"deps_file": self.deps_file}
        else:  # (depname, depsfilename)
          self.recursedeps[ent[0]] = {"deps_file": ent[1]}
      logging.warning('Found recursedeps %r.', repr(self.recursedeps))

      if rel_prefix:
        logging.warning('Updating recursedeps by prepending %s.', rel_prefix)
        rel_deps = {}
        for depname, options in self.recursedeps.iteritems():
          rel_deps[
              os.path.normpath(os.path.join(rel_prefix, depname))] = options
        self.recursedeps = rel_deps
    # To get gn_args from another DEPS, that DEPS must be recursed into.
    if self._gn_args_from:
      assert self.recursedeps and self._gn_args_from in self.recursedeps, \
              'The "gclient_gn_args_from" value must be in recursedeps.'

    # If present, save 'target_os' in the local_target_os property.
    if 'target_os' in local_scope:
      self.local_target_os = local_scope['target_os']

    deps = local_scope.get('deps', {})
    deps_to_add = self._deps_to_objects(
        self._postprocess_deps(deps, rel_prefix), use_relative_paths)

    # override named sets of hooks by the custom hooks
    hooks_to_run = []
    hook_names_to_suppress = [c.get('name', '') for c in self.custom_hooks]
    for hook in local_scope.get('hooks', []):
      if hook.get('name', '') not in hook_names_to_suppress:
        hooks_to_run.append(hook)

    # add the replacements and any additions
    for hook in self.custom_hooks:
      if 'action' in hook:
        hooks_to_run.append(hook)

    if self.recursion_limit:
      self._pre_deps_hooks = [
          Hook.from_dict(hook, variables=self.get_vars(), verbose=True,
                         conditions=self.condition)
          for hook in local_scope.get('pre_deps_hooks', [])
      ]

    self.add_dependencies_and_close(deps_to_add, hooks_to_run)
    logging.info('ParseDepsFile(%s) done' % self.name)

  def _get_option(self, attr, default):
    obj = self
    while not hasattr(obj, '_options'):
      obj = obj.parent
    return getattr(obj._options, attr, default)

  def add_dependencies_and_close(self, deps_to_add, hooks):
    """Adds the dependencies, hooks and mark the parsing as done."""
    for dep in deps_to_add:
      if dep.verify_validity():
        self.add_dependency(dep)
    self._mark_as_parsed([
        Hook.from_dict(
            h, variables=self.get_vars(), verbose=self.root._options.verbose,
            conditions=self.condition)
        for h in hooks
    ])

  def findDepsFromNotAllowedHosts(self):
    """Returns a list of depenecies from not allowed hosts.

    If allowed_hosts is not set, allows all hosts and returns empty list.
    """
    if not self._allowed_hosts:
      return []
    bad_deps = []
    for dep in self._dependencies:
      # Don't enforce this for custom_deps.
      if dep.name in self._custom_deps:
        continue
      if isinstance(dep.url, basestring):
        parsed_url = urlparse.urlparse(dep.url)
        if parsed_url.netloc and parsed_url.netloc not in self._allowed_hosts:
          bad_deps.append(dep)
    return bad_deps

  def FuzzyMatchUrl(self, candidates):
    """Attempts to find this dependency in the list of candidates.

    It looks first for the URL of this dependency in the list of
    candidates. If it doesn't succeed, and the URL ends in '.git', it will try
    looking for the URL minus '.git'. Finally it will try to look for the name
    of the dependency.

    Args:
      candidates: list, dict. The list of candidates in which to look for this
          dependency. It can contain URLs as above, or dependency names like
          "src/some/dep".

    Returns:
      If this dependency is not found in the list of candidates, returns None.
      Otherwise, it returns under which name did we find this dependency:
       - Its parsed url: "https://example.com/src.git'
       - Its parsed url minus '.git': "https://example.com/src"
       - Its name: "src"
    """
    if self.url:
      origin, _ = gclient_utils.SplitUrlRevision(self.url)
      if origin in candidates:
        return origin
      if origin.endswith('.git') and origin[:-len('.git')] in candidates:
        return origin[:-len('.git')]
      if origin + '.git' in candidates:
        return origin + '.git'
    if self.name in candidates:
      return self.name
    return None

  # Arguments number differs from overridden method
  # pylint: disable=arguments-differ
  def run(self, revision_overrides, command, args, work_queue, options,
          patch_refs):
    """Runs |command| then parse the DEPS file."""
    logging.info('Dependency(%s).run()' % self.name)
    assert self._file_list == []
    if not self.should_process:
      return
    # When running runhooks, there's no need to consult the SCM.
    # All known hooks are expected to run unconditionally regardless of working
    # copy state, so skip the SCM status check.
    run_scm = command not in (
        'flatten', 'runhooks', 'recurse', 'validate', None)
    file_list = [] if not options.nohooks else None
    revision_override = revision_overrides.pop(
        self.FuzzyMatchUrl(revision_overrides), None)
    if run_scm and self.url:
      # Create a shallow copy to mutate revision.
      options = copy.copy(options)
      options.revision = revision_override
      self._used_revision = options.revision
      self._used_scm = self.CreateSCM(out_cb=work_queue.out_cb)
      self._got_revision = self._used_scm.RunCommand(command, options, args,
                                                     file_list)

      patch_repo = self.url.split('@')[0]
      patch_ref = patch_refs.pop(self.FuzzyMatchUrl(patch_refs), None)
      if command == 'update' and patch_ref is not None:
        self._used_scm.apply_patch_ref(patch_repo, patch_ref, options,
                                       file_list)

      if file_list:
        file_list = [os.path.join(self.name, f.strip()) for f in file_list]

      # TODO(phajdan.jr): We should know exactly when the paths are absolute.
      # Convert all absolute paths to relative.
      for i in range(len(file_list or [])):
        # It depends on the command being executed (like runhooks vs sync).
        if not os.path.isabs(file_list[i]):
          continue
        prefix = os.path.commonprefix(
            [self.root.root_dir.lower(), file_list[i].lower()])
        file_list[i] = file_list[i][len(prefix):]
        # Strip any leading path separators.
        while file_list[i].startswith(('\\', '/')):
          file_list[i] = file_list[i][1:]

    if self.recursion_limit:
      self.ParseDepsFile(expand_vars=(command != 'flatten'))

    self._run_is_done(file_list or [])

    if self.recursion_limit:
      if command in ('update', 'revert') and not options.noprehooks:
        self.RunPreDepsHooks()
      # Parse the dependencies of this dependency.
      for s in self.dependencies:
        if s.should_process:
          work_queue.enqueue(s)

    if command == 'recurse':
      # Skip file only checkout.
      scm = self.GetScmName()
      if not options.scm or scm in options.scm:
        cwd = os.path.normpath(os.path.join(self.root.root_dir, self.name))
        # Pass in the SCM type as an env variable.  Make sure we don't put
        # unicode strings in the environment.
        env = os.environ.copy()
        if scm:
          env['GCLIENT_SCM'] = str(scm)
        if self.url:
          env['GCLIENT_URL'] = str(self.url)
        env['GCLIENT_DEP_PATH'] = str(self.name)
        if options.prepend_dir and scm == 'git':
          print_stdout = False
          def filter_fn(line):
            """Git-specific path marshaling. It is optimized for git-grep."""

            def mod_path(git_pathspec):
              match = re.match('^(\\S+?:)?([^\0]+)$', git_pathspec)
              modified_path = os.path.join(self.name, match.group(2))
              branch = match.group(1) or ''
              return '%s%s' % (branch, modified_path)

            match = re.match('^Binary file ([^\0]+) matches$', line)
            if match:
              print('Binary file %s matches\n' % mod_path(match.group(1)))
              return

            items = line.split('\0')
            if len(items) == 2 and items[1]:
              print('%s : %s' % (mod_path(items[0]), items[1]))
            elif len(items) >= 2:
              # Multiple null bytes or a single trailing null byte indicate
              # git is likely displaying filenames only (such as with -l)
              print('\n'.join(mod_path(path) for path in items if path))
            else:
              print(line)
        else:
          print_stdout = True
          filter_fn = None

        if self.url is None:
          print('Skipped omitted dependency %s' % cwd, file=sys.stderr)
        elif os.path.isdir(cwd):
          try:
            gclient_utils.CheckCallAndFilter(
                args, cwd=cwd, env=env, print_stdout=print_stdout,
                filter_fn=filter_fn,
                )
          except subprocess2.CalledProcessError:
            if not options.ignore:
              raise
        else:
          print('Skipped missing %s' % cwd, file=sys.stderr)

  def GetScmName(self):
    raise NotImplementedError()

  def CreateSCM(self, out_cb=None):
    raise NotImplementedError()

  def HasGNArgsFile(self):
    return self._gn_args_file is not None

  def WriteGNArgsFile(self):
    lines = ['# Generated from %r' % self.deps_file]
    variables = self.get_vars()
    for arg in self._gn_args:
      value = variables[arg]
      if isinstance(value, basestring):
        value = gclient_eval.EvaluateCondition(value, variables)
      lines.append('%s = %s' % (arg, ToGNString(value)))
    with open(os.path.join(self.root.root_dir, self._gn_args_file), 'w') as f:
      f.write('\n'.join(lines))

  @gclient_utils.lockedmethod
  def _run_is_done(self, file_list):
    # Both these are kept for hooks that are run as a separate tree traversal.
    self._file_list = file_list
    self._processed = True

  def GetHooks(self, options):
    """Evaluates all hooks, and return them in a flat list.

    RunOnDeps() must have been called before to load the DEPS.
    """
    result = []
    if not self.should_process or not self.recursion_limit:
      # Don't run the hook when it is above recursion_limit.
      return result
    # If "--force" was specified, run all hooks regardless of what files have
    # changed.
    if self.deps_hooks:
      # TODO(maruel): If the user is using git, then we don't know
      # what files have changed so we always run all hooks. It'd be nice to fix
      # that.
      result.extend(self.deps_hooks)
    for s in self.dependencies:
      result.extend(s.GetHooks(options))
    return result

  def RunHooksRecursively(self, options, progress):
    assert self.hooks_ran == False
    self._hooks_ran = True
    hooks = self.GetHooks(options)
    if progress:
      progress._total = len(hooks)
    for hook in hooks:
      if progress:
        progress.update(extra=hook.name or '')
      hook.run(self.root.root_dir)
    if progress:
      progress.end()

  def RunPreDepsHooks(self):
    assert self.processed
    assert self.deps_parsed
    assert not self.pre_deps_hooks_ran
    assert not self.hooks_ran
    for s in self.dependencies:
      assert not s.processed
    self._pre_deps_hooks_ran = True
    for hook in self.pre_deps_hooks:
      hook.run(self.root.root_dir)

  def GetCipdRoot(self):
    if self.root is self:
      # Let's not infinitely recurse. If this is root and isn't an
      # instance of GClient, do nothing.
      return None
    return self.root.GetCipdRoot()

  def subtree(self, include_all):
    """Breadth first recursion excluding root node."""
    dependencies = self.dependencies
    for d in dependencies:
      if d.should_process or include_all:
        yield d
    for d in dependencies:
      for i in d.subtree(include_all):
        yield i

  @gclient_utils.lockedmethod
  def add_dependency(self, new_dep):
    self._dependencies.append(new_dep)

  @gclient_utils.lockedmethod
  def _mark_as_parsed(self, new_hooks):
    self._deps_hooks.extend(new_hooks)
    self._deps_parsed = True

  @property
  @gclient_utils.lockedmethod
  def dependencies(self):
    return tuple(self._dependencies)

  @property
  @gclient_utils.lockedmethod
  def deps_hooks(self):
    return tuple(self._deps_hooks)

  @property
  @gclient_utils.lockedmethod
  def pre_deps_hooks(self):
    return tuple(self._pre_deps_hooks)

  @property
  @gclient_utils.lockedmethod
  def deps_parsed(self):
    """This is purely for debugging purposes. It's not used anywhere."""
    return self._deps_parsed

  @property
  @gclient_utils.lockedmethod
  def processed(self):
    return self._processed

  @property
  @gclient_utils.lockedmethod
  def pre_deps_hooks_ran(self):
    return self._pre_deps_hooks_ran

  @property
  @gclient_utils.lockedmethod
  def hooks_ran(self):
    return self._hooks_ran

  @property
  @gclient_utils.lockedmethod
  def allowed_hosts(self):
    return self._allowed_hosts

  @property
  @gclient_utils.lockedmethod
  def file_list(self):
    return tuple(self._file_list)

  @property
  def used_scm(self):
    """SCMWrapper instance for this dependency or None if not processed yet."""
    return self._used_scm

  @property
  @gclient_utils.lockedmethod
  def got_revision(self):
    return self._got_revision

  @property
  def file_list_and_children(self):
    result = list(self.file_list)
    for d in self.dependencies:
      result.extend(d.file_list_and_children)
    return tuple(result)

  def __str__(self):
    out = []
    for i in ('name', 'url', 'custom_deps',
              'custom_vars', 'deps_hooks', 'file_list', 'should_process',
              'processed', 'hooks_ran', 'deps_parsed', 'requirements',
              'allowed_hosts'):
      # First try the native property if it exists.
      if hasattr(self, '_' + i):
        value = getattr(self, '_' + i, False)
      else:
        value = getattr(self, i, False)
      if value:
        out.append('%s: %s' % (i, value))

    for d in self.dependencies:
      out.extend(['  ' + x for x in str(d).splitlines()])
      out.append('')
    return '\n'.join(out)

  def __repr__(self):
    return '%s: %s' % (self.name, self.url)

  def hierarchy(self, include_url=True):
    """Returns a human-readable hierarchical reference to a Dependency."""
    def format_name(d):
      if include_url:
        return '%s(%s)' % (d.name, d.url)
      return d.name
    out = format_name(self)
    i = self.parent
    while i and i.name:
      out = '%s -> %s' % (format_name(i), out)
      i = i.parent
    return out

  def hierarchy_data(self):
    """Returns a machine-readable hierarchical reference to a Dependency."""
    d = self
    out = []
    while d and d.name:
      out.insert(0, (d.name, d.url))
      d = d.parent
    return tuple(out)

  def get_vars(self):
    """Returns a dictionary of effective variable values
    (DEPS file contents with applied custom_vars overrides)."""
    # Provide some built-in variables.
    result = {
        'checkout_android': 'android' in self.target_os,
        'checkout_chromeos': 'chromeos' in self.target_os,
        'checkout_fuchsia': 'fuchsia' in self.target_os,
        'checkout_ios': 'ios' in self.target_os,
        'checkout_linux': 'unix' in self.target_os,
        'checkout_mac': 'mac' in self.target_os,
        'checkout_win': 'win' in self.target_os,
        'host_os': _detect_host_os(),

        'checkout_arm': 'arm' in self.target_cpu,
        'checkout_arm64': 'arm64' in self.target_cpu,
        'checkout_x86': 'x86' in self.target_cpu,
        'checkout_mips': 'mips' in self.target_cpu,
        'checkout_ppc': 'ppc' in self.target_cpu,
        'checkout_s390': 's390' in self.target_cpu,
        'checkout_x64': 'x64' in self.target_cpu,
        'host_cpu': detect_host_arch.HostArch(),
    }
    # Variable precedence:
    # - built-in
    # - DEPS vars
    # - parents, from first to last
    # - custom_vars overrides
    result.update(self._vars)
    if self.parent:
      parent_vars = self.parent.get_vars()
      result.update(parent_vars)
    result.update(self.custom_vars or {})
    return result


_PLATFORM_MAPPING = {
  'cygwin': 'win',
  'darwin': 'mac',
  'linux2': 'linux',
  'win32': 'win',
  'aix6': 'aix',
}


def _detect_host_os():
  return _PLATFORM_MAPPING[sys.platform]


class GitDependency(Dependency):
  """A Dependency object that represents a single git checkout."""

  #override
  def GetScmName(self):
    """Always 'git'."""
    return 'git'

  #override
  def CreateSCM(self, out_cb=None):
    """Create a Wrapper instance suitable for handling this git dependency."""
    return gclient_scm.GitWrapper(
        self.url, self.root.root_dir, self.name, self.outbuf, out_cb,
        print_outbuf=self.print_outbuf)


class GClient(GitDependency):
  """Object that represent a gclient checkout. A tree of Dependency(), one per
  solution or DEPS entry."""

  DEPS_OS_CHOICES = {
    "aix6": "unix",
    "win32": "win",
    "win": "win",
    "cygwin": "win",
    "darwin": "mac",
    "mac": "mac",
    "unix": "unix",
    "linux": "unix",
    "linux2": "unix",
    "linux3": "unix",
    "android": "android",
    "ios": "ios",
  }

  DEFAULT_CLIENT_FILE_TEXT = ("""\
solutions = [
  { "name"        : "%(solution_name)s",
    "url"         : "%(solution_url)s",
    "deps_file"   : "%(deps_file)s",
    "managed"     : %(managed)s,
    "custom_deps" : {
    },
    "custom_vars": %(custom_vars)r,
  },
]
cache_dir = %(cache_dir)r
""")

  DEFAULT_SNAPSHOT_FILE_TEXT = ("""\
# Snapshot generated with gclient revinfo --snapshot
solutions = %(solution_list)s
""")

  def __init__(self, root_dir, options):
    # Do not change previous behavior. Only solution level and immediate DEPS
    # are processed.
    self._recursion_limit = 2
    GitDependency.__init__(self, None, None, None, None, True, None, None, None,
                           'unused', True, None, None, True)
    self._options = options
    if options.deps_os:
      enforced_os = options.deps_os.split(',')
    else:
      enforced_os = [self.DEPS_OS_CHOICES.get(sys.platform, 'unix')]
    if 'all' in enforced_os:
      enforced_os = self.DEPS_OS_CHOICES.itervalues()
    self._enforced_os = tuple(set(enforced_os))
    self._enforced_cpu = detect_host_arch.HostArch(),
    self._root_dir = root_dir
    self._cipd_root = None
    self.config_content = None

  def _CheckConfig(self):
    """Verify that the config matches the state of the existing checked-out
    solutions."""
    for dep in self.dependencies:
      if dep.managed and dep.url:
        scm = dep.CreateSCM()
        actual_url = scm.GetActualRemoteURL(self._options)
        if actual_url and not scm.DoesRemoteURLMatch(self._options):
          mirror = scm.GetCacheMirror()
          if mirror:
            mirror_string = '%s (exists=%s)' % (mirror.mirror_path,
                                                mirror.exists())
          else:
            mirror_string = 'not used'
          raise gclient_utils.Error('''
Your .gclient file seems to be broken. The requested URL is different from what
is actually checked out in %(checkout_path)s.

The .gclient file contains:
URL: %(expected_url)s (%(expected_scm)s)
Cache mirror: %(mirror_string)s

The local checkout in %(checkout_path)s reports:
%(actual_url)s (%(actual_scm)s)

You should ensure that the URL listed in .gclient is correct and either change
it or fix the checkout.
'''  % {'checkout_path': os.path.join(self.root_dir, dep.name),
        'expected_url': dep.url,
        'expected_scm': dep.GetScmName(),
        'mirror_string' : mirror_string,
        'actual_url': actual_url,
        'actual_scm': dep.GetScmName()})

  def SetConfig(self, content):
    assert not self.dependencies
    config_dict = {}
    self.config_content = content
    try:
      exec(content, config_dict)
    except SyntaxError as e:
      gclient_utils.SyntaxErrorToError('.gclient', e)

    # Append any target OS that is not already being enforced to the tuple.
    target_os = config_dict.get('target_os', [])
    if config_dict.get('target_os_only', False):
      self._enforced_os = tuple(set(target_os))
    else:
      self._enforced_os = tuple(set(self._enforced_os).union(target_os))

    # Append any target CPU that is not already being enforced to the tuple.
    target_cpu = config_dict.get('target_cpu', [])
    if config_dict.get('target_cpu_only', False):
      self._enforced_cpu = tuple(set(target_cpu))
    else:
      self._enforced_cpu = tuple(set(self._enforced_cpu).union(target_cpu))

    cache_dir = config_dict.get('cache_dir', self._options.cache_dir)
    if cache_dir:
      cache_dir = os.path.join(self.root_dir, cache_dir)
      cache_dir = os.path.abspath(cache_dir)

    gclient_scm.GitWrapper.cache_dir = cache_dir
    git_cache.Mirror.SetCachePath(cache_dir)

    if not target_os and config_dict.get('target_os_only', False):
      raise gclient_utils.Error('Can\'t use target_os_only if target_os is '
                                'not specified')

    if not target_cpu and config_dict.get('target_cpu_only', False):
      raise gclient_utils.Error('Can\'t use target_cpu_only if target_cpu is '
                                'not specified')

    deps_to_add = []
    for s in config_dict.get('solutions', []):
      try:
        deps_to_add.append(GitDependency(
            self, s['name'], s['url'], s['url'],
            s.get('managed', True),
            s.get('custom_deps', {}),
            s.get('custom_vars', {}),
            s.get('custom_hooks', []),
            s.get('deps_file', 'DEPS'),
            True,
            None,
            None,
            True))
      except KeyError:
        raise gclient_utils.Error('Invalid .gclient file. Solution is '
                                  'incomplete: %s' % s)
    self.add_dependencies_and_close(deps_to_add, config_dict.get('hooks', []))
    logging.info('SetConfig() done')

  def SaveConfig(self):
    gclient_utils.FileWrite(os.path.join(self.root_dir,
                                         self._options.config_filename),
                            self.config_content)

  @staticmethod
  def LoadCurrentConfig(options):
    """Searches for and loads a .gclient file relative to the current working
    dir. Returns a GClient object."""
    if options.spec:
      client = GClient('.', options)
      client.SetConfig(options.spec)
    else:
      if options.verbose:
        print('Looking for %s starting from %s\n' % (
            options.config_filename, os.getcwd()))
      path = gclient_utils.FindGclientRoot(os.getcwd(), options.config_filename)
      if not path:
        if options.verbose:
          print('Couldn\'t find configuration file.')
        return None
      client = GClient(path, options)
      client.SetConfig(gclient_utils.FileRead(
          os.path.join(path, options.config_filename)))

    if (options.revisions and
        len(client.dependencies) > 1 and
        any('@' not in r for r in options.revisions)):
      print(
          ('You must specify the full solution name like --revision %s@%s\n'
           'when you have multiple solutions setup in your .gclient file.\n'
           'Other solutions present are: %s.') % (
              client.dependencies[0].name,
              options.revisions[0],
              ', '.join(s.name for s in client.dependencies[1:])),
          file=sys.stderr)
    return client

  def SetDefaultConfig(self, solution_name, deps_file, solution_url,
                       managed=True, cache_dir=None, custom_vars=None):
    self.SetConfig(self.DEFAULT_CLIENT_FILE_TEXT % {
      'solution_name': solution_name,
      'solution_url': solution_url,
      'deps_file': deps_file,
      'managed': managed,
      'cache_dir': cache_dir,
      'custom_vars': custom_vars or {},
    })

  def _SaveEntries(self):
    """Creates a .gclient_entries file to record the list of unique checkouts.

    The .gclient_entries file lives in the same directory as .gclient.
    """
    # Sometimes pprint.pformat will use {', sometimes it'll use { ' ... It
    # makes testing a bit too fun.
    result = 'entries = {\n'
    for entry in self.root.subtree(False):
      result += '  %s: %s,\n' % (pprint.pformat(entry.name),
          pprint.pformat(entry.url))
    result += '}\n'
    file_path = os.path.join(self.root_dir, self._options.entries_filename)
    logging.debug(result)
    gclient_utils.FileWrite(file_path, result)

  def _ReadEntries(self):
    """Read the .gclient_entries file for the given client.

    Returns:
      A sequence of solution names, which will be empty if there is the
      entries file hasn't been created yet.
    """
    scope = {}
    filename = os.path.join(self.root_dir, self._options.entries_filename)
    if not os.path.exists(filename):
      return {}
    try:
      exec(gclient_utils.FileRead(filename), scope)
    except SyntaxError as e:
      gclient_utils.SyntaxErrorToError(filename, e)
    return scope.get('entries', {})

  def _EnforceRevisions(self):
    """Checks for revision overrides."""
    revision_overrides = {}
    if self._options.head:
      return revision_overrides
    if not self._options.revisions:
      for s in self.dependencies:
        if not s.managed:
          self._options.revisions.append('%s@unmanaged' % s.name)
    if not self._options.revisions:
      return revision_overrides
    solutions_names = [s.name for s in self.dependencies]
    index = 0
    for revision in self._options.revisions:
      if not '@' in revision:
        # Support for --revision 123
        revision = '%s@%s' % (solutions_names[index], revision)
      name, rev = revision.split('@', 1)
      revision_overrides[name] = rev
      index += 1
    return revision_overrides

  def _EnforcePatchRefs(self):
    """Checks for patch refs."""
    patch_refs = {}
    if not self._options.patch_refs:
      return patch_refs
    for given_patch_ref in self._options.patch_refs:
      patch_repo, _, patch_ref = given_patch_ref.partition('@')
      if not patch_repo or not patch_ref:
        raise gclient_utils.Error(
            'Wrong revision format: %s should be of the form '
            'patch_repo@patch_ref.' % given_patch_ref)
      patch_refs[patch_repo] = patch_ref
    return patch_refs

  def RunOnDeps(self, command, args, ignore_requirements=False, progress=True):
    """Runs a command on each dependency in a client and its dependencies.

    Args:
      command: The command to use (e.g., 'status' or 'diff')
      args: list of str - extra arguments to add to the command line.
    """
    if not self.dependencies:
      raise gclient_utils.Error('No solution specified')

    revision_overrides = {}
    patch_refs = {}
    # It's unnecessary to check for revision overrides for 'recurse'.
    # Save a few seconds by not calling _EnforceRevisions() in that case.
    if command not in ('diff', 'recurse', 'runhooks', 'status', 'revert',
                       'validate'):
      self._CheckConfig()
      revision_overrides = self._EnforceRevisions()

    if command == 'update':
      patch_refs = self._EnforcePatchRefs()
    # Disable progress for non-tty stdout.
    should_show_progress = (
        setup_color.IS_TTY and not self._options.verbose and progress)
    pm = None
    if should_show_progress:
      if command in ('update', 'revert'):
        pm = Progress('Syncing projects', 1)
      elif command in ('recurse', 'validate'):
        pm = Progress(' '.join(args), 1)
    work_queue = gclient_utils.ExecutionQueue(
        self._options.jobs, pm, ignore_requirements=ignore_requirements,
        verbose=self._options.verbose)
    for s in self.dependencies:
      if s.should_process:
        work_queue.enqueue(s)
    work_queue.flush(revision_overrides, command, args, options=self._options,
                     patch_refs=patch_refs)

    if revision_overrides:
      print('Please fix your script, having invalid --revision flags will soon '
            'be considered an error.', file=sys.stderr)

    if patch_refs:
      raise gclient_utils.Error(
          'The following --patch-ref flags were not used. Please fix it:\n%s' %
          ('\n'.join(
              patch_repo + '@' + patch_ref
              for patch_repo, patch_ref in patch_refs.iteritems())))

    if self._cipd_root:
      self._cipd_root.run(command)

    # Once all the dependencies have been processed, it's now safe to write
    # out the gn_args_file and run the hooks.
    if command == 'update':
      gn_args_dep = self.dependencies[0]
      if gn_args_dep._gn_args_from:
        deps_map = dict([(dep.name, dep) for dep in gn_args_dep.dependencies])
        gn_args_dep = deps_map.get(gn_args_dep._gn_args_from)
      if gn_args_dep and gn_args_dep.HasGNArgsFile():
        gn_args_dep.WriteGNArgsFile()

    if not self._options.nohooks:
      if should_show_progress:
        pm = Progress('Running hooks', 1)
      self.RunHooksRecursively(self._options, pm)

    if command == 'update':
      # Notify the user if there is an orphaned entry in their working copy.
      # Only delete the directory if there are no changes in it, and
      # delete_unversioned_trees is set to true.
      entries = [i.name for i in self.root.subtree(False) if i.url]
      full_entries = [os.path.join(self.root_dir, e.replace('/', os.path.sep))
                      for e in entries]

      for entry, prev_url in self._ReadEntries().iteritems():
        if not prev_url:
          # entry must have been overridden via .gclient custom_deps
          continue
        # Fix path separator on Windows.
        entry_fixed = entry.replace('/', os.path.sep)
        e_dir = os.path.join(self.root_dir, entry_fixed)
        # Use entry and not entry_fixed there.
        if (entry not in entries and
            (not any(path.startswith(entry + '/') for path in entries)) and
            os.path.exists(e_dir)):
          # The entry has been removed from DEPS.
          scm = gclient_scm.GitWrapper(
              prev_url, self.root_dir, entry_fixed, self.outbuf)

          # Check to see if this directory is now part of a higher-up checkout.
          scm_root = None
          try:
            scm_root = gclient_scm.scm.GIT.GetCheckoutRoot(scm.checkout_path)
          except subprocess2.CalledProcessError:
            pass
          if not scm_root:
            logging.warning('Could not find checkout root for %s. Unable to '
                            'determine whether it is part of a higher-level '
                            'checkout, so not removing.' % entry)
            continue

          # This is to handle the case of third_party/WebKit migrating from
          # being a DEPS entry to being part of the main project.
          # If the subproject is a Git project, we need to remove its .git
          # folder. Otherwise git operations on that folder will have different
          # effects depending on the current working directory.
          if os.path.abspath(scm_root) == os.path.abspath(e_dir):
            e_par_dir = os.path.join(e_dir, os.pardir)
            if gclient_scm.scm.GIT.IsInsideWorkTree(e_par_dir):
              par_scm_root = gclient_scm.scm.GIT.GetCheckoutRoot(e_par_dir)
              # rel_e_dir : relative path of entry w.r.t. its parent repo.
              rel_e_dir = os.path.relpath(e_dir, par_scm_root)
              if gclient_scm.scm.GIT.IsDirectoryVersioned(
                  par_scm_root, rel_e_dir):
                save_dir = scm.GetGitBackupDirPath()
                # Remove any eventual stale backup dir for the same project.
                if os.path.exists(save_dir):
                  gclient_utils.rmtree(save_dir)
                os.rename(os.path.join(e_dir, '.git'), save_dir)
                # When switching between the two states (entry/ is a subproject
                # -> entry/ is part of the outer project), it is very likely
                # that some files are changed in the checkout, unless we are
                # jumping *exactly* across the commit which changed just DEPS.
                # In such case we want to cleanup any eventual stale files
                # (coming from the old subproject) in order to end up with a
                # clean checkout.
                gclient_scm.scm.GIT.CleanupDir(par_scm_root, rel_e_dir)
                assert not os.path.exists(os.path.join(e_dir, '.git'))
                print(('\nWARNING: \'%s\' has been moved from DEPS to a higher '
                       'level checkout. The git folder containing all the local'
                       ' branches has been saved to %s.\n'
                       'If you don\'t care about its state you can safely '
                       'remove that folder to free up space.') %
                      (entry, save_dir))
                continue

          if scm_root in full_entries:
            logging.info('%s is part of a higher level checkout, not removing',
                         scm.GetCheckoutRoot())
            continue

          file_list = []
          scm.status(self._options, [], file_list)
          modified_files = file_list != []
          if (not self._options.delete_unversioned_trees or
              (modified_files and not self._options.force)):
            # There are modified files in this entry. Keep warning until
            # removed.
            print(('\nWARNING: \'%s\' is no longer part of this client.  '
                   'It is recommended that you manually remove it.\n') %
                      entry_fixed)
          else:
            # Delete the entry
            print('\n________ deleting \'%s\' in \'%s\'' % (
                entry_fixed, self.root_dir))
            gclient_utils.rmtree(e_dir)
      # record the current list of entries for next time
      self._SaveEntries()
    return 0

  def PrintRevInfo(self):
    if not self.dependencies:
      raise gclient_utils.Error('No solution specified')
    # Load all the settings.
    work_queue = gclient_utils.ExecutionQueue(
        self._options.jobs, None, False, verbose=self._options.verbose)
    for s in self.dependencies:
      if s.should_process:
        work_queue.enqueue(s)
    work_queue.flush({}, None, [], options=self._options, patch_refs=None)

    def ShouldPrintRevision(dep):
      return (not self._options.filter
              or dep.FuzzyMatchUrl(self._options.filter))

    if self._options.snapshot:
      json_output = []
      # First level at .gclient
      for d in self.dependencies:
        entries = {}
        def GrabDeps(dep):
          """Recursively grab dependencies."""
          for d in dep.dependencies:
            d.PinToActualRevision()
            if ShouldPrintRevision(d):
              entries[d.name] = d.url
            GrabDeps(d)
        GrabDeps(d)
        json_output.append({
            'name': d.name,
            'solution_url': d.url,
            'deps_file': d.deps_file,
            'managed': d.managed,
            'custom_deps': entries,
        })
      if self._options.output_json == '-':
        print(json.dumps(json_output, indent=2, separators=(',', ': ')))
      elif self._options.output_json:
        with open(self._options.output_json, 'w') as f:
          json.dump(json_output, f)
      else:
        # Print the snapshot configuration file
        print(self.DEFAULT_SNAPSHOT_FILE_TEXT % {
            'solution_list': pprint.pformat(json_output, indent=2),
        })
    else:
      entries = {}
      for d in self.root.subtree(False):
        if self._options.actual:
          d.PinToActualRevision()
        if ShouldPrintRevision(d):
          entries[d.name] = d.url
      if self._options.output_json:
        json_output = {
            name: {
                'url': rev.split('@')[0] if rev else None,
                'rev': rev.split('@')[1] if rev and '@' in rev else None,
            }
            for name, rev in entries.iteritems()
        }
        if self._options.output_json == '-':
          print(json.dumps(json_output, indent=2, separators=(',', ': ')))
        else:
          with open(self._options.output_json, 'w') as f:
            json.dump(json_output, f)
      else:
        keys = sorted(entries.keys())
        for x in keys:
          print('%s: %s' % (x, entries[x]))
    logging.info(str(self))

  def ParseDepsFile(self, expand_vars=None):
    """No DEPS to parse for a .gclient file."""
    raise gclient_utils.Error('Internal error')

  def PrintLocationAndContents(self):
    # Print out the .gclient file.  This is longer than if we just printed the
    # client dict, but more legible, and it might contain helpful comments.
    print('Loaded .gclient config in %s:\n%s' % (
        self.root_dir, self.config_content))

  def GetCipdRoot(self):
    if not self._cipd_root:
      self._cipd_root = gclient_scm.CipdRoot(
          self.root_dir,
          # TODO(jbudorick): Support other service URLs as necessary.
          # Service URLs should be constant over the scope of a cipd
          # root, so a var per DEPS file specifying the service URL
          # should suffice.
          'https://chrome-infra-packages.appspot.com')
    return self._cipd_root

  @property
  def root_dir(self):
    """Root directory of gclient checkout."""
    return self._root_dir

  @property
  def enforced_os(self):
    """What deps_os entries that are to be parsed."""
    return self._enforced_os

  @property
  def recursion_limit(self):
    """How recursive can each dependencies in DEPS file can load DEPS file."""
    return self._recursion_limit

  @property
  def try_recursedeps(self):
    """Whether to attempt using recursedeps-style recursion processing."""
    return True

  @property
  def target_os(self):
    return self._enforced_os

  @property
  def target_cpu(self):
    return self._enforced_cpu


class CipdDependency(Dependency):
  """A Dependency object that represents a single CIPD package."""

  def __init__(
      self, parent, name, dep_value, cipd_root,
      custom_vars, should_process, relative, condition):
    package = dep_value['package']
    version = dep_value['version']
    url = urlparse.urljoin(
        cipd_root.service_url, '%s@%s' % (package, version))
    super(CipdDependency, self).__init__(
        parent, name + ':' + package, url, url, None, None, custom_vars,
        None, None, should_process, relative, condition)
    if relative:
      # TODO(jbudorick): Implement relative if necessary.
      raise gclient_utils.Error(
          'Relative CIPD dependencies are not currently supported.')
    self._cipd_package = None
    self._cipd_root = cipd_root
    self._cipd_subdir = os.path.relpath(
        os.path.join(self.root.root_dir, name), cipd_root.root_dir)
    self._package_name = package
    self._package_version = version

  #override
  def run(self, revision_overrides, command, args, work_queue, options,
          patch_refs):
    """Runs |command| then parse the DEPS file."""
    logging.info('CipdDependency(%s).run()' % self.name)
    if not self.should_process:
      return
    self._CreatePackageIfNecessary()
    super(CipdDependency, self).run(revision_overrides, command, args,
                                    work_queue, options, patch_refs)

  def _CreatePackageIfNecessary(self):
    # We lazily create the CIPD package to make sure that only packages
    # that we want (as opposed to all packages defined in all DEPS files
    # we parse) get added to the root and subsequently ensured.
    if not self._cipd_package:
      self._cipd_package = self._cipd_root.add_package(
          self._cipd_subdir, self._package_name, self._package_version)

  def ParseDepsFile(self, expand_vars=None):
    """CIPD dependencies are not currently allowed to have nested deps."""
    self.add_dependencies_and_close([], [])

  #override
  def verify_validity(self):
    """CIPD dependencies allow duplicate name for packages in same directory."""
    logging.info('Dependency(%s).verify_validity()' % self.name)
    return True

  #override
  def GetScmName(self):
    """Always 'cipd'."""
    return 'cipd'

  #override
  def CreateSCM(self, out_cb=None):
    """Create a Wrapper instance suitable for handling this CIPD dependency."""
    self._CreatePackageIfNecessary()
    return gclient_scm.CipdWrapper(
        self.url, self.root.root_dir, self.name, self.outbuf, out_cb,
        root=self._cipd_root, package=self._cipd_package)

  def ToLines(self):
    """Return a list of lines representing this in a DEPS file."""
    s = []
    self._CreatePackageIfNecessary()
    if self._cipd_package.authority_for_subdir:
      condition_part = (['    "condition": %r,' % self.condition]
                        if self.condition else [])
      s.extend([
          '  # %s' % self.hierarchy(include_url=False),
          '  "%s": {' % (self.name.split(':')[0],),
          '    "packages": [',
      ])
      for p in self._cipd_root.packages(self._cipd_subdir):
        s.extend([
            '      {',
            '        "package": "%s",' % p.name,
            '        "version": "%s",' % p.version,
            '      },',
        ])

      s.extend([
          '    ],',
          '    "dep_type": "cipd",',
      ] + condition_part + [
          '  },',
          '',
      ])
    return s


#### gclient commands.


@subcommand.usage('[command] [args ...]')
def CMDrecurse(parser, args):
  """Operates [command args ...] on all the dependencies.

  Runs a shell command on all entries.
  Sets GCLIENT_DEP_PATH environment variable as the dep's relative location to
  root directory of the checkout.
  """
  # Stop parsing at the first non-arg so that these go through to the command
  parser.disable_interspersed_args()
  parser.add_option('-s', '--scm', action='append', default=[],
                    help='Choose scm types to operate upon.')
  parser.add_option('-i', '--ignore', action='store_true',
                    help='Ignore non-zero return codes from subcommands.')
  parser.add_option('--prepend-dir', action='store_true',
                    help='Prepend relative dir for use with git <cmd> --null.')
  parser.add_option('--no-progress', action='store_true',
                    help='Disable progress bar that shows sub-command updates')
  options, args = parser.parse_args(args)
  if not args:
    print('Need to supply a command!', file=sys.stderr)
    return 1
  root_and_entries = gclient_utils.GetGClientRootAndEntries()
  if not root_and_entries:
    print(
        'You need to run gclient sync at least once to use \'recurse\'.\n'
        'This is because .gclient_entries needs to exist and be up to date.',
        file=sys.stderr)
    return 1

  # Normalize options.scm to a set()
  scm_set = set()
  for scm in options.scm:
    scm_set.update(scm.split(','))
  options.scm = scm_set

  options.nohooks = True
  client = GClient.LoadCurrentConfig(options)
  if not client:
    raise gclient_utils.Error('client not configured; see \'gclient config\'')
  return client.RunOnDeps('recurse', args, ignore_requirements=True,
                          progress=not options.no_progress)


@subcommand.usage('[args ...]')
def CMDfetch(parser, args):
  """Fetches upstream commits for all modules.

  Completely git-specific. Simply runs 'git fetch [args ...]' for each module.
  """
  (options, args) = parser.parse_args(args)
  return CMDrecurse(OptionParser(), [
      '--jobs=%d' % options.jobs, '--scm=git', 'git', 'fetch'] + args)


class Flattener(object):
  """Flattens a gclient solution."""

  def __init__(self, client, pin_all_deps=False):
    """Constructor.

    Arguments:
      client (GClient): client to flatten
      pin_all_deps (bool): whether to pin all deps, even if they're not pinned
          in DEPS
    """
    self._client = client

    self._deps_string = None
    self._deps_files = set()

    self._allowed_hosts = set()
    self._deps = {}
    self._hooks = []
    self._pre_deps_hooks = []
    self._vars = {}

    self._flatten(pin_all_deps=pin_all_deps)

  @property
  def deps_string(self):
    assert self._deps_string is not None
    return self._deps_string

  @property
  def deps_files(self):
    return self._deps_files

  def _pin_dep(self, dep):
    """Pins a dependency to specific full revision sha.

    Arguments:
      dep (Dependency): dependency to process
    """
    if dep.url is None:
      return

    # Make sure the revision is always fully specified (a hash),
    # as opposed to refs or tags which might change. Similarly,
    # shortened shas might become ambiguous; make sure to always
    # use full one for pinning.
    revision = gclient_utils.SplitUrlRevision(dep.url)[1]
    if not revision or not gclient_utils.IsFullGitSha(revision):
      dep.PinToActualRevision()

  def _flatten(self, pin_all_deps=False):
    """Runs the flattener. Saves resulting DEPS string.

    Arguments:
      pin_all_deps (bool): whether to pin all deps, even if they're not pinned
          in DEPS
    """
    for solution in self._client.dependencies:
      self._add_dep(solution)
      self._flatten_dep(solution)

    if pin_all_deps:
      for dep in self._deps.itervalues():
        self._pin_dep(dep)

    def add_deps_file(dep):
      # Only include DEPS files referenced by recursedeps.
      if not (dep.parent is None or
              (dep.name in (dep.parent.recursedeps or {}))):
        return
      deps_file = dep.deps_file
      deps_path = os.path.join(self._client.root_dir, dep.name, deps_file)
      if not os.path.exists(deps_path):
        # gclient has a fallback that if deps_file doesn't exist, it'll try
        # DEPS. Do the same here.
        deps_file = 'DEPS'
        deps_path = os.path.join(self._client.root_dir, dep.name, deps_file)
        if not os.path.exists(deps_path):
          return
      assert dep.url
      self._deps_files.add((dep.url, deps_file, dep.hierarchy_data()))
    for dep in self._deps.itervalues():
      add_deps_file(dep)

    gn_args_dep = self._deps.get(self._client.dependencies[0]._gn_args_from,
                                 self._client.dependencies[0])
    self._deps_string = '\n'.join(
        _GNSettingsToLines(gn_args_dep._gn_args_file, gn_args_dep._gn_args) +
        _AllowedHostsToLines(self._allowed_hosts) +
        _DepsToLines(self._deps) +
        _HooksToLines('hooks', self._hooks) +
        _HooksToLines('pre_deps_hooks', self._pre_deps_hooks) +
        _VarsToLines(self._vars) +
        ['# %s, %s' % (url, deps_file)
         for url, deps_file, _ in sorted(self._deps_files)] +
        [''])  # Ensure newline at end of file.

  def _add_dep(self, dep):
    """Helper to add a dependency to flattened DEPS.

    Arguments:
      dep (Dependency): dependency to add
    """
    assert dep.name not in self._deps or self._deps.get(dep.name) == dep, (
        dep.name, self._deps.get(dep.name))
    if dep.url:
      self._deps[dep.name] = dep

  def _flatten_dep(self, dep):
    """Visits a dependency in order to flatten it (see CMDflatten).

    Arguments:
      dep (Dependency): dependency to process
    """
    logging.debug('_flatten_dep(%s)', dep.name)

    assert dep.deps_parsed, (
        "Attempted to flatten %s but it has not been processed." % dep.name)

    self._allowed_hosts.update(dep.allowed_hosts)

    # Only include vars explicitly listed in the DEPS files or gclient solution,
    # not automatic, local overrides (i.e. not all of dep.get_vars()).
    hierarchy = dep.hierarchy(include_url=False)
    for key, value in dep._vars.iteritems():
      # Make sure there are no conflicting variables. It is fine however
      # to use same variable name, as long as the value is consistent.
      assert key not in self._vars or self._vars[key][1] == value
      self._vars[key] = (hierarchy, value)
    # Override explicit custom variables.
    for key, value in dep.custom_vars.iteritems():
      # Do custom_vars that don't correspond to DEPS vars ever make sense? DEPS
      # conditionals shouldn't be using vars that aren't also defined in the
      # DEPS (presubmit actually disallows this), so any new custom_var must be
      # unused in the DEPS, so no need to add it to the flattened output either.
      if key not in self._vars:
        continue
      # Don't "override" existing vars if it's actually the same value.
      elif self._vars[key][1] == value:
        continue
      # Anything else is overriding a default value from the DEPS.
      self._vars[key] = (hierarchy + ' [custom_var override]', value)

    self._pre_deps_hooks.extend([(dep, hook) for hook in dep.pre_deps_hooks])
    self._hooks.extend([(dep, hook) for hook in dep.deps_hooks])

    for sub_dep in dep.dependencies:
      self._add_dep(sub_dep)

    deps_by_name = {d.name: d for d in dep.dependencies}
    for recurse_dep_name in (dep.recursedeps or []):
      self._flatten_dep(deps_by_name[recurse_dep_name])


def CMDflatten(parser, args):
  """Flattens the solutions into a single DEPS file."""
  parser.add_option('--output-deps', help='Path to the output DEPS file')
  parser.add_option(
      '--output-deps-files',
      help=('Path to the output metadata about DEPS files referenced by '
            'recursedeps.'))
  parser.add_option(
      '--pin-all-deps', action='store_true',
      help=('Pin all deps, even if not pinned in DEPS. CAVEAT: only does so '
            'for checked out deps, NOT deps_os.'))
  options, args = parser.parse_args(args)

  options.nohooks = True
  options.process_all_deps = True
  client = GClient.LoadCurrentConfig(options)

  # Only print progress if we're writing to a file. Otherwise, progress updates
  # could obscure intended output.
  code = client.RunOnDeps('flatten', args, progress=options.output_deps)
  if code != 0:
    return code

  flattener = Flattener(client, pin_all_deps=options.pin_all_deps)

  if options.output_deps:
    with open(options.output_deps, 'w') as f:
      f.write(flattener.deps_string)
  else:
    print(flattener.deps_string)

  deps_files = [{'url': d[0], 'deps_file': d[1], 'hierarchy': d[2]}
                for d in sorted(flattener.deps_files)]
  if options.output_deps_files:
    with open(options.output_deps_files, 'w') as f:
      json.dump(deps_files, f)

  return 0


def _GNSettingsToLines(gn_args_file, gn_args):
  s = []
  if gn_args_file:
    s.extend([
        'gclient_gn_args_file = "%s"' % gn_args_file,
        'gclient_gn_args = %r' % gn_args,
    ])
  return s


def _AllowedHostsToLines(allowed_hosts):
  """Converts |allowed_hosts| set to list of lines for output."""
  if not allowed_hosts:
    return []
  s = ['allowed_hosts = [']
  for h in sorted(allowed_hosts):
    s.append('  "%s",' % h)
  s.extend([']', ''])
  return s


def _DepsToLines(deps):
  """Converts |deps| dict to list of lines for output."""
  if not deps:
    return []
  s = ['deps = {']
  for _, dep in sorted(deps.iteritems()):
    s.extend(dep.ToLines())
  s.extend(['}', ''])
  return s


def _DepsOsToLines(deps_os):
  """Converts |deps_os| dict to list of lines for output."""
  if not deps_os:
    return []
  s = ['deps_os = {']
  for dep_os, os_deps in sorted(deps_os.iteritems()):
    s.append('  "%s": {' % dep_os)
    for name, dep in sorted(os_deps.iteritems()):
      condition_part = (['      "condition": %r,' % dep.condition]
                        if dep.condition else [])
      s.extend([
          '    # %s' % dep.hierarchy(include_url=False),
          '    "%s": {' % (name,),
          '      "url": "%s",' % (dep.raw_url,),
      ] + condition_part + [
          '    },',
          '',
      ])
    s.extend(['  },', ''])
  s.extend(['}', ''])
  return s


def _HooksToLines(name, hooks):
  """Converts |hooks| list to list of lines for output."""
  if not hooks:
    return []
  s = ['%s = [' % name]
  for dep, hook in hooks:
    s.extend([
        '  # %s' % dep.hierarchy(include_url=False),
        '  {',
    ])
    if hook.name is not None:
      s.append('    "name": "%s",' % hook.name)
    if hook.pattern is not None:
      s.append('    "pattern": "%s",' % hook.pattern)
    if hook.condition is not None:
      s.append('    "condition": %r,' % hook.condition)
    s.extend(
        # Hooks run in the parent directory of their dep.
        ['    "cwd": "%s",' % os.path.normpath(os.path.dirname(dep.name))] +
        ['    "action": ['] +
        ['        "%s",' % arg for arg in hook.action] +
        ['    ]', '  },', '']
    )
  s.extend([']', ''])
  return s


def _HooksOsToLines(hooks_os):
  """Converts |hooks| list to list of lines for output."""
  if not hooks_os:
    return []
  s = ['hooks_os = {']
  for hook_os, os_hooks in hooks_os.iteritems():
    s.append('  "%s": [' % hook_os)
    for dep, hook in os_hooks:
      s.extend([
          '    # %s' % dep.hierarchy(include_url=False),
          '    {',
      ])
      if hook.name is not None:
        s.append('      "name": "%s",' % hook.name)
      if hook.pattern is not None:
        s.append('      "pattern": "%s",' % hook.pattern)
      if hook.condition is not None:
        s.append('    "condition": %r,' % hook.condition)
      s.extend(
          # Hooks run in the parent directory of their dep.
          ['      "cwd": "%s",' % os.path.normpath(os.path.dirname(dep.name))] +
          ['      "action": ['] +
          ['          "%s",' % arg for arg in hook.action] +
          ['      ]', '    },', '']
      )
    s.extend(['  ],', ''])
  s.extend(['}', ''])
  return s


def _VarsToLines(variables):
  """Converts |variables| dict to list of lines for output."""
  if not variables:
    return []
  s = ['vars = {']
  for key, tup in sorted(variables.iteritems()):
    hierarchy, value = tup
    s.extend([
        '  # %s' % hierarchy,
        '  "%s": %r,' % (key, value),
        '',
    ])
  s.extend(['}', ''])
  return s


def CMDgrep(parser, args):
  """Greps through git repos managed by gclient.

  Runs 'git grep [args...]' for each module.
  """
  # We can't use optparse because it will try to parse arguments sent
  # to git grep and throw an error. :-(
  if not args or re.match('(-h|--help)$', args[0]):
    print(
        'Usage: gclient grep [-j <N>] git-grep-args...\n\n'
        'Example: "gclient grep -j10 -A2 RefCountedBase" runs\n"git grep '
        '-A2 RefCountedBase" on each of gclient\'s git\nrepos with up to '
        '10 jobs.\n\nBonus: page output by appending "|& less -FRSX" to the'
        ' end of your query.',
        file=sys.stderr)
    return 1

  jobs_arg = ['--jobs=1']
  if re.match(r'(-j|--jobs=)\d+$', args[0]):
    jobs_arg, args = args[:1], args[1:]
  elif re.match(r'(-j|--jobs)$', args[0]):
    jobs_arg, args = args[:2], args[2:]

  return CMDrecurse(
      parser,
      jobs_arg + ['--ignore', '--prepend-dir', '--no-progress', '--scm=git',
                  'git', 'grep', '--null', '--color=Always'] + args)


def CMDroot(parser, args):
  """Outputs the solution root (or current dir if there isn't one)."""
  (options, args) = parser.parse_args(args)
  client = GClient.LoadCurrentConfig(options)
  if client:
    print(os.path.abspath(client.root_dir))
  else:
    print(os.path.abspath('.'))


@subcommand.usage('[url]')
def CMDconfig(parser, args):
  """Creates a .gclient file in the current directory.

  This specifies the configuration for further commands. After update/sync,
  top-level DEPS files in each module are read to determine dependent
  modules to operate on as well. If optional [url] parameter is
  provided, then configuration is read from a specified Subversion server
  URL.
  """
  # We do a little dance with the --gclientfile option.  'gclient config' is the
  # only command where it's acceptable to have both '--gclientfile' and '--spec'
  # arguments.  So, we temporarily stash any --gclientfile parameter into
  # options.output_config_file until after the (gclientfile xor spec) error
  # check.
  parser.remove_option('--gclientfile')
  parser.add_option('--gclientfile', dest='output_config_file',
                    help='Specify an alternate .gclient file')
  parser.add_option('--name',
                    help='overrides the default name for the solution')
  parser.add_option('--deps-file', default='DEPS',
                    help='overrides the default name for the DEPS file for the '
                         'main solutions and all sub-dependencies')
  parser.add_option('--unmanaged', action='store_true', default=False,
                    help='overrides the default behavior to make it possible '
                         'to have the main solution untouched by gclient '
                         '(gclient will check out unmanaged dependencies but '
                         'will never sync them)')
  parser.add_option('--custom-var', action='append', dest='custom_vars',
                    default=[],
                    help='overrides variables; key=value syntax')
  parser.set_defaults(config_filename=None)
  (options, args) = parser.parse_args(args)
  if options.output_config_file:
    setattr(options, 'config_filename', getattr(options, 'output_config_file'))
  if ((options.spec and args) or len(args) > 2 or
      (not options.spec and not args)):
    parser.error('Inconsistent arguments. Use either --spec or one or 2 args')

  custom_vars = {}
  for arg in options.custom_vars:
    kv = arg.split('=', 1)
    if len(kv) != 2:
      parser.error('Invalid --custom-var argument: %r' % arg)
    custom_vars[kv[0]] = gclient_eval.EvaluateCondition(kv[1], {})

  client = GClient('.', options)
  if options.spec:
    client.SetConfig(options.spec)
  else:
    base_url = args[0].rstrip('/')
    if not options.name:
      name = base_url.split('/')[-1]
      if name.endswith('.git'):
        name = name[:-4]
    else:
      # specify an alternate relpath for the given URL.
      name = options.name
      if not os.path.abspath(os.path.join(os.getcwd(), name)).startswith(
          os.getcwd()):
        parser.error('Do not pass a relative path for --name.')
      if any(x in ('..', '.', '/', '\\') for x in name.split(os.sep)):
        parser.error('Do not include relative path components in --name.')

    deps_file = options.deps_file
    client.SetDefaultConfig(name, deps_file, base_url,
                            managed=not options.unmanaged,
                            cache_dir=options.cache_dir,
                            custom_vars=custom_vars)
  client.SaveConfig()
  return 0


@subcommand.epilog("""Example:
  gclient pack > patch.txt
    generate simple patch for configured client and dependences
""")
def CMDpack(parser, args):
  """Generates a patch which can be applied at the root of the tree.

  Internally, runs 'git diff' on each checked out module and
  dependencies, and performs minimal postprocessing of the output. The
  resulting patch is printed to stdout and can be applied to a freshly
  checked out tree via 'patch -p0 < patchfile'.
  """
  parser.add_option('--deps', dest='deps_os', metavar='OS_LIST',
                    help='override deps for the specified (comma-separated) '
                         'platform(s); \'all\' will process all deps_os '
                         'references')
  parser.remove_option('--jobs')
  (options, args) = parser.parse_args(args)
  # Force jobs to 1 so the stdout is not annotated with the thread ids
  options.jobs = 1
  client = GClient.LoadCurrentConfig(options)
  if not client:
    raise gclient_utils.Error('client not configured; see \'gclient config\'')
  if options.verbose:
    client.PrintLocationAndContents()
  return client.RunOnDeps('pack', args)


def CMDstatus(parser, args):
  """Shows modification status for every dependencies."""
  parser.add_option('--deps', dest='deps_os', metavar='OS_LIST',
                    help='override deps for the specified (comma-separated) '
                         'platform(s); \'all\' will process all deps_os '
                         'references')
  (options, args) = parser.parse_args(args)
  client = GClient.LoadCurrentConfig(options)
  if not client:
    raise gclient_utils.Error('client not configured; see \'gclient config\'')
  if options.verbose:
    client.PrintLocationAndContents()
  return client.RunOnDeps('status', args)


@subcommand.epilog("""Examples:
  gclient sync
      update files from SCM according to current configuration,
      *for modules which have changed since last update or sync*
  gclient sync --force
      update files from SCM according to current configuration, for
      all modules (useful for recovering files deleted from local copy)
  gclient sync --revision src@31000
      update src directory to r31000

JSON output format:
If the --output-json option is specified, the following document structure will
be emitted to the provided file. 'null' entries may occur for subprojects which
are present in the gclient solution, but were not processed (due to custom_deps,
os_deps, etc.)

{
  "solutions" : {
    "<name>": {  # <name> is the posix-normalized path to the solution.
      "revision": [<git id hex string>|null],
      "scm": ["git"|null],
    }
  }
}
""")
def CMDsync(parser, args):
  """Checkout/update all modules."""
  parser.add_option('-f', '--force', action='store_true',
                    help='force update even for unchanged modules')
  parser.add_option('-n', '--nohooks', action='store_true',
                    help='don\'t run hooks after the update is complete')
  parser.add_option('-p', '--noprehooks', action='store_true',
                    help='don\'t run pre-DEPS hooks', default=False)
  parser.add_option('-r', '--revision', action='append',
                    dest='revisions', metavar='REV', default=[],
                    help='Enforces revision/hash for the solutions with the '
                         'format src@rev. The src@ part is optional and can be '
                         'skipped. You can also specify URLs instead of paths '
                         'and gclient will find the solution corresponding to '
                         'the given URL. If a path is also specified, the URL '
                         'takes precedence. -r can be used multiple times when '
                         '.gclient has multiple solutions configured, and will '
                         'work even if the src@ part is skipped.')
  parser.add_option('--patch-ref', action='append',
                    dest='patch_refs', metavar='GERRIT_REF', default=[],
                    help='Patches the given reference with the format dep@ref. '
                         'For dep, you can specify URLs as well as paths, with '
                         'URLs taking preference. The reference will be '
                         'applied to the necessary path, will be rebased on '
                         'top what the dep was synced to, and then will do a '
                         'soft reset. Use --no-rebase-patch-ref and '
                         '--reset-patch-ref to disable this behavior.')
  parser.add_option('--with_branch_heads', action='store_true',
                    help='Clone git "branch_heads" refspecs in addition to '
                         'the default refspecs. This adds about 1/2GB to a '
                         'full checkout. (git only)')
  parser.add_option('--with_tags', action='store_true',
                    help='Clone git tags in addition to the default refspecs.')
  parser.add_option('-H', '--head', action='store_true',
                    help='DEPRECATED: only made sense with safesync urls.')
  parser.add_option('-D', '--delete_unversioned_trees', action='store_true',
                    help='Deletes from the working copy any dependencies that '
                         'have been removed since the last sync, as long as '
                         'there are no local modifications. When used with '
                         '--force, such dependencies are removed even if they '
                         'have local modifications. When used with --reset, '
                         'all untracked directories are removed from the '
                         'working copy, excluding those which are explicitly '
                         'ignored in the repository.')
  parser.add_option('-R', '--reset', action='store_true',
                    help='resets any local changes before updating (git only)')
  parser.add_option('-M', '--merge', action='store_true',
                    help='merge upstream changes instead of trying to '
                         'fast-forward or rebase')
  parser.add_option('-A', '--auto_rebase', action='store_true',
                    help='Automatically rebase repositories against local '
                         'checkout during update (git only).')
  parser.add_option('--deps', dest='deps_os', metavar='OS_LIST',
                    help='override deps for the specified (comma-separated) '
                         'platform(s); \'all\' will process all deps_os '
                         'references')
  parser.add_option('--process-all-deps', action='store_true',
                    help='Check out all deps, even for different OS-es, '
                         'or with conditions evaluating to false')
  parser.add_option('--upstream', action='store_true',
                    help='Make repo state match upstream branch.')
  parser.add_option('--output-json',
                    help='Output a json document to this path containing '
                         'summary information about the sync.')
  parser.add_option('--no-history', action='store_true',
                    help='GIT ONLY - Reduces the size/time of the checkout at '
                    'the cost of no history. Requires Git 1.9+')
  parser.add_option('--shallow', action='store_true',
                    help='GIT ONLY - Do a shallow clone into the cache dir. '
                         'Requires Git 1.9+')
  parser.add_option('--no_bootstrap', '--no-bootstrap',
                    action='store_true',
                    help='Don\'t bootstrap from Google Storage.')
  parser.add_option('--ignore_locks', action='store_true',
                    help='GIT ONLY - Ignore cache locks.')
  parser.add_option('--break_repo_locks', action='store_true',
                    help='GIT ONLY - Forcibly remove repo locks (e.g. '
                      'index.lock). This should only be used if you know for '
                      'certain that this invocation of gclient is the only '
                      'thing operating on the git repos (e.g. on a bot).')
  parser.add_option('--lock_timeout', type='int', default=5000,
                    help='GIT ONLY - Deadline (in seconds) to wait for git '
                         'cache lock to become available. Default is %default.')
  # TODO(agable): Remove these when the oldest CrOS release milestone is M56.
  parser.add_option('-t', '--transitive', action='store_true',
                    help='DEPRECATED: This is a no-op.')
  parser.add_option('-m', '--manually_grab_svn_rev', action='store_true',
                    help='DEPRECATED: This is a no-op.')
  # TODO(phajdan.jr): Remove validation options once default (crbug/570091).
  parser.add_option('--validate-syntax', action='store_true', default=True,
                    help='Validate the .gclient and DEPS syntax')
  parser.add_option('--disable-syntax-validation', action='store_false',
                    dest='validate_syntax',
                    help='Disable validation of .gclient and DEPS syntax.')
  parser.add_option('--no-rebase-patch-ref', action='store_false',
                    dest='rebase_patch_ref', default=True,
                    help='Bypass rebase of the patch ref after checkout.')
  parser.add_option('--no-reset-patch-ref', action='store_false',
                    dest='reset_patch_ref', default=True,
                    help='Bypass calling reset after patching the ref.')
  (options, args) = parser.parse_args(args)
  client = GClient.LoadCurrentConfig(options)

  if not client:
    raise gclient_utils.Error('client not configured; see \'gclient config\'')

  if options.revisions and options.head:
    # TODO(maruel): Make it a parser.error if it doesn't break any builder.
    print('Warning: you cannot use both --head and --revision')

  if options.verbose:
    client.PrintLocationAndContents()
  ret = client.RunOnDeps('update', args)
  if options.output_json:
    slns = {}
    for d in client.subtree(True):
      normed = d.name.replace('\\', '/').rstrip('/') + '/'
      if normed in slns and not d.should_process:
        # If an unprocessed dependency would override an existing dependency,
        # ignore it.
        continue
      slns[normed] = {
          'revision': d.got_revision,
          'scm': d.used_scm.name if d.used_scm else None,
          'url': str(d.url) if d.url else None,
      }
    with open(options.output_json, 'wb') as f:
      json.dump({'solutions': slns}, f)
  return ret


CMDupdate = CMDsync


def CMDvalidate(parser, args):
  """Validates the .gclient and DEPS syntax."""
  options, args = parser.parse_args(args)
  options.validate_syntax = True
  client = GClient.LoadCurrentConfig(options)
  rv = client.RunOnDeps('validate', args)
  if rv == 0:
    print('validate: SUCCESS')
  else:
    print('validate: FAILURE')
  return rv


def CMDdiff(parser, args):
  """Displays local diff for every dependencies."""
  parser.add_option('--deps', dest='deps_os', metavar='OS_LIST',
                    help='override deps for the specified (comma-separated) '
                         'platform(s); \'all\' will process all deps_os '
                         'references')
  (options, args) = parser.parse_args(args)
  client = GClient.LoadCurrentConfig(options)
  if not client:
    raise gclient_utils.Error('client not configured; see \'gclient config\'')
  if options.verbose:
    client.PrintLocationAndContents()
  return client.RunOnDeps('diff', args)


def CMDrevert(parser, args):
  """Reverts all modifications in every dependencies.

  That's the nuclear option to get back to a 'clean' state. It removes anything
  that shows up in git status."""
  parser.add_option('--deps', dest='deps_os', metavar='OS_LIST',
                    help='override deps for the specified (comma-separated) '
                         'platform(s); \'all\' will process all deps_os '
                         'references')
  parser.add_option('-n', '--nohooks', action='store_true',
                    help='don\'t run hooks after the revert is complete')
  parser.add_option('-p', '--noprehooks', action='store_true',
                    help='don\'t run pre-DEPS hooks', default=False)
  parser.add_option('--upstream', action='store_true',
                    help='Make repo state match upstream branch.')
  parser.add_option('--break_repo_locks', action='store_true',
                    help='GIT ONLY - Forcibly remove repo locks (e.g. '
                      'index.lock). This should only be used if you know for '
                      'certain that this invocation of gclient is the only '
                      'thing operating on the git repos (e.g. on a bot).')
  (options, args) = parser.parse_args(args)
  # --force is implied.
  options.force = True
  options.reset = False
  options.delete_unversioned_trees = False
  options.merge = False
  client = GClient.LoadCurrentConfig(options)
  if not client:
    raise gclient_utils.Error('client not configured; see \'gclient config\'')
  return client.RunOnDeps('revert', args)


def CMDrunhooks(parser, args):
  """Runs hooks for files that have been modified in the local working copy."""
  parser.add_option('--deps', dest='deps_os', metavar='OS_LIST',
                    help='override deps for the specified (comma-separated) '
                         'platform(s); \'all\' will process all deps_os '
                         'references')
  parser.add_option('-f', '--force', action='store_true', default=True,
                    help='Deprecated. No effect.')
  (options, args) = parser.parse_args(args)
  client = GClient.LoadCurrentConfig(options)
  if not client:
    raise gclient_utils.Error('client not configured; see \'gclient config\'')
  if options.verbose:
    client.PrintLocationAndContents()
  options.force = True
  options.nohooks = False
  return client.RunOnDeps('runhooks', args)


def CMDrevinfo(parser, args):
  """Outputs revision info mapping for the client and its dependencies.

  This allows the capture of an overall 'revision' for the source tree that
  can be used to reproduce the same tree in the future. It is only useful for
  'unpinned dependencies', i.e. DEPS/deps references without a git hash.
  A git branch name isn't 'pinned' since the actual commit can change.
  """
  parser.add_option('--deps', dest='deps_os', metavar='OS_LIST',
                    help='override deps for the specified (comma-separated) '
                         'platform(s); \'all\' will process all deps_os '
                         'references')
  parser.add_option('-a', '--actual', action='store_true',
                    help='gets the actual checked out revisions instead of the '
                         'ones specified in the DEPS and .gclient files')
  parser.add_option('-s', '--snapshot', action='store_true',
                    help='creates a snapshot .gclient file of the current '
                         'version of all repositories to reproduce the tree, '
                         'implies -a')
  parser.add_option('--filter', action='append', dest='filter',
                     help='Display revision information only for the specified '
                          'dependencies (filtered by URL or path).')
  parser.add_option('--output-json',
                    help='Output a json document to this path containing '
                         'information about the revisions.')
  (options, args) = parser.parse_args(args)
  client = GClient.LoadCurrentConfig(options)
  if not client:
    raise gclient_utils.Error('client not configured; see \'gclient config\'')
  client.PrintRevInfo()
  return 0


def CMDgetdep(parser, args):
  """Gets revision information and variable values from a DEPS file."""
  parser.add_option('--var', action='append',
                    dest='vars', metavar='VAR', default=[],
                    help='Gets the value of a given variable.')
  parser.add_option('-r', '--revision', action='append',
                    dest='revisions', metavar='DEP', default=[],
                    help='Gets the revision/version for the given dependency. '
                         'If it is a git dependency, dep must be a path. If it '
                         'is a CIPD dependency, dep must be of the form '
                         'path:package.')
  parser.add_option('--deps-file', default='DEPS',
                    # TODO(ehmaldonado): Try to find the DEPS file pointed by
                    # .gclient first.
                    help='The DEPS file to be edited. Defaults to the DEPS '
                         'file in the current directory.')
  (options, args) = parser.parse_args(args)

  if not os.path.isfile(options.deps_file):
    raise gclient_utils.Error(
        'DEPS file %s does not exist.' % options.deps_file)
  with open(options.deps_file) as f:
    contents = f.read()
  local_scope = gclient_eval.Exec(
      contents, expand_vars=True, filename=options.deps_file)

  for var in options.vars:
    print(gclient_eval.GetVar(local_scope, var))

  for name in options.revisions:
    if ':' in name:
      name, _, package = name.partition(':')
      if not name or not package:
        parser.error(
            'Wrong CIPD format: %s:%s should be of the form path:pkg.'
            % (name, package))
      print(gclient_eval.GetCIPD(local_scope, name, package))
    else:
      print(gclient_eval.GetRevision(local_scope, name))


def CMDsetdep(parser, args):
  """Modifies dependency revisions and variable values in a DEPS file"""
  parser.add_option('--var', action='append',
                    dest='vars', metavar='VAR=VAL', default=[],
                    help='Sets a variable to the given value with the format '
                         'name=value.')
  parser.add_option('-r', '--revision', action='append',
                    dest='revisions', metavar='DEP@REV', default=[],
                    help='Sets the revision/version for the dependency with '
                         'the format dep@rev. If it is a git dependency, dep '
                         'must be a path and rev must be a git hash or '
                         'reference (e.g. src/dep@deadbeef). If it is a CIPD '
                         'dependency, dep must be of the form path:package and '
                         'rev must be the package version '
                         '(e.g. src/pkg:chromium/pkg@2.1-cr0).')
  parser.add_option('--deps-file', default='DEPS',
                    # TODO(ehmaldonado): Try to find the DEPS file pointed by
                    # .gclient first.
                    help='The DEPS file to be edited. Defaults to the DEPS '
                         'file in the current directory.')
  (options, args) = parser.parse_args(args)
  if args:
    parser.error('Unused arguments: "%s"' % '" "'.join(args))
  if not options.revisions and not options.vars:
    parser.error(
        'You must specify at least one variable or revision to modify.')

  if not os.path.isfile(options.deps_file):
    raise gclient_utils.Error(
        'DEPS file %s does not exist.' % options.deps_file)
  with open(options.deps_file) as f:
    contents = f.read()
  local_scope = gclient_eval.Exec(
      contents, expand_vars=True, filename=options.deps_file)

  for var in options.vars:
    name, _, value = var.partition('=')
    if not name or not value:
      parser.error(
          'Wrong var format: %s should be of the form name=value.' % var)
    if name in local_scope['vars']:
      gclient_eval.SetVar(local_scope, name, value)
    else:
      gclient_eval.AddVar(local_scope, name, value)

  for revision in options.revisions:
    name, _, value = revision.partition('@')
    if not name or not value:
      parser.error(
          'Wrong dep format: %s should be of the form dep@rev.' % revision)
    if ':' in name:
      name, _, package = name.partition(':')
      if not name or not package:
        parser.error(
            'Wrong CIPD format: %s:%s should be of the form path:pkg@version.'
            % (name, package))
      gclient_eval.SetCIPD(local_scope, name, package, value)
    else:
      gclient_eval.SetRevision(local_scope, name, value)

  with open(options.deps_file, 'w') as f:
    f.write(gclient_eval.RenderDEPSFile(local_scope))


def CMDverify(parser, args):
  """Verifies the DEPS file deps are only from allowed_hosts."""
  (options, args) = parser.parse_args(args)
  client = GClient.LoadCurrentConfig(options)
  if not client:
    raise gclient_utils.Error('client not configured; see \'gclient config\'')
  client.RunOnDeps(None, [])
  # Look at each first-level dependency of this gclient only.
  for dep in client.dependencies:
    bad_deps = dep.findDepsFromNotAllowedHosts()
    if not bad_deps:
      continue
    print("There are deps from not allowed hosts in file %s" % dep.deps_file)
    for bad_dep in bad_deps:
      print("\t%s at %s" % (bad_dep.name, bad_dep.url))
    print("allowed_hosts:", ', '.join(dep.allowed_hosts))
    sys.stdout.flush()
    raise gclient_utils.Error(
        'dependencies from disallowed hosts; check your DEPS file.')
  return 0

class OptionParser(optparse.OptionParser):
  gclientfile_default = os.environ.get('GCLIENT_FILE', '.gclient')

  def __init__(self, **kwargs):
    optparse.OptionParser.__init__(
        self, version='%prog ' + __version__, **kwargs)

    # Some arm boards have issues with parallel sync.
    if platform.machine().startswith('arm'):
      jobs = 1
    else:
      jobs = max(8, gclient_utils.NumLocalCpus())

    self.add_option(
        '-j', '--jobs', default=jobs, type='int',
        help='Specify how many SCM commands can run in parallel; defaults to '
             '%default on this machine')
    self.add_option(
        '-v', '--verbose', action='count', default=0,
        help='Produces additional output for diagnostics. Can be used up to '
             'three times for more logging info.')
    self.add_option(
        '--gclientfile', dest='config_filename',
        help='Specify an alternate %s file' % self.gclientfile_default)
    self.add_option(
        '--spec',
        help='create a gclient file containing the provided string. Due to '
            'Cygwin/Python brokenness, it can\'t contain any newlines.')
    self.add_option(
        '--cache-dir',
        help='(git only) Cache all git repos into this dir and do '
             'shared clones from the cache, instead of cloning '
             'directly from the remote. (experimental)',
        default=os.environ.get('GCLIENT_CACHE_DIR'))
    self.add_option(
        '--no-nag-max', default=False, action='store_true',
        help='Ignored for backwards compatibility.')

  def parse_args(self, args=None, values=None):
    """Integrates standard options processing."""
    options, args = optparse.OptionParser.parse_args(self, args, values)
    levels = [logging.ERROR, logging.WARNING, logging.INFO, logging.DEBUG]
    logging.basicConfig(
        level=levels[min(options.verbose, len(levels) - 1)],
        format='%(module)s(%(lineno)d) %(funcName)s:%(message)s')
    if options.config_filename and options.spec:
      self.error('Cannot specifiy both --gclientfile and --spec')
    if (options.config_filename and
        options.config_filename != os.path.basename(options.config_filename)):
      self.error('--gclientfile target must be a filename, not a path')
    if not options.config_filename:
      options.config_filename = self.gclientfile_default
    options.entries_filename = options.config_filename + '_entries'
    if options.jobs < 1:
      self.error('--jobs must be 1 or higher')

    # These hacks need to die.
    if not hasattr(options, 'revisions'):
      # GClient.RunOnDeps expects it even if not applicable.
      options.revisions = []
    if not hasattr(options, 'head'):
      options.head = None
    if not hasattr(options, 'nohooks'):
      options.nohooks = True
    if not hasattr(options, 'noprehooks'):
      options.noprehooks = True
    if not hasattr(options, 'deps_os'):
      options.deps_os = None
    if not hasattr(options, 'force'):
      options.force = None
    return (options, args)


def disable_buffering():
  # Make stdout auto-flush so buildbot doesn't kill us during lengthy
  # operations. Python as a strong tendency to buffer sys.stdout.
  sys.stdout = gclient_utils.MakeFileAutoFlush(sys.stdout)
  # Make stdout annotated with the thread ids.
  sys.stdout = gclient_utils.MakeFileAnnotated(sys.stdout)


def main(argv):
  """Doesn't parse the arguments here, just find the right subcommand to
  execute."""
  if sys.hexversion < 0x02060000:
    print(
        '\nYour python version %s is unsupported, please upgrade.\n' %
        sys.version.split(' ', 1)[0],
        file=sys.stderr)
    return 2
  if not sys.executable:
    print(
        '\nPython cannot find the location of it\'s own executable.\n',
        file=sys.stderr)
    return 2
  fix_encoding.fix_encoding()
  disable_buffering()
  setup_color.init()
  dispatcher = subcommand.CommandDispatcher(__name__)
  try:
    return dispatcher.execute(OptionParser(), argv)
  except KeyboardInterrupt:
    gclient_utils.GClientChildren.KillAllRemainingChildren()
    raise
  except (gclient_utils.Error, subprocess2.CalledProcessError) as e:
    print('Error: %s' % str(e), file=sys.stderr)
    return 1
  finally:
    gclient_utils.PrintWarnings()
  return 0


if '__main__' == __name__:
  try:
    sys.exit(main(sys.argv[1:]))
  except KeyboardInterrupt:
    sys.stderr.write('interrupted\n')
    sys.exit(1)

# vim: ts=2:sw=2:tw=80:et:
