# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from branch_utility import BranchUtility, ChannelInfo
from test_data.canned_data import (CANNED_BRANCHES, CANNED_CHANNELS)


class TestBranchUtility(object):
  '''Mimics BranchUtility to return valid-ish data without needing omahaproxy
  data.
  '''

  def __init__(self, versions, channels):
    ''' Parameters: |version| is a mapping of versions to branches, and
    |channels| is a mapping of channels to versions.
    '''
    self._versions = versions
    self._channels = channels

  @staticmethod
  def CreateWithCannedData():
    '''Returns a TestBranchUtility that uses 'canned' test data pulled from
    older branches of SVN data.
    '''
    return TestBranchUtility(CANNED_BRANCHES, CANNED_CHANNELS)

  def GetAllChannelInfo(self):
    return tuple(self.GetChannelInfo(channel)
            for channel in BranchUtility.GetAllChannelNames())

  def GetChannelInfo(self, channel):
    version = self._channels[channel]
    return ChannelInfo(channel, self.GetBranchForVersion(version), version)

  def GetStableChannelInfo(self, version):
    return ChannelInfo('stable', self.GetBranchForVersion(version), version)

  def GetBranchForVersion(self, version):
    return self._versions[version]

  def GetChannelForVersion(self, version):
    if version <= self._channels['stable']:
      return 'stable'
    for channel in self._channels.iterkeys():
      if self._channels[channel] == version:
        return channel

  def Older(self, channel_info):
    versions = self._versions.keys()
    index = versions.index(channel_info.version)
    if index == len(versions) - 1:
      return None
    version = versions[index + 1]
    return ChannelInfo(self.GetChannelForVersion(version),
                       self.GetBranchForVersion(version),
                       version)

  def Newer(self, channel_info):
    versions = self._versions.keys()
    index = versions.index(channel_info.version)
    if not index:
      return None
    version = versions[index - 1]
    return ChannelInfo(self.GetChannelForVersion(version),
                       self.GetBranchForVersion(version),
                       version)
