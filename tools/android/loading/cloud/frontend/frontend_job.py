# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from google.appengine.ext import ndb


class FrontendJob(ndb.Model):
  """Class representing a frontend job.

  A frontend job is a Clovis task sent by the user, and associated metadata
  (such as the username, the start time...).
  It is persisted in the Google Cloud datastore.

  All frontend jobs are ancestors of a single entity called 'FrontendJobList'.
  This allows to benefit from strong consistency when querying the job
  associated to a tag.
  """
  # Base URL path to get information about a job.
  SHOW_JOB_URL = '/show_job'

  # ndb properties persisted in the datastore. Indexing is not needed.
  email = ndb.StringProperty(indexed=False)
  status = ndb.StringProperty(indexed=False)
  task_url = ndb.StringProperty(indexed=False)
  eta = ndb.DateTimeProperty(indexed=False)
  start_time = ndb.DateTimeProperty(auto_now_add=True, indexed=False)
  # Not indexed by default.
  clovis_task = ndb.TextProperty(compressed=True, indexed=False)
  log = ndb.TextProperty(indexed=False)

  @classmethod
  def _GetParentKeyFromTag(cls, tag):
    """Gets the key that can be used to retrieve a frontend job from the job
    list.
    """
    return ndb.Key('FrontendJobList', tag)

  @classmethod
  def CreateForTag(cls, tag):
    """Creates a frontend job associated with tag."""
    parent_key = cls._GetParentKeyFromTag(tag)
    return cls(parent=parent_key)

  @classmethod
  def GetFromTag(cls, tag):
    """Gets the frontend job associated with tag."""
    parent_key = cls._GetParentKeyFromTag(tag)
    return cls.query(ancestor=parent_key).get()

  @classmethod
  def DeleteForTag(cls, tag):
    """Deletes the frontend job assowiated with tag."""
    parent_key = cls._GetParentKeyFromTag(tag)
    frontend_job = cls.query(ancestor=parent_key).get(keys_only=True)
    if frontend_job:
      frontend_job.delete()

  @classmethod
  def ListJobs(cls):
    """Lists all the frontend jobs.

    Returns:
      list of strings: The list of tags corresponding to existing frontend jobs.
    """
    return [key.parent().string_id() for key in cls.query().fetch(
        100, keys_only=True)]

  @classmethod
  def GetJobURL(cls, tag):
    """Gets the URL that can be used to get information about a specific job."""
    return cls.SHOW_JOB_URL + '?tag=' + tag

  def RenderAsHtml(self):
    """Render a short job description as a HTML table.

    The log and ClovisTask are not included, because they are potentially very
    large.
    """
    html = '<table>'

    for p in FrontendJob._properties:
      if p == 'log' or p == 'clovis_task':
        continue
      value = getattr(self, p)
      if value:
        html += '<tr><td>' + p + '</td><td>' + str(value) + '</td></tr>'

    html += '</table>'
    return html
