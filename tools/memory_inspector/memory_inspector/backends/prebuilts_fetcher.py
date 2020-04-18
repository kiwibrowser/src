# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module fetches and syncs prebuilts from Google Cloud Storage (GCS).

See prebuilts/README for a description of the respository <> GCS sync mechanism.
"""

import hashlib
import logging
import os
import urllib

from memory_inspector import constants


# Bypass the GCS download logic in unittests and use the *_ForTests mock.
in_test_harness = False


def GetIfChanged(local_file_path):
  """Downloads the file from GCS, only if the local one is outdated."""
  is_changed = _IsChanged(local_file_path)

  # In test harness mode we are only interested in checking that the proper
  # .sha1 files have been checked in (hence the call to _IsChanged() above).
  if in_test_harness:
    return _GetIfChanged_ForTests(local_file_path)

  if not is_changed:
    return
  obj_name = _GetRemoteFileID(local_file_path)
  url = constants.PREBUILTS_BASE_URL + obj_name
  logging.info('Downloading %s prebuilt from %s.' % (local_file_path, url))
  urllib.urlretrieve(url, local_file_path)
  assert(not _IsChanged(local_file_path)), ('GCS download for %s failed.' %
                                            local_file_path)


def _IsChanged(local_file_path):
  """Checks whether the local_file_path exists and matches the expected hash."""
  is_changed = True
  expected_hash = _GetRemoteFileID(local_file_path)
  if os.path.exists(local_file_path):
    with open(local_file_path, 'rb') as f:
      local_hash = hashlib.sha1(f.read()).hexdigest()
    is_changed = (local_hash != expected_hash)
  return is_changed


def _GetRemoteFileID(local_file_path):
  """Returns the checked-in hash which identifies the name of file in GCS."""
  hash_path = local_file_path + '.sha1'
  with open(hash_path, 'rb') as f:
    return f.read(1024).rstrip()


def _GetIfChanged_ForTests(local_file_path):
  """This is the mock version of |GetIfChanged| used only in unittests."""
  # Just truncate / create an empty file.
  open(local_file_path, 'wb').close()
