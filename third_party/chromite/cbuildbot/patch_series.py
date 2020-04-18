# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module that handles patches applied to a repo checkout."""

from __future__ import print_function

import contextlib
import functools
import sys

from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import gerrit
from chromite.lib import git
from chromite.lib import parallel
from chromite.lib import patch as cros_patch

# Third-party libraries bundled with chromite need to be listed after the
# first chromite import.
import digraph


site_config = config_lib.GetConfig()


MAX_PLAN_RECURSION = 150


class PatchRejected(cros_patch.PatchException):
  """Raised if a patch was rejected by the CQ because the CQ failed."""

  def ShortExplanation(self):
    return 'was rejected by the CQ.'


class PatchNotEligible(cros_patch.PatchException):
  """Raised if a patch was not eligible for transaction."""

  def ShortExplanation(self):
    return ('was not eligible (wrong manifest branch, wrong labels, or '
            'otherwise filtered from eligible set).')


class PatchSeriesTooLong(cros_patch.PatchException):
  """Exception thrown when a patch series is too long."""

  def __init__(self, patch, max_length):
    cros_patch.PatchException.__init__(self, patch)
    self.max_length = max_length

  def ShortExplanation(self):
    return ("The Pre-CQ cannot handle a patch series longer than %s patches. "
            "Please wait for some patches to be submitted before marking more "
            "patches as ready. "  % (self.max_length,))

  def __str__(self):
    return self.ShortExplanation()


# Note: This exception differs slightly in meaning from
# PatchExceededRecursionLimit. That exception is caused by a RuntimeError when
# we hit recursion depth, where as this one is thrown by us before we reach the
# python recursion limit.
class PatchReachedRecursionLimit(cros_patch.PatchException):
  """Raised if we gave up on a too-recursive patch plan."""

  def ShortExplanation(self):
    return ('was part of a dependency stack that reached our recursion '
            'depth limit. Try breaking this stack into smaller pieces.')


class PatchExceededRecursionLimit(cros_patch.PatchException):
  """Raised if we encountered recursion limit while trying to apply patch."""

  def ShortExplanation(self):
    return ('was part of a dependency stack that exceeded our recursion '
            'depth. Try breaking this stack into smaller pieces.')


class GerritHelperNotAvailable(gerrit.GerritException):
  """Exception thrown when a specific helper is requested but unavailable."""

  def __init__(self, remote=site_config.params.EXTERNAL_REMOTE):
    gerrit.GerritException.__init__(self)
    # Stringify the pool so that serialization doesn't try serializing
    # the actual HelperPool.
    self.remote = remote
    self.args = (remote,)

  def __str__(self):
    return (
        "Needed a remote=%s gerrit_helper, but one isn't allowed by this "
        "HelperPool instance.") % (self.remote,)


def _PatchWrapException(functor):
  """Decorator to intercept patch exceptions and wrap them.

  Specifically, for known/handled Exceptions, it intercepts and
  converts it into a DependencyError- via that, preserving the
  cause, while casting it into an easier to use form (one that can
  be chained in addition).
  """
  def f(self, parent, *args, **kwargs):
    try:
      return functor(self, parent, *args, **kwargs)
    except gerrit.GerritException as e:
      if isinstance(e, gerrit.QueryNotSpecific):
        e = ("%s\nSuggest you use gerrit numbers instead (prefixed with a * "
             "if it's an internal change)." % e)
      new_exc = cros_patch.PatchException(parent, e)
      raise new_exc.__class__, new_exc, sys.exc_info()[2]
    except cros_patch.PatchException as e:
      if e.patch.id == parent.id:
        raise
      new_exc = cros_patch.DependencyError(parent, e)
      raise new_exc.__class__, new_exc, sys.exc_info()[2]

  f.__name__ = functor.__name__
  return f


def _FetchChangesForRepo(fetched_changes, by_repo, repo):
  """Fetch the changes for a given `repo`.

  Args:
    fetched_changes: A dict from change ids to changes which is updated by
      this method.
    by_repo: A mapping from repositories to changes.
    repo: The repository we should fetch the changes for.
  """
  changes = by_repo[repo]
  refs = set(c.ref for c in changes if not c.HasBeenFetched(repo))
  cmd = ['fetch', '-f', changes[0].project_url] + list(refs)
  git.RunGit(repo, cmd, print_cmd=True)

  for change in changes:
    sha1 = change.HasBeenFetched(repo) or change.sha1
    change.UpdateMetadataFromRepo(repo, sha1=sha1)
    fetched_changes[change.id] = change


