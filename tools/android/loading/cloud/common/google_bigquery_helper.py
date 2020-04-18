# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import uuid

from googleapiclient import (discovery, errors)

import common.google_error_helper as google_error_helper

# Name of the dataset.
BIGQUERY_DATASET = 'clovis_dataset'
# Name of the table used as a template for new tables.
BIGQUERY_TABLE_TEMPLATE = 'report'


def GetBigQueryService(credentials):
  """Returns the BigQuery service."""
  return discovery.build('bigquery', 'v2', credentials=credentials)


def GetBigQueryTableID(clovis_report_task):
  """Returns the ID of the BigQuery table associated with the task.
  This ID is appended at the end of the table name.

  Args:
    clovis_report_task: (ClovisTask) The task, must be a 'report' task.

  Returns:
    str: The table ID.
  """
  assert (clovis_report_task.Action() == 'report')
  # Name the table after the last path component of the trace bucket.
  trace_bucket = clovis_report_task.ActionParams()['trace_bucket']
  table_id = os.path.basename(os.path.normpath(trace_bucket))
  task_name = clovis_report_task.BackendParams().get('task_name')
  if task_name is not None:
    table_id += '_' + task_name
  # BigQuery table names can contain only alpha numeric characters and
  # underscores.
  return ''.join(c for c in table_id if c.isalnum() or c == '_')


def GetBigQueryTableURL(project_name, table_id):
  """Returns the full URL for the BigQuery table associated with table_id."""
  return 'https://bigquery.cloud.google.com/table/%s:%s.%s_%s' % (
      project_name, BIGQUERY_DATASET, BIGQUERY_TABLE_TEMPLATE, table_id)


def InsertInTemplatedBigQueryTable(bigquery_service, project_name, table_id,
                                   rows, logger):
  """Inserts rows in the BigQuery table corresponding to table_id.
  Assumes that the BigQuery dataset and table template already exist.

  Args:
    bigquery_service: The BigQuery service.
    project_name: (str) Name of the Google Cloud project.
    table_id: (str) table_id as returned by GetBigQueryTableID().
    rows: (list) Rows to insert in the table.
    logger: (logging.Logger) The logger.

  Returns:
    dict: The BigQuery service response.
  """
  rows_data = [{'json': row, 'insertId': str(uuid.uuid4())} for row in rows]
  body = {'rows': rows_data, 'templateSuffix':'_'+table_id}
  logger.info('BigQuery API request:\n' + str(body))
  response = bigquery_service.tabledata().insertAll(
      projectId=project_name, datasetId=BIGQUERY_DATASET,
      tableId=BIGQUERY_TABLE_TEMPLATE, body=body).execute()
  logger.info('BigQuery API response:\n' + str(response))
  return response


def DoesBigQueryTableExist(bigquery_service, project_name, table_id, logger):
  """Returns wether the BigQuery table identified by table_id exists.

  Raises a HttpError exception if the call to BigQuery API fails.

  Args:
    bigquery_service: The BigQuery service.
    project_name: (str) Name of the Google Cloud project.
    table_id: (str) table_id as returned by GetBigQueryTableID().

  Returns:
    bool: True if the table exists.
  """
  table_name = BIGQUERY_TABLE_TEMPLATE + '_' + table_id
  logger.info('Getting table information for %s.' % table_name)
  try:
    table = bigquery_service.tables().get(projectId=project_name,
                                          datasetId=BIGQUERY_DATASET,
                                          tableId=table_name).execute()
    return bool(table)

  except errors.HttpError as http_error:
    error_content = google_error_helper.GetErrorContent(http_error)
    error_reason = google_error_helper.GetErrorReason(error_content)
    if error_reason == google_error_helper.REASON_NOT_FOUND:
      return False
    else:
      logger.error('BigQuery API error (reason: "%s"):\n%s' % (
          error_reason, http_error))
      if error_content:
        logger.error('Error details:\n%s' % error_content)
      raise  # Re-raise the exception.

  return False
