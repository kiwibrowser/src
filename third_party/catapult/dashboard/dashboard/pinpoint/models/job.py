# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import os
import sys
import traceback
import uuid

from google.appengine.api import datastore_errors
from google.appengine.api import taskqueue
from google.appengine.ext import ndb
from google.appengine.runtime import apiproxy_errors

from dashboard.common import utils
from dashboard.pinpoint.models import job_state
from dashboard.pinpoint.models import results2
from dashboard.services import issue_tracker_service


# We want this to be fast to minimize overhead while waiting for tasks to
# finish, but don't want to consume too many resources.
_TASK_INTERVAL = 60


_CRYING_CAT_FACE = u'\U0001f63f'
_ROUND_PUSHPIN = u'\U0001f4cd'


OPTION_STATE = 'STATE'
OPTION_TAGS = 'TAGS'


COMPARISON_MODES = job_state.COMPARISON_MODES


def JobFromId(job_id):
  """Get a Job object from its ID. Its ID is just its key as a hex string.

  Users of Job should not have to import ndb. This function maintains an
  abstraction layer that separates users from the Datastore details.
  """
  job_key = ndb.Key('Job', int(job_id, 16))
  return job_key.get()


class Job(ndb.Model):
  """A Pinpoint job."""

  created = ndb.DateTimeProperty(required=True, auto_now_add=True)
  # Don't use `auto_now` for `updated`. When we do data migration, we need
  # to be able to modify the Job without changing the Job's completion time.
  updated = ndb.DateTimeProperty(required=True, auto_now_add=True)

  # The name of the Task Queue task this job is running on. If it's present, the
  # job is running. The task is also None for Task Queue retries.
  task = ndb.StringProperty()

  # The string contents of any Exception that was thrown to the top level.
  # If it's present, the job failed.
  exception = ndb.TextProperty()

  # Request parameters.
  arguments = ndb.JsonProperty(required=True)

  # TODO: The bug id is only used for posting bug comments when a job starts and
  # completes. This probably should not be the responsibility of Pinpoint.
  bug_id = ndb.IntegerProperty()

  # Email of the job creator.
  user = ndb.StringProperty()

  state = ndb.PickleProperty(required=True, compressed=True)

  tags = ndb.JsonProperty()

  @classmethod
  def New(cls, quests, changes, arguments=None, bug_id=None,
          comparison_mode=None, pin=None, tags=None, user=None):
    """Creates a new Job, adds Changes to it, and puts it in the Datstore.

    Args:
      quests: An iterable of Quests for the Job to run.
      changes: An iterable of the initial Changes to run on.
      arguments: A dict with the original arguments used to start the Job.
      bug_id: A monorail issue id number to post Job updates to.
      comparison_mode: Either 'functional' or 'performance', which the Job uses
          to figure out whether to perform a functional or performance bisect.
          If None, the Job will not automatically add any Attempts or Changes.
      pin: A Change (Commits + Patch) to apply to every Change in this Job.
      tags: A dict of key-value pairs used to filter the Jobs listings.
      user: The email of the Job creator.

    Returns:
      A Job object.
    """
    state = job_state.JobState(quests, comparison_mode=comparison_mode, pin=pin)
    job = cls(state=state, arguments=arguments or {},
              bug_id=bug_id, tags=tags, user=user)

    for c in changes:
      job.AddChange(c)

    job.put()
    return job

  @property
  def job_id(self):
    return '%x' % self.key.id()

  @property
  def status(self):
    if self.task:
      return 'Running'

    if self.exception:
      return 'Failed'

    return 'Completed'

  @property
  def url(self):
    return 'https://%s/job/%s' % (os.environ['HTTP_HOST'], self.job_id)

  def AddChange(self, change):
    self.state.AddChange(change)

  def Start(self):
    """Starts the Job and updates it in the Datastore.

    This method is designed to return fast, so that Job creation is responsive
    to the user. It schedules the Job on the task queue without running
    anything. It also posts a bug comment, and updates the Datastore.
    """
    self._Schedule()
    self.put()

    title = _ROUND_PUSHPIN + ' Pinpoint job started.'
    comment = '\n'.join((title, self.url))
    self._PostBugComment(comment, send_email=False)

  def _Complete(self):
    try:
      results2.ScheduleResults2Generation(self)
    except taskqueue.Error:
      pass

    # Format bug comment.

    if not self.state.comparison_mode:
      # There is no comparison metric.
      title = "<b>%s Job complete. See results below.</b>" % _ROUND_PUSHPIN
      self._PostBugComment('\n'.join((title, self.url)))
      return

    # There is a comparison metric.
    differences = tuple(self.state.Differences())

    if not differences:
      title = "<b>%s Couldn't reproduce a difference.</b>" % _ROUND_PUSHPIN
      self._PostBugComment('\n'.join((title, self.url)))
      return

    # Include list of Changes.
    owner = None
    sheriff = None
    cc_list = set()
    commit_details = []
    for _, change in differences:
      if change.patch:
        commit_info = change.patch.AsDict()
      else:
        commit_info = change.last_commit.AsDict()

      # TODO: Assign the largest difference, not the last one.
      owner = commit_info['author']
      sheriff = utils.GetSheriffForAutorollCommit(commit_info)
      cc_list.add(commit_info['author'])
      commit_details.append(_FormatCommitForBug(commit_info))

    # Header.
    if len(differences) == 1:
      status = 'Found a significant difference after 1 commit.'
    else:
      status = ('Found significant differences after each of %d commits.' %
                len(differences))

    title = '<b>%s %s</b>' % (_ROUND_PUSHPIN, status)
    header = '\n'.join((title, self.url))

    # Body.
    body = '\n\n'.join(commit_details)
    if sheriff:
      owner = sheriff
      body += '\n\nAssigning to sheriff %s because "%s" is a roll.' % (
          sheriff, commit_info['subject'])

    # Footer.
    footer = ('Understanding performance regressions:\n'
              '  http://g.co/ChromePerformanceRegressions')

    # Bring it all together.
    comment = '\n\n'.join((header, body, footer))
    current_bug_status = self._GetBugStatus()
    if (not current_bug_status or
        current_bug_status in ['Untriaged', 'Unconfirmed', 'Available']):
      # Set the bug status and owner if this bug is opened and unowned.
      self._PostBugComment(comment, status='Assigned',
                           cc_list=sorted(cc_list), owner=owner)
    else:
      # Only update the comment and cc list if this bug is assigned or closed.
      self._PostBugComment(comment, cc_list=sorted(cc_list))

  def Fail(self):
    self.exception = traceback.format_exc()

    title = _CRYING_CAT_FACE + ' Pinpoint job stopped with an error.'
    comment = '\n'.join((title, self.url, '', sys.exc_value.message))
    self._PostBugComment(comment)

  def _Schedule(self):
    # Set a task name to deduplicate retries. This adds some latency, but we're
    # not latency-sensitive. If Job.Run() works asynchronously in the future,
    # we don't need to worry about duplicate tasks.
    # https://github.com/catapult-project/catapult/issues/3900
    task_name = str(uuid.uuid4())
    try:
      task = taskqueue.add(
          queue_name='job-queue', url='/api/run/' + self.job_id,
          name=task_name, countdown=_TASK_INTERVAL)
    except apiproxy_errors.DeadlineExceededError:
      task = taskqueue.add(
          queue_name='job-queue', url='/api/run/' + self.job_id,
          name=task_name, countdown=_TASK_INTERVAL)

    self.task = task.name

  def Run(self):
    """Runs this Job.

    Loops through all Attempts and checks the status of each one, kicking off
    tasks as needed. Does not block to wait for all tasks to finish. Also
    compares adjacent Changes' results and adds any additional Attempts or
    Changes as needed. If there are any incomplete tasks, schedules another
    Run() call on the task queue.
    """
    self.exception = None  # In case the Job succeeds on retry.
    self.task = None  # In case an exception is thrown.

    try:
      if self.state.comparison_mode:
        self.state.Explore()
      work_left = self.state.ScheduleWork()

      # Schedule moar task.
      if work_left:
        self._Schedule()
      else:
        self._Complete()
    except BaseException:
      self.Fail()
      raise
    finally:
      # Don't use `auto_now` for `updated`. When we do data migration, we need
      # to be able to modify the Job without changing the Job's completion time.
      self.updated = datetime.datetime.now()
      try:
        self.put()
      except (datastore_errors.Timeout,
              datastore_errors.TransactionFailedError):
        # Retry once.
        self.put()
      except datastore_errors.BadRequestError:
        if self.task:
          queue = taskqueue.Queue('job-queue')
          queue.delete_tasks(taskqueue.Task(name=self.task))
        self.task = None

        # The _JobState is too large to fit in an ndb property.
        # Load the Job from before we updated it, and fail it.
        job = self.key.get(use_cache=False)
        job.task = None
        job.Fail()
        job.updated = datetime.datetime.now()
        job.put()
        raise

  def AsDict(self, options=None):
    d = {
        'job_id': self.job_id,

        'arguments': self.arguments,
        'bug_id': self.bug_id,
        'comparison_mode': self.state.comparison_mode,
        'user': self.user,

        'created': self.created.isoformat(),
        'updated': self.updated.isoformat(),
        'exception': self.exception,
        'status': self.status,
    }
    if not options:
      return d

    if OPTION_STATE in options:
      d.update(self.state.AsDict())
    if OPTION_TAGS in options:
      d['tags'] = {'tags': self.tags}
    return d

  def _PostBugComment(self, *args, **kwargs):
    if not self.bug_id:
      return

    issue_tracker = issue_tracker_service.IssueTrackerService(
        utils.ServiceAccountHttp())
    issue_tracker.AddBugComment(self.bug_id, *args, **kwargs)

  def _GetBugStatus(self):
    if not self.bug_id:
      return None

    issue_tracker = issue_tracker_service.IssueTrackerService(
        utils.ServiceAccountHttp())
    issue_data = issue_tracker.GetIssue(self.bug_id)
    return issue_data.get('status')


def _FormatCommitForBug(commit_info):
  subject = '<b>%s</b> by %s' % (commit_info['subject'], commit_info['author'])
  return '\n'.join((subject, commit_info['url']))
