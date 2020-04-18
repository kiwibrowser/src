# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from common.clovis_task import ClovisTask
from failure_database import FailureDatabase
from report_task_handler import ReportTaskHandler
from trace_task_handler import TraceTaskHandler


class ClovisTaskHandler(object):
  """Handles all the supported clovis tasks."""

  def __init__(self, project_name, base_path, failure_database,
               google_storage_accessor, bigquery_service, binaries_path,
               ad_rules_filename, tracking_rules_filename, logger,
               instance_name=None):
    """Creates a ClovisTaskHandler.

    Args:
      project_name (str): Name of the project.
      base_path(str): Base path where results are written.
      failure_database (FailureDatabase): Failure Database.
      google_storage_accessor (GoogleStorageAccessor): Cloud storage accessor.
      bigquery_service (googleapiclient.discovery.Resource): Bigquery service.
      binaries_path(str): Path to the directory where Chrome executables are.
      ad_rules_filename (str): Path to the ad filtering rules.
      tracking_rules_filename (str): Path to the tracking filtering rules.
      instance_name(str, optional): Name of the ComputeEngine instance.
    """
    self._failure_database = failure_database
    self._handlers = {
        'trace': TraceTaskHandler(
            base_path, failure_database, google_storage_accessor, binaries_path,
            logger, instance_name),
        'report': ReportTaskHandler(
            project_name, failure_database, google_storage_accessor,
            bigquery_service, logger, ad_rules_filename,
            tracking_rules_filename)}

  def Run(self, clovis_task):
    """Runs a clovis_task.

    Args:
      clovis_task(ClovisTask): The task to run.
    """
    handler = self._handlers.get(clovis_task.Action())
    if not handler:
      self._logger.error('Unsupported task action: %s' % clovis_task.Action())
      self._failure_database.AddFailure('unsupported_action',
                                        clovis_task.Action())
      return
    handler.Run(clovis_task)

  def Finalize(self):
    """Called once before the handler is destroyed."""
    for handler in self._handlers.values():
      handler.Finalize()
