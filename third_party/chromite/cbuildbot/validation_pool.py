# -*- coding: utf-8 -*-
# Copyright (c) 2011-2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module that handles interactions with a Validation Pool.

The validation pool is the set of commits that are ready to be validated i.e.
ready for the commit queue to try.
"""

from __future__ import print_function

import cPickle
import functools
import httplib
import os
import random
import time
from xml.dom import minidom

from chromite.cbuildbot import lkgm_manager
from chromite.cbuildbot import patch_series
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cl_messages
from chromite.lib import clactions
from chromite.lib import cros_logging as logging
from chromite.lib import cros_build_lib
from chromite.lib import failures_lib
from chromite.lib import gerrit
from chromite.lib import git
from chromite.lib import gob_util
from chromite.lib import metrics
from chromite.lib import parallel
from chromite.lib import patch as cros_patch
from chromite.lib import timeout_util
from chromite.lib import tree_status
from chromite.lib import triage_lib


site_config = config_lib.GetConfig()


PRE_CQ = constants.PRE_CQ
CQ = constants.CQ

CQ_CONFIG = constants.CQ_MASTER
PRE_CQ_LAUNCHER_CONFIG = constants.PRE_CQ_LAUNCHER_CONFIG

# Set of configs that can reject a CL from the pre-CQ / CQ pipeline.
# TODO(davidjames): Any Pre-CQ config can reject CLs now, so this is wrong.
# This is only used for fail counts. Maybe it makes sense to just get rid of
# the fail count?
CQ_PIPELINE_CONFIGS = {CQ_CONFIG, PRE_CQ_LAUNCHER_CONFIG}

# The gerrit-on-borg team tells us that delays up to 2 minutes can be
# normal.  Setting timeout to 3 minutes to be safe-ish.
SUBMITTED_WAIT_TIMEOUT = 3 * 60 # Time in seconds.

# Default timeout (second) for computing dependency map.
COMPUTE_DEPENDENCY_MAP_TIMEOUT = 5 * 60

# The url prefix of the CL status page.
CL_STATUS_URL_PREFIX = 'https://chromeos-cl-viewer-ui.googleplex.com/cl_status'

class TreeIsClosedException(Exception):
  """Raised when the tree is closed and we wanted to submit changes."""

  def __init__(self, closed_or_throttled=False):
    """Initialization.

    Args:
      closed_or_throttled: True if the exception is being thrown on a
                           possibly 'throttled' tree. False if only
                           thrown on a 'closed' tree. Default: False
    """
    if closed_or_throttled:
      status_text = 'closed or throttled'
      opposite_status_text = 'open'
    else:
      status_text = 'closed'
      opposite_status_text = 'throttled or open'

    super(TreeIsClosedException, self).__init__(
        'Tree is %s.  Please set tree status to %s to '
        'proceed.' % (status_text, opposite_status_text))


class FailedToSubmitAllChangesException(failures_lib.StepFailure):
  """Raised if we fail to submit any change."""

  def __init__(self, changes, num_submitted):
    super(FailedToSubmitAllChangesException, self).__init__(
        'FAILED TO SUBMIT ALL CHANGES:  Could not verify that changes %s were '
        'submitted.'
        '\nSubmitted %d changes successfully.' %
        (' '.join(str(c) for c in changes), num_submitted))


class InternalCQError(cros_patch.PatchException):
  """Exception thrown when CQ has an unexpected/unhandled error."""

  def __init__(self, patch, message):
    cros_patch.PatchException.__init__(self, patch, message=message)

  def ShortExplanation(self):
    return 'failed to apply due to a CQ issue: %s' % (self.message,)


class InconsistentReloadException(Exception):
  """Raised if patches applied by the CQ cannot be found anymore."""


class PatchModified(cros_patch.PatchException):
  """Raised if a patch is modified while the CQ is running."""

  def __init__(self, patch, patch_number):
    cros_patch.PatchException.__init__(self, patch)
    self.new_patch_number = patch_number
    self.args = (patch, patch_number)

  def ShortExplanation(self):
    return ('was modified while the CQ was in the middle of testing it. '
            'Patch set %s was uploaded.' % self.new_patch_number)


class PatchFailedToSubmit(cros_patch.PatchException):
  """Raised if we fail to submit a change."""

  def ShortExplanation(self):
    error = 'could not be submitted by the CQ.'
    if self.message:
      error += ' The error message from Gerrit was: %s' % (self.message,)
    else:
      error += ' The Gerrit server might be having trouble.'
    return error


class PatchConflict(cros_patch.PatchException):
  """Raised if a patch needs to be rebased."""

  def ShortExplanation(self):
    return ('could not be submitted because Gerrit reported a conflict. Did '
            'you modify your patch during the CQ run? Or do you just need to '
            'rebase?')


class PatchSubmittedWithoutDeps(cros_patch.DependencyError):
  """Exception thrown when a patch was submitted incorrectly."""

  def ShortExplanation(self):
    dep_error = cros_patch.DependencyError.ShortExplanation(self)
    return ('was submitted, even though it %s\n'
            '\n'
            'You may want to revert your patch, and investigate why its'
            'dependencies failed to submit.\n'
            '\n'
            'This error only occurs when we have a dependency cycle, and we '
            'submit one change before realizing that a later change cannot '
            'be submitted.' % (dep_error,))


class ValidationPool(object):
  """Class that handles interactions with a validation pool.

  This class can be used to acquire a set of commits that form a pool of
  commits ready to be validated and committed.

  Usage:  Use ValidationPool.AcquirePool -- a static
  method that grabs the commits that are ready for validation.
  """

  # Global variable to control whether or not we should allow CL's to get tried
  # and/or committed when the tree is throttled.
  # TODO(sosa): Remove this global once metrics show that this is the direction
  # we want to go (and remove all additional throttled_ok logic from this
  # module.
  THROTTLED_OK = True
  GLOBAL_DRYRUN = False
  DEFAULT_TIMEOUT = 60 * 60 * 4
  SLEEP_TIMEOUT = 30
  # Buffer time to leave when using the global build deadline as the sync stage
  # timeout. We need some time to possibly extend the global build deadline
  # after the sync timeout is hit.
  EXTENSION_TIMEOUT_BUFFER = 10 * 60
  INCONSISTENT_SUBMIT_MSG = ('Gerrit thinks that the change was not submitted, '
                             'even though we hit the submit button.')

  # The grace period (in seconds) before we reject a patch due to dependency
  # errors.
  REJECTION_GRACE_PERIOD = 30 * 60

  # How many CQ runs to go back when making a decision about the CQ health.
  # Note this impacts the max exponential fallback (1.5^4 ~= 5 max exponential
  # divisor)
  CQ_SEARCH_HISTORY = 4


  def __init__(self, overlays, build_root, build_number, builder_name,
               is_master, dryrun, candidates=None, non_os_changes=None,
               conflicting_changes=None, pre_cq_trybot=False,
               tree_was_open=True, applied=None, buildbucket_id=None,
               builder_run=None):
    """Initializes an instance by setting default variables to instance vars.

    Generally use AcquirePool as an entry pool to a pool rather than this
    method.

    Args:
      overlays: One of constants.VALID_OVERLAYS.
      build_root: Build root directory.
      build_number: Build number for this validation attempt.
      builder_name: Builder name on buildbot dashboard.
      is_master: True if this is the master builder for the Commit Queue.
      dryrun: If set to True, do not submit anything to Gerrit.
    Optional Args:
      candidates: List of changes to consider validating.
      non_os_changes: List of changes that are part of this validation
        pool but aren't part of the cros checkout.
      conflicting_changes: Changes that failed to apply but we're keeping around
        because they conflict with other changes in flight.
      pre_cq_trybot: If set to True, this is a Pre-CQ trybot. (Note: The Pre-CQ
        launcher is NOT considered a Pre-CQ trybot.)
      tree_was_open: Whether the tree was open when the pool was created.
      applied: List of CLs that have been applied to the current repo.
      buildbucket_id: Buildbucket id of the current build as a string .
                      None if not buildbucket scheduled.
      builder_run: BuilderRun instance used to fetch cidb handle and metadata
        instance. Please note due to the pickling logic, this MUST be the last
        kwarg listed.
    """
    self.build_root = build_root

    # These instances can be instantiated via both older, or newer pickle
    # dumps.  Thus we need to assert the given args since we may be getting
    # a value we no longer like (nor work with).
    if overlays not in constants.VALID_OVERLAYS:
      raise ValueError("Unknown/unsupported overlay: %r" % (overlays,))

    self._helper_pool = self.GetGerritHelpersForOverlays(overlays)

    if not isinstance(build_number, int):
      raise ValueError("Invalid build_number: %r" % (build_number,))

    if not isinstance(builder_name, basestring):
      raise ValueError("Invalid builder_name: %r" % (builder_name,))

    if (buildbucket_id is not None and
        not isinstance(buildbucket_id, basestring)):
      raise ValueError("Invalid buildbucket_id: %r" % (builder_name,))

    for changes_name, changes_value in (
        ('candidates', candidates),
        ('non_os_changes', non_os_changes),
        ('applied', applied)):
      if not changes_value:
        continue
      if not all(isinstance(x, cros_patch.GitRepoPatch) for x in changes_value):
        raise ValueError(
            'Invalid %s: all elements must be a GitRepoPatch derivative, got %r'
            % (changes_name, changes_value))

    if conflicting_changes and not all(
        isinstance(x, cros_patch.PatchException)
        for x in conflicting_changes):
      raise ValueError(
          'Invalid conflicting_changes: all elements must be a '
          'cros_patch.PatchException derivative, got %r'
          % (conflicting_changes,))

    self.is_master = bool(is_master)
    self.pre_cq_trybot = pre_cq_trybot
    self._run = builder_run
    self.dryrun = bool(dryrun) or self.GLOBAL_DRYRUN
    if pre_cq_trybot:
      self.queue = 'A trybot'
    elif builder_name == constants.PRE_CQ_LAUNCHER_NAME:
      self.queue = 'The Pre-Commit Queue'
    else:
      self.queue = 'The Commit Queue'

    # See optional args for types of changes.
    self.candidates = candidates or []
    self.non_manifest_changes = non_os_changes or []
    self.applied = applied or []
    self.applied_patches = None
    # Whether this pool picked up new chumpped CLs.
    self.has_chump_cls = False

    # Note, we hold onto these CLs since they conflict against our current CLs
    # being tested; if our current ones succeed, we notify the user to deal
    # w/ the conflict.  If the CLs we're testing fail, then there is no
    # reason we can't try these again in the next run.
    self.changes_that_failed_to_apply_earlier = conflicting_changes or []

    # Private vars only used for pickling and self._build_log.
    self._overlays = overlays
    self._build_number = build_number
    self._builder_name = builder_name
    self._buildbucket_id = buildbucket_id

    # Set to False if the tree was not open when we acquired changes.
    self.tree_was_open = tree_was_open

    # A set of changes filtered by throttling, default to None.
    self.filtered_set = None

  def GetAppliedPatches(self):
    """Get the applied_patches instance.

    Returns:
      Return applied_patches (a patch_series.PatchSeries instance) if it's
      not None so we can reuse the cached Gerrit query results; else,
      create and return a patch_series.PatchSeries instance.
    """
    return self.applied_patches or patch_series.PatchSeries(
        self.build_root, helper_pool=self._helper_pool)

  def HasPickedUpCLs(self):
    """Returns True if this pool has picked up chump CLs or applied new CLs."""
    return self.has_chump_cls or self.applied

  @property
  def build_log(self):
    if self._buildbucket_id:
      return tree_status.ConstructLegolandBuildURL(self._buildbucket_id)

    if self._run:
      return tree_status.ConstructDashboardURL(
          self._run.GetWaterfall(), self._builder_name, self._build_number)

  @staticmethod
  def GetGerritHelpersForOverlays(overlays):
    """Discern the allowed GerritHelpers to use based on the given overlay."""
    cros_internal = cros = False
    if overlays in [constants.PUBLIC_OVERLAYS, constants.BOTH_OVERLAYS, False]:
      cros = True

    if overlays in [constants.PRIVATE_OVERLAYS, constants.BOTH_OVERLAYS]:
      cros_internal = True

    return patch_series.HelperPool.SimpleCreate(
        cros_internal=cros_internal, cros=cros)

  def __reduce__(self):
    """Used for pickling to re-create validation pool."""
    # NOTE: self._run is specifically excluded from the validation pool
    # pickle. We do not want the un-pickled validation pool to have a reference
    # to its own un-pickled BuilderRun instance. Instead, we want to to refer
    # to the new builder run's metadata instance. This is accomplished by
    # setting the BuilderRun at un-pickle time, in ValidationPool.Load(...).
    return (
        self.__class__,
        (
            self._overlays,
            self.build_root, self._build_number, self._builder_name,
            self.is_master, self.dryrun, self.candidates,
            self.non_manifest_changes,
            self.changes_that_failed_to_apply_earlier,
            self.pre_cq_trybot,
            self.tree_was_open,
            self.applied,
            self._buildbucket_id))

  @classmethod
  @failures_lib.SetFailureType(failures_lib.BuilderFailure)
  def AcquirePreCQPool(cls, *args, **kwargs):
    """See ValidationPool.__init__ for arguments."""
    kwargs.setdefault('tree_was_open', True)
    kwargs.setdefault('pre_cq_trybot', True)
    kwargs.setdefault('is_master', True)
    kwargs.setdefault('applied', [])
    pool = cls(*args, **kwargs)
    return pool

  @staticmethod
  def _WaitForQuery(query):
    """Helper method to return msg to print out when waiting for a |query|."""
    # Dictionary that maps CQ Queries to msg's to display.
    if query == constants.CQ_READY_QUERY:
      return 'new CLs'
    else:
      return 'waiting for tree to open'

  def AcquireChanges(self, gerrit_query, ready_fn, change_filter):
    """Helper method for AcquirePool. Adds changes to pool based on args.

    Queries gerrit using the given flags, filters out any unwanted changes, and
    handles draft changes.

    Args:
      gerrit_query: gerrit query to use.
      ready_fn: CR function (see constants).
      change_filter: If set, filters with change_filter(pool, changes,
        non_manifest_changes) to remove unwanted patches.
    """
    # Iterate through changes from all gerrit instances we care about.
    for helper in self._helper_pool:
      changes = helper.Query(gerrit_query, sort='lastUpdated')
      changes.reverse()
      logging.info('Queried changes: %s', cros_patch.GetChangesAsString(
          changes))

      if ready_fn:
        # The query passed in may include a dictionary of flags to use for
        # revalidating the query results. We need to do this because Gerrit
        # caches are sometimes stale and need sanity checking.
        changes = [x for x in changes if ready_fn(x)]
        logging.info('Ready changes: %s', cros_patch.GetChangesAsString(
            changes))

      # Tell users to publish drafts before marking them commit ready.
      for change in changes:
        if change.HasApproval('COMR', ('1', '2')) and change.IsDraft():
          self.HandleDraftChange(change)

      changes, non_manifest_changes = ValidationPool._FilterNonCrosProjects(
          changes, git.ManifestCheckout.Cached(self.build_root))
      self.candidates.extend(changes)
      self.non_manifest_changes.extend(non_manifest_changes)

    # Filter out unwanted changes.
    unfiltered_str = cros_patch.GetChangesAsString(self.candidates)
    self.candidates, self.non_manifest_changes = change_filter(
        self, self.candidates, self.non_manifest_changes)
    if self.candidates:
      filtered_str = cros_patch.GetChangesAsString(self.candidates)
      logging.info('Raw changes: %s', unfiltered_str)
      logging.info('Filtered changes: %s', filtered_str)

    return self.candidates or self.non_manifest_changes

  @classmethod
  def AcquirePool(cls, overlays, repo, build_number, builder_name,
                  buildbucket_id, query, dryrun=False, check_tree_open=True,
                  change_filter=None, builder_run=None):
    """Acquires the current pool from Gerrit.

    Polls Gerrit and checks for which changes are ready to be committed.
    Should only be called from master builders.

    Args:
      overlays: One of constants.VALID_OVERLAYS.
      repo: The repo used to sync, to filter projects, and to apply patches
        against.
      build_number: Corresponding build number for the build.
      builder_name: Builder name on buildbot dashboard.
      buildbucket_id: Buildbucket id of the current build as a string .
                      None if not buildbucket scheduled.
      query: constants.CQ_READY_QUERY, PRECQ_READY_QUERY, or a custom
        query description of the form (<query_str>, None).
      dryrun: Don't submit anything to gerrit.
      check_tree_open: If True, only return when the tree is open.
      change_filter: If set, use change_filter(pool, changes,
        non_manifest_changes) to filter out unwanted patches.
      builder_run: instance used to record CL actions to metadata and cidb.

    Returns:
      ValidationPool object.

    Raises:
      TreeIsClosedException: if the tree is closed (or throttled, if not
                             |THROTTLED_OK|).
    """
    if change_filter is None:
      change_filter = lambda _, x, y: (x, y)

    # We choose a longer wait here as we haven't committed to anything yet. By
    # doing this here we can reduce the number of builder cycles.
    timeout = cls.DEFAULT_TIMEOUT
    build_id, db = builder_run.GetCIDBHandle()
    if db:
      time_to_deadline = db.GetTimeToDeadline(build_id)
      if time_to_deadline is not None:
        # We must leave enough time before the deadline to allow us to extend
        # the deadline in case we hit this timeout.
        timeout = time_to_deadline - cls.EXTENSION_TIMEOUT_BUFFER

    end_time = time.time() + timeout
    status = constants.TREE_OPEN

    while True:
      current_time = time.time()
      time_left = end_time - current_time
      # Wait until the tree becomes open.
      if check_tree_open:
        try:
          status = tree_status.WaitForTreeStatus(
              period=cls.SLEEP_TIMEOUT, timeout=time_left,
              throttled_ok=cls.THROTTLED_OK)
        except timeout_util.TimeoutError:
          raise TreeIsClosedException(
              closed_or_throttled=not cls.THROTTLED_OK)

      # Sync so that we are up-to-date on what is committed.
      repo.Sync()

      gerrit_query, ready_fn = query
      tree_was_open = (status == constants.TREE_OPEN)

      try:
        experimental_builders = tree_status.GetExperimentalBuilders()
        builder_run.attrs.metadata.UpdateWithDict({
            constants.METADATA_EXPERIMENTAL_BUILDERS: experimental_builders
        })
      except timeout_util.TimeoutError:
        logging.error('Timeout getting experimental builders from the tree'
                      'status.')

      pool = ValidationPool(
          overlays=overlays,
          build_root=repo.directory,
          build_number=build_number,
          builder_name=builder_name,
          is_master=True,
          dryrun=dryrun,
          builder_run=builder_run,
          tree_was_open=tree_was_open,
          applied=[],
          buildbucket_id=buildbucket_id)

      if pool.AcquireChanges(gerrit_query, ready_fn, change_filter):
        break

      if dryrun or time_left < 0 or cls.ShouldExitEarly():
        break

      # Iterated through all queries with no changes.
      logging.info('Waiting for %s (%d minutes left)...',
                   cls._WaitForQuery(query), time_left / 60)
      time.sleep(cls.SLEEP_TIMEOUT)

    return pool

  def _GetFailStreak(self):
    """Returns the fail streak for the validation pool.

    Queries CIDB for the last CQ_SEARCH_HISTORY builds from the current build_id
    and returns how many of them haven't passed in a row. This is used for
    tree throttled validation pool logic.
    """
    # TODO(sosa): Remove Google Storage Fail Streak Counter.
    build_id, db = self._run.GetCIDBHandle()
    if not db:
      return 0

    builds = db.GetBuildHistory(self._run.config.name,
                                ValidationPool.CQ_SEARCH_HISTORY,
                                ignore_build_id=build_id)
    number_of_failures = 0
    # Iterate through the ordered list of builds until you get one that is
    # passed.
    for build in builds:
      if build['status'] != constants.BUILDER_STATUS_PASSED:
        number_of_failures += 1
      else:
        break

    return number_of_failures

  def AddPendingCommitsIntoPool(self, manifest):
    """Add the pending commits from |manifest| into pool.

    Args:
      manifest: path to the manifest.
    """
    manifest_dom = minidom.parse(manifest)
    pending_commits = manifest_dom.getElementsByTagName(
        lkgm_manager.PALADIN_COMMIT_ELEMENT)
    for pc in pending_commits:
      attr_names = cros_patch.ALL_ATTRS
      attr_dict = {}
      for name in attr_names:
        attr_dict[name] = pc.getAttribute(name)
      patch = cros_patch.GerritFetchOnlyPatch.FromAttrDict(attr_dict)

      self.candidates.append(patch)

  @classmethod
  def AcquirePoolFromManifest(cls, manifest, overlays, repo, build_number,
                              builder_name, buildbucket_id, is_master, dryrun,
                              builder_run=None):
    """Acquires the current pool from a given manifest.

    This function assumes that you have already synced to the given manifest.

    Args:
      manifest: path to the manifest where the pool resides.
      overlays: One of constants.VALID_OVERLAYS.
      repo: The repo used to filter projects and to apply patches against.
      build_number: Corresponding build number for the build.
      builder_name: Builder name on buildbot dashboard.
      buildbucket_id: Buildbucket id of the current build as a string .
                      None if not buildbucket scheduled.
      is_master: Boolean that indicates whether this is a pool for a master.
        config or not.
      dryrun: Don't submit anything to gerrit.
      builder_run: BuilderRun instance used to record CL actions to metadata and
        cidb.

    Returns:
      ValidationPool object.
    """
    pool = ValidationPool(
        overlays=overlays,
        build_root=repo.directory,
        build_number=build_number,
        builder_name=builder_name,
        is_master=is_master,
        dryrun=dryrun,
        buildbucket_id=buildbucket_id,
        builder_run=builder_run,
        applied=[])
    pool.AddPendingCommitsIntoPool(manifest)
    return pool

  @classmethod
  def ShouldExitEarly(cls):
    """Return whether we should exit early.

    This function is intended to be overridden by tests or by subclasses.
    """
    return False

  @staticmethod
  def _FilterNonCrosProjects(changes, manifest):
    """Filters changes to a tuple of relevant changes.

    There are many code reviews that are not part of Chromium OS and/or
    only relevant on a different branch. This method returns a tuple of (
    relevant reviews in a manifest, relevant reviews not in the manifest). Note
    that this function must be run while chromite is checked out in a
    repo-managed checkout.

    Args:
      changes: List of GerritPatch objects.
      manifest: The manifest to check projects/branches against.

    Returns:
      Tuple of (relevant reviews in a manifest,
                relevant reviews not in the manifest).
    """

    def IsCrosReview(change):
      return (change.project.startswith('chromiumos/') or
              change.project.startswith('chromeos/') or
              change.project.startswith('aosp/') or
              change.project.startswith('weave/'))

    # First we filter to only Chromium OS repositories.
    changes = [c for c in changes if IsCrosReview(c)]

    changes_in_manifest = []
    changes_not_in_manifest = []
    for change in changes:
      # TODO: temp log to debug crbug.com/661704
      logging.info('Checking for change %s', change)
      if change.GetCheckout(manifest, strict=False):
        logging.info('Found manifest change %s', change)
        changes_in_manifest.append(change)
      elif change.IsMergeable():
        logging.info('Found non-manifest change %s', change)
        changes_not_in_manifest.append(change)
      else:
        logging.info('Non-manifest change %s is not commit ready yet', change)

    return changes_in_manifest, changes_not_in_manifest

  def _FilterDependencyErrors(self, errors):
    """Filter out ignorable DependencyError exceptions.

    If a dependency isn't marked as ready, or a dependency fails to apply,
    we only complain after REJECTION_GRACE_PERIOD has passed since the patch
    was uploaded.

    This helps in three situations:
      1) crbug.com/705023: if the error is a DependencyError, and the dependent
         CL was filtered by a throttled tree, do not reject the CL set.
      2) If the developer is in the middle of marking a stack of changes as
         ready, we won't reject their work until the grace period has passed.
      3) If the developer marks a big circular stack of changes as ready, and
         some change in the middle of the stack doesn't apply, the user will
         get a chance to rebase their change before we mark all the changes as
         'not ready'.

    This function filters out dependency errors that can be ignored due to
    the grace period.

    Args:
      errors: List of exceptions to filter.

    Returns:
      List of unfiltered exceptions.
    """
    reject_timestamp = time.time() - self.REJECTION_GRACE_PERIOD
    results = []
    for error in errors:
      results.append(error)

      if self.filtered_set and isinstance(error, cros_patch.DependencyError):
        root_error = error.GetRootError()
        if (isinstance(root_error, patch_series.PatchNotEligible) and
            root_error.patch in self.filtered_set):
          logging.info('Ignoring dependency errors for %s as its dependency '
                       'change %s was filtered out by throttling.', error.patch,
                       root_error.patch)
          results.pop()
          continue

      is_ready = error.patch.HasReadyFlag()
      if not is_ready or reject_timestamp < error.patch.approval_timestamp:
        while error is not None:
          if isinstance(error, cros_patch.DependencyError):
            if is_ready:
              logging.info('Ignoring dependency errors for %s due to grace '
                           'period', error.patch)
            else:
              logging.info('Ignoring dependency errors for %s until it is '
                           'marked trybot ready or commit ready', error.patch)
            results.pop()
            break
          error = getattr(error, 'error', None)
    return results

  @classmethod
  def PrintLinksToChanges(cls, changes):
    """Print links to the specified |changes|.

    This method prints a link to list of |changes| by using the
    information stored in |changes|. It should not attempt to query
    Google Storage or Gerrit.

    Args:
      changes: A list of cros_patch.GerritPatch instances to generate
        transactions for.
    """
    def SortKeyForChanges(change):
      return (-change.total_fail_count, -change.fail_count,
              os.path.basename(change.project), change.gerrit_number)

    # Now, sort and print the changes.
    for change in sorted(changes, key=SortKeyForChanges):
      project = os.path.basename(change.project)
      gerrit_number = cros_patch.AddPrefix(change, change.gerrit_number)
      author = change.owner
      # Show the owner, unless it's a non-standard email address.
      if (change.owner_email and
          not (change.owner_email.endswith(constants.GOOGLE_EMAIL) or
               change.owner_email.endswith(constants.CHROMIUM_EMAIL))):
        # We cannot print '@' in the link because it is used to separate
        # the display text and the URL by the buildbot annotator.
        author = change.owner_email.replace('@', '-AT-')

      s = '%s | %s | %s' % (project, author, gerrit_number)

      # Print a count of how many times a given CL has failed the CQ.
      if change.total_fail_count:
        s += ' | fails:%d' % (change.fail_count,)
        if change.total_fail_count > change.fail_count:
          s += '(%d)' % (change.total_fail_count,)

      # Add a note if the latest patchset has already passed the CQ.
      if change.pass_count > 0:
        s += ' | passed:%d' % change.pass_count

      # Add the subject line of the commit message.
      if change.commit_message:
        s += ' | %s' % cros_build_lib.TruncateStringToLine(
            change.commit_message, 80)

      logging.PrintBuildbotLink(s, change.url)

  def FilterChangesForThrottledTree(self):
    """Apply Throttled Tree logic to select patch candidates.

    This should be called before any CLs are applied.

    If the tree is throttled, we only test a random subset of our candidate
    changes. Call this to select that subset, and throw away unrelated changes.

    If the three was open when this pool was created, it does nothing.
    """
    if self.tree_was_open:
      return

    # By filtering the candidates before any calls to Apply, we can make sure
    # that repeated calls to Apply always consider the same list of candidates.
    fail_streak = self._GetFailStreak()
    test_pool_size = max(1, int(len(self.candidates) / (1.5**fail_streak)))
    random.shuffle(self.candidates)

    removed = self.candidates[test_pool_size:]
    if removed:
      self.filtered_set = set(removed)
      logging.info('As the tree is throttled, it only picks a random subset of '
                   'candidate changes. Changes not picked up in this run: %s ',
                   cros_patch.GetChangesAsString(removed))

    self.candidates = self.candidates[:test_pool_size]

  def ApplyPoolIntoRepo(self, manifest=None, filter_fn=lambda p: True):
    """Applies changes from pool into the directory specified by the buildroot.

    This method applies changes in the order specified. If the build
    is running as the master, it also respects the dependency
    order. Otherwise, the changes should already be listed in an order
    that will not break the dependency order.

    It is safe to call this method more than once, probably with different
    filter functions. A given patch will never be applied more than  once.

    Args:
      manifest: A manifest object to use for mapping projects to repositories.
      filter_fn: Takes a patch argument, returns bool for 'should apply'.

    Returns:
      True if we managed to apply any changes.
    """
    # applied is a list of applied GerritPatch instances.
    # failed_tot and failed_inflight are lists of PatchException instances.
    applied = []
    failed_tot = []
    failed_inflight = []

    self.applied_patches = patch_series.PatchSeries(
        self.build_root, helper_pool=self._helper_pool)

    if self.is_master:
      try:
        candidates = [c for c in self.candidates if
                      c not in self.applied and filter_fn(c)]

        # pylint: disable=E1123
        applied, failed_tot, failed_inflight = self.applied_patches.Apply(
            candidates, manifest=manifest)
      except (KeyboardInterrupt, RuntimeError, SystemExit):
        raise
      except Exception as e:
        msg = (
            'Unhandled exception occurred while applying changes: %s\n\n'
            'To be safe, we have kicked out all of the CLs, so that the '
            'commit queue does not go into an infinite loop retrying '
            'patches.' % (e,)
        )
        links = cros_patch.GetChangesAsString(self.candidates)
        logging.error('%s\nAffected Patches are: %s', msg, links)
        errors = [InternalCQError(patch, msg) for patch in self.candidates]
        self._HandleApplyFailure(errors)
        raise

      _, db = self._run.GetCIDBHandle()
      if db:
        action_history = db.GetActionsForChanges(applied)
        for change in applied:
          change.total_fail_count = clactions.GetCLActionCount(
              change, CQ_PIPELINE_CONFIGS, constants.CL_ACTION_KICKED_OUT,
              action_history, latest_patchset_only=False)
          change.fail_count = clactions.GetCLActionCount(
              change, CQ_PIPELINE_CONFIGS, constants.CL_ACTION_KICKED_OUT,
              action_history)
          change.pass_count = clactions.GetCLActionCount(
              change, CQ_PIPELINE_CONFIGS, constants.CL_ACTION_SUBMIT_FAILED,
              action_history)

    else:
      # Slaves do not need to create transactions and should simply
      # apply the changes serially, based on the order that the
      # changes were listed on the manifest.
      self.applied_patches.FetchChanges(self.candidates, manifest=manifest)
      for change in self.candidates:
        try:
          # pylint: disable=E1123
          self.applied_patches.ApplyChange(change, manifest=manifest)
        except cros_patch.PatchException as e:
          # Fail if any patch cannot be applied.
          self._HandleApplyFailure([InternalCQError(change, e)])
          raise
        else:
          applied.append(change)

    self.RecordPatchesInMetadataAndDatabase(applied)
    self.PrintLinksToChanges(applied)

    if self.is_master and not self.pre_cq_trybot:
      inputs = [[change, self.build_log] for change in applied]
      parallel.RunTasksInProcessPool(self.HandleApplySuccess, inputs)

    # We only filter out dependency errors in the CQ and Pre-CQ masters.
    # On Pre-CQ trybots, we want to reject patches immediately, because
    # otherwise the pre-cq master will think we just dropped the patch
    # on the floor and never tested it.
    if not self.pre_cq_trybot:
      failed_tot = self._FilterDependencyErrors(failed_tot)
      failed_inflight = self._FilterDependencyErrors(failed_inflight)

    if failed_tot:
      logging.info(
          'The following changes could not cleanly be applied to ToT: %s',
          ' '.join([c.patch.id for c in failed_tot]))
      self._HandleApplyFailure(failed_tot)

    if failed_inflight:
      logging.info(
          'The following changes could not cleanly be applied against the '
          'current stack of patches; if this stack fails, they will be tried '
          'in the next run.  Inflight failed changes: %s',
          ' '.join([c.patch.id for c in failed_inflight]))
      for x in failed_inflight:
        self._HandleFailedToApplyDueToInflightConflict(x.patch)

    self.changes_that_failed_to_apply_earlier.extend(failed_inflight)
    self.applied.extend(applied)

    return bool(self.applied)

  def GetDependMapForChanges(self, changes, patches):
    """Get a dependency map for changes.

    Generate and return a dict mapping each change to a set of changes which
    depend on this change.
    For instance, say "A -> B" means "A depends on B"
    Suppose we have changes:
    A -> B -> C

    D -> E -> F
         ^
         |
         G

    H -> I (mutual dependency)
    |    |
      <-

    We return the map:
    {B : {A},
     C : {A, B},
     E : {D, G},
     F : {D, E, G}
     H : {I},
     I : {H}}

    Args:
      changes: A list of changes to parse to generate the dependency map.
      patches: A patch_series.PatchesSeries instance to get patch dependency.

    Returns:
      A dict mapping a change (patch.GerritPatch instance) to a set of changes
      (patch.GerritPatch instances) depending on this change.
    """
    # 1. We want the set of nodes S = {x : x has a path to n in G}.
    # 2. There is a path from x to n in G if and only if there is a path from
    #    n to x in flip(G).
    # 3. So S = {x : x has a path to n in G}
    #         = {x : n has a path to x in flip(G)}
    logging.info('Computing dependency map for changes: %s',
                 cros_patch.GetChangesAsString(changes))
    flipped_graph = {}
    for change in changes:
      gerrit_deps, cq_deps = patches.GetDepChangesForChange(change)
      for dep in gerrit_deps + cq_deps:
        # Maps each change to the changes directly depending on it.
        flipped_graph.setdefault(dep, set()).add(change)

    try:
      return self.GetTransitiveDependMap(changes, flipped_graph)
    except timeout_util.TimeoutError as e:
      logging.error('Timeout error at getting transitive dependency map for '
                    'changes: %s', e)

    return flipped_graph

  @timeout_util.TimeoutDecorator(COMPUTE_DEPENDENCY_MAP_TIMEOUT)
  def GetTransitiveDependMap(self, changes, flipped_graph):
    """Get the transitive dependency map for given changes.

    Args:
      changes: A list of changes to parse to generate the dependency map.
      flipped_graph: A dict mapping a change (patch.GerritPatch instance) to a
        set of changes (patch.GerritPatch instances) directly depending on it.

    Returns:
      A dict mapping a change (patch.GerritPatch instance) to a set of changes
      (patch.GerritPatch instances) directly or indirectly depending on it.
    """
    transitive_dependency_map = {}
    for change in changes:
      logging.info('Getting transitive dependency map for change: %s ',
                   change.PatchLink())
      # Update dependency_map to map each change to all changes directly or
      # indirectly depending on it.
      visited = self._DepthFirstSearch(flipped_graph, change)
      visited.remove(change)
      if visited:
        transitive_dependency_map[change] = visited

    return transitive_dependency_map

  def _DepthFirstSearch(self, graph, node):
    """Returns a set of nodes reachable from a node in a graph.

    Performs depth-first-search, keeping track of a set of visited nodes to
    avoid exponential blowup from diamond-shaped graphs, and to avoid infinite
    loops from cycles. Returns the set of visited nodes, including the start
    node.

    Args:
      graph: The graph as an adjacency map. It maps nodes to a collection of
             their neighbors.
      node: The current node we are at.

    Returns:
      A set of nodes reachable from the start node.
    """
    visited = set()
    visiting = [node]
    while visiting:
      node = visiting.pop()
      visited.add(node)
      children = graph.get(node, set())
      # Don't re-visit nodes, or the algorithm becomes exponential-time.
      visiting.extend(children - visited)

    return visited


  @staticmethod
  def Load(filename, builder_run=None):
    """Loads the validation pool from the file.

    Args:
      filename: path of file to load from.
      builder_run: BuilderRun instance to use in unpickled validation pool, used
        for fetching cidb handle for access to metadata.
    """
    with open(filename, 'rb') as p_file:
      pool = cPickle.load(p_file)
      # pylint: disable=protected-access
      pool._run = builder_run
      return pool

  def Save(self, filename):
    """Serializes the validation pool."""
    with open(filename, 'wb') as p_file:
      cPickle.dump(self, p_file, protocol=cPickle.HIGHEST_PROTOCOL)

  # Note: All submit code, all gerrit code, and basically everything other
  # than patch resolution/applying needs to use .change_id from patch objects.
  # Basically all code from this point forward.
  def _SubmitChangeWithDeps(self, patches, change, errors, limit_to,
                            reason=None):
    """Submit |change| and its dependencies via Gerrit Submit API.

    This method is only used for non-manifest changes.

    If you call this function multiple times with the same PatchSeries, each
    CL will only be submitted once.

    Args:
      patches: A patch_series.PatchSeries object.
      change: The change (a GerritPatch object) to submit.
      errors: A dictionary. This dictionary should contain all patches that have
        encountered errors, and map them to the associated exception object.
      limit_to: The list of patches that were approved by this CQ run. We will
        only consider submitting patches that are in this list.
      reason: string reason for submission to be recorded in cidb. (Should be
        None or constant with name STRATEGY_* from constants.py)

    Returns:
      A copy of the errors object. If new errors have occurred while submitting
      this change, and those errors have prevented a change from being
      submitted, they will be added to the errors object.
    """
    # Find out what patches we need to submit.
    errors = errors.copy()
    try:
      plan = patches.CreateTransaction(change, limit_to=limit_to)
    except cros_patch.PatchException as e:
      errors[change] = e
      return errors

    submitted = []
    dep_error = None
    for dep_change in plan:
      # Has this change failed to submit before?
      dep_error = errors.get(dep_change)
      if dep_error is not None:
        break

    if dep_error is None:
      for dep_change in plan:
        try:
          success = self._SubmitChangeUsingGerrit(dep_change, reason=reason)
          if success or self.dryrun:
            submitted.append(dep_change)
        except (gob_util.GOBError, gerrit.GerritException) as e:
          if (isinstance(e, gob_util.GOBError) and
              e.http_status == httplib.CONFLICT):
            if e.reason == gob_util.GOB_ERROR_REASON_CLOSED_CHANGE:
              submitted.append(dep_change)
            else:
              dep_error = PatchConflict(dep_change)
          else:
            dep_error = PatchFailedToSubmit(dep_change, str(e))

        if dep_change not in submitted:
          if dep_error is None:
            msg = self.INCONSISTENT_SUBMIT_MSG
            dep_error = PatchFailedToSubmit(dep_change, msg)

          # Log any errors we saw.
          logging.error('%s', dep_error)
          errors[dep_change] = dep_error
          break

    if (dep_error is not None and change not in errors and
        change not in submitted):
      # One of the dependencies failed to submit. Report an error.
      errors[change] = cros_patch.DependencyError(change, dep_error)

    # Track submitted patches so that we don't submit them again.
    patches.InjectCommittedPatches(submitted)

    # Look for incorrectly submitted patches. We only have this problem
    # when we have a dependency cycle, and we submit one change before
    # realizing that a later change cannot be submitted. For now, just
    # print an error message and notify the developers.
    #
    # If you see this error a lot, consider implementing a best-effort
    # attempt at reverting changes.
    for submitted_change in submitted:
      gdeps, pdeps = patches.GetDepChangesForChange(submitted_change)
      for dep in gdeps + pdeps:
        dep_error = errors.get(dep)
        if dep_error is not None:
          error = PatchSubmittedWithoutDeps(submitted_change, dep_error)
          self._HandleIncorrectSubmission(error)
          logging.error('%s was erroneously submitted.', submitted_change)
          break

    return errors

  def SubmitChanges(self, verified_cls, check_tree_open=True,
                    throttled_ok=True):
    """Submits the given changes to Gerrit.

    Args:
      verified_cls: A dictionary mapping the fully verified changes to their
        string reasons for submission.
      check_tree_open: Whether to check that the tree is open before submitting
        changes. If this is False, TreeIsClosedException will never be raised.
      throttled_ok: if |check_tree_open|, treat a throttled tree as open

    Returns:
      (submitted, errors) where submitted is a set of changes that were
      submitted, and errors is a map {change: error} containing changes that
      failed to submit.

    Raises:
      TreeIsClosedException: if the tree is closed.
    """
    assert self.is_master, 'Non-master builder calling SubmitPool'
    assert not self.pre_cq_trybot, 'Trybot calling SubmitPool'

    # TODO(pprabhu) It is bad form for master-paladin to do work after its
    # deadline has passed. Extend the deadline after waiting for slave
    # completion and ensure that none of the follow up stages go beyond the
    # deadline.
    if (check_tree_open and not self.dryrun and not
        tree_status.IsTreeOpen(period=self.SLEEP_TIMEOUT,
                               timeout=self.DEFAULT_TIMEOUT,
                               throttled_ok=throttled_ok)):
      raise TreeIsClosedException(
          closed_or_throttled=not throttled_ok)

    changes = verified_cls.keys()
    # Filter out changes that were modified during the CQ run.
    filtered_changes, errors = self.FilterModifiedChanges(changes)

    patches = patch_series.PatchSeries(
        self.build_root, helper_pool=self._helper_pool, is_submitting=True)

    patches.InjectLookupCache(filtered_changes)

    # Partition the changes into local changes and remote changes.  Local
    # changes have a local repository associated with them, so we can do a
    # batched git push for them.  Remote changes must be submitted via Gerrit.
    by_repo_cls = {}
    for change in filtered_changes:
      by_repo_cls.setdefault(
          patches.GetGitRepoForChange(change, strict=False), set()
          ).add(change)
    remote_changes = {c:verified_cls[c] for c in by_repo_cls.pop(None, set())}

    by_repo_cls, reapply_errors = patches.ReapplyChanges(by_repo_cls)

    # Map the changes in by_repo_cls to their submission reasons.
    by_repo = dict()
    for repo, cls in by_repo_cls.iteritems():
      by_repo[repo] = {cl:verified_cls[cl] for cl in cls}

    submitted_locals, local_submission_errors = self.SubmitLocalChanges(
        by_repo)
    submitted_remotes, remote_errors = self.SubmitRemoteChanges(
        patches, remote_changes)

    errors.update(reapply_errors)
    errors.update(local_submission_errors)
    errors.update(remote_errors)
    for patch, error in errors.iteritems():
      logging.error("Could not submit %s, error: %s", patch, error)
      logging.PrintBuildbotLink(
          "Could not submit %s, error: %s" % (patch, error), patch.url)
      self._HandleCouldNotSubmit(patch, error)

    return submitted_locals | submitted_remotes, errors

  def SubmitRemoteChanges(self, patches, verified_cls):
    """Submits non-manifest changes via Gerrit.

    This function first splits the patches into disjoint transactions so that we
    can submit in parallel. We merge together changes to the same project into
    the same transaction because it helps avoid Gerrit performance problems
    (Gerrit chokes when two people hit submit at the same time in the same
    project).

    Args:
      patches: patch_series.PatchSeries instance associated with the changes.
      verified_cls: A dictionary mapping changes to their submission reasons.

    Returns:
      (submitted, errors) where submitted is a set of changes that were
      submitted, and errors is a map {change: error} containing changes that
      failed to submit.
    """
    changes = verified_cls.keys()
    plans, failed = patches.CreateDisjointTransactions(
        changes, merge_projects=True)
    errors = {}
    for error in failed:
      errors[error.patch] = error

    # Submit each disjoint transaction in parallel.
    with parallel.Manager() as manager:
      p_errors = manager.dict(errors)
      def _SubmitPlan(*plan):
        for change in plan:
          p_errors.update(self._SubmitChangeWithDeps(
              patches, change, dict(p_errors),
              plan, reason=verified_cls[change]))
      parallel.RunTasksInProcessPool(_SubmitPlan, plans, processes=4)

      submitted_changes = set(changes) - set(p_errors.keys())
      return (submitted_changes, dict(p_errors))

  def SubmitLocalChanges(self, by_repo):
    """Submit a set of local changes, i.e. changes which are in the manifest.

    Precondition: we must have already checked that all the changes are
    submittable, such as having a +2 in Gerrit.

    Args:
      by_repo: A mapping from repo paths to a dictionary contains changes to
        that repo and their corresponding submission reasons.

    Returns:
      (submitted, errors) where submitted is a set of changes that were
      submitted, and errors is a map {change: error} containing changes that
      failed to submit.
    """
    merged_errors = {}
    submitted = set()
    for repo, verified_cls in by_repo.iteritems():
      changes, errors = self._SubmitRepo(repo, verified_cls)
      submitted |= set(changes)
      merged_errors.update(errors)
    return submitted, merged_errors

  def _SubmitRepo(self, repo, verified_cls):
    """Submit a sequence of changes from the same repository.

    The changes must be from a repository that is checked out locally, we can do
    a single git push, and then verify that Gerrit updated its metadata for each
    patch.

    Args:
      repo: the path to the repository containing the changes
      verified_cls: a dictionary mapping changes from a single repository to
        their submission reasons.

    Returns:
      (submitted, errors) where submitted is a set of changes that were
      submitted, and errors is a map {change: error} containing changes that
      failed to submit.
    """
    changes = verified_cls.keys()
    branches = set((change.tracking_branch,) for change in changes)
    push_branch = functools.partial(self.PushRepoBranch, repo, changes)
    push_results = parallel.RunTasksInProcessPool(push_branch, branches)

    sha1s = {}
    errors = {}
    for sha1s_for_branch, branch_errors in filter(bool, push_results):
      sha1s.update(sha1s_for_branch)
      errors.update(branch_errors)

    for change in changes:
      push_success = change not in errors
      self._CheckChangeWasSubmitted(change, push_success,
                                    reason=verified_cls[change],
                                    sha1=sha1s.get(change))

    return set(changes) - set(errors), errors

  def PushRepoBranch(self, repo, changes, branch):
    """Pushes a branch of a repo to the remote.

    Args:
      repo: the path to the repository containing the changes
      changes: a sequence of changes from a single branch of a repository.
      branch: the tracking branch name.

    Returns:
      (sha1, errors) where sha1s is a mapping from changes to their sha1s, and
      errors is a map {change: error} containing changes that failed to submit.
    """

    project_url = next(iter(changes)).project_url
    remote_ref = git.GetTrackingBranch(repo)
    push_to = git.RemoteRef(project_url, branch)

    use_merge = any(c.IsMerge(repo) for c in changes)

    for _ in range(3):
      # try to resync and push.
      try:
        git.SyncPushBranch(repo, remote_ref.remote, remote_ref.ref,
                           use_merge=use_merge, print_cmd=True)
      except cros_build_lib.RunCommandError:
        # TODO(phobbs) parse the sync failure output and find which change was
        # at fault.
        logging.error('git rebase failed for %s:%s; it is likely that a change '
                      'was chumped in the middle of the CQ run.',
                      repo, branch, exc_info=True)
        break

      try:
        git.GitPush(repo, 'HEAD', push_to, skip=self.dryrun, print_cmd=True)
        return {}
      except cros_build_lib.RunCommandError:
        logging.warn('git push failed for %s:%s; was a change chumped in the '
                     'middle of the CQ run?',
                     repo, branch, exc_info=True)

    errors = dict(
        (change, PatchFailedToSubmit(change, 'Failed to push to %s'))
        for change in changes)

    sha1s = dict(
        (change, change.GetLocalSHA1(repo, remote_ref.ref))
        for change in changes)

    return sha1s, errors

  def RecordPatchesInMetadataAndDatabase(self, changes):
    """Mark all patches as having been picked up in metadata.json and cidb.

    If self._run is None, then this function does nothing.
    """
    if not self._run:
      return

    metadata = self._run.attrs.metadata
    timestamp = int(time.time())

    for change in changes:
      metadata.RecordCLAction(change, constants.CL_ACTION_PICKED_UP,
                              timestamp)
      # TODO(akeshet): If a separate query for each insert here becomes
      # a performance issue, consider batch inserting all the cl actions
      # with a single query.
      self._InsertCLActionToDatabase(change, constants.CL_ACTION_PICKED_UP)

  @classmethod
  def FilterModifiedChanges(cls, changes):
    """Filter out changes that were modified while the CQ was in-flight.

    Args:
      changes: A list of changes (as PatchQuery objects).

    Returns:
      This returns a tuple (unmodified_changes, errors).

      unmodified_changes: A reloaded list of changes, only including mergeable,
                          unmodified and unsubmitted changes.
      errors: A dictionary. This dictionary will contain all patches that have
        encountered errors, and map them to the associated exception object.
    """
    # Reload all of the changes from the Gerrit server so that we have a
    # fresh view of their approval status. This is needed so that our filtering
    # that occurs below will be mostly up-to-date.
    unmodified_changes, errors = [], {}
    reloaded_changes = list(cls.ReloadChanges(changes))
    old_changes = cros_patch.PatchCache(changes)

    if list(changes) != list(reloaded_changes):
      logging.error('Changes: %s', map(str, changes))
      logging.error('Reloaded changes: %s', map(str, reloaded_changes))
      for change in set(changes) - set(reloaded_changes):
        logging.error('%s disappeared after reloading', change)
      for change in set(reloaded_changes) - set(changes):
        logging.error('%s appeared after reloading', change)
      raise InconsistentReloadException()

    for reloaded_change in reloaded_changes:
      old_change = old_changes[reloaded_change]
      if reloaded_change.IsAlreadyMerged():
        logging.warning('%s is already merged. It was most likely chumped '
                        'during the current CQ run.', reloaded_change)
      elif reloaded_change.patch_number != old_change.patch_number:
        # If users upload new versions of a CL while the CQ is in-flight, then
        # their CLs are no longer tested. These CLs should be rejected.
        errors[old_change] = PatchModified(reloaded_change,
                                           reloaded_change.patch_number)
      elif not reloaded_change.IsMergeable():
        # Get the reason why this change is not mergeable anymore.
        errors[old_change] = reloaded_change.GetMergeException()
        errors[old_change].patch = old_change
      else:
        unmodified_changes.append(old_change)

    return unmodified_changes, errors

  @classmethod
  def ReloadChanges(cls, changes):
    """Reload the specified |changes| from the server.

    Args:
      changes: A list of PatchQuery objects.

    Returns:
      A list of GerritPatch objects.
    """
    return gerrit.GetGerritPatchInfoWithPatchQueries(changes)

  def _SubmitChangeUsingGerrit(self, change, reason=None):
    """Submits patch using Gerrit Review.

    This uses the Gerrit "submit" API, then waits for the patch to move out of
    "NEW" state, ideally into "MERGED" status.  It records in CIDB whether the
    Gerrit's status != "NEW".

    Args:
      change: GerritPatch to submit.
      reason: string reason for submission to be recorded in cidb. (Should be
        None or constant with name STRATEGY_* from constants.py)

    Returns:
      Whether the push succeeded, indicated by Gerrit review status not being
      "NEW".
    """
    logging.info('Change %s will be submitted', change)
    self._helper_pool.ForChange(change).SubmitChange(
        change, dryrun=self.dryrun)
    return self._CheckChangeWasSubmitted(change, True, reason)

  def _CheckChangeWasSubmitted(self, change, push_success, reason, sha1=None):
    """Confirms that a change is in "submitted" state in Gerrit.

    First, we force Gerrit to double-check whether the change has been merged,
    then we poll Gerrit until either the change is merged or we timeout. Then,
    we update cidb with information about whether the change was pushed
    successfully.

    Args:
      change: The change to check
      push_success: Whether we were successful in pushing the change.
      reason: string reason for submission to be recorded in cidb. (Should be
        None or constant with name STRATEGY_* from constants.py)
      sha1: Optional hint to Gerrit about what sha1 the pushed commit has.

    Returns:
      Whether the push succeeded and the Gerrit review is not in "NEW" state.
      Ideally it would be in "MERGED" state, but it is safe to proceed with it
      only in "SUBMITTED" state.
    """
    # TODO(phobbs): Use a helper process to check that Gerrit marked the change
    # as merged asynchronously.
    helper = self._helper_pool.ForChange(change)

    # Force Gerrit to check whether the change is merged.
    gob_util.CheckChange(helper.host, change.gerrit_number, sha1=sha1)

    updated_change = helper.QuerySingleRecord(change.gerrit_number)
    if push_success and updated_change.status == 'SUBMITTED':
      def _Query():
        return helper.QuerySingleRecord(change.gerrit_number)
      def _Retry(value):
        return value and value.status == 'SUBMITTED'

      # If we succeeded in pushing but the change is 'NEW' give gerrit some time
      # to resolve that to 'MERGED' or fail outright.
      try:
        updated_change = timeout_util.WaitForSuccess(
            _Retry, _Query, timeout=SUBMITTED_WAIT_TIMEOUT, period=1)
      except timeout_util.TimeoutError:
        # The change really is stuck on submitted, not merged, then.
        logging.warning('Timed out waiting for gerrit to notice that we'
                        ' submitted change %s, but status is still "%s".',
                        change.gerrit_number_str, updated_change.status)
        helper.SetReview(change, msg='This change was pushed, but we timed out'
                         'waiting for Gerrit to notice that it was submitted.')

    if push_success and not updated_change.status == 'MERGED':
      logging.warning(
          'Change %s was pushed without errors, but gerrit is'
          ' reporting it with status "%s" (expected "MERGED").',
          change.gerrit_number_str, updated_change.status)
      if updated_change.status == 'SUBMITTED':
        # So far we have never seen a SUBMITTED CL that did not eventually
        # transition to MERGED.  If it is stuck on SUBMITTED treat as MERGED.
        logging.info('Proceeding now with the assumption that change %s'
                     ' will eventually transition to "MERGED".',
                     change.gerrit_number_str)
      else:
        logging.error('Gerrit likely was unable to merge change %s.',
                      change.gerrit_number_str)

    succeeded = push_success and (updated_change.status != 'NEW')
    if self._run:
      self._RecordSubmitInCIDB(change, succeeded, reason)
    return succeeded

  def _RecordSubmitInCIDB(self, change, succeeded, reason):
    """Records in CIDB whether the submit succeeded."""
    action = (constants.CL_ACTION_SUBMITTED if succeeded
              else constants.CL_ACTION_SUBMIT_FAILED)

    metadata = self._run.attrs.metadata
    timestamp = int(time.time())
    metadata.RecordCLAction(change, action, timestamp)
    # NOTE(akeshet): The same |reason| will be recorded, regardless of whether
    # the change was submitted successfully or unsuccessfully. This is
    # probably what we want, because it gives us a way to determine why we
    # tried to submit changes that failed to submit.
    self._InsertCLActionToDatabase(change, action, reason)

  def RemoveReady(self, change, reason=None):
    """Remove the commit ready and trybot ready bits for |change|.

    Args:
      change: An instance of cros_patch.GerritPatch.
      reason: The reason to remove the ready bit for the |change|. None by
        default.
    """
    try:
      self._helper_pool.ForChange(change).RemoveReady(
          change, dryrun=self.dryrun)
    except gob_util.GOBError as e:
      if (e.http_status == httplib.CONFLICT and
          e.reason == gob_util.GOB_ERROR_REASON_CLOSED_CHANGE):
        logging.warning('The change is closed. Ignore the GOB CONFLICT error.')
      else:
        raise

    if self._run:
      metadata = self._run.attrs.metadata
      timestamp = int(time.time())
      metadata.RecordCLAction(change, constants.CL_ACTION_KICKED_OUT,
                              timestamp)

    if reason in constants.SUSPECT_REASONS:
      metrics.Counter(constants.MON_CL_REJECT_COUNT).increment(
          fields={'reason': reason})

    self._InsertCLActionToDatabase(change, constants.CL_ACTION_KICKED_OUT,
                                   reason)
    if self.pre_cq_trybot:
      self.UpdateCLPreCQStatus(change, constants.CL_STATUS_FAILED)

  def MarkForgiven(self, change, reason=None):
    """Mark |change| as forgiven with |reason|.

    Args:
      change: A GerritPatch or GerritPatchTuple object.
      reason: string reason for submission to be recorded in cidb. (Should be
        None or constant with name STRATEGY_* from constants.py)
    """
    self._InsertCLActionToDatabase(change, constants.CL_ACTION_FORGIVEN, reason)

  def _InsertCLActionToDatabase(self, change, action, reason=None):
    """If cidb is set up and not None, insert given cl action to cidb.

    Args:
      change: A GerritPatch or GerritPatchTuple object.
      action: The action taken, should be one of constants.CL_ACTIONS
      reason: string reason for submission to be recorded in cidb. (Should be
        None or constant with name STRATEGY_* from constants.py)
    """
    build_id, db = self._run.GetCIDBHandle()
    if db:
      db.InsertCLActions(
          build_id,
          [clactions.CLAction.FromGerritPatchAndAction(change, action, reason)])

  def SubmitNonManifestChanges(self, check_tree_open=True, reason=None):
    """Commits changes to Gerrit from Pool that aren't part of the checkout.

    Args:
      check_tree_open: Whether to check that the tree is open before submitting
        changes. If this is False, TreeIsClosedException will never be raised.
      reason: string reason for submission to be recorded in cidb. (Should be
        None or constant with name STRATEGY_* from constants.py)

    Raises:
      TreeIsClosedException: if the tree is closed.
    """
    verified_cls = {c:reason for c in self.non_manifest_changes}
    self.SubmitChanges(verified_cls,
                       check_tree_open=check_tree_open)

  def SubmitPool(self, check_tree_open=True, throttled_ok=True, reason=None):
    """Commits changes to Gerrit from Pool.  This is only called by a master.

    Args:
      check_tree_open: Whether to check that the tree is open before submitting
        changes. If this is False, TreeIsClosedException will never be raised.
      throttled_ok: if |check_tree_open|, treat a throttled tree as open
      reason: string reason for submission to be recorded in cidb. (Should be
        None or constant with name STRATEGY_* from constants.py)

    Raises:
      TreeIsClosedException: if the tree is closed.
      FailedToSubmitAllChangesException: if we can't submit a change.
    """
    # Note that SubmitChanges can throw an exception if it can't
    # submit all changes; in that particular case, don't mark the inflight
    # failures patches as failed in gerrit- some may apply next time we do
    # a CQ run (since the submit state has changed, we have no way of
    # knowing).  They *likely* will still fail, but this approach tries
    # to minimize wasting the developers time.
    verified_cls = {c:reason for c in self.applied}
    submitted, errors = self.SubmitChanges(verified_cls,
                                           check_tree_open=check_tree_open,
                                           throttled_ok=throttled_ok)
    if errors:
      raise FailedToSubmitAllChangesException(errors, len(submitted))

    if self.changes_that_failed_to_apply_earlier:
      self._HandleApplyFailure(self.changes_that_failed_to_apply_earlier)

  def SubmitPartialPool(self, changes, messages, changes_by_config,
                        subsys_by_config, passed_in_history_slaves_by_change,
                        failing, inflight, no_stat):
    """If the build failed, push any CLs that don't care about the failure.

    In this function we calculate what CLs are definitely innocent and submit
    those CLs.

    Each project can specify a list of stages it does not care about in its
    COMMIT-QUEUE.ini file. Changes to that project will be submitted even if
    those stages fail. Or if unignored fail stage is only HWTestStage, submit
    changes that are unrelated to the failed hardware subsystems.

    Args:
      changes: A list of GerritPatch instances to examine.
      messages: A list of build_failure_message.BuildFailureMessage or NoneType
        objects from the failed slaves.
      changes_by_config: A dictionary of relevant changes indexed by the
        config names.
      subsys_by_config: A dictionary of pass/fail HWTest subsystems indexed
        by the config names.
      passed_in_history_slaves_by_change: A dict mapping changes to their
        relevant slaves (build config name strings) which passed in history.
      failing: Names of the builders that failed.
      inflight: Names of the builders that timed out.
      no_stat: Set of builder names of slave builders that had status None.

    Returns:
      A set of the non-submittable changes.
    """
    fully_verified = triage_lib.CalculateSuspects.GetFullyVerifiedChanges(
        changes, changes_by_config, subsys_by_config,
        passed_in_history_slaves_by_change, failing, inflight, no_stat,
        messages, self.build_root)
    fully_verified_cls = fully_verified.keys()
    if fully_verified_cls:
      logging.info('The following changes will be submitted using '
                   'board-aware submission logic: %s',
                   cros_patch.GetChangesAsString(fully_verified_cls))
    self.SubmitChanges(fully_verified)

    # Return the list of non-submittable changes.
    return set(changes) - set(fully_verified_cls)

  def _HandleApplyFailure(self, failures):
    """Handles changes that were not able to be applied cleanly.

    Args:
      failures: List of cros_patch.PatchException instances to handle.
    """
    for failure in failures:
      logging.info('Change %s did not apply cleanly.', failure.patch)
      if self.is_master:
        self._HandleCouldNotApply(failure)

  def _HandleCouldNotApply(self, failure):
    """Handler for when Paladin fails to apply a change.

    This handler notifies set CodeReview-2 to the review forcing the developer
    to re-upload a rebased change.

    Args:
      failure: cros_patch.PatchException instance to operate upon.
    """
    msg = ('%(queue)s failed to apply your change in %(build_log)s .'
           ' %(failure)s')
    self.SendNotification(failure.patch, msg, failure=failure)
    self.RemoveReady(failure.patch)

  def _HandleIncorrectSubmission(self, failure):
    """Handler for when Paladin incorrectly submits a change."""
    msg = ('%(queue)s incorrectly submitted your change in %(build_log)s .'
           '  %(failure)s')
    self.SendNotification(failure.patch, msg, failure=failure)
    self.RemoveReady(failure.patch)

  def HandleDraftChange(self, change):
    """Handler for when the latest patch set of |change| is not published.

    This handler removes the commit ready bit from the specified changes and
    sends the developer a message explaining why.

    Args:
      change: GerritPatch instance to operate upon.
    """
    msg = ('%(queue)s could not apply your change because the latest patch '
           'set is not published. Please publish your draft patch set before '
           'marking your commit as ready.')
    self.SendNotification(change, msg)
    self.RemoveReady(change)

  def _HandleFailedToApplyDueToInflightConflict(self, change):
    """Handler for when a patch conflicts with another patch in the CQ run.

    This handler simply comments on the affected change, explaining why it
    is being skipped in the current CQ run.

    Args:
      change: GerritPatch instance to operate upon.
    """
    msg = ('%(queue)s could not apply your change because it conflicts with '
           'other change(s) that it is testing. If those changes do not pass '
           'your change will be retried. Otherwise it will be rejected at '
           'the end of this CQ run.')
    self.SendNotification(change, msg)

  def HandleNoConfigTargetFailure(self, change, config):
    """Handler for when the target config not found.

    This handler removes the commit queue ready and trybot ready bits,
    and sends out the notifications explaining the config errors.

    Args:
      change: GerritPatch instance to operate upon.
      config: The name (string) of the config to test.
    """
    msg = ('No configuration target found for %s.\nYou can check the available '
           'configs by running `cbuildbot --list --all`.\nThe config may have '
           'been changed or removed, you can try to rebase your CL so it can '
           'get re-screened by the Pre-cq-launcher.' % config)
    self.SendNotification(change, msg)
    self.RemoveReady(change)

  def HandleValidationTimeout(self, changes=None, sanity=True):
    """Handles changes that timed out.

    If sanity is set, then this handler removes the commit ready bit
    from infrastructure changes and sends the developer a message explaining
    why.

    Args:
      changes: A list of cros_patch.GerritPatch instances to mark as failed.
        By default, mark all of the changes as failed.
      sanity: A boolean indicating whether the build was considered sane. If
        not sane, none of the changes will have their CommitReady bit modified.
    """
    if changes is None:
      changes = self.applied

    logging.info('Validation timed out for all changes.')
    base_msg = ('%(queue)s timed out while verifying your change in '
                '%(build_log)s . This means that a supporting builder did not '
                'finish building your change within the specified timeout.')

    blamed = triage_lib.CalculateSuspects.FilterChangesForInfraFail(changes)

    for change in changes:
      logging.info('Validation timed out for change %s.', change)
      if sanity and change in blamed:
        msg = ('%s If you believe this happened in error, just re-mark your '
               'commit as ready. Your change will then get automatically '
               'retried.' % base_msg)
        self.SendNotification(change, msg)
        self.RemoveReady(change)
      else:
        msg = ('NOTE: The Commit Queue will retry your change automatically.'
               '\n\n'
               '%s The build failure may have been caused by infrastructure '
               'issues, so your change will not be blamed for the failure.'
               % base_msg)
        self.SendNotification(change, msg)
        self.MarkForgiven(change)

  def SendNotification(self, change, msg, **kwargs):
    if not kwargs.get('build_log'):
      kwargs['build_log'] = self.build_log
    kwargs.setdefault('queue', self.queue)
    d = dict(**kwargs)
    try:
      msg %= d
    except (TypeError, ValueError) as e:
      logging.error(
          "Generation of message %s for change %s failed: dict was %r, "
          "exception %s", msg, change, d, e)
      raise e.__class__(
          "Generation of message %s for change %s failed: dict was %r, "
          "exception %s" % (msg, change, d, e))
    cl_messages.PaladinMessage(
        msg, change, self._helper_pool.ForChange(change)).Send(self.dryrun)

  def HandlePreCQSuccess(self, changes):
    """Handler that is called when |changes| passed all pre-cq configs."""
    msg = ('%(queue)s has successfully verified your change.')
    def ProcessChange(change):
      self.SendNotification(change, msg)

    inputs = [[change] for change in changes]
    parallel.RunTasksInProcessPool(ProcessChange, inputs)

  def HandlePreCQPerConfigSuccess(self):
    """Handler that is called when a pre-cq tryjob verifies a change."""
    def ProcessChange(change):
      # Note: This function has no unit test coverage. Be careful when
      # modifying.
      if self._run:
        metadata = self._run.attrs.metadata
        timestamp = int(time.time())
        metadata.RecordCLAction(change, constants.CL_ACTION_VERIFIED,
                                timestamp)
        self._InsertCLActionToDatabase(change, constants.CL_ACTION_VERIFIED)

    # Process the changes in parallel.
    inputs = [[change] for change in self.applied]
    parallel.RunTasksInProcessPool(ProcessChange, inputs)

  def _HandleCouldNotSubmit(self, change, error=''):
    """Handler that is called when Paladin can't submit a change.

    This should be rare, but if an admin overrides the commit queue and commits
    a change that conflicts with this change, it'll apply, build/validate but
    receive an error when submitting.

    Args:
      change: GerritPatch instance to operate upon.
      error: The reason why the change could not be submitted.
    """
    self.SendNotification(
        change,
        '%(queue)s failed to submit your change in %(build_log)s . '
        '%(error)s', error=error)
    self.RemoveReady(change)

  def _ChangeFailedValidation(self, change, messages, suspects, sanity,
                              infra_fail, lab_fail, no_stat):
    """Handles a validation failure for an individual change.

    Args:
      change: The change to mark as failed.
      messages: A list of build failure messages from supporting builders.
          These must be build_failure_message.BuildFailureMessage objects.
      suspects: An instance of triage_lib.SuspectChanges.
      sanity: A boolean indicating whether the build was considered sane. If
        not sane, none of the changes will have their CommitReady bit modified.
      infra_fail: The build failed purely due to infrastructure failures.
      lab_fail: The build failed purely due to test lab infrastructure failures.
      no_stat: A list of builders which failed prematurely without reporting
        status.
    """
    retry = not sanity or lab_fail or change not in suspects.keys()
    if self._ShouldSendFailureNotification(change, retry):
      cl_status_url = self._GetCLStatusURL(change)
      msg = cl_messages.CreateValidationFailureMessage(
          self.pre_cq_trybot, change, suspects, messages,
          sanity, infra_fail, lab_fail, no_stat, retry, cl_status_url)
      self.SendNotification(change, '%(details)s', details=msg)
    if retry:
      self.MarkForgiven(change)
    else:
      self.RemoveReady(change, reason=suspects.get(change))

  def _ShouldSendFailureNotification(self, change, retry):
    """Decides if we should send a failure notification.

    Args:
      change: The change to mark as failed.
      retry: Whether the change will be retried soon.

    Returns:
      True if we should send a failure notification. False otherwise.
    """
    # If we are on CQ, always send a notification.
    if not self.pre_cq_trybot:
      return True

    # We are on pre-CQ. Its notable difference from CQ is that
    # HandleValidationFailure() is called on individual pre-CQ bots, not on
    # a single master bot. Therefore we have to be careful not to send spammy
    # notifications.

    # If we will retry soon, skip sending a notification.
    if retry:
      return False

    # This is a real pre-CQ failure. Send a notification only if this is the
    # first failure.
    # This has race conditions among bots, but sending several notifications is
    # still acceptable.
    _, db = self._run.GetCIDBHandle()
    action_history = db.GetActionsForChanges([change])
    pre_cq_status = clactions.GetCLPreCQStatus(change, action_history)
    return pre_cq_status != constants.CL_STATUS_FAILED

  def HandleValidationFailure(self, messages, changes=None, sanity=True,
                              no_stat=None, failed_hwtests=None):
    """Handles a list of validation failure messages from slave builders.

    This handler parses a list of failure messages from our list of builders
    and calculates which changes were likely responsible for the failure. The
    changes that were responsible for the failure have their Commit Ready bit
    stripped and the other changes are left marked as Commit Ready.

    Args:
      messages: A list of build failure messages from supporting builders.
          These must be build_failure_message.BuildFailureMessage objects or
          NoneType objects.
      changes: A list of cros_patch.GerritPatch instances to mark as failed.
        By default, mark all of the changes as failed.
      sanity: A boolean indicating whether the build was considered sane. If
        not sane, none of the changes will have their CommitReady bit modified.
      no_stat: A list of builders which failed prematurely without reporting
        status. If not None, this implies there were infrastructure issues.
      failed_hwtests: A list of names (strings) of failed hwtests.
    """
    if changes is None:
      changes = self.applied

    candidates = []

    if self.pre_cq_trybot:
      build_id, db = self._run.GetCIDBHandle()
      action_history = []
      if db:
        action_history = db.GetActionsForChanges(changes)

      cancelled_pre_cqs = clactions.GetCancelledPreCQBuilds(action_history)
      cancelled_build_ids = set([x.build_id for x in cancelled_pre_cqs])
      if build_id in cancelled_build_ids:
        logging.info('This Pre-CQ build was cancelled on demand, do not blame '
                     'on CLs.')
      else:
        for change in changes:
          # Don't reject changes that have already passed the pre-cq.
          pre_cq_status = clactions.GetCLPreCQStatus(
              change, action_history)
          if pre_cq_status == constants.CL_STATUS_PASSED:
            continue
          candidates.append(change)
    else:
      candidates.extend(changes)

    # Determine the cause of the failures and the changes that are likely at
    # fault for the failure.
    lab_fail = triage_lib.CalculateSuspects.OnlyLabFailures(messages, no_stat)
    infra_fail = triage_lib.CalculateSuspects.OnlyInfraFailures(
        messages, no_stat)
    suspects = triage_lib.CalculateSuspects.FindSuspects(
        candidates, messages, build_root=self.build_root, infra_fail=infra_fail,
        lab_fail=lab_fail, failed_hwtests=failed_hwtests, sanity=sanity)

    # Send out failure notifications for each change.
    inputs = [[change, messages, suspects, sanity, infra_fail,
               lab_fail, no_stat] for change in candidates]
    parallel.RunTasksInProcessPool(self._ChangeFailedValidation, inputs)

  def _GetCLStatusURL(self, change):
    """Construct and return the CL Status URL.

    Args:
      change: GerritPatch instance to query CL status.

    Returns:
      The URL string of the CL Status page.
    """
    host = (constants.INTERNAL_GERRIT_HOST if change.internal
            else constants.EXTERNAL_GERRIT_HOST)
    return '%s/%s/%s/%s' % (CL_STATUS_URL_PREFIX, host, change.gerrit_number,
                            change.patch_number)

  def HandleApplySuccess(self, change, build_log=None):
    """Handler for when Paladin successfully applies (picks up) a change.

    This handler notifies a developer that their change is being tried as
    part of a Paladin run defined by a build_log.

    Args:
      change: GerritPatch instance to operate upon.
      action_history: List of CLAction instances.
      build_log: The URL to the build log.
    """
    cl_status_url = self._GetCLStatusURL(change)
    msg = ('%(queue)s has picked up your change.\n'
           'You can follow along at %(build_log)s.\n'
           'You can find the CL status and build information at '
           '%(cl_status_url)s.')
    self.SendNotification(change, msg, build_log=build_log,
                          cl_status_url=cl_status_url)

  # Note: This function doesn't need to be a ValidationPool instance method.
  def UpdateCLPreCQStatus(self, change, status):
    """Update the pre-CQ |status| of |change|."""
    action = clactions.TranslatePreCQStatusToAction(status)
    self._InsertCLActionToDatabase(change, action)

  # Note: Only the PreCQLauncherStage still uses this function. The commit queue
  # goes directly to AcquirePool -> patch_series.CreateDisjointTransactions.
  # It's possible that this function, which is basically a wrapper around
  # patch_series with a bit of failure handling, can be eliminated or folded
  # into PreCQLauncherStage for clarity.
  def CreateDisjointTransactions(self, manifest, changes, max_txn_length=None):
    """Create a list of disjoint transactions from the changes in the pool.

    Side effect: Reject and comment (on Gerrit) on changes that failed to
    apply.

    Args:
      manifest: Manifest to use.
      changes: List of changes to use.
      max_txn_length: The maximum length of any given transaction.  By default,
        do not limit the length of transactions.

    Returns:
      a tuple of (plans, failures) where

      plans = A list of disjoint transactions. Each transaction can be tried
      independently, without involving patches from other transactions.
      Each change in the pool will included in exactly one of transactions,
      unless the patch does not apply for some reason.

      failures = A list of cros_patch.PatchException instances for patches that
      failed to apply. Note: this ignores patches that dependencies on
      not-yet-ready patches, for up to REJECTION_GRACE_PERIOD from their last
      approval.
    """
    patches = patch_series.PatchSeries(
        self.build_root, forced_manifest=manifest)
    plans, failed = patches.CreateDisjointTransactions(
        changes, max_txn_length=max_txn_length)
    failed = self._FilterDependencyErrors(failed)
    if failed:
      self._HandleApplyFailure(failed)
    return plans, failed
