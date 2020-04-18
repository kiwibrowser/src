# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import datetime
import math
import os
import sys
import time
import traceback

import cloudstorage
import flask
from google.appengine.api import (app_identity, taskqueue)
from google.appengine.ext import deferred
from oauth2client.client import GoogleCredentials

import common.clovis_paths
from common.clovis_task import ClovisTask
import common.google_bigquery_helper
import common.google_instance_helper
from common.loading_trace_database import LoadingTraceDatabase
import email_helper
from frontend_job import FrontendJob
from memory_logs import MemoryLogs


# Global variables.
logging.Formatter.converter = time.gmtime
clovis_logger = logging.getLogger('clovis_frontend')
clovis_logger.setLevel(logging.DEBUG)
project_name = app_identity.get_application_id()
instance_helper = common.google_instance_helper.GoogleInstanceHelper(
    credentials=GoogleCredentials.get_application_default(),
    project=project_name,
    logger=clovis_logger)
app = flask.Flask(__name__)


def RenderJobCreationPage(message, memory_logs=None):
  """Renders the log.html template.

  Args:
    message (str): Main content of the page.
    memory_logs (MemoryLogs): Optional logs.
  """
  log = None
  if memory_logs:
    log = memory_logs.Flush().split('\n')
  return flask.render_template('log.html', body=message, log=log,
                               title='Job Creation Status')


def PollWorkers(tag, start_time, timeout_hours, email_address, task_url):
  """Checks if there are workers associated with tag, by polling the instance
  group. When all workers are finished, the instance group and the instance
  template are destroyed.
  After some timeout delay, the instance group is destroyed even if there are
  still workers associated to it, which has the effect of killing all these
  workers.

  Args:
    tag (string): Tag of the task that is polled.
    start_time (float): Time when the polling started, as returned by
                        time.time().
    timeout_hours (int): Timeout after which workers are terminated.
    email_address (str): Email address to notify when the task is complete.
    task_url (str): URL where the results of the task can be found.
  """
  if (time.time() - start_time) > (3600 * timeout_hours):
    clovis_logger.error('Worker timeout for tag %s, shuting down.' % tag)
    Finalize(tag, email_address, 'TIMEOUT', task_url)
    return

  clovis_logger.info('Polling workers for tag: ' + tag)
  live_instance_count = instance_helper.GetInstanceCount(tag)
  clovis_logger.info('%i live instances for tag %s.' % (
      live_instance_count, tag))

  if live_instance_count > 0 or live_instance_count == -1:
    clovis_logger.info('Retry later, instances still alive for tag: ' + tag)
    poll_interval_minutes = 10
    deferred.defer(PollWorkers, tag, start_time, timeout_hours, email_address,
                   task_url, _countdown=(60 * poll_interval_minutes))
    return

  Finalize(tag, email_address, 'SUCCESS', task_url)


def Finalize(tag, email_address, status, task_url):
  """Cleans up the remaining ComputeEngine resources and notifies the user.

  Args:
    tag (str): Tag of the task to finalize.
    email_address (str): Email address of the user to be notified.
    status (str): Status of the task, indicating the success or the cause of
                  failure.
    task_url (str): URL where the results of the task can be found.
  """
  email_helper.SendEmailTaskComplete(
      to_address=email_address, tag=tag, status=status, task_url=task_url,
      logger=clovis_logger)
  clovis_logger.info('Scheduling instance group destruction for tag: ' + tag)
  deferred.defer(DeleteInstanceGroup, tag)
  FrontendJob.DeleteForTag(tag)


def GetEstimatedTaskDurationInSeconds(task):
  """Returns an estimation of the time required to run the task.

  Args:
    task: (ClovisTask) The task.

  Returns:
    float: Time estimation in seconds, or -1 in case of failure.
  """
  action_params = task.ActionParams()
  if task.Action() == 'trace':
    estimated_trace_time_s = 40.0
    return (len(action_params['urls']) * action_params.get('repeat_count', 1) *
            estimated_trace_time_s)
  elif task.Action() == 'report':
    estimated_report_time_s = 20.0
    return len(action_params['traces']) * estimated_report_time_s
  else:
    clovis_logger.error('Unexpected action.')
    return -1


