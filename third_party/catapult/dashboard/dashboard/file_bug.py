# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provides the web interface for filing a bug on the issue tracker."""

import json
import logging
import re

from google.appengine.api import app_identity
from google.appengine.api import urlfetch
from google.appengine.api import users
from google.appengine.ext import ndb

from dashboard import auto_bisect
from dashboard import oauth2_decorator
from dashboard import short_uri
from dashboard.common import namespaced_stored_object
from dashboard.common import request_handler
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import bug_data
from dashboard.models import bug_label_patterns
from dashboard.services import crrev_service
from dashboard.services import gitiles_service
from dashboard.services import issue_tracker_service

# A list of bug labels to suggest for all performance regression bugs.
_DEFAULT_LABELS = [
    'Type-Bug-Regression',
    'Pri-2',
]
_OMAHA_PROXY_URL = 'https://omahaproxy.appspot.com/all.json'


class FileBugHandler(request_handler.RequestHandler):
  """Uses oauth2 to file a new bug with a set of alerts."""

  def post(self):
    """A POST request for this endpoint is the same as a GET request."""
    self.get()

  @oauth2_decorator.DECORATOR.oauth_required
  def get(self):
    """Either shows the form to file a bug, or if filled in, files the bug.

    The form to file a bug is popped up from the triage-dialog polymer element.
    The default summary, description and label strings are constructed there.

    Request parameters:
      summary: Bug summary string.
      description: Bug full description string.
      owner: Bug owner email address.
      keys: Comma-separated Alert keys in urlsafe format.

    Outputs:
      HTML, using the template 'bug_result.html'.
    """
    if not utils.IsValidSheriffUser():
      self.RenderHtml('bug_result.html', {
          'error': 'You must be logged in with a chromium.org account '
                   'to file bugs.'
      })
      return

    summary = self.request.get('summary')
    description = self.request.get('description')
    labels = self.request.get_all('label')
    components = self.request.get_all('component')
    keys = self.request.get('keys')
    if not keys:
      self.RenderHtml('bug_result.html', {
          'error': 'No alerts specified to add bugs to.'
      })
      return

    if self.request.get('finish'):
      self._CreateBug(summary, description, labels, components, keys)
    else:
      self._ShowBugDialog(summary, description, keys)

  def _ShowBugDialog(self, summary, description, urlsafe_keys):
    """Sends a HTML page with a form for filing the bug.

    Args:
      summary: The default bug summary string.
      description: The default bug description string.
      urlsafe_keys: Comma-separated Alert keys in urlsafe format.
    """
    alert_keys = [ndb.Key(urlsafe=k) for k in urlsafe_keys.split(',')]
    labels, components = _FetchLabelsAndComponents(alert_keys)
    owner_components = _FetchBugComponents(alert_keys)

    self.RenderHtml('bug_result.html', {
        'bug_create_form': True,
        'keys': urlsafe_keys,
        'summary': summary,
        'description': description,
        'labels': labels,
        'components': components.union(owner_components),
        'owner': '',
        'cc': users.get_current_user(),
    })

  def _CreateBug(self, summary, description, labels, components, urlsafe_keys):
    """Creates a bug, associates it with the alerts, sends a HTML response.

    Args:
      summary: The new bug summary string.
      description: The new bug description string.
      labels: List of label strings for the new bug.
      components: List of component strings for the new bug.
      urlsafe_keys: Comma-separated alert keys in urlsafe format.
    """
    # Only project members (@chromium.org accounts) can be owners of bugs.
    owner = self.request.get('owner')
    if owner and not owner.endswith('@chromium.org'):
      self.RenderHtml('bug_result.html', {
          'error': 'Owner email address must end with @chromium.org.'
      })
      return

    alert_keys = [ndb.Key(urlsafe=k) for k in urlsafe_keys.split(',')]
    alerts = ndb.get_multi(alert_keys)

    if not description:
      description = 'See the link to graphs below.'

    milestone_label = _MilestoneLabel(alerts)
    if milestone_label:
      labels.append(milestone_label)

    cc = self.request.get('cc')

    http = oauth2_decorator.DECORATOR.http()
    user_issue_tracker_service = issue_tracker_service.IssueTrackerService(http)

    new_bug_response = user_issue_tracker_service.NewBug(
        summary, description, labels=labels, components=components, owner=owner,
        cc=cc)
    if 'error' in new_bug_response:
      self.RenderHtml('bug_result.html', {'error': new_bug_response['error']})
      return

    bug_id = new_bug_response['bug_id']
    bug_data.Bug(id=bug_id).put()

    for a in alerts:
      a.bug_id = bug_id

    ndb.put_multi(alerts)

    comment_body = _AdditionalDetails(bug_id, alerts)
    # Add the bug comment with the service account, so that there are no
    # permissions issues.
    dashboard_issue_tracker_service = issue_tracker_service.IssueTrackerService(
        utils.ServiceAccountHttp())
    dashboard_issue_tracker_service.AddBugComment(bug_id, comment_body)

    template_params = {'bug_id': bug_id}
    if all(k.kind() == 'Anomaly' for k in alert_keys):
      logging.info('Kicking bisect for bug ' + str(bug_id))
      culprit_rev = _GetSingleCLForAnomalies(alerts)
      if culprit_rev is not None:
        _AssignBugToCLAuthor(bug_id, alerts[0], dashboard_issue_tracker_service)
      else:
        bisect_result = auto_bisect.StartNewBisectForBug(bug_id)
        if 'error' in bisect_result:
          logging.info('Failed to kick bisect for ' + str(bug_id))
          template_params['bisect_error'] = bisect_result['error']
        else:
          logging.info('Successfully kicked bisect for ' + str(bug_id))
          template_params.update(bisect_result)
    else:
      kinds = set()
      for k in alert_keys:
        kinds.add(k.kind())
      logging.info(
          'Didn\'t kick bisect for bug id %s because alerts had kinds %s',
          bug_id, list(kinds))

    self.RenderHtml('bug_result.html', template_params)


