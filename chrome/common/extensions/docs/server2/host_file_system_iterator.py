# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class HostFileSystemIterator(object):
  '''Provides methods for iterating through host file systems, in both
  ascending (oldest to newest version) and descending order.
  '''

  def __init__(self, file_system_provider, branch_utility):
    self._file_system_provider = file_system_provider
    self._branch_utility = branch_utility

  def _ForEach(self, channel_info, callback, get_next):
    '''Iterates through a sequence of file systems defined by |get_next| until
    |callback| returns False, or until the end of the sequence of file systems
    is reached. Returns the BranchUtility.ChannelInfo of the last file system
    for which |callback| returned True.
    '''
    last_true = None
    while channel_info is not None:
      if channel_info.branch == 'master':
        file_system = self._file_system_provider.GetMaster()
      else:
        file_system = self._file_system_provider.GetBranch(channel_info.branch)
      if not callback(file_system, channel_info):
        return last_true
      last_true = channel_info
      channel_info = get_next(channel_info)
    return last_true

  def Ascending(self, channel_info, callback):
    return self._ForEach(channel_info, callback, self._branch_utility.Newer)

  def Descending(self, channel_info, callback):
    return self._ForEach(channel_info, callback, self._branch_utility.Older)
