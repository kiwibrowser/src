# -*- coding: utf-8 -*-
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing helper class and methods for interacting with Gerrit."""

from __future__ import print_function

import operator

from chromite.lib import config_lib
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import git
from chromite.lib import gob_util
from chromite.lib import parallel
from chromite.lib import patch as cros_patch


site_config = config_lib.GetConfig()


class GerritException(Exception):
  """Base exception, thrown for gerrit failures"""


class QueryHasNoResults(GerritException):
  """Exception thrown when a query returns no results."""


class QueryNotSpecific(GerritException):
  """Thrown when a query needs to identify one CL, but matched multiple."""


class GerritHelper(object):
  """Helper class to manage interaction with the gerrit-on-borg service."""

  # Maximum number of results to return per query.
  _GERRIT_MAX_QUERY_RETURN = 500

  # Number of processes to run in parallel when fetching from Gerrit. The
  # Gerrit team recommended keeping this small to avoid putting too much
  # load on the server.
  _NUM_PROCESSES = 10

  # Fields that appear in gerrit change query results.
  MORE_CHANGES = '_more_changes'

  def __init__(self, host, remote, print_cmd=True):
    """Initialize.

    Args:
      host: Hostname (without protocol prefix) of the gerrit server.
      remote: The symbolic name of a known remote git host,
          taken from cbuildbot.contants.
      print_cmd: Determines whether all RunCommand invocations will be echoed.
          Set to False for quiet operation.
    """
    self.host = host
    self.remote = remote
    self.print_cmd = bool(print_cmd)
    self._version = None

  @classmethod
  def FromRemote(cls, remote, **kwargs):
    if remote == site_config.params.INTERNAL_REMOTE:
      host = site_config.params.INTERNAL_GERRIT_HOST
    elif remote == site_config.params.EXTERNAL_REMOTE:
      host = site_config.params.EXTERNAL_GERRIT_HOST
    else:
      raise ValueError('Remote %s not supported.' % remote)
    return cls(host, remote, **kwargs)

  @classmethod
  def FromGob(cls, gob, **kwargs):
    """Return a helper for a GoB instance."""
    host = site_config.params.GOB_HOST % ('%s-review' % gob)
    # TODO(phobbs) this will be wrong when "gob" isn't in GOB_REMOTES.
    # We should get rid of remotes altogether and just use the host.
    return cls(host, site_config.params.GOB_REMOTES.get(gob, gob), **kwargs)

  def SetAssignee(self, change, assignee, dryrun=False):
    """Set assignee on a gerrit change.

    Args:
      change: ChangeId or change number for a gerrit review.
      assignee: email address of reviewer to add.
      dryrun: If True, only print what would have been done.
    """
    if dryrun:
      logging.info('Would have added %s to "%s"', assignee, change)
    else:
      gob_util.AddAssignee(self.host, change, assignee)

  def SetPrivate(self, change, private):
    """Sets the private bit on the given CL.

    Args:
      change: CL number.
      private: bool to indicate what value to set for the private bit.
    """
    if private:
      gob_util.MarkPrivate(self.host, change)
    else:
      gob_util.MarkNotPrivate(self.host, change)

  def SetReviewers(self, change, add=(), remove=(), dryrun=False):
    """Modify the list of reviewers on a gerrit change.

    Args:
      change: ChangeId or change number for a gerrit review.
      add: Sequence of email addresses of reviewers to add.
      remove: Sequence of email addresses of reviewers to remove.
      dryrun: If True, only print what would have been done.
    """
    if add:
      if dryrun:
        logging.info('Would have added %s to "%s"', add, change)
      else:
        gob_util.AddReviewers(self.host, change, add)
    if remove:
      if dryrun:
        logging.info('Would have removed %s to "%s"', remove, change)
      else:
        gob_util.RemoveReviewers(self.host, change, remove)

  def GetChangeDetail(self, change_num, verbose=False):
    """Return detailed information about a gerrit change.

    Args:
      change_num: A gerrit change number.
      verbose: (optional) Whether to print more properties of the change
    """
    if verbose:
      o_params = ('ALL_REVISIONS', 'ALL_FILES', 'ALL_COMMITS',
                  'DETAILED_LABELS', 'MESSAGES', 'DOWNLOAD_COMMANDS', 'CHECK')
    else:
      o_params = ('CURRENT_REVISION', 'CURRENT_COMMIT')

    return gob_util.GetChangeDetail(
        self.host, change_num, o_params=o_params)

  def GrabPatchFromGerrit(self, project, change, commit, must_match=True):
    """Return a cros_patch.GerritPatch representing a gerrit change.

    Args:
      project: The name of the gerrit project for the change.
      change: A ChangeId or gerrit number for the change.
      commit: The git commit hash for a patch associated with the change.
      must_match: Raise an exception if the change is not found.
    """
    query = {'project': project, 'commit': commit, 'must_match': must_match}
    return self.QuerySingleRecord(change, **query)

  def IsChangeCommitted(self, change, must_match=False):
    """Check whether a gerrit change has been merged.

    Args:
      change: A gerrit change number.
      must_match: Raise an exception if the change is not found.  If this is
          False, then a missing change will return None.
    """
    change = gob_util.GetChange(self.host, change)
    if not change:
      if must_match:
        raise QueryHasNoResults('Could not query for change %s' % change)
      return
    return change.get('status') == 'MERGED'

  def GetLatestSHA1ForBranch(self, project, branch):
    """Return the git hash at the tip of a branch."""
    url = '%s://%s/%s' % (gob_util.GIT_PROTOCOL, self.host, project)
    cmd = ['ls-remote', url, 'refs/heads/%s' % branch]
    try:
      result = git.RunGit('.', cmd, print_cmd=self.print_cmd)
      if result:
        return result.output.split()[0]
    except cros_build_lib.RunCommandError:
      logging.error('Command "%s" failed.', cros_build_lib.CmdToStr(cmd),
                    exc_info=True)

  def QuerySingleRecord(self, change=None, **kwargs):
    """Free-form query of a gerrit change that expects a single result.

    Args:
      change: A gerrit change number.
      **kwargs:
        dryrun: Don't query the gerrit server; just return None.
        must_match: Raise an exception if the query comes back empty.  If this
            is False, an unsatisfied query will return None.
        Refer to Query() docstring for remaining arguments.

    Returns:
      If kwargs['raw'] == True, return a python dict representing the
      change; otherwise, return a cros_patch.GerritPatch object.
    """
    query_kwds = kwargs
    dryrun = query_kwds.get('dryrun')
    must_match = query_kwds.pop('must_match', True)
    results = self.Query(change, **query_kwds)
    if dryrun:
      return None
    elif not results:
      if must_match:
        raise QueryHasNoResults('Query %s had no results' % (change,))
      return None
    elif len(results) != 1:
      raise QueryNotSpecific('Query %s returned too many results: %s'
                             % (change, results))
    return results[0]

  def Query(self, change=None, sort=None, current_patch=True, options=(),
            dryrun=False, raw=False, start=None, bypass_cache=True,
            verbose=False, **kwargs):
    """Free-form query for gerrit changes.

    Args:
      change: ChangeId, git commit hash, or gerrit number for a change.
      sort: A functor to extract a sort key from a cros_patch.GerritChange
          object, for sorting results..  If this is None, results will not be
          sorted.
      current_patch: If True, ask the gerrit server for extra information about
          the latest uploaded patch.
      options: Deprecated.
      dryrun: If True, don't query the gerrit server; return an empty list.
      raw: If True, return a list of python dict's representing the query
          results.  Otherwise, return a list of cros_patch.GerritPatch.
      start: Offset in the result set to start at.
      bypass_cache: Query each change to make sure data is up to date.
      verbose: Whether to get all revisions and details about a change.
      kwargs: A dict of query parameters, as described here:
        https://gerrit-review.googlesource.com/Documentation/rest-api-changes.html#list-changes

    Returns:
      A list of python dicts or cros_patch.GerritChange.
    """
    query_kwds = kwargs
    if options:
      raise GerritException('"options" argument unsupported on gerrit-on-borg.')
    url_prefix = gob_util.GetGerritFetchUrl(self.host)
    # All possible params are documented at
    # pylint: disable=C0301
    # https://gerrit-review.googlesource.com/Documentation/rest-api-changes.html#list-changes
    o_params = ['DETAILED_ACCOUNTS', 'ALL_REVISIONS', 'DETAILED_LABELS']
    if current_patch:
      o_params.extend(['CURRENT_COMMIT', 'CURRENT_REVISION'])

    if change and cros_patch.ParseGerritNumber(change) and not query_kwds:
      if dryrun:
        logging.info('Would have run gob_util.GetChangeDetail(%s, %s)',
                     self.host, change)
        return []
      change = self.GetChangeDetail(change, verbose=verbose)
      if change is None:
        return []
      patch_dict = cros_patch.GerritPatch.ConvertQueryResults(change, self.host)
      if raw:
        return [patch_dict]
      return [cros_patch.GerritPatch(patch_dict, self.remote, url_prefix)]

    # TODO: We should allow querying using a cros_patch.PatchQuery
    # object directly.
    if change and cros_patch.ParseSHA1(change):
      # Use commit:sha1 for accurate query results (crbug.com/358381).
      kwargs['commit'] = change
      change = None
    elif change and cros_patch.ParseChangeID(change):
      # Use change:change-id for accurate query results (crbug.com/357876).
      kwargs['change'] = change
      change = None
    elif change and cros_patch.ParseFullChangeID(change):
      change = cros_patch.ParseFullChangeID(change)
      kwargs['change'] = change.change_id
      kwargs['project'] = change.project
      kwargs['branch'] = change.branch
      change = None

    if change and query_kwds.get('change'):
      raise GerritException('Bad query params: provided a change-id-like query,'
                            ' and a "change" search parameter')

    if dryrun:
      logging.info('Would have run gob_util.QueryChanges(%s, %s, '
                   'first_param=%s, limit=%d)', self.host, repr(query_kwds),
                   change, self._GERRIT_MAX_QUERY_RETURN)
      return []

    start = 0
    moar = gob_util.QueryChanges(
        self.host, query_kwds, first_param=change, start=start,
        limit=self._GERRIT_MAX_QUERY_RETURN, o_params=o_params)
    result = list(moar)
    while moar and self.MORE_CHANGES in moar[-1]:
      start += len(moar)
      moar = gob_util.QueryChanges(
          self.host, query_kwds, first_param=change, start=start,
          limit=self._GERRIT_MAX_QUERY_RETURN, o_params=o_params)
      result.extend(moar)

    # NOTE: Query results are served from the gerrit cache, which may be stale.
    # To make sure the patch information is accurate, re-request each query
    # result directly, circumventing the cache.  For reference:
    #   https://code.google.com/p/chromium/issues/detail?id=302072
    if bypass_cache:
      result = self.GetMultipleChangeDetail(
          [x['_number'] for x in result], verbose=verbose)

    result = [cros_patch.GerritPatch.ConvertQueryResults(
        x, self.host) for x in result]
    if sort:
      result = sorted(result, key=operator.itemgetter(sort))
    if raw:
      return result
    return [cros_patch.GerritPatch(x, self.remote, url_prefix) for x in result]

  def GetMultipleChangeDetail(self, changes, verbose=False):
    """Query the gerrit server for multiple changes using GetChangeDetail.

    Args:
      changes: A sequence of gerrit change numbers.
      verbose: (optional) Whether to return more properties of the change

    Returns:
      A list of the raw output of GetChangeDetail.
    """
    inputs = [[change] for change in changes]
    return parallel.RunTasksInProcessPool(
        lambda c: self.GetChangeDetail(c, verbose=verbose),
        inputs, processes=self._NUM_PROCESSES)

  def QueryMultipleCurrentPatchset(self, changes):
    """Query the gerrit server for multiple changes.

    Args:
      changes: A sequence of gerrit change numbers.

    Returns:
      A list of cros_patch.GerritPatch.
    """
    if not changes:
      return

    url_prefix = gob_util.GetGerritFetchUrl(self.host)
    results = self.GetMultipleChangeDetail(changes)
    for change, change_detail in zip(changes, results):
      if not change_detail:
        raise GerritException('Change %s not found on server %s.'
                              % (change, self.host))
      patch_dict = cros_patch.GerritPatch.ConvertQueryResults(
          change_detail, self.host)
      yield change, cros_patch.GerritPatch(patch_dict, self.remote, url_prefix)

  @staticmethod
  def _to_changenum(change):
    """Unequivocally return a gerrit change number.

    The argument may either be an number, which will be returned unchanged;
    or an instance of GerritPatch, in which case its gerrit number will be
    returned.
    """
    # TODO(davidjames): Deprecate the ability to pass in strings to these
    # functions -- API users should just pass in a GerritPatch instead or use
    # the gob_util APIs directly.
    if isinstance(change, cros_patch.GerritPatch):
      return change.gerrit_number

    return change

  def SetReview(self, change, msg=None, labels=None, dryrun=False):
    """Update the review labels on a gerrit change.

    Args:
      change: A gerrit change number.
      msg: A text comment to post to the review.
      labels: A dict of label/value to set on the review.
      dryrun: If True, don't actually update the review.
    """
    if not msg and not labels:
      return
    if dryrun:
      if msg:
        logging.info('Would have added message "%s" to change "%s".', msg,
                     change)
      if labels:
        for key, val in labels.iteritems():
          logging.info('Would have set label "%s" to "%s" for change "%s".',
                       key, val, change)
      return
    gob_util.SetReview(self.host, self._to_changenum(change),
                       msg=msg, labels=labels, notify='ALL')

  def SetTopic(self, change, topic, dryrun=False):
    """Update the topic on a gerrit change.

    Args:
      change: A gerrit change number.
      topic: The topic to set the review to.
      dryrun: If True, don't actually set the topic.
    """
    if dryrun:
      logging.info('Would have set topic "%s" for change "%s".', topic, change)
      return
    gob_util.SetTopic(self.host, self._to_changenum(change), topic=topic)

  def SetHashtags(self, change, add, remove, dryrun=False):
    """Add/Remove hashtags for a gerrit change.

    Args:
      change: A gerrit change number.
      add: a list of hashtags to add.
      remove: a list of hashtags to remove.
      dryrun: If True, don't actually set the hashtag.
    """
    if dryrun:
      logging.info('Would add %r and remove %r for change %s',
                   add, remove, change)
      return
    gob_util.SetHashtags(self.host, self._to_changenum(change),
                         add=add, remove=remove)

  def RemoveReady(self, change, dryrun=False):
    """Set the 'Commit-Queue' and 'Trybot-Ready' labels on a |change| to '0'."""
    if dryrun:
      logging.info('Would have reset Commit-Queue and Trybot-Ready label for '
                   '%s', change)
      return
    gob_util.ResetReviewLabels(self.host, self._to_changenum(change),
                               label='Commit-Queue', notify='OWNER')
    gob_util.ResetReviewLabels(self.host, self._to_changenum(change),
                               label='Trybot-Ready', notify='OWNER')

  def SubmitChange(self, change, dryrun=False):
    """Land (merge) a gerrit change using the JSON API."""
    if dryrun:
      logging.info('Would have submitted change %s', change)
      return
    if isinstance(change, cros_patch.GerritPatch):
      rev = change.sha1
    else:
      rev = None
    gob_util.SubmitChange(self.host, self._to_changenum(change), revision=rev)

  def AbandonChange(self, change, dryrun=False):
    """Mark a gerrit change as 'Abandoned'."""
    if dryrun:
      logging.info('Would have abandoned change %s', change)
      return
    gob_util.AbandonChange(self.host, self._to_changenum(change))

  def RestoreChange(self, change, dryrun=False):
    """Re-activate a previously abandoned gerrit change."""
    if dryrun:
      logging.info('Would have restored change %s', change)
      return
    gob_util.RestoreChange(self.host, self._to_changenum(change))

  def DeleteDraft(self, change, dryrun=False):
    """Delete a gerrit change iff all its revisions are drafts."""
    if dryrun:
      logging.info('Would have deleted draft change %s', change)
      return
    gob_util.DeleteDraft(self.host, self._to_changenum(change))

  def GetAccount(self):
    """Get information about the user account."""
    return gob_util.GetAccount(self.host)


def GetGerritPatchInfo(patches):
  """Query Gerrit server for patch information using string queries.

  Args:
    patches: A list of patch IDs to query. Internal patches start with a '*'.

  Returns:
    A list of GerritPatch objects describing each patch.  Only the first
    instance of a requested patch is returned.

  Raises:
    PatchException if a patch can't be found.
    ValueError if a query string cannot be converted to a PatchQuery object.
  """
  return GetGerritPatchInfoWithPatchQueries(
      [cros_patch.ParsePatchDep(p) for p in patches])


def GetGerritPatchInfoWithPatchQueries(patches):
  """Query Gerrit server for patch information using PatchQuery objects.

  Args:
    patches: A list of PatchQuery objects to query.

  Returns:
    A list of GerritPatch objects describing each patch.  Only the first
    instance of a requested patch is returned.

  Raises:
    PatchException if a patch can't be found.
  """
  seen = set()
  results = []
  order = {k.ToGerritQueryText(): idx for (idx, k) in enumerate(patches)}
  for remote in site_config.params.CHANGE_PREFIX.keys():
    helper = GetGerritHelper(remote)
    raw_ids = [x.ToGerritQueryText() for x in patches if x.remote == remote]
    for k, change in helper.QueryMultipleCurrentPatchset(raw_ids):
      # return a unique list, while maintaining the ordering of the first
      # seen instance of each patch.  Do this to ensure whatever ordering
      # the user is trying to enforce, we honor; lest it break on
      # cherry-picking.
      if change.id not in seen:
        results.append((order[k], change))
        seen.add(change.id)

  return [change for _idx, change in sorted(results)]


def GetGerritHelper(remote=None, gob=None, **kwargs):
  """Return a GerritHelper instance for interacting with the given remote."""
  if gob:
    return GerritHelper.FromGob(gob, **kwargs)
  else:
    return GerritHelper.FromRemote(remote, **kwargs)


def GetGerritHelperForChange(change):
  """Return a usable GerritHelper instance for this change.

  If you need a GerritHelper for a specific change, get it via this
  function.
  """
  return GetGerritHelper(change.remote)


def GetCrosInternal(**kwargs):
  """Convenience method for accessing private ChromeOS gerrit."""
  return GetGerritHelper(site_config.params.INTERNAL_REMOTE, **kwargs)


def GetCrosExternal(**kwargs):
  """Convenience method for accessing public ChromiumOS gerrit."""
  return GetGerritHelper(site_config.params.EXTERNAL_REMOTE, **kwargs)


def GetChangeRef(change_number, patchset=None):
  """Given a change number, return the refs/changes/* space for it.

  Args:
    change_number: The gerrit change number you want a refspec for.
    patchset: If given it must either be an integer or '*'.  When given,
      the returned refspec is for that exact patchset.  If '*' is given, it's
      used for pulling down all patchsets for that change.

  Returns:
    A git refspec.
  """
  change_number = int(change_number)
  s = 'refs/changes/%02i/%i' % (change_number % 100, change_number)
  if patchset is not None:
    s += '/%s' % ('*' if patchset == '*' else int(patchset))
  return s