def _AdditionalDetails(bug_id, alerts):
  """Returns a message with additional information to add to a bug."""
  base_url = '%s/group_report' % _GetServerURL()
  bug_page_url = '%s?bug_id=%s' % (base_url, bug_id)
  sid = short_uri.GetOrCreatePageState(json.dumps(_UrlsafeKeys(alerts)))
  alerts_url = '%s?sid=%s' % (base_url, sid)
  comment = '<b>All graphs for this bug:</b>\n  %s\n\n' % bug_page_url
  comment += ('(For debugging:) Original alerts at time of bug-filing:\n  %s\n'
              % alerts_url)
  bot_names = anomaly.GetBotNamesFromAlerts(alerts)
  if bot_names:
    comment += '\n\nBot(s) for this bug\'s original alert(s):\n\n'
    comment += '\n'.join(sorted(bot_names))
  else:
    comment += '\nCould not extract bot names from the list of alerts.'
  return comment


def _GetServerURL():
  return 'https://' + app_identity.get_default_version_hostname()


def _UrlsafeKeys(alerts):
  return [a.key.urlsafe() for a in alerts]


def _ComponentFromCrLabel(label):
  return label.replace('Cr-', '').replace('-', '>')

def _FetchLabelsAndComponents(alert_keys):
  """Fetches a list of bug labels and components for the given Alert keys."""
  labels = set(_DEFAULT_LABELS)
  components = set()
  alerts = ndb.get_multi(alert_keys)
  sheriff_keys = set(alert.sheriff for alert in alerts)
  sheriff_labels = [sheriff.labels for sheriff in ndb.get_multi(sheriff_keys)]
  tags = [item for sublist in sheriff_labels for item in sublist]
  for tag in tags:
    if tag.startswith('Cr-'):
      components.add(_ComponentFromCrLabel(tag))
    else:
      labels.add(tag)
  if any(a.internal_only for a in alerts):
    # This is a Chrome-specific behavior, and should ideally be made
    # more general (maybe there should be a list in datastore of bug
    # labels to add for internal bugs).
    labels.add('Restrict-View-Google')
  for test in {a.GetTestMetadataKey() for a in alerts}:
    labels_components = bug_label_patterns.GetBugLabelsForTest(test)
    for item in labels_components:
      if item.startswith('Cr-'):
        components.add(_ComponentFromCrLabel(item))
      else:
        labels.add(item)
  return labels, components

def _FetchBugComponents(alert_keys):
  """Fetches the ownership bug components of the most recent alert on a per-test
     path basis from the given alert keys.
  """
  alerts = ndb.get_multi(alert_keys)
  sorted_alerts = reversed(sorted(alerts, key=lambda alert: alert.timestamp))

  most_recent_components = {}

  for alert in sorted_alerts:
    alert_test = alert.test.id()
    if (alert.ownership and alert.ownership.get('component') and
        most_recent_components.get(alert_test) is None):
      most_recent_components[alert_test] = alert.ownership['component']

  return set(most_recent_components.values())

