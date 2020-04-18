# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
import os
import sys

from app_yaml_helper import AppYamlHelper
from third_party.json_schema_compiler.memoize import memoize


@memoize
def GetAppVersion():
  return GetAppVersionNonMemoized()


# This one is for running from tests, which memoization messes up.
def GetAppVersionNonMemoized():
  if 'CURRENT_VERSION_ID' in os.environ:
    # The version ID looks like 2-0-25.36712548 or 2-0-25.23/223; we only
    # want the 2-0-25.
    return re.compile('[./]').split(os.environ['CURRENT_VERSION_ID'])[0]
  # Not running on appengine, get it from the app.yaml file ourselves.
  app_yaml_path = os.path.join(os.path.split(__file__)[0], 'app.yaml')
  with open(app_yaml_path, 'r') as app_yaml:
    return AppYamlHelper.ExtractVersion(app_yaml.read())


def _IsServerSoftware(name):
  return os.environ.get('SERVER_SOFTWARE', '').find(name) == 0


def IsComputeEngine():
  return _IsServerSoftware('Compute Engine')


def IsDevServer():
  return _IsServerSoftware('Development')


def IsReleaseServer():
  return _IsServerSoftware('Google App Engine')


def IsPreviewServer():
  return sys.argv and os.path.basename(sys.argv[0]) == 'preview.py'


def IsAppEngine():
  return IsDevServer() or IsReleaseServer()


def IsTest():
  return sys.argv and os.path.basename(sys.argv[0]).endswith('_test.py')


class UnknownEnvironmentError(Exception):
  pass

