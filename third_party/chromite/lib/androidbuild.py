# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Library to use the androidbuild API to list and fetch builds."""

from __future__ import print_function

import apiclient
import httplib2
import oauth2client.client
import os
import pwd
import urllib


# Locations where to look for credentials JSON files, relative to the user's
# home directory.
HOMEDIR_JSON_CREDENTIALS_PATH = '.ab_creds.json'

# Scope URL on which we need authorization.
DEFAULT_SCOPE_URL = 'https://www.googleapis.com/auth/androidbuild.internal'

# Default service name and version to connect to.
DEFAULT_API_SERVICE_NAME = 'androidbuildinternal'
DEFAULT_API_VERSION = 'v2beta1'

# Default chunk size to use when downloading files through androidbuild API.
DEFAULT_MEDIA_IO_CHUNKSIZE = 20 * 1024 * 1024  # 20MiB


class Error(Exception):
  """Base exception on the androidbuild module."""


class CredentialsNotFoundError(Error):
  """Credentials file not found."""


def FindCredentialsFile(override_json_credentials_path=None,
                        homedir_json_credentials_path=None):
  """Find the path to an existing credentials file.

  Returns the path of the first one that is found.

  Args:
    override_json_credentials_path: Path to always use, whenever specified.
        This is meant for a file specified on a --json-key-file argument on the
        command line. Whenever present, always use this value.
    homedir_json_credentials_path: Optional override for the file to be looked
        for in the user's home directory. Defaults to '.ab_creds.json'.

  Returns:
    The resolved path to the file that was found.

  Raises:
    CredentialsNotFoundError: If none of the files exist.
  """
  if override_json_credentials_path is not None:
    return override_json_credentials_path

  if homedir_json_credentials_path is None:
    homedir_json_credentials_path = HOMEDIR_JSON_CREDENTIALS_PATH

  # Check for the file in the user's homedir:
  user_homedir = os.path.expanduser('~')
  if user_homedir:
    json_path = os.path.join(user_homedir, homedir_json_credentials_path)
    if os.path.exists(json_path):
      return json_path

  # If not found, check at ~$PORTAGE_USERNAME. That might be the case if the
  # tool is being used from within an ebuild script.
  portage_username = os.environ.get('PORTAGE_USERNAME')
  if portage_username:
    try:
      portage_homedir = pwd.getpwnam(portage_username).pw_dir
    except KeyError:
      # User $PORTAGE_USERNAME does not exist.
      pass
    else:
      json_path = os.path.join(portage_homedir, homedir_json_credentials_path)
      if os.path.exists(json_path):
        return json_path

  raise CredentialsNotFoundError(
      'Could not find the JSON credentials at [%s] and no JSON file was '
      'specified in command line arguments.' % homedir_json_credentials_path)


def LoadCredentials(json_credentials_path=None, scope_url=None):
  """Load the credentials from a local file.

  Returns a scoped credentials object which can be used to .authorize() an
  httlib2.Http() instance used by an apiclient.

  This method works both with service accounts (JSON generated from Pantheon's
  API manager under Credentials section), or with authenticated users (using a
  scheme similar to the one used by `gcloud auth login`.)

  Args:
    json_credentials_path: Path to a JSON file with credentials for a service
        account or for authenticated user. Defaults to looking for one using
        FindCredentialsFile().
    scope_url: URL in which the credentials should be scoped.

  Returns:
    A scoped oauth2client.client.Credentials object that can be used to
    authorize an Http instance used by an apiclient object.
  """
  json_credentials_path = FindCredentialsFile(json_credentials_path)

  # This is the way to support both service account credentials (JSON generated
  # from Pantheon) or authenticated users (similar to `gcloud auth login`).
  google_creds = oauth2client.client.GoogleCredentials.from_stream(
      json_credentials_path)

  if scope_url is None:
    scope_url = DEFAULT_SCOPE_URL

  # We need to rescope the credentials which are currently unscoped.
  scoped_creds = google_creds.create_scoped(scope_url)
  return scoped_creds


def GetApiClient(creds, api_service_name=None, api_version=None):
  """Build an API client for androidbuild and authorize it.

  Args:
    creds: The scoped oauth2client.client.Credentials to use for authorization.
    api_service_name: Optional override for the API service name.
        Defaults to 'androidbuildinternal' (from DEFAULT_API_SERVICE_NAME.)
    api_version: Optional override for the API version.
        Defaults to 'v2beta1' (from DEFAULT_API_VERSION.)

  Returns:
    An apiclient.discovery.Resource that supports the androidbuild API methods.
  """
  if api_service_name is None:
    api_service_name = DEFAULT_API_SERVICE_NAME
  if api_version is None:
    api_version = DEFAULT_API_VERSION

  base_http_client = httplib2.Http()
  auth_http_client = creds.authorize(base_http_client)
  ab_client = apiclient.discovery.build(api_service_name, api_version,
                                        http=auth_http_client)
  return ab_client


