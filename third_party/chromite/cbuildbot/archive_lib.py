# -*- coding: utf-8 -*-
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module with utilities for archiving functionality."""

from __future__ import print_function

import os

from chromite.cbuildbot import commands
from chromite.lib import config_lib

from chromite.lib import cros_logging as logging
from chromite.lib import gs
from chromite.lib import osutils


def GetBaseUploadURI(config, archive_base=None, bot_id=None):
  """Get the base URL where artifacts from this builder are uploaded.

  Each build run stores its artifacts in a subdirectory of the base URI.
  We also have LATEST files under the base URI which help point to the
  latest build available for a given builder.

  Args:
    config: The build config to examine.
    archive_base: Optional. The root URL under which objects from all
      builders are uploaded. If not specified, we use the default archive
      bucket.
    bot_id: The bot ID to archive files under.

  Returns:
    Google Storage URI (i.e. 'gs://...') under which all archived files
      should be uploaded.  In other words, a path like a directory, even
      through GS has no real directories.
  """
  if not bot_id:
    bot_id = config.name

  if archive_base:
    gs_base = archive_base
  elif config.gs_path == config_lib.GS_PATH_DEFAULT:
    gs_base = config_lib.GetConfig().params.ARCHIVE_URL
  else:
    gs_base = config.gs_path

  return os.path.join(gs_base, bot_id)


def GetUploadACL(config):
  """Get the ACL we should use to upload artifacts for a given config."""
  if config.internal:
    # Use the bucket default ACL.
    return None

  return 'public-read'


class Archive(object):
  """Class to represent the archive for one builder run.

  An Archive object is a read-only object with attributes and methods useful
  for archive purposes.  Most of the attributes are supported as properties
  because they depend on the ChromeOS version and if they are calculated too
  soon (i.e. before the sync stage) they will raise an exception.

  Attributes:
    archive_path: The full local path where output from this builder is stored.
    download_url: The URL where we can download directory artifacts.
    download_url_file: The URL where we can download file artifacts.
    upload_url: The Google Storage location where we should upload artifacts.
    version: The ChromeOS version for this archive.
  """
  # TODO(davidriley): The use of a special download url for directories and
  # files is a workaround for b/27653354. If that is ultimately fixed, revisit
  # this workaround.

  _BUILDBOT_ARCHIVE = 'buildbot_archive'
  _TRYBOT_ARCHIVE = 'trybot_archive'

  def __init__(self, bot_id, version_getter, options, config):
    """Initialize.

    Args:
      bot_id: The bot id associated with this archive.
      version_getter: Functor that should return the ChromeOS version for
        this run when called, if the version is known.  Typically, this
        is BuilderRun.GetVersion.
      options: The command options object for this run.
      config: The build config for this run.
    """
    self._options = options
    self._config = config
    self._version_getter = version_getter
    self._version = None

    self.bot_id = bot_id

  @property
  def version(self):
    if self._version is None:
      self._version = self._version_getter()

    return self._version

  @property
  def archive_path(self):
    return os.path.join(self.GetLocalArchiveRoot(), self.bot_id, self.version)

  @property
  def upload_url(self):
    base_upload_url = GetBaseUploadURI(
        self._config,
        archive_base=self._options.archive_base,
        bot_id=self.bot_id)
    return '%s/%s' % (base_upload_url, self.version)

  @property
  def upload_acl(self):
    """Get the ACL we should use to upload artifacts for a given config."""
    return GetUploadACL(self._config)

  @property
  def download_url(self):
    if self._options.buildbot or self._options.remote_trybot:
      # Translate the gs:// URI to the URL for downloading the same files.
      # TODO(akeshet): The use of a special download url is a workaround for
      # b/27653354. If that is ultimately fixed, revisit this workaround.
      # This download link works for directories.
      return self.upload_url.replace(
          'gs://', gs.PRIVATE_BASE_HTTPS_DOWNLOAD_URL)
    else:
      return self.archive_path

  @property
  def download_url_file(self):
    if self._options.buildbot or self._options.remote_trybot:
      # Translate the gs:// URI to the URL for downloading the same files.
      # TODO(akeshet): The use of a special download url is a workaround for
      # b/27653354. If that is ultimately fixed, revisit this workaround.
      # This download link works for files.
      return self.upload_url.replace('gs://', gs.PRIVATE_BASE_HTTPS_URL)
    else:
      return self.archive_path

  def GetLocalArchiveRoot(self, trybot=None):
    """Return the location on disk where archive images are kept."""
    buildroot = os.path.abspath(self._options.buildroot)

    if trybot is None:
      trybot = not self._options.buildbot or self._options.debug

    archive_base = self._TRYBOT_ARCHIVE if trybot else self._BUILDBOT_ARCHIVE
    return os.path.join(buildroot, archive_base)

  def SetupArchivePath(self):
    """Create a fresh directory for archiving a build."""
    logging.info('Preparing local archive directory at "%s".',
                 self.archive_path)
    if self._options.buildbot:
      # Buildbot: Clear out any leftover build artifacts, if present, for
      # this particular run.  The Clean stage is responsible for trimming
      # back the number of archive paths to the last X runs.
      osutils.RmDir(self.archive_path, ignore_missing=True)
    else:
      # Clear the list of uploaded file if it exists.  In practice, the Clean
      # stage deletes everything in the archive root, so this may not be
      # doing anything at all.
      osutils.SafeUnlink(os.path.join(self.archive_path,
                                      commands.UPLOADED_LIST_FILENAME))

    osutils.SafeMakedirs(self.archive_path)

  def UpdateLatestMarkers(self, manifest_branch, debug, upload_urls=None):
    """Update the LATEST markers in GS archive area.

    Args:
      manifest_branch: The name of the branch in the manifest for this run.
      debug: Boolean debug value for this run.
      upload_urls: Google storage urls to upload the Latest Markers to.
    """
    if not upload_urls:
      upload_urls = [self.upload_url]
    # self.version will be one of these forms, shown through examples:
    # R35-1234.5.6 or R35-1234.5.6-b123.  In either case, we want "1234.5.6".
    version_marker = self.version.split('-')[1]

    filenames = ('LATEST-%s' % manifest_branch,
                 'LATEST-%s' % version_marker)
    base_archive_path = os.path.dirname(self.archive_path)
    base_upload_urls = [os.path.dirname(url) for url in upload_urls]
    for base_upload_url in base_upload_urls:
      for filename in filenames:
        latest_path = os.path.join(base_archive_path, filename)
        osutils.WriteFile(latest_path, self.version, mode='w')
        commands.UploadArchivedFile(
            base_archive_path, [base_upload_url], filename,
            debug, acl=self.upload_acl)
