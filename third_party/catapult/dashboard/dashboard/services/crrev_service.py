# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Functions for getting commit information from Gitiles."""

from dashboard.services import request


def GetNumbering(number, numbering_identifier, numbering_type, project, repo):
  url = 'https://cr-rev.appspot.com/_ah/api/crrev/v1/get_numbering'
  params = {
      'number': number,
      'numbering_identifier': numbering_identifier,
      'numbering_type': numbering_type,
      'project': project,
      'repo': repo
  }

  return request.RequestJson(url, 'GET', **params)