def FetchArtifact(ab_client, branch, target, build_id, filepath, output=None):
  """Fetches an artifact using the API client.

  Args:
    ab_client: The androidbuild API client.
    branch: The name of the git branch. (Currently UNUSED!)
    target: The name of the build target.
    build_id: The id of the build.
    filepath: Path to the file to download.
    output: Path where to store the artifact. Defaults to filepath.

  Raises:
    apiclient.errors.HttpError: If the requested artifact does not exist.
  """
  # The "branch" is unused, so silent pylint warnings about it:
  _ = branch

  # Get the media id to download.
  # NOTE: For some reason the git branch is not needed here, which looks weird.
  # That means in the ab:// URL the branch name will be essentially ignored.
  media_id = ab_client.buildartifact().get_media(
      target=target,
      buildId=build_id,
      attemptId='latest',
      resourceId=filepath)

  if output is None:
    output = filepath

  # Create directory structure, if needed.
  outdir = os.path.dirname(output)
  if outdir and not os.path.isdir(outdir):
    os.makedirs(outdir)

  with open(output, 'wb') as f:
    downloader = apiclient.http.MediaIoBaseDownload(
        f, media_id, chunksize=DEFAULT_MEDIA_IO_CHUNKSIZE)
    done = False
    while not done:
      _, done = downloader.next_chunk()


def FindRecentBuilds(ab_client, branch, target,
                     build_type='submitted',
                     build_attempt_status=None,
                     build_successful=None):
  """Queries for the latest build_ids from androidbuild.

  Args:
    ab_client: The androidbuild API client.
    branch: The name of the git branch.
    target: The name of the build target.
    build_type: (Optional) The type of the build, defaults to 'submitted'.
    build_attempt_status: (Optional) Status of attempt, use 'complete' to look
      for completed builds only.
    build_successful: (Optional) Whether to only return successful builds.

  Returns:
    A list of numeric build_ids, sorted from most recent to oldest (in reverse
    numerical order.)
  """
  kwargs = {
      'branch': branch,
      'target': target,
  }
  if build_type is not None:
    kwargs['buildType'] = build_type
  if build_attempt_status is not None:
    kwargs['buildAttemptStatus'] = build_attempt_status
  if build_successful is not None:
    kwargs['successful'] = build_successful
  builds = ab_client.build().list(**kwargs).execute().get('builds')

  # Extract the build_ids, convert to int, arrange newest to oldest.
  return sorted((int(build['buildId']) for build in builds), reverse=True)


def FindLatestGreenBuildId(ab_client, branch, target):
  """Finds the latest build_id that has a green build.

  Args:
    ab_client: The androidbuild API client.
    branch: The name of the git branch.
    target: The name of the build target.

  Returns:
    A numeric build_id for the latest green build.
    Returns None if no green builds were found for this branch and target.
  """
  build_ids = FindRecentBuilds(ab_client, branch, target,
                               build_successful=True)
  if build_ids:
    return build_ids[0]
  else:
    return None


def SplitAbUrl(ab_url):
  """Splits an ab://... URL into its fields.

  The URL has the following format:
    ab://android-build/<branch>/<target>/<build_id>/<filepath>

  The "android-build" part is the <host> or <bucket> and for now is required to
  be the literal "android-build" (we reserve it to extend the URL format in the
  future.)

  <branch> is the git branch and <target> is the board name plus one of -user
  or -userdebug or -eng or such. <build_id> is the numeric identifier of the
  build. Finally, <filepath> is the path to the artifact itself.

  The two last components (<build_id> and <filepath>) may be absent from the
  URL. An ab:// URL without a <branch> or <target> is invalid (for now.)

  Args:
    ab_url: An ab://... URL.

  Returns:
    A 4-tuple: branch, target, build_id, filepath. The two last components will
    be set to None if they are absent from the URL. The returned <build_id>
    component will be an integer, all others will be strings.

  Raises:
    ValueError: If the URL is not a valid ab://... URL.
  """
  # splittype turns 'ab://bucket/path' into ('ab', '//bucket/path').
  protocol, remainder = urllib.splittype(ab_url)
  if protocol != 'ab':
    raise ValueError('URL [%s] must start with ab:// protocol.' % ab_url)

  # splithost turns '//bucket/path' into ('bucket', '/path').
  bucket, remainder = urllib.splithost(remainder)
  if bucket != 'android-build':
    raise ValueError('URL [%s] must use "android-build" bucket.' % ab_url)

  # Split the remaining fields of the path.
  parts = remainder.split('/', 4)

  if len(parts) < 3:
    raise ValueError(
        'URL [%s] is too short and does not specify a target.' % ab_url)

  # First field will be empty.
  assert parts[0] == ''
  branch = urllib.unquote(parts[1])
  target = urllib.unquote(parts[2])

  if not branch:
    raise ValueError('URL [%s] has an empty branch.' % ab_url)

  if not target:
    raise ValueError('URL [%s] has an empty target.' % ab_url)

  # Check if build_id is present. If present, it must be numeric.
  if len(parts) > 3:
    build_id_str = urllib.unquote(parts[3])
    if not build_id_str.isdigit():
      raise ValueError('URL [%s] has a non-numeric build_id component [%s].' %
                       (ab_url, build_id_str))
    build_id = int(build_id_str)
  else:
    build_id = None

  # Last, use the remainder of the URL as the filepath.
  if len(parts) > 4:
    filepath = urllib.unquote(parts[4])
  else:
    filepath = None

  return (branch, target, build_id, filepath)
