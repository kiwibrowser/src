# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Functions for interfacing with the Isolate Server.

The Isolate Server is content-addressable cache. Swarming inputs and outputs are
stored by the Isolate Server.

API explorer: https://isolateserver.appspot.com/_ah/api/explorer
"""

import base64
import json
import zlib

from dashboard.services import request


def Retrieve(server, digest):
  url = server + '/_ah/api/isolateservice/v1/retrieve'
  body = {
      'namespace': {'namespace': 'default-gzip'},
      'digest': digest,
  }

  isolate_info = request.RequestJson(url, 'POST', body)

  if 'url' in isolate_info:
    return zlib.decompress(request.Request(isolate_info['url'], 'GET'))

  if 'content' in isolate_info:
    return zlib.decompress(base64.b64decode(isolate_info['content']))

  raise NotImplementedError(
      'Isolate information for %s is in an unknown format: %s' %
      (digest, json.dumps(isolate_info)))
