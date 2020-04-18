# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import math

from googleapiclient import errors

import common.google_bigquery_helper
from common.loading_trace_database import LoadingTraceDatabase
import common.google_error_helper as google_error_helper
from failure_database import FailureDatabase
from loading_trace import LoadingTrace
from report import LoadingReport


def LoadRemoteTrace(storage_accessor, remote_trace_path, logger):
  """Loads and returns the LoadingTrace located at the remote trace path.

  Args:
    storage_accessor: (GoogleStorageAccessor) Used to download the trace from
                                              CloudStorage.
    remote_trace_path: (str) Path to the trace file.
  """

  # Cut the gs://<bucket_name> prefix from trace paths if needed.
  prefix = 'gs://%s/' % storage_accessor.BucketName()
  prefix_length = len(prefix)
  if remote_trace_path.startswith(prefix):
    remote_trace_path = remote_trace_path[prefix_length:]

  trace_string = storage_accessor.DownloadAsString(
      remote_trace_path)
  if not trace_string:
    logger.error('Failed to download: ' + remote_trace_path)
    return None

  trace_dict = json.loads(trace_string)
  if not trace_dict:
    logger.error('Failed to parse: ' + remote_trace_path)
    return None

  trace = LoadingTrace.FromJsonDict(trace_dict)
  if not trace:
    logger.error('Invalid format for: ' + remote_trace_path)
    return None

  return trace


class ReportTaskHandler(object):
  """Handles 'report' tasks.

  This handler loads the traces given in the task parameters, generates a report
  from them, and add them to a BigQuery table.
  The BigQuery table is implicitly created from a template (using the stream
  mode), and identified by the task tag.
  """

  def __init__(self, project_name, failure_database, google_storage_accessor,
               bigquery_service, logger, ad_rules_filename,
               tracking_rules_filename):
    self._project_name = project_name
    self._failure_database = failure_database
    self._google_storage_accessor = google_storage_accessor
    self._bigquery_service = bigquery_service
    self._logger = logger
    self._ad_rules_filename = ad_rules_filename
    self._tracking_rules_filename = tracking_rules_filename

  def _IsBigQueryValueValid(self, value):
    """Returns whether a value is valid and can be uploaded to BigQuery."""
    if value is None:
      return False
    # BigQuery rejects NaN.
    if type(value) is float and (math.isnan(value) or math.isinf(value)):
      return False
    return True

  def _StreamRowsToBigQuery(self, rows, table_id):
    """Uploads a list of rows to the BigQuery table associated with the given
    table_id.

    Args:
      rows: (list of dict) Each dictionary is a row to add to the table.
      table_id: (str) Identifier of the BigQuery table to update.
    """
    try:
      response = common.google_bigquery_helper.InsertInTemplatedBigQueryTable(
          self._bigquery_service, self._project_name, table_id, rows,
          self._logger)
    except errors.HttpError as http_error:
      # Handles HTTP error response codes (such as 404), typically indicating a
      # problem in parameters other than 'body'.
      error_content = google_error_helper.GetErrorContent(http_error)
      error_reason = google_error_helper.GetErrorReason(error_content)
      self._logger.error('BigQuery API error (reason: "%s"):\n%s' % (
            error_reason, http_error))
      self._failure_database.AddFailure('big_query_error', error_reason)
      if error_content:
        self._logger.error('Error details:\n%s' % error_content)
      return

    # Handles other errors, typically when the body is ill-formatted.
    insert_errors = response.get('insertErrors')
    if insert_errors:
      self._logger.error('BigQuery API error:\n' + str(insert_errors))
      for insert_error in insert_errors:
        self._failure_database.AddFailure('big_query_insert_error',
                                          str(insert_error.get('errors')))

  def Finalize(self):
    """Called once before the handler is destroyed."""
    pass

  def Run(self, clovis_task):
    """Runs a 'report' clovis_task.

    Args:
      clovis_task: (ClovisTask) The task to run.
    """
    if clovis_task.Action() != 'report':
      self._logger.error('Unsupported task action: %s' % clovis_task.Action())
      self._failure_database.AddFailure(FailureDatabase.CRITICAL_ERROR,
                                        'report_task_handler_run')
      return

    ad_rules = open(self._ad_rules_filename).readlines()
    tracking_rules = open(self._tracking_rules_filename).readlines()

    rows = []
    for path in clovis_task.ActionParams()['traces']:
      self._logger.info('Generating report for: ' + path)
      trace = LoadRemoteTrace(self._google_storage_accessor, path, self._logger)
      if not trace:
        self._logger.error('Failed loading trace at: ' + path)
        self._failure_database.AddFailure('missing_trace_for_report', path)
        continue
      report = LoadingReport(trace, ad_rules, tracking_rules).GenerateReport()
      if not report:
        self._logger.error('Failed generating report for: ' + path)
        self._failure_database.AddFailure('report_generation_failed', path)
        continue
      # Filter out bad values.
      for key, value in report.items():
        if not self._IsBigQueryValueValid(value):
          url = report.get('url')
          self._logger.error('Invalid %s for URL:%s' % (key, url))
          self._failure_database.AddFailure('invalid_bigquery_value', url)
          del report[key]
      rows.append(report)

    if rows:
      table_id = common.google_bigquery_helper.GetBigQueryTableID(clovis_task)
      self._StreamRowsToBigQuery(rows, table_id)
