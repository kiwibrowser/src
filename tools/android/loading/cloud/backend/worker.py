# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import json
import logging
import os
import random
import sys
import time

from googleapiclient import discovery
from oauth2client.client import GoogleCredentials

# NOTE: The parent directory needs to be first in sys.path to avoid conflicts
# with catapult modules that have colliding names, as catapult inserts itself
# into the path as the second element. This is an ugly and fragile hack.
_CLOUD_DIR = os.path.join(os.path.dirname(os.path.realpath(__file__)),
                          os.pardir)
sys.path.insert(0, os.path.join(_CLOUD_DIR, os.pardir))
# Add _CLOUD_DIR to the path to access common code through the same path as the
# frontend.
sys.path.append(_CLOUD_DIR)

from common.clovis_task import ClovisTask
import common.google_bigquery_helper
from common.google_instance_helper import GoogleInstanceHelper
from clovis_task_handler import ClovisTaskHandler
from failure_database import FailureDatabase
from google_storage_accessor import GoogleStorageAccessor


class Worker(object):
  def __init__(self, config, logger):
    """See README.md for the config format."""
    self._project_name = config['project_name']
    self._taskqueue_tag = config['taskqueue_tag']
    self._src_path = config['src_path']
    self._instance_name = config.get('instance_name')
    self._worker_log_path = config.get('worker_log_path')
    self._credentials = GoogleCredentials.get_application_default()
    self._logger = logger
    self._self_destruct = config.get('self_destruct')
    if self._self_destruct and not self._instance_name:
      self._logger.error('Self destruction requires an instance name.')

    # Separate the task storage path into the bucket and the base path under
    # the bucket.
    storage_path_components = config['task_storage_path'].split('/')
    self._bucket_name = storage_path_components[0]
    self._base_path_in_bucket = ''
    if len(storage_path_components) > 1:
      self._base_path_in_bucket = '/'.join(storage_path_components[1:])
      if not self._base_path_in_bucket.endswith('/'):
        self._base_path_in_bucket += '/'

    self._google_storage_accessor = GoogleStorageAccessor(
        credentials=self._credentials, project_name=self._project_name,
        bucket_name=self._bucket_name)

    if self._instance_name:
      failure_database_filename = \
          'failure_database_%s.json' % self._instance_name
    else:
      failure_database_filename = 'failure_dabatase.json'
    self._failure_database_path = os.path.join(self._base_path_in_bucket,
                                               failure_database_filename)

    # Recover any existing failures in case the worker died.
    self._failure_database = self._GetFailureDatabase()

    if self._failure_database.ToJsonDict():
      # Script is restarting after a crash, or there are already files from a
      # previous run in the directory.
      self._failure_database.AddFailure(FailureDatabase.DIRTY_STATE_ERROR,
                                        'failure_database')

    bigquery_service = common.google_bigquery_helper.GetBigQueryService(
        self._credentials)
    self._clovis_task_handler = ClovisTaskHandler(
        self._project_name, self._base_path_in_bucket, self._failure_database,
        self._google_storage_accessor, bigquery_service,
        config['binaries_path'], config['ad_rules_filename'],
        config['tracking_rules_filename'], self._logger, self._instance_name)

    self._UploadFailureDatabase()

  def Start(self):
    """Main worker loop.

    Repeatedly pulls tasks from the task queue and processes them. Returns when
    the queue is empty.
    """
    task_api = discovery.build('taskqueue', 'v1beta2',
                               credentials=self._credentials)
    queue_name = 'clovis-queue'
    # Workaround for
    # https://code.google.com/p/googleappengine/issues/detail?id=10199
    project = 's~' + self._project_name

    while True:
      self._logger.debug('Fetching new task.')
      (clovis_task, task_id) = self._FetchClovisTask(project, task_api,
                                                     queue_name)
      if not clovis_task:
        break

      self._logger.info('Processing task %s' % task_id)
      self._clovis_task_handler.Run(clovis_task)
      self._UploadFailureDatabase()
      self._logger.debug('Deleting task %s' % task_id)
      task_api.tasks().delete(project=project, taskqueue=queue_name,
                              task=task_id).execute()
      self._logger.info('Finished task %s' % task_id)
    self._Finalize()

  def _GetFailureDatabase(self):
    """Downloads the failure database from CloudStorage."""
    self._logger.info('Downloading failure database')
    failure_database_string = self._google_storage_accessor.DownloadAsString(
        self._failure_database_path)
    return FailureDatabase(failure_database_string)

  def _UploadFailureDatabase(self):
    """Uploads the failure database to CloudStorage."""
    if not self._failure_database.is_dirty:
      return
    self._logger.info('Uploading failure database')
    self._google_storage_accessor.UploadString(
        self._failure_database.ToJsonString(),
        self._failure_database_path)
    self._failure_database.is_dirty = False

  def _FetchClovisTask(self, project_name, task_api, queue_name):
    """Fetches a ClovisTask from the task queue.

    Params:
      project_name(str): The name of the Google Cloud project.
      task_api: The TaskQueue service.
      queue_name(str): The name of the task queue.

    Returns:
      (ClovisTask, str): The fetched ClovisTask and its task ID, or (None, None)
                         if no tasks are found.
    """
    response = task_api.tasks().lease(
        project=project_name, taskqueue=queue_name, numTasks=1, leaseSecs=600,
        groupByTag=True, tag=self._taskqueue_tag).execute()
    if (not response.get('items')) or (len(response['items']) < 1):
      return (None, None)  # The task queue is empty.

    google_task = response['items'][0]
    task_id = google_task['id']

    # Delete the task without processing if it already failed multiple times.
    # TODO(droger): This is a workaround for internal bug b/28442122, revisit
    # once it is fixed.
    retry_count = google_task['retry_count']
    max_retry_count = 3
    skip_task = retry_count >= max_retry_count
    if skip_task:
      task_api.tasks().delete(project=project_name, taskqueue=queue_name,
                              task=task_id).execute()

    clovis_task = ClovisTask.FromBase64(google_task['payloadBase64'])

    if retry_count > 0:
      self._failure_database.AddFailure('task_queue_retry',
                                        clovis_task.ToJsonString())
      self._UploadFailureDatabase()

    if skip_task:
      return self._FetchClovisTask(project_name, task_api, queue_name)

    return (clovis_task, task_id)

  def _Finalize(self):
    """Called before exiting."""
    self._logger.info('Done')
    self._clovis_task_handler.Finalize()
    # Upload the worker log.
    if self._worker_log_path:
      self._logger.info('Uploading worker log.')
      remote_log_path = os.path.join(self._base_path_in_bucket, 'worker_log')
      if self._instance_name:
        remote_log_path += '_' + self._instance_name
      self._google_storage_accessor.UploadFile(self._worker_log_path,
                                               remote_log_path)
    # Self destruct.
    if self._self_destruct:
      # Workaround for ComputeEngine internal bug b/28760288.
      random_delay = random.random() * 600.0  # Up to 10 minutes.
      self._logger.info(
          'Wait %.0fs to avoid load spikes on compute engine.' % random_delay)
      time.sleep(random_delay)

      self._logger.info('Starting instance destruction: ' + self._instance_name)
      google_instance_helper = GoogleInstanceHelper(
          self._credentials, self._project_name, self._logger)
      success = google_instance_helper.DeleteInstance(self._taskqueue_tag,
                                                      self._instance_name)
      if not success:
        self._logger.error('Self destruction failed.')
    # Do not add anything after this line, as the instance might be killed at
    # any time.

if __name__ == '__main__':
  parser = argparse.ArgumentParser(
      description='ComputeEngine Worker for Clovis')
  parser.add_argument('--config', required=True,
                      help='Path to the configuration file.')
  args = parser.parse_args()

  # Configure logging.
  logging.basicConfig(level=logging.WARNING,
                      format='[%(asctime)s][%(levelname)s] %(message)s',
                      datefmt='%y-%m-%d %H:%M:%S')
  logging.Formatter.converter = time.gmtime
  worker_logger = logging.getLogger('worker')
  worker_logger.setLevel(logging.INFO)

  worker_logger.info('Reading configuration')
  with open(args.config) as config_json:
    worker = Worker(json.load(config_json), worker_logger)
    worker.Start()