class HelperPool(object):
  """Pool of allowed GerritHelpers to be used by CQ/PatchSeries."""

  def __init__(self, cros_internal=None, cros=None):
    """Initialize this instance with the given handlers.

    Most likely you want the classmethod SimpleCreate which takes boolean
    options.

    If a given handler is None, then it's disabled; else the passed in
    object is used.
    """
    self.pool = {
        site_config.params.EXTERNAL_REMOTE : cros,
        site_config.params.INTERNAL_REMOTE : cros_internal
    }

  @classmethod
  def SimpleCreate(cls, cros_internal=True, cros=True):
    """Classmethod helper for creating a HelperPool from boolean options.

    Args:
      cros_internal: If True, allow access to a GerritHelper for internal.
      cros: If True, allow access to a GerritHelper for external.

    Returns:
      An appropriately configured HelperPool instance.
    """
    if cros:
      cros = gerrit.GetGerritHelper(site_config.params.EXTERNAL_REMOTE)
    else:
      cros = None

    if cros_internal:
      cros_internal = gerrit.GetGerritHelper(site_config.params.INTERNAL_REMOTE)
    else:
      cros_internal = None

    return cls(cros_internal=cros_internal, cros=cros)

  def ForChange(self, change):
    """Return the helper to use for a particular change.

    If no helper is configured, an Exception is raised.
    """
    return self.GetHelper(change.remote)

  def GetHelper(self, remote):
    """Return the helper to use for a given remote.

    If no helper is configured, an Exception is raised.
    """
    helper = self.pool.get(remote)
    if not helper:
      raise GerritHelperNotAvailable(remote)

    return helper

  def __iter__(self):
    for helper in self.pool.itervalues():
      if helper:
        yield helper


class _ManifestShim(object):
  """A fake manifest that only contains a single repository.

  This fake manifest is used to allow us to filter out patches for
  the PatchSeries class. It isn't a complete implementation -- we just
  implement the functions that PatchSeries uses. It works via duck typing.

  All of the below methods accept the same arguments as the corresponding
  methods in git.ManifestCheckout.*, but they do not make any use of the
  arguments -- they just always return information about this project.
  """

  def __init__(self, path, tracking_branch, remote='origin'):

    tracking_branch = 'refs/remotes/%s/%s' % (
        remote, git.StripRefs(tracking_branch),
    )
    attrs = dict(local_path=path, path=path, tracking_branch=tracking_branch)
    self.checkout = git.ProjectCheckout(attrs)

  def FindCheckouts(self, *_args, **_kwargs):
    """Returns the list of checkouts.

    In this case, we only have one repository so we just return that repository.
    We accept the same arguments as git.ManifestCheckout.FindCheckouts, but we
    do not make any use of them.

    Returns:
      A list of ProjectCheckout objects.
    """
    return [self.checkout]


