# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module to download and run the CIPD client.

CIPD is the Chrome Infra Package Deployer, a simple method of resolving a
package/version into a GStorage link and installing them.
"""

from __future__ import print_function

import hashlib
import json
import os
import pprint
import tempfile
import urllib
import urlparse

import chromite.lib.cros_logging as log
from chromite.lib import cache
from chromite.lib import osutils
from chromite.lib import path_util
from chromite.lib import cros_build_lib

import httplib2

# The version of CIPD to download.
# TODO(phobbs) we could make a call to the 'resolveVersion' endpoint
#   to resolve 'latest' into an instance_id for us.
CIPD_INSTANCE_ID = '03f354ad7a6031c7924d9b69a85f83269cc3c2e0'
CIPD_PACKAGE = 'infra/tools/cipd/linux-amd64'

CHROME_INFRA_PACKAGES_API_BASE = (
    'https://chrome-infra-packages.appspot.com/_ah/api/repo/v1/')


def _ChromeInfraRequest(endpoint, request_args=None):
  """Makes a request to the Chrome Infra Packages API with httplib2.

  Args:
    endpoint: The endpoint to make a request to.
    request_args: Keyword arguments to put into the request string.

  Returns:
    A tuple of (headers, content) returned by the server. The body content is
    assumed to be JSON.
  """
  uri = ''.join([CHROME_INFRA_PACKAGES_API_BASE,
                 endpoint,
                 '?',
                 urllib.urlencode(request_args or {})])
  result = httplib2.Http().request(uri=uri)
  try:
    return result[0], json.loads(result[1])
  except Exception as e:
    e.message = 'Encountered exception requesting "%s":\n' + e.message
    raise


def _DownloadCIPD(instance_id):
  """Finds the CIPD download link and requests the binary.

  The 'client' endpoit of the chrome infra packages API responds with a sha1 and
  a Google Storage link. After downloading the binary, we validate that the sha1
  of the response and return it.

  Args:
    instance_id: The version of CIPD to download.

  Returns:
    the CIPD binary as a string.
  """
  args = {'instance_id': instance_id, 'package_name': CIPD_PACKAGE}
  _, body = _ChromeInfraRequest('client', request_args=args)
  if 'client_binary' not in body:
    log.error(
        'Error requesting the link to download CIPD from. Got:\n%s',
        pprint.pformat(body))
    return

  http = httplib2.Http(cache=None)
  response, binary = http.request(uri=body['client_binary']['fetch_url'])
  assert response['status'] == '200', (
      'Got a %s response from Google Storage.' % response['status'])
  digest = unicode(hashlib.sha1(binary).hexdigest())
  assert digest == body['client_binary']['sha1'], (
      'The binary downloaded does not match the expected SHA1.')
  return binary


class CipdCache(cache.RemoteCache):
  """Supports caching of the CIPD download."""
  def _Fetch(self, key, path):
    instance_id = urlparse.urlparse(key).netloc
    binary = _DownloadCIPD(instance_id)
    log.info('Fetched CIPD package %s:%s', CIPD_PACKAGE, instance_id)
    osutils.WriteFile(path, binary)
    os.chmod(path, 0755)


def GetCIPDFromCache(instance_id=CIPD_INSTANCE_ID):
  """Checks the cache, downloading CIPD if it is missing.

  Args:
    instance_id: The version of CIPD to download. Default CIPD_INSTANCE_ID

  Returns:
    Path to the CIPD binary.
  """
  cache_dir = os.path.join(path_util.GetCacheDir(), 'cipd')
  bin_cache = CipdCache(cache_dir)
  key = (instance_id,)
  ref = bin_cache.Lookup(key)
  ref.SetDefault('cipd://' + instance_id)
  return ref.path


@cros_build_lib.Memoize
def InstallPackage(cipd_path, package, instance_id, destination,
                   service_account_json=None):
  """Installs a package at a given destination using cipd.

  Args:
    cipd_path: The path to a cipd executable. GetCIPDFromCache can give this.
    package: A package name.
    instance_id: The version of the package to install.
    destination: The folder to install the package under.
    service_account_json: The path of the service account credentials.

  Returns:
    The path of the package.
  """
  destination = os.path.join(destination, package)

  service_account_flag = []
  if service_account_json:
    service_account_flag = ['-service-account-json', service_account_json]

  with tempfile.NamedTemporaryFile() as f:
    f.write('%s %s' % (package, instance_id))
    f.flush()

    cros_build_lib.RunCommand(
        [cipd_path, 'ensure', '-root', destination, '-list', f.name]
        + service_account_flag,
        capture_output=True)

  return destination


def CreatePackage(cipd_path, package, in_dir, tags, refs,
                  cred_path=None):
  """Create (build and register) a package using cipd.

  Args:
    cipd_path: The path to a cipd executable. GetCIPDFromCache can give this.
    package: A package name.
    in_dir: The directory to create the package from.
    tags: A mapping of tags to apply to the package.
    refs: An Iterable of refs to apply to the package.
    cred_path: The path of the service account credentials.
  """
  args = [
      cipd_path, 'create',
      '-name', package,
      '-in', in_dir,
  ]
  for key, value in tags.iteritems():
    args.extend(['-tag', '%s:%s' % (key, value)])
  for ref in refs:
    args.extend(['-ref', ref])
  if cred_path:
    args.extend(['-service-account-json', cred_path])

  cros_build_lib.RunCommand(args, capture_output=True)


def BuildPackage(cipd_path, package, in_dir, outfile):
  """Build a package using cipd.

  Args:
    cipd_path: The path to a cipd executable. GetCIPDFromCache can give this.
    package: A package name.
    in_dir: The directory to create the package from.
    outfile: Output file.  Should have extension .cipd
  """
  args = [
      cipd_path, 'pkg-build',
      '-name', package,
      '-in', in_dir,
      '-out', outfile,
  ]
  cros_build_lib.RunCommand(args, capture_output=True)


def RegisterPackage(cipd_path, package_file, tags, refs, cred_path=None):
  """Register and upload a package using cipd.

  Args:
    cipd_path: The path to a cipd executable. GetCIPDFromCache can give this.
    package_file: The path to a .cipd package file.
    tags: A mapping of tags to apply to the package.
    refs: An Iterable of refs to apply to the package.
    cred_path: The path of the service account credentials.
  """
  args = [cipd_path, 'pkg-register', package_file]
  for key, value in tags.iteritems():
    args.extend(['-tag', '%s:%s' % (key, value)])
  for ref in refs:
    args.extend(['-ref', ref])
  if cred_path:
    args.extend(['-service-account-json', cred_path])
  cros_build_lib.RunCommand(args, capture_output=True)
