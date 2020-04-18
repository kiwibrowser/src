# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helper functions to manage errors returned by Google Compute APIs."""

import json


# Error reason returned by GetErrorReason() when a resource is not found.
REASON_NOT_FOUND = 'notFound'


def GetErrorContent(error):
  """Returns the contents of an error returned by Google Compute APIs as a
  dictionary or None.
  """
  if not error.resp.get('content-type', '').startswith('application/json'):
    return None
  return json.loads(error.content)


def GetErrorReason(error_content):
  """Returns the error reason as a string."""
  if not error_content:
    return None
  if (not error_content.get('error') or
      not error_content['error'].get('errors')):
    return None
  error_list = error_content['error']['errors']
  if not error_list:
    return None
  return error_list[0].get('reason')
