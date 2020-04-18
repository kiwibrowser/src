# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from dashboard.api import api_request_handler
from dashboard.common import datastore_hooks
from dashboard.common import utils
from dashboard.models import try_job
from dashboard.services import issue_tracker_service


class BugsHandler(api_request_handler.ApiRequestHandler):
  """API handler for bug requests.

  Convenience methods for getting bug data; only available to internal users.
  """

  def AuthorizedPost(self, *args):
    """Returns alert data in response to API requests.

    Argument:
      bug_id: issue id on the chromium issue tracker

    Outputs:
      JSON data for the bug, see README.md.
    """
    # Users must log in with privileged access to see all bugs.
    if not datastore_hooks.IsUnalteredQueryPermitted():
      raise api_request_handler.BadRequestError('No access.')

    try:
      bug_id = int(args[0])
    except ValueError:
      raise api_request_handler.BadRequestError(
          'Invalid bug ID "%s".' % args[0])
    service = issue_tracker_service.IssueTrackerService(
        utils.ServiceAccountHttp())
    issue = service.GetIssue(bug_id)
    comments = service.GetIssueComments(bug_id)
    bisects = try_job.TryJob.query(try_job.TryJob.bug_id == bug_id).fetch()
    return {'bug': {
        'author': issue.get('author', {}).get('name'),
        'owner': issue.get('owner', {}).get('name'),
        'legacy_bisects': [{
            'status': b.status,
            'bot': b.bot,
            'bug_id': b.bug_id,
            'buildbucket_link': (
                'https://chromeperf.appspot.com/buildbucket_job_status/%s' %
                b.buildbucket_job_id),
            'command': b.GetConfigDict()['command'],
            'culprit': self._GetCulpritInfo(b),
            'metric': (b.results_data or {}).get('metric'),
            'started_timestamp': b.last_ran_timestamp.isoformat(),
        } for b in bisects],
        'cc': [cc.get('name') for cc in issue.get('cc', [])],
        'comments': [{
            'content': comment.get('content'),
            'author': comment.get('author'),
            'published': comment.get('published'),
        } for comment in comments],
        'components': issue.get('components', []),
        'id': bug_id,
        'labels': issue.get('labels', []),
        'published': issue.get('published'),
        'updated': issue.get('updated'),
        'state': issue.get('state'),
        'status': issue.get('status'),
        'summary': issue.get('summary'),
    }}

  def _GetCulpritInfo(self, try_job_entity):
    if not try_job_entity.results_data:
      return None
    culprit = try_job_entity.results_data.get('culprit_data')
    if not culprit:
      return None
    return {
        'cl': culprit.get('cl'),
        'subject': culprit.get('subject'),
    }