def CreateInstanceTemplate(task, task_dir):
  """Create the Compute Engine instance template that will be used to create the
  instances.
  """
  backend_params = task.BackendParams()
  instance_count = backend_params.get('instance_count', 0)
  if instance_count <= 0:
    clovis_logger.info('No template required.')
    return True
  bucket = backend_params.get('storage_bucket')
  if not bucket:
    clovis_logger.error('Missing bucket in backend_params.')
    return False
  return instance_helper.CreateTemplate(task.BackendParams()['tag'], bucket,
                                        task_dir)


def CreateInstances(task):
  """Creates the Compute engine requested by the task."""
  backend_params = task.BackendParams()
  instance_count = backend_params.get('instance_count', 0)
  if instance_count <= 0:
    clovis_logger.info('No instances to create.')
    return True
  return instance_helper.CreateInstances(backend_params['tag'], instance_count)


def DeleteInstanceGroup(tag, try_count=0):
  """Deletes the instance group associated with tag, and schedules the deletion
  of the instance template."""
  clovis_logger.info('Instance group destruction for tag: ' + tag)
  if not instance_helper.DeleteInstanceGroup(tag):
    clovis_logger.info('Instance group destruction failed for: ' + tag)
    if try_count <= 5:
      deferred.defer(DeleteInstanceGroup, tag, try_count + 1, _countdown=60)
      return
    clovis_logger.error('Giving up group destruction for: ' + tag)
  clovis_logger.info('Scheduling instance template destruction for tag: ' + tag)
  # Wait a little before deleting the instance template, because it may still be
  # considered in use, causing failures.
  deferred.defer(DeleteInstanceTemplate, tag, _countdown=30)


def DeleteInstanceTemplate(tag, try_count=0):
  """Deletes the instance template associated with tag."""
  clovis_logger.info('Instance template destruction for tag: ' + tag)
  if not instance_helper.DeleteTemplate(tag):
    clovis_logger.info('Instance template destruction failed for: ' + tag)
    if try_count <= 5:
      deferred.defer(DeleteInstanceTemplate, tag, try_count + 1, _countdown=60)
      return
    clovis_logger.error('Giving up template destruction for: ' + tag)
  clovis_logger.info('Cleanup complete for tag: ' + tag)


def SplitClovisTask(task):
  """Splits a ClovisTask in smaller ClovisTasks.

  Args:
    task: (ClovisTask) The task to split.

  Returns:
    list: The list of ClovisTasks.
  """
  # For report task, need to find the traces first.
  if task.Action() == 'report':
    trace_bucket = task.ActionParams().get('trace_bucket')
    if not trace_bucket:
      clovis_logger.error('Missing trace bucket for report task.')
      return None

    # Allow passing the trace bucket as absolute or relative to the base bucket.
    base_bucket = task.BackendParams().get('storage_bucket', '')
    if not trace_bucket.startswith(base_bucket):
      trace_bucket = os.path.join(base_bucket, trace_bucket)

    traces = GetTracePaths(trace_bucket)
    if not traces:
      clovis_logger.error('No traces found in bucket: ' + trace_bucket)
      return None
    task.ActionParams()['traces'] = traces

  # Compute the split key.
  # Keep the tasks small, but large enough to avoid "rate exceeded" errors when
  # pulling them from the TaskQueue.
  split_params_for_action = {'trace': ('urls', 3), 'report': ('traces', 5)}
  (split_key, slice_size) = split_params_for_action.get(task.Action(),
                                                        (None, 0))
  if not split_key:
    clovis_logger.error('Cannot split task with action: ' + task.Action())
    return None

  # Split the task using the split key.
  clovis_logger.debug('Splitting task by: ' + split_key)
  action_params = task.ActionParams()
  values = action_params[split_key]
  sub_tasks = []
  for i in range(0, len(values), slice_size):
    sub_task_params = action_params.copy()
    sub_task_params[split_key] = [v for v in values[i:i+slice_size]]
    sub_tasks.append(ClovisTask(task.Action(), sub_task_params,
                                task.BackendParams()))
  return sub_tasks


