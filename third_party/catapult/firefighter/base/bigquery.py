# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import io
import json
import logging
import time
import uuid

from google.appengine.api import app_identity

from apiclient import http
from apiclient.discovery import build
from oauth2client import client

from base import exceptions


# urlfetch max size is 10 MB. Assume 1000 bytes per row and split the
# insert into chunks of 10,000 rows.
INSERTION_MAX_ROWS = 10000


class BigQuery(object):
  """Methods for interfacing with BigQuery."""

  def __init__(self, project_id=None):
    self._service = _Service()
    if project_id:
      self._project_id = project_id
    else:
      self._project_id = app_identity.get_application_id()

  def InsertRowsAsync(self, dataset_id, table_id, rows,
                      truncate=False, num_retries=5):
    responses = []
    for i in xrange(0, len(rows), INSERTION_MAX_ROWS):
      rows_chunk = rows[i:i+INSERTION_MAX_ROWS]
      logging.info('Inserting %d rows into %s.%s.',
                   len(rows_chunk), dataset_id, table_id)
      body = {
          'configuration': {
              'jobReference': {
                  'projectId': self._project_id,
                  'jobId': str(uuid.uuid4()),
              },
              'load': {
                  'destinationTable': {
                      'projectId': self._project_id,
                      'datasetId': dataset_id,
                      'tableId': table_id,
                  },
                  'sourceFormat': 'NEWLINE_DELIMITED_JSON',
                  'writeDisposition':
                      'WRITE_TRUNCATE' if truncate else 'WRITE_APPEND',
              }
          }
      }

      # Format rows as newline-delimited JSON.
      media_buffer = io.BytesIO()
      for row in rows_chunk:
        json.dump(row, media_buffer, separators=(',', ':'))
        print >> media_buffer
      media_body = http.MediaIoBaseUpload(
          media_buffer, mimetype='application/octet-stream')

      responses.append(self._service.jobs().insert(
          projectId=self._project_id,
          body=body, media_body=media_body).execute(num_retries=num_retries))

      # Only truncate on the first insert!
      truncate = False

    # TODO(dtu): Return a Job object.
    return responses

  def InsertRowsSync(self, dataset_id, table_id, rows, num_retries=5):
    for i in xrange(0, len(rows), INSERTION_MAX_ROWS):
      rows_chunk = rows[i:i+INSERTION_MAX_ROWS]
      logging.info('Inserting %d rows into %s.%s.',
                   len(rows_chunk), dataset_id, table_id)
      rows_chunk = [{'insertId': str(uuid.uuid4()), 'json': row}
                    for row in rows_chunk]
      insert_data = {'rows': rows_chunk}
      response = self._service.tabledata().insertAll(
          projectId=self._project_id,
          datasetId=dataset_id,
          tableId=table_id,
          body=insert_data).execute(num_retries=num_retries)

      if 'insertErrors' in response:
        raise exceptions.QueryError(response['insertErrors'])

  def QueryAsync(self, query, num_retries=5):
    logging.debug(query)
    body = {
        'jobReference': {
            'projectId': self._project_id,
            'jobId': str(uuid.uuid4()),
        },
        'configuration': {
            'query': {
                'query': query,
                'priority': 'INTERACTIVE',
            }
        }
    }
    return self._service.jobs().insert(
        projectId=self._project_id,
        body=body).execute(num_retries=num_retries)

  def QuerySync(self, query, timeout=60, num_retries=5):
    """Query Bigtable and return the results as a dict.

    Args:
      query: Query string.
      timeout: Timeout in seconds.
      num_retries: Number of attempts.

    Returns:
      Query results. The format is specified in the "rows" field here:
      https://developers.google.com/resources/api-libraries/documentation/bigquery/v2/python/latest/bigquery_v2.jobs.html#getQueryResults
    """
    logging.debug(query)
    query_data = {
        'query': query,
        'timeoutMs': timeout * 1000,
    }
    start_time = time.time()
    response = self._service.jobs().query(
        projectId=self._project_id,
        body=query_data).execute(num_retries=num_retries)

    if 'errors' in response:
      raise exceptions.QueryError(response['errors'])

    # TODO(dtu): Fetch subsequent pages of rows for big queries.
    # TODO(dtu): Reformat results as dicts.
    result = response.get('rows', [])
    logging.debug('Query fetched %d rows in %fs.',
                  len(result), time.time() - start_time)
    return result

  def IsJobDone(self, job):
    response = self._service.jobs().get(**job['jobReference']).execute()
    if response['status']['state'] == 'DONE':
      return response
    else:
      return None

  def PollJob(self, job, timeout):
    # TODO(dtu): Take multiple jobs as parameters.
    start_time = time.time()
    iteration = 0

    while True:
      elapsed_time = time.time() - start_time

      response = self.IsJobDone(job)
      if response:
        if 'errors' in response['status']:
          raise exceptions.QueryError(response['status']['errors'])
        logging.debug('Polled job for %d seconds.', int(elapsed_time))
        return response

      if elapsed_time >= timeout:
        break
      time.sleep(min(1.5 ** iteration, timeout - elapsed_time))
      iteration += 1

    raise exceptions.TimeoutError()


def _Service():
  """Returns an initialized and authorized BigQuery client."""
  # pylint: disable=no-member
  credentials = client.GoogleCredentials.get_application_default()
  if credentials.create_scoped_required():
    credentials = credentials.create_scoped(
        'https://www.googleapis.com/auth/bigquery')
  return build('bigquery', 'v2', credentials=credentials)
