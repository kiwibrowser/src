# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""App Engine config.

This module is loaded before others and can be used to set up the
App Engine environment. See:
  https://cloud.google.com/appengine/docs/python/tools/appengineconfig
"""

import os

from google.appengine.ext import vendor

import dashboard

# The names used below are special constant names which other code depends on.
# pylint: disable=invalid-name

appstats_SHELL_OK = True
appstats_CALC_RPC_COSTS = True

# Allows remote_api from the peng team to support the crosbolt dashboard.
remoteapi_CUSTOM_ENVIRONMENT_AUTHENTICATION = (
    'LOAS_PEER_USERNAME', ['chromeos-peng-performance'])


def webapp_add_wsgi_middleware(app):
  from google.appengine.ext.appstats import recording
  app = recording.appstats_wsgi_middleware(app)
  return app

# pylint: enable=invalid-name

def _AddThirdPartyLibraries():
  """Registers the third party libraries with App Engine.

  In order for third-party libraries to be available in the App Engine
  runtime environment, they must be added with vendor.add. The directories
  added this way must be inside the App Engine project directory.
  """
  # The deploy script is expected to add links to third party libraries
  # before deploying. If the directories aren't there (e.g. when running tests)
  # then just ignore it.
  for library_dir in dashboard.THIRD_PARTY_LIBRARIES:
    if os.path.exists(library_dir):
      vendor.add(os.path.join(os.path.dirname(__file__), library_dir))


_AddThirdPartyLibraries()

# This is at the bottom because datastore_hooks may depend on third_party
# modules.
from dashboard.common import datastore_hooks
datastore_hooks.InstallHooks()