def GetTracePaths(bucket):
  """Returns a list of trace files in a bucket.

  Finds and loads the trace databases, and returns their content as a list of
  paths.

  This function assumes a specific structure for the files in the bucket. These
  assumptions must match the behavior of the backend:
  - The trace databases are located in the bucket.
  - The trace databases files are the only objects with the
    TRACE_DATABASE_PREFIX prefix in their name.

  Returns:
    list: The list of paths to traces, as strings.
  """
  traces = []
  prefix = os.path.join('/', bucket, common.clovis_paths.TRACE_DATABASE_PREFIX)
  file_stats = cloudstorage.listbucket(prefix)

  for file_stat in file_stats:
    database_file = file_stat.filename
    clovis_logger.info('Loading trace database: ' + database_file)

    with cloudstorage.open(database_file) as remote_file:
      json_string = remote_file.read()
    if not json_string:
      clovis_logger.warning('Failed to download: ' + database_file)
      continue

    database = LoadingTraceDatabase.FromJsonString(json_string)
    if not database:
      clovis_logger.warning('Failed to parse: ' + database_file)
      continue

    for path in database.ToJsonDict():
      traces.append(path)

  return traces


def GetTaskURL(task, task_dir):
  """Returns the URL where the task output are generated, or None.

  Args:
    task: (ClovisTask) The task.
    task_dir: (str) Working directory for the backend, it is a subdirectory of
                    the deployment bucket.

  Returns:
    str: The URL.
  """
  clovis_logger.info('Building task result URL.')

  if task.Action() == 'trace':
    storage_bucket = task.BackendParams().get('storage_bucket')
    if not storage_bucket:
      clovis_logger.error('Missing storage_bucket for trace action.')
      return None
    return 'https://console.cloud.google.com/storage/%s/%s' % (storage_bucket,
                                                               task_dir)

  elif task.Action() == 'report':
    table_id = common.google_bigquery_helper.GetBigQueryTableID(task)
    task_url = common.google_bigquery_helper.GetBigQueryTableURL(project_name,
                                                                 table_id)
    # Abort if the table already exists.
    bigquery_service = common.google_bigquery_helper.GetBigQueryService(
        GoogleCredentials.get_application_default())
    try:
      table_exists = common.google_bigquery_helper.DoesBigQueryTableExist(
          bigquery_service, project_name, table_id, clovis_logger)
    except Exception:
      return None
    if table_exists:
      clovis_logger.error('BigQuery table %s already exists.' % task_url)
      return None
    return task_url

  else:
    clovis_logger.error('Unsupported action: %s.' % task.Action())
    return None


def StartFromJsonString(http_body_str):
  """Main function handling a JSON task posted by the user."""
  # Set up logging.
  memory_logs = MemoryLogs(clovis_logger)
  memory_logs.Start()

  # Load the task from JSON.
  task = ClovisTask.FromJsonString(http_body_str)
  if not task:
    clovis_logger.error('Invalid JSON task.')
    return RenderJobCreationPage(
        'Invalid JSON task:\n' + http_body_str, memory_logs)

  task_tag = task.BackendParams()['tag']
  clovis_logger.info('Start processing %s task with tag %s.' % (task.Action(),
                                                                task_tag))
  user_email = email_helper.GetUserEmail()

  # Write the job to the datastore.
  frontend_job = FrontendJob.CreateForTag(task_tag)
  frontend_job.email = user_email
  frontend_job.status = 'not_started'
  frontend_job.clovis_task = task.ToJsonString()
  frontend_job.put()

  # Process the job on the queue, to avoid timeout issues.
  deferred.defer(SpawnTasksOnBackgroundQueue, task_tag)

  return RenderJobCreationPage(
      flask.Markup(
          '<a href="%s">See progress.</a>' % FrontendJob.GetJobURL(task_tag)),
      memory_logs)


def SpawnTasksOnBackgroundQueue(task_tag):
  """Spawns Clovis tasks associated with task_tag from the backgound queue.

  This function is mostly a wrapper around SpawnTasks() that catches exceptions.
  It is assumed that a FrontendJob for task_tag exists.
  """
  memory_logs = MemoryLogs(clovis_logger)
  memory_logs.Start()
  clovis_logger.info('Spawning tasks on background queue.')

  try:
    frontend_job = FrontendJob.GetFromTag(task_tag)
    frontend_job.status = 'will_start'
    SpawnTasks(frontend_job)
  except Exception as e:
    clovis_logger.error('Exception spawning tasks: ' + str(e))
    clovis_logger.error(traceback.print_exc())

  # Update the task.
  if frontend_job:
    frontend_job.log = memory_logs.Flush()
    frontend_job.put()