def _MilestoneLabel(alerts):
  """Returns a milestone label string, or None.

  Because revision numbers for other repos may not be easily reconcilable with
  Chromium milestones, do not label them (see
  https://github.com/catapult-project/catapult/issues/2906).
  """
  revisions = [a.end_revision for a in alerts if hasattr(a, 'end_revision')]
  if not revisions:
    return None
  end_revision = min(revisions)
  for a in alerts:
    if a.end_revision == end_revision:
      row_key = utils.GetRowKey(a.test, a.end_revision)
      row = row_key.get()
      if hasattr(row, 'r_commit_pos'):
        end_revision = row.r_commit_pos
      else:
        return None
      break
  try:
    milestone = _GetMilestoneForRevision(end_revision)
  except KeyError:
    logging.error('List of versions not in the expected format')
  if not milestone:
    return None
  logging.info('Matched rev %s to milestone %s.', end_revision, milestone)
  return 'M-%d' % milestone


def _GetMilestoneForRevision(revision):
  """Finds the oldest milestone for a given revision from OmahaProxy.

  The purpose of this function is to resolve the milestone that would be blocked
  by a suspected regression. We do this by locating in the list of current
  versions, regardless of platform and channel, all the version strings (e.g.
  36.0.1234.56) that match revisions (commit positions) later than the earliest
  possible end_revision of the suspected regression; we then parse out the
  first numeric part of such strings, assume it to be the corresponding
  milestone, and return the lowest one in the set.

  Args:
    revision: An integer or string containing an integer.

  Returns:
    An integer representing the lowest milestone matching the given revision or
    the highest milestone if the given revision exceeds all defined milestones.
    Note that the default is 0 when no milestones at all are found. If the
    given revision is None, then None is returned.
  """
  if revision is None:
    return None
  milestones = set()
  default_milestone = 0
  all_versions = _GetAllCurrentVersionsFromOmahaProxy()
  for os in all_versions:
    for version in os['versions']:
      try:
        milestone = int(version['current_version'].split('.')[0])
        version_commit = version.get('branch_base_position')
        if version_commit and int(revision) < int(version_commit):
          milestones.add(milestone)
        if milestone > default_milestone:
          default_milestone = milestone
      except ValueError:
        # Sometimes 'N/A' is given. We ignore these entries.
        logging.warn('Could not cast one of: %s, %s, %s as an int',
                     revision, version['branch_base_position'],
                     version['current_version'].split('.')[0])
  if milestones:
    return min(milestones)
  return default_milestone


def _GetAllCurrentVersionsFromOmahaProxy():
  """Retrieves a the list current versions from OmahaProxy and parses it."""
  try:
    response = urlfetch.fetch(_OMAHA_PROXY_URL)
    if response.status_code == 200:
      return json.loads(response.content)
  except urlfetch.Error:
    logging.error('Error pulling list of current versions (omahaproxy).')
  except ValueError:
    logging.error('OmahaProxy did not return valid JSON.')
  return []


def _GetSingleCLForAnomalies(alerts):
  """If all anomalies were caused by the same culprit, return it. Else None."""
  revision = alerts[0].start_revision
  if not all(a.start_revision == revision and
             a.end_revision == revision for a in alerts):
    return None
  return revision

def _AssignBugToCLAuthor(bug_id, alert, service):
  """Assigns the bug to the author of the given revision."""
  repository_url = None
  repositories = namespaced_stored_object.Get('repositories')
  test_path = utils.TestPath(alert.test)
  if test_path.startswith('ChromiumPerf'):
    repository_url = repositories['chromium']['repository_url']
  elif test_path.startswith('ClankInternal'):
    repository_url = repositories['clank']['repository_url']
  if not repository_url:
    # Can't get committer info from this repository.
    return

  rev = str(auto_bisect.GetRevisionForBisect(alert.end_revision, alert.test))
  # TODO(sullivan, dtu): merge this with similar pinoint code.
  if (re.match(r'^[0-9]{5,7}$', rev) and
      repository_url == repositories['chromium']['repository_url']):
    # This is a commit position, need the git hash.
    result = crrev_service.GetNumbering(
        number=rev,
        numbering_identifier='refs/heads/master',
        numbering_type='COMMIT_POSITION',
        project='chromium',
        repo='chromium/src')
    rev = result['git_sha']
  if not re.match(r'[a-fA-F0-9]{40}$', rev):
    # This still isn't a git hash; can't assign bug.
    return

  commit_info = gitiles_service.CommitInfo(repository_url, rev)
  author = commit_info['author']['email']
  sheriff = utils.GetSheriffForAutorollCommit(commit_info)
  if sheriff:
    service.AddBugComment(
        bug_id,
        ('Assigning to sheriff %s because this autoroll is '
         'the only CL in range:\n%s') % (sheriff, commit_info['message']),
        status='Assigned',
        owner=sheriff)
  else:
    service.AddBugComment(
        bug_id,
        'Assigning to %s because this is the only CL in range:\n%s' % (
            author, commit_info['message']),
        status='Assigned',
        owner=author)
