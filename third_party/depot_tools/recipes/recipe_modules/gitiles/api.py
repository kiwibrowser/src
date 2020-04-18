# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64

from recipe_engine import recipe_api


class Gitiles(recipe_api.RecipeApi):
  """Module for polling a git repository using the Gitiles web interface."""

  def _fetch(self, url, step_name, fmt, attempts=None, add_json_log=True,
             log_limit=None, log_start=None, **kwargs):
    """Fetches information from Gitiles.

    Arguments:
      log_limit: for log URLs, limit number of results. None implies 1 page,
        as returned by Gitiles.
      log_start: for log URLs, the start cursor for paging.
      add_json_log: if True, will spill out json into log.
    """
    assert fmt in ('json', 'text')
    args = [
        '--json-file', self.m.json.output(add_json_log=add_json_log),
        '--url', url,
        '--format', fmt,
    ]
    if attempts:
      args.extend(['--attempts', attempts])
    if log_limit is not None:
      args.extend(['--log-limit', log_limit])
    if log_start is not None:
      args.extend(['--log-start', log_start])
    accept_statuses = kwargs.pop('accept_statuses', None)
    if accept_statuses:
      args.extend([
          '--accept-statuses',
          ','.join([str(s) for s in accept_statuses])])
    a = self.m.python(
        step_name, self.resource('gerrit_client.py'), args, **kwargs)
    return a

  def refs(self, url, step_name='refs', attempts=None):
    """Returns a list of refs in the remote repository."""
    step_result = self._fetch(
        self.m.url.join(url, '+refs'),
        step_name,
        fmt='json',
        attempts=attempts)

    refs = sorted(str(ref) for ref in step_result.json.output)
    step_result.presentation.logs['refs'] = refs
    return refs

  def log(self, url, ref, limit=0, cursor=None,
          step_name=None, attempts=None, **kwargs):
    """Returns the most recent commits under the given ref with properties.

    Args:
      url (str): URL of the remote repository.
      ref (str): Name of the desired ref (see Gitiles.refs).
      limit (int): Number of commits to limit the fetching to.
        Gitiles does not return all commits in one call; instead paging is
        used. 0 implies to return whatever first gerrit responds with.
        Otherwise, paging will be used to fetch at least this many
        commits, but all fetched commits will be returned.
      cursor (str or None): The paging cursor used to fetch the next page.
      step_name (str): Custom name for this step (optional).

    Returns:
      A tuple of (commits, cursor).
      Commits are a list of commits (as Gitiles dict structure) in reverse
      chronological order. The number of commits may be higher than limit
      argument.
      Cursor can be used for subsequent calls to log for paging. If None,
      signals that there are no more commits to fetch.
    """
    assert limit >= 0
    step_name = step_name or 'gitiles log: %s%s' % (
        ref, ' from %s' % cursor if cursor else '')

    step_result = self._fetch(
        self.m.url.join(url, '+log/%s' % ref),
        step_name,
        log_limit=limit,
        log_start=cursor,
        attempts=attempts,
        fmt='json',
        add_json_log=True,
        **kwargs)

    # The output is formatted as a JSON dict with a "log" key. The "log" key
    # is a list of commit dicts, which contain information about the commit.
    commits = step_result.json.output['log']
    cursor = step_result.json.output.get('next')

    step_result.presentation.step_text = (
        '<br />%d commits fetched' % len(commits))
    return commits, cursor

  def commit_log(self, url, commit, step_name=None, attempts=None):
    """Returns: (dict) the Gitiles commit log structure for a given commit.

    Args:
      url (str): The base repository URL.
      commit (str): The commit hash.
      step_name (str): If not None, override the step name.
      attempts (int): Number of times to try the request before failing.
    """
    step_name = step_name or 'commit log: %s' % commit

    commit_url = '%s/+/%s' % (url, commit)
    step_result = self._fetch(commit_url, step_name, attempts=attempts,
                              fmt='json')
    return step_result.json.output

  def download_file(self, repository_url, file_path, branch='master',
                    step_name=None, attempts=None, **kwargs):
    """Downloads raw file content from a Gitiles repository.

    Args:
      repository_url (str): Full URL to the repository.
      branch (str): Branch of the repository.
      file_path (str): Relative path to the file from the repository root.
      step_name (str): Custom name for this step (optional).
      attempts (int): Number of times to try the request before failing.

    Returns:
      Raw file content.
    """
    fetch_url = self.m.url.join(repository_url, '+/%s/%s' % (branch, file_path))
    step_result = self._fetch(
        fetch_url,
        step_name or 'fetch %s:%s' % (branch, file_path,),
        attempts=attempts,
        fmt='text',
        add_json_log=False,
        **kwargs)
    if step_result.json.output['value'] is None:
      return None
    return base64.b64decode(step_result.json.output['value'])