def SpawnTasks(frontend_job):
  """ Spawns Clovis tasks associated with the frontend job."""
  user_email = frontend_job.email
  task = ClovisTask.FromJsonString(frontend_job.clovis_task)
  task_tag = task.BackendParams()['tag']

  # Delete the clovis task from the FrontendJob because it can make the object
  # very heavy and it is no longer needed.
  frontend_job.clovis_task = None

  # Compute the task directory.
  frontend_job.status = 'building_task_dir'
  task_dir_components = []
  user_name = None
  if user_email:
    user_name = user_email[:user_email.find('@')]
  if user_name:
    task_dir_components.append(user_name)
  task_name = task.BackendParams().get('task_name')
  if task_name:
    task_dir_components.append(task_name)
  task_dir_components.append(task_tag)
  task_dir = os.path.join(task.Action(), '_'.join(task_dir_components))

  # Build the URL where the result will live.
  frontend_job.status = 'building_task_url'
  task_url = GetTaskURL(task, task_dir)
  if task_url:
    clovis_logger.info('Task result URL: ' + task_url)
    frontend_job.task_url = task_url
  else:
    frontend_job.status = 'task_url_error'
    return

  # Split the task in smaller tasks.
  frontend_job.status = 'splitting_task'
  frontend_job.put()
  sub_tasks = SplitClovisTask(task)
  if not sub_tasks:
    frontend_job.status = 'task_split_error'
    return

  # Compute estimates for the work duration, in order to compute the instance
  # count and the timeout.
  frontend_job.status = 'estimating_duration'
  sequential_duration_s = \
      GetEstimatedTaskDurationInSeconds(sub_tasks[0]) * len(sub_tasks)
  if sequential_duration_s <= 0:
    frontend_job.status = 'time_estimation_error'
    return

  # Compute the number of required instances if not specified.
  if task.BackendParams().get('instance_count') is None:
    frontend_job.status = 'estimating_instance_count'
    target_parallel_duration_s = 1800.0 # 30 minutes.
    task.BackendParams()['instance_count'] = math.ceil(
        sequential_duration_s / target_parallel_duration_s)

  # Check the instance quotas.
  clovis_logger.info(
      'Requesting %i instances.' % task.BackendParams()['instance_count'])
  frontend_job.status = 'checking_instance_quotas'
  max_instances = instance_helper.GetAvailableInstanceCount()
  if max_instances == -1:
    frontend_job.status = 'instance_count_error'
    return
  elif max_instances == 0 and task.BackendParams()['instance_count'] > 0:
    frontend_job.status = 'no_instance_available_error'
    return
  elif max_instances < task.BackendParams()['instance_count']:
    clovis_logger.warning(
        'Instance count limited by quota: %i available / %i requested.' % (
            max_instances, task.BackendParams()['instance_count']))
    task.BackendParams()['instance_count'] = max_instances

  # Compute the timeout if there is none specified.
  expected_duration_s = sequential_duration_s / (
      task.BackendParams()['instance_count'])
  frontend_job.eta = datetime.datetime.now() + datetime.timedelta(
      seconds=expected_duration_s)
  if not task.BackendParams().get('timeout_hours'):
    # Timeout is at least 1 hour.
    task.BackendParams()['timeout_hours'] = max(
        1, 5 * expected_duration_s / 3600.0)
  clovis_logger.info(
      'Timeout delay: %.1f hours. ' % task.BackendParams()['timeout_hours'])

  frontend_job.status = 'queueing_tasks'
  if not EnqueueTasks(sub_tasks, task_tag):
    frontend_job.status = 'task_creation_error'
    return

  # Start polling the progress.
  clovis_logger.info('Creating worker polling task.')
  first_poll_delay_minutes = 10
  deferred.defer(PollWorkers, task_tag, time.time(),
                 task.BackendParams()['timeout_hours'], user_email,
                 task_url, _countdown=(60 * first_poll_delay_minutes))

  # Start the instances if required.
  frontend_job.status = 'creating_instances'
  frontend_job.put()
  if not CreateInstanceTemplate(task, task_dir):
    frontend_job.status = 'instance_template_error'
    return
  if not CreateInstances(task):
    frontend_job.status = 'instance_creation_error'
    return

  frontend_job.status = 'started'