class PatchSeries(object):
  """Class representing a set of patches applied to a repo checkout."""

  def __init__(self, path, helper_pool=None, forced_manifest=None,
               deps_filter_fn=None, is_submitting=False):
    """Constructor.

    Args:
      path: Path to the buildroot.
      helper_pool: Pool of allowed GerritHelpers to be used for fetching
        patches. Defaults to allowing both internal and external fetches.
      forced_manifest: A manifest object to use for mapping projects to
        repositories. Defaults to the buildroot.
      deps_filter_fn: A function which specifies what patches you would
        like to accept. It is passed a patch and is expected to return
        True or False.
      is_submitting: Whether we are currently submitting patchsets. This is
        used to print better error messages.
    """
    self.manifest = forced_manifest

    if helper_pool is None:
      helper_pool = HelperPool.SimpleCreate(cros_internal=True, cros=True)
    self._helper_pool = helper_pool
    self._path = path
    if deps_filter_fn is None:
      deps_filter_fn = lambda x: True
    self.deps_filter_fn = deps_filter_fn
    self._is_submitting = is_submitting

    self.failed_tot = {}

    # A mapping of ChangeId to exceptions if the patch failed against
    # ToT.  Primarily used to keep the resolution/applying from going
    # down known bad paths.
    self._committed_cache = cros_patch.PatchCache()
    self._lookup_cache = cros_patch.PatchCache()
    self._change_deps_cache = {}

  def _ManifestDecorator(functor):
    """Method decorator that sets self.manifest automatically.

    This function automatically initializes the manifest, and allows callers to
    override the manifest if needed.
    """
    # pylint: disable=E0213,W0212,E1101,E1102
    def f(self, *args, **kwargs):
      manifest = kwargs.pop('manifest', None)
      # Wipe is used to track if we need to reset manifest to None, and
      # to identify if we already had a forced_manifest via __init__.
      wipe = self.manifest is None
      if manifest:
        if not wipe:
          raise ValueError("manifest can't be specified when one is forced "
                           "via __init__")
      elif wipe:
        manifest = git.ManifestCheckout.Cached(self._path)
      else:
        manifest = self.manifest

      try:
        self.manifest = manifest
        return functor(self, *args, **kwargs)
      finally:
        if wipe:
          self.manifest = None

    f.__name__ = functor.__name__
    f.__doc__ = functor.__doc__
    return f

  @_ManifestDecorator
  def GetGitRepoForChange(self, change, strict=False, manifest=None):
    """Get the project path associated with the specified change.

    Args:
      change: The change to operate on.
      strict: If True, throw ChangeNotInManifest rather than returning
        None. Default: False.
      manifest: A ManifestCheckout instance representing what we're working on.

    Returns:
      The project path if found in the manifest. Otherwise returns
      None (if strict=False).
    """
    project_dir = None
    if manifest is None:
      manifest = self.manifest
    if manifest:
      checkout = change.GetCheckout(manifest, strict=strict)
      if checkout is not None:
        project_dir = checkout.GetPath(absolute=True)

    return project_dir

  @_ManifestDecorator
  def ApplyChange(self, change):
    # Always enable content merging.
    return change.ApplyAgainstManifest(self.manifest, trivial=False)

  def _LookupHelper(self, patch):
    """Returns the helper for the given cros_patch.PatchQuery object."""
    return self._helper_pool.GetHelper(patch.remote)

  def _GetGerritPatch(self, query):
    """Query the configured helpers looking for a given change.

    Args:
      project: The gerrit project to query.
      query: A cros_patch.PatchQuery object.

    Returns:
      A GerritPatch object.
    """
    helper = self._LookupHelper(query)
    query_text = query.ToGerritQueryText()
    change = helper.QuerySingleRecord(
        query_text, must_match=not git.IsSHA1(query_text))

    if not change:
      return

    # If the query was a gerrit number based query, check the projects/change-id
    # to see if we already have it locally, but couldn't map it since we didn't
    # know the gerrit number at the time of the initial injection.
    existing = self._lookup_cache[change]
    if cros_patch.ParseGerritNumber(query_text) and existing is not None:
      keys = change.LookupAliases()
      self._lookup_cache.InjectCustomKeys(keys, existing)
      return existing

    self.InjectLookupCache([change])
    if change.IsAlreadyMerged():
      self.InjectCommittedPatches([change])
    return change

  def _LookupUncommittedChanges(self, deps, limit_to=None):
    """Given a set of deps (changes), return unsatisfied dependencies.

    Args:
      deps: A list of cros_patch.PatchQuery objects representing
        sequence of dependencies for the leaf that we need to identify
        as either merged, or needing resolving.
      limit_to: If non-None, then this must be a mapping (preferably a
        cros_patch.PatchCache for translation reasons) of which non-committed
        changes are allowed to be used for a transaction.

    Returns:
      A sequence of cros_patch.GitRepoPatch instances (or derivatives) that
      need to be resolved for this change to be mergable.

    Raises:
      Some variety of cros_patch.PatchException if an unsatisfiable required
      dependency is encountered.
    """
    unsatisfied = []
    for dep in deps:
      if dep in self._committed_cache:
        continue

      try:
        self._LookupHelper(dep)
      except GerritHelperNotAvailable:
        # Internal dependencies are irrelevant to external builders.
        logging.info("Skipping internal dependency: %s", dep)
        continue

      dep_change = self._lookup_cache[dep]

      if dep_change is None:
        dep_change = self._GetGerritPatch(dep)
      if dep_change is None:
        continue
      if getattr(dep_change, 'IsAlreadyMerged', lambda: False)():
        continue
      elif limit_to is not None and dep_change not in limit_to:
        if self._is_submitting:
          raise PatchRejected(dep_change)
        else:
          raise dep_change.GetMergeException() or PatchNotEligible(dep_change)

      unsatisfied.append(dep_change)

    # Perform last minute custom filtering.
    return [x for x in unsatisfied if self.deps_filter_fn(x)]

  def CreateTransaction(self, change, limit_to=None):
    """Given a change, resolve it into a transaction.

    In this case, a transaction is defined as a group of commits that
    must land for the given change to be merged- specifically its
    parent deps, and its CQ-DEPEND.

    Args:
      change: A cros_patch.GitRepoPatch instance to generate a transaction
        for.
      limit_to: If non-None, limit the allowed uncommitted patches to
        what's in that container/mapping.

    Returns:
      A sequence of the necessary cros_patch.GitRepoPatch objects for
      this transaction.

    Raises:
      DependencyError: If we could not resolve a dependency.
      GerritException or GOBError: If there is a failure in querying gerrit.
    """
    plan = []
    gerrit_deps_seen = cros_patch.PatchCache()
    cq_deps_seen = cros_patch.PatchCache()
    self._AddChangeToPlanWithDeps(change, plan, gerrit_deps_seen,
                                  cq_deps_seen, limit_to=limit_to)
    return plan

  def CreateTransactions(self, changes, limit_to=None):
    """Create a list of transactions from a list of changes.

    Args:
      changes: A list of cros_patch.GitRepoPatch instances to generate
        transactions for.
      limit_to: See CreateTransaction docs.

    Returns:
      A list of (change, plan, e) tuples for the given list of changes. The
      plan represents the necessary GitRepoPatch objects for a given change. If
      an exception occurs while creating the transaction, e will contain the
      exception. (Otherwise, e will be None.)
    """
    for change in changes:
      try:
        logging.info('Attempting to create transaction for %s', change)
        plan = self.CreateTransaction(change, limit_to=limit_to)
      except cros_patch.PatchException as e:
        yield (change, (), e)
      except RuntimeError as e:
        if 'maximum recursion depth' in e.message:
          yield (change, (), PatchExceededRecursionLimit(change))
        else:
          raise
      else:
        yield (change, plan, None)

  def CreateDisjointTransactions(self, changes, max_txn_length=None,
                                 merge_projects=False):
    """Create a list of disjoint transactions from a list of changes.

    Args:
      changes: A list of cros_patch.GitRepoPatch instances to generate
        transactions for.
      max_txn_length: The maximum length of any given transaction.  By default,
        do not limit the length of transactions.
      merge_projects: If set, put all changes to a given project in the same
        transaction.

    Returns:
      A list of disjoint transactions and a list of exceptions. Each transaction
      can be tried independently, without involving patches from other
      transactions. Each change in the pool will included in exactly one of the
      transactions, unless the patch does not apply for some reason.
    """
    # Gather the dependency graph for the specified changes.
    deps, edges, failed = {}, {}, []
    for change, plan, ex in self.CreateTransactions(changes, limit_to=changes):
      if ex is not None:
        logging.info('Failed creating transaction for %s: %s', change, ex)
        failed.append(ex)
      else:
        # Save off the ordered dependencies of this change.
        deps[change] = plan

        # Mark every change in the transaction as bidirectionally connected.
        for change_dep in plan:
          edges.setdefault(change_dep, set()).update(plan)

    if merge_projects:
      projects = {}
      for change in deps:
        projects.setdefault(change.project, []).append(change)
      for project in projects:
        for change in projects[project]:
          edges.setdefault(change, set()).update(projects[project])

    # Calculate an unordered group of strongly connected components.
    unordered_plans = digraph.StronglyConnectedComponents(list(edges), edges)

    # Sort the groups according to our ordered dependency graph.
    ordered_plans = []
    for unordered_plan in unordered_plans:
      ordered_plan, seen = [], set()
      for change in unordered_plan:
        # Iterate over the required CLs, adding them to our plan in order.
        new_changes = list(dep_change for dep_change in deps.get(change, [])
                           if dep_change not in seen)
        new_plan_size = len(ordered_plan) + len(new_changes)
        if not max_txn_length or new_plan_size <= max_txn_length:
          seen.update(new_changes)
          ordered_plan.extend(new_changes)

      if ordered_plan:
        # We found a transaction that is <= max_txn_length. Process the
        # transaction. Ignore the remaining patches for now; they will be
        # processed later (once the current transaction has been pushed).
        ordered_plans.append(ordered_plan)
      else:
        # We couldn't find any transactions that were <= max_txn_length.
        # This should only happen if circular dependencies prevent us from
        # truncating a long list of patches. Reject the whole set of patches
        # and complain.
        for change in unordered_plan:
          failed.append(PatchSeriesTooLong(change, max_txn_length))

    return ordered_plans, failed

  @_PatchWrapException
  def _AddChangeToPlanWithDeps(self, change, plan, gerrit_deps_seen,
                               cq_deps_seen, limit_to=None,
                               include_cq_deps=True,
                               remaining_depth=MAX_PLAN_RECURSION):
    """Add a change and its dependencies into a |plan|.

    Args:
      change: The change to add to the plan.
      plan: The list of changes to apply, in order. This function will append
        |change| and any necessary dependencies to |plan|.
      gerrit_deps_seen: The changes whose Gerrit dependencies have already been
        processed.
      cq_deps_seen: The changes whose CQ-DEPEND and Gerrit dependencies have
        already been processed.
      limit_to: If non-None, limit the allowed uncommitted patches to
        what's in that container/mapping.
      include_cq_deps: If True, include CQ dependencies in the list
        of dependencies. Defaults to True.
      remaining_depth: Amount of permissible recursion depth from this call.

    Raises:
      DependencyError: If we could not resolve a dependency.
      GerritException or GOBError: If there is a failure in querying gerrit.
    """
    if change in self._committed_cache:
      return

    if remaining_depth == 0:
      raise PatchReachedRecursionLimit(change)

    # Get a list of the changes that haven't been committed.
    # These are returned as cros_patch.PatchQuery objects.
    gerrit_deps, cq_deps = self.GetDepsForChange(change)

    # Only process the Gerrit dependencies for each change once. We prioritize
    # Gerrit dependencies over CQ dependencies, since Gerrit dependencies might
    # be required in order for the change to apply.
    old_plan_len = len(plan)
    if change not in gerrit_deps_seen:
      gerrit_deps = self._LookupUncommittedChanges(
          gerrit_deps, limit_to=limit_to)
      gerrit_deps_seen.Inject(change)
      for dep in gerrit_deps:
        self._AddChangeToPlanWithDeps(dep, plan, gerrit_deps_seen, cq_deps_seen,
                                      limit_to=limit_to, include_cq_deps=False,
                                      remaining_depth=remaining_depth - 1)

    if include_cq_deps and change not in cq_deps_seen:
      cq_deps = self._LookupUncommittedChanges(
          cq_deps, limit_to=limit_to)
      cq_deps_seen.Inject(change)
      for dep in plan[old_plan_len:] + cq_deps:
        # Add the requested change (plus deps) to our plan, if it we aren't
        # already in the process of doing that.
        if dep not in cq_deps_seen:
          self._AddChangeToPlanWithDeps(dep, plan, gerrit_deps_seen,
                                        cq_deps_seen, limit_to=limit_to,
                                        remaining_depth=remaining_depth - 1)

    # If there are cyclic dependencies, we might have already applied this
    # patch as part of dependency resolution. If not, apply this patch.
    if change not in plan:
      plan.append(change)

  @_PatchWrapException
  def GetDepChangesForChange(self, change):
    """Look up the Gerrit/CQ dependency changes for |change|.

    Returns:
      (gerrit_deps, cq_deps): The change's Gerrit dependencies and CQ
      dependencies, as lists of GerritPatch objects.

    Raises:
      DependencyError: If we could not resolve a dependency.
      GerritException or GOBError: If there is a failure in querying gerrit.
    """
    gerrit_deps, cq_deps = self.GetDepsForChange(change)

    def _DepsToChanges(deps):
      dep_changes = []
      unprocessed_deps = []
      for dep in deps:
        dep_change = self._committed_cache[dep]
        if dep_change:
          dep_changes.append(dep_change)
        else:
          unprocessed_deps.append(dep)

      for dep in unprocessed_deps:
        dep_changes.extend(self._LookupUncommittedChanges(deps))

      return dep_changes

    return _DepsToChanges(gerrit_deps), _DepsToChanges(cq_deps)

  @_PatchWrapException
  def GetDepsForChange(self, change):
    """Look up the Gerrit/CQ deps for |change|.

    Returns:
      A tuple of PatchQuery objects representing change's Gerrit
      dependencies, and CQ dependencies.

    Raises:
      DependencyError: If we could not resolve a dependency.
      GerritException or GOBError: If there is a failure in querying gerrit.
    """
    val = self._change_deps_cache.get(change)
    if val is None:
      git_repo = self.GetGitRepoForChange(change)
      val = self._change_deps_cache[change] = (
          change.GerritDependencies(),
          change.PaladinDependencies(git_repo))

    return val

  def InjectCommittedPatches(self, changes):
    """Record that the given patches are already committed.

    This is primarily useful for external code to notify this object
    that changes were applied to the tree outside its purview- specifically
    useful for dependency resolution.
    """
    self._committed_cache.Inject(*changes)

  def InjectLookupCache(self, changes):
    """Inject into the internal lookup cache the given changes.

    Uses |changes| rather than asking gerrit for them for dependencies.
    """
    self._lookup_cache.Inject(*changes)

  def FetchChanges(self, changes, manifest=None):
    """Fetch the specified changes, if needed.

    If we're an external builder, internal changes are filtered out.

    Args:
      changes: A list of changes to fetch.
      manifest: A ManifestCheckout instance representing what we're working on.

    Returns:
      A list of the filtered changes and a list of
      cros_patch.ChangeNotInManifest instances for changes not in manifest.
    """
    by_repo = {}
    changes_to_fetch = []
    not_in_manifest = []
    for change in changes:
      try:
        self._helper_pool.ForChange(change)
      except GerritHelperNotAvailable:
        # Internal patches are irrelevant to external builders.
        logging.info("Skipping internal patch: %s", change)
        continue

      repo = None
      try:
        repo = self.GetGitRepoForChange(change, strict=True, manifest=manifest)
      except cros_patch.ChangeNotInManifest as e:
        logging.info('Skipping patch %s as it\'s not in manifest.', change)
        not_in_manifest.append(e)
        continue

      by_repo.setdefault(repo, []).append(change)
      changes_to_fetch.append(change)

    # Fetch changes in parallel. The change.Fetch() method modifies the
    # 'change' object, so make sure we grab all of that information.
    with parallel.Manager() as manager:
      fetched_changes = manager.dict()

      fetch_repo = functools.partial(
          _FetchChangesForRepo, fetched_changes, by_repo)
      parallel.RunTasksInProcessPool(fetch_repo, [[repo] for repo in by_repo])

      return [fetched_changes[c.id] for c in changes_to_fetch], not_in_manifest

  def ReapplyChanges(self, by_repo):
    """Make sure that all of the local changes still apply.

    Syncs all of the repositories in the manifest and reapplies the changes on
    top of the tracking branch for each repository.

    Args:
      by_repo: A mapping from repo paths to changes in that repo.

    Returns:
      a new by_repo dict containing only the patches that apply correctly, and
      errors, a dict of patches to exceptions encountered while applying them.
    """
    self.ResetCheckouts(constants.PATCH_BRANCH, fetch=True)
    local_changes = reduce(set.union, by_repo.values(), set())
    applied_changes, failed_tot, failed_inflight = self.Apply(local_changes)
    errors = {}
    for exception in failed_tot + failed_inflight:
      errors[exception.patch] = exception

    # Filter out only the changes that applied.
    by_repo = dict(by_repo)
    for repo in by_repo:
      by_repo[repo] &= set(applied_changes)

    return by_repo, errors

  @_ManifestDecorator
  def ResetCheckouts(self, branch, fetch=False):
    """Updates |branch| in all Git checkouts in the manifest to their remotes.

    Args:
      branch: The branch to update.
      fetch: Indicates whether to sync the remotes before resetting.
    """
    if not self.manifest:
      logging.info("No manifest, skipping reset.")
      return

    def _Reset(checkout):
      path = checkout.GetPath()

      # There is no need to reset the branch if it doesn't exist.
      if not git.DoesCommitExistInRepo(path, branch):
        return

      if fetch:
        git.RunGit(path, ['fetch', '--all'])

      def _LogBranch():
        branches = git.RunGit(path, ['branch', '-vv']).output.splitlines()
        branch_line = [b for b in branches if branch in b]
        logging.info(branch_line)

      _LogBranch()
      git.RunGit(path, ['checkout', '-f', branch])
      logging.info('Resetting to %s', checkout['tracking_branch'])
      git.RunGit(path, ['reset', checkout['tracking_branch'], '--hard'])
      _LogBranch()

    parallel.RunTasksInProcessPool(
        _Reset,
        [[c] for c in self.manifest.ListCheckouts()])

  @_ManifestDecorator
  def Apply(self, changes, frozen=True, honor_ordering=False,
            changes_filter=None):
    """Applies changes from pool into the build root specified by the manifest.

    This method resolves each given change down into a set of transactions-
    the change and its dependencies- that must go in, then tries to apply
    the largest transaction first, working its way down.

    If a transaction cannot be applied, then it is rolled back
    in full- note that if a change is involved in multiple transactions,
    if an earlier attempt fails, that change can be retried in a new
    transaction if the failure wasn't caused by the patch being incompatible
    to ToT.

    Args:
      changes: A sequence of cros_patch.GitRepoPatch instances to resolve
        and apply.
      frozen: If True, then resolving of the given changes is explicitly
        limited to just the passed in changes, or known committed changes.
        This is basically CQ/Paladin mode, used to limit the changes being
        pulled in/committed to just what we allow.
      honor_ordering: Apply normally will reorder the transactions it
        computes, trying the largest first, then degrading through smaller
        transactions if the larger of the two fails.  If honor_ordering
        is False, then the ordering given via changes is preserved-
        this is mainly of use for cbuildbot induced patching, and shouldn't
        be used for CQ patching.
      changes_filter: If not None, must be a functor taking two arguments:
        series, changes; it must return the changes to work on.
        This is invoked after the initial changes have been fetched,
        thus this is a way for consumers to do last minute checking of the
        changes being inspected, and expand the changes if necessary.
        Primarily this is of use for cbuildbot patching when dealing w/
        uploaded/remote patches.

    Returns:
      A tuple of changes-applied, Exceptions for the changes that failed
      against ToT, and Exceptions that failed inflight;  These exceptions
      are cros_patch.PatchException instances.
    """
    resolved, applied, failed = [], [], []

    # Prefetch the changes; we need accurate change_id/id's, which is
    # guaranteed via Fetch.
    changes, not_in_manifest = self.FetchChanges(changes)
    failed.extend(not_in_manifest)
    if changes_filter:
      changes = changes_filter(self, changes)

    self.InjectLookupCache(changes)
    limit_to = cros_patch.PatchCache(changes) if frozen else None

    planned = set()
    for change, plan, ex in self.CreateTransactions(changes, limit_to=limit_to):
      if ex is not None:
        logging.info("Failed creating transaction for %s: %s", change, ex)
        failed.append(ex)
      else:
        resolved.append((change, plan))
        logging.info("Transaction for %s is %s.",
                     change, ', '.join(map(str, resolved[-1][-1])))
        planned.update(plan)

    if not resolved:
      # No work to do; either no changes were given to us, or all failed
      # to be resolved.
      return [], failed, []

    if not honor_ordering:
      # Sort by length, falling back to the order the changes were given to us.
      # This is done to prefer longer transactions (more painful to rebase)
      # over shorter transactions.
      position = dict((change, idx) for idx, change in enumerate(changes))
      def mk_key(data):
        change, plan = data
        ids = [x.id for x in plan]
        return -len(ids), position[change]
      resolved.sort(key=mk_key)

    for inducing_change, transaction_changes in resolved:
      try:
        with self._Transaction(transaction_changes):
          logging.debug("Attempting transaction for %s: changes: %s",
                        inducing_change,
                        ', '.join(map(str, transaction_changes)))
          self._ApplyChanges(inducing_change, transaction_changes)
      except cros_patch.PatchException as e:
        logging.info("Failed applying transaction for %s: %s",
                     inducing_change, e)
        failed.append(e)
      else:
        applied.extend(transaction_changes)
        self.InjectCommittedPatches(transaction_changes)

    # Uniquify while maintaining order.
    def _uniq(l):
      s = set()
      for x in l:
        if x not in s:
          yield x
          s.add(x)

    applied = list(_uniq(applied))
    self._is_submitting = True

    failed = [x for x in failed if x.patch not in applied]
    failed_tot = [x for x in failed if not x.inflight]
    failed_inflight = [x for x in failed if x.inflight]
    return applied, failed_tot, failed_inflight

  @contextlib.contextmanager
  def _Transaction(self, commits):
    """ContextManager used to rollback changes to a build root if necessary.

    Specifically, if an unhandled non system exception occurs, this context
    manager will roll back all relevant modifications to the git repos
    involved.

    Args:
      commits: A sequence of cros_patch.GitRepoPatch instances that compromise
        this transaction- this is used to identify exactly what may be changed,
        thus what needs to be tracked and rolled back if the transaction fails.
    """
    # First, the book keeping code; gather required data so we know what
    # to rollback to should this transaction fail.  Specifically, we track
    # what was checked out for each involved repo, and if it was a branch,
    # the sha1 of the branch; that information is enough to rewind us back
    # to the original repo state.
    project_state = set(
        map(functools.partial(self.GetGitRepoForChange, strict=True), commits))
    resets = []
    for project_dir in project_state:
      current_sha1 = git.RunGit(
          project_dir, ['rev-list', '-n1', 'HEAD']).output.strip()
      resets.append((project_dir, current_sha1))
      assert current_sha1

    committed_cache = self._committed_cache.copy()

    try:
      yield
    except Exception:
      logging.info("Rewinding transaction: failed changes: %s .",
                   ', '.join(map(str, commits)), exc_info=True)

      for project_dir, sha1 in resets:
        git.RunGit(project_dir, ['reset', '--hard', sha1])

      self._committed_cache = committed_cache
      raise

  @_PatchWrapException
  def _ApplyChanges(self, _inducing_change, changes):
    """Apply a given ordered sequence of changes.

    Args:
      _inducing_change: The core GitRepoPatch instance that lead to this
        sequence of changes; basically what this transaction was computed from.
        Needs to be passed in so that the exception wrapping machinery can
        convert any failures, assigning blame appropriately.
      manifest: A ManifestCheckout instance representing what we're working on.
      changes: A ordered sequence of GitRepoPatch instances to apply.
    """
    # Bail immediately if we know one of the requisite patches won't apply.
    for change in changes:
      failure = self.failed_tot.get(change.id)
      if failure is not None:
        raise failure

    applied = []
    for change in changes:
      if change in self._committed_cache:
        continue

      try:
        self.ApplyChange(change)
      except cros_patch.PatchException as e:
        if not e.inflight:
          self.failed_tot[change.id] = e
        raise
      applied.append(change)

    logging.debug('Done investigating changes.  Applied %s',
                  ' '.join([c.id for c in applied]))

  @classmethod
  def WorkOnSingleRepo(cls, git_repo, tracking_branch, **kwargs):
    """Classmethod to generate a PatchSeries that targets a single git repo.

    It does this via forcing a fake manifest, which in turn points
    tracking branch/paths/content-merging at what is passed through here.

    Args:
      git_repo: Absolute path to the git repository to operate upon.
      tracking_branch: Which tracking branch patches should apply against.
      kwargs: See PatchSeries.__init__ for the various optional args;
        note forced_manifest cannot be used here.

    Returns:
      A PatchSeries instance w/ a forced manifest.
    """

    if 'forced_manifest' in kwargs:
      raise ValueError("RawPatchSeries doesn't allow a forced_manifest "
                       "argument.")
    kwargs['forced_manifest'] = _ManifestShim(git_repo, tracking_branch)

    return cls(git_repo, **kwargs)