def EnqueueTasks(tasks, task_tag):
  """Enqueues a list of tasks in the Google Cloud task queue, for consumption by
  Google Compute Engine.
  """
  q = taskqueue.Queue('clovis-queue')
  # Add tasks to the queue by groups.
  # TODO(droger): This supports thousands of tasks, but maybe not millions.
  # Defer the enqueuing if it times out.
  group_size = 100
  callbacks = []
  try:
    for i in range(0, len(tasks), group_size):
      group = tasks[i:i+group_size]
      taskqueue_tasks = [
          taskqueue.Task(payload=task.ToJsonString(), method='PULL',
                         tag=task_tag)
          for task in group]
      rpc = taskqueue.create_rpc()
      q.add_async(task=taskqueue_tasks, rpc=rpc)
      callbacks.append(rpc)
    for callback in callbacks:
      callback.get_result()
  except Exception as e:
    clovis_logger.error('Exception:' + type(e).__name__ + ' ' + str(e.args))
    return False
  clovis_logger.info('Pushed %i tasks with tag: %s.' % (len(tasks), task_tag))
  return True


@app.route('/')
def Root():
  """Home page: show the new task form."""
  return flask.render_template('form.html')


@app.route('/form_sent', methods=['POST'])
def StartFromForm():
  """HTML form endpoint."""
  data_stream = flask.request.files.get('json_task')
  if not data_stream:
    return RenderJobCreationPage('Failed, no content.')
  http_body_str = data_stream.read()
  return StartFromJsonString(http_body_str)


@app.route('/kill_job')
def KillJob():
  tag = flask.request.args.get('tag')
  page_title = 'Kill Job'
  if not tag:
    return flask.render_template('log.html', body='Failed: Invalid tag.',
                                 title=page_title)

  frontend_job = FrontendJob.GetFromTag(tag)

  if not frontend_job:
    return flask.render_template('log.html', body='Job not found.',
                                 title=page_title)
  Finalize(tag, frontend_job.email, 'CANCELED', frontend_job.task_url)

  body = 'Killed job %s.' % tag
  return flask.render_template('log.html', body=body, title=page_title)


@app.route('/list_jobs')
def ShowJobList():
  """Shows a list of all active jobs."""
  tags = FrontendJob.ListJobs()
  page_title = 'Active Jobs'

  if not tags:
    return flask.render_template('log.html', body='No active job.',
                                 title=page_title)

  html = ''
  for tag in tags:
    html += flask.Markup(
        '<li><a href="%s">%s</a></li>') % (FrontendJob.GetJobURL(tag), tag)
  html += flask.Markup('</ul>')
  return flask.render_template('log.html', body=html, title=page_title)


@app.route(FrontendJob.SHOW_JOB_URL)
def ShowJob():
  """Shows basic information abour a job."""
  tag = flask.request.args.get('tag')
  page_title = 'Job Information'
  if not tag:
    return flask.render_template('log.html', body='Invalid tag.',
                                 title=page_title)

  frontend_job = FrontendJob.GetFromTag(tag)

  if not frontend_job:
    return flask.render_template('log.html', body='Job not found.',
                                 title=page_title)

  log = None
  if frontend_job.log:
    log = frontend_job.log.split('\n')

  body = flask.Markup(frontend_job.RenderAsHtml())
  body += flask.Markup('<a href="/kill_job?tag=%s">Kill</a>' % tag)
  return flask.render_template('log.html', log=log, title=page_title,
                               body=body)


@app.errorhandler(404)
def PageNotFound(e):  # pylint: disable=unused-argument
  """Return a custom 404 error."""
  return 'Sorry, Nothing at this URL.', 404


@app.errorhandler(500)
def ApplicationError(e):
  """Return a custom 500 error."""
  return 'Sorry, unexpected error: {}'.format(e), 499
