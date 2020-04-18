# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import operator

from environment_wrappers import CreateUrlFetcher
import url_constants


class ChannelInfo(object):
  '''Represents a Chrome channel with three pieces of information. |channel| is
  one of 'stable', 'beta', 'dev', or 'master'. |branch| and |version| correspond
  with each other, and represent different releases of Chrome. Note that
  |branch| and |version| can occasionally be the same for separate channels
  (i.e. 'beta' and 'dev'), so all three fields are required to uniquely
  identify a channel.
  '''

  def __init__(self, channel, branch, version):
    assert isinstance(channel, basestring), channel
    assert isinstance(branch, basestring), branch
    # TODO(kalman): Assert that this is a string. One day Chromium will probably
    # be served out of a git repository and the versions will no longer be ints.
    assert isinstance(version, int) or version == 'master', version
    self.channel = channel
    self.branch = branch
    self.version = version

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

  def __ne__(self, other):
    return not (self == other)

  def __repr__(self):
    return '%s%s' % (type(self).__name__, repr(self.__dict__))

  def __str__(self):
    return repr(self)


class BranchUtility(object):
  '''Provides methods for working with Chrome channel, branch, and version
  data served from OmahaProxy.
  '''

  def __init__(self, fetch_url, history_url, fetcher, object_store_creator):
    self._fetcher = fetcher
    def create_object_store(category):
      return object_store_creator.Create(BranchUtility, category=category)
    self._branch_object_store = create_object_store('branch')
    self._version_object_store = create_object_store('version')
    self._fetch_result = self._fetcher.FetchAsync(fetch_url)
    self._history_result = self._fetcher.FetchAsync(history_url)

  @staticmethod
  def Create(object_store_creator):
    return BranchUtility(url_constants.OMAHA_PROXY_URL,
                         url_constants.OMAHA_DEV_HISTORY,
                         CreateUrlFetcher(),
                         object_store_creator)

  @staticmethod
  def GetAllChannelNames():
    return ('stable', 'beta', 'dev', 'master')

  @staticmethod
  def NewestChannel(channels):
    channels = set(channels)
    for channel in reversed(BranchUtility.GetAllChannelNames()):
      if channel in channels:
        return channel

  def Newer(self, channel_info):
    '''Given a ChannelInfo object, returns a new ChannelInfo object
    representing the next most recent Chrome version/branch combination.
    '''
    if channel_info.channel == 'master':
      return None
    if channel_info.channel == 'stable':
      stable_info = self.GetChannelInfo('stable')
      if channel_info.version < stable_info.version:
        return self.GetStableChannelInfo(channel_info.version + 1)
    names = self.GetAllChannelNames()
    return self.GetAllChannelInfo()[names.index(channel_info.channel) + 1]

  def Older(self, channel_info):
    '''Given a ChannelInfo object, returns a new ChannelInfo object
    representing the previous Chrome version/branch combination.
    '''
    if channel_info.channel == 'stable':
      return self.GetStableChannelInfo(channel_info.version - 1)
    names = self.GetAllChannelNames()
    return self.GetAllChannelInfo()[names.index(channel_info.channel) - 1]

  @staticmethod
  def SplitChannelNameFromPath(path):
    '''Splits the channel name out of |path|, returning the tuple
    (channel_name, real_path). If the channel cannot be determined then returns
    (None, path).
    '''
    if '/' in path:
      first, second = path.split('/', 1)
    else:
      first, second = (path, '')
    if first in BranchUtility.GetAllChannelNames():
      return (first, second)
    return (None, path)

  def GetAllBranches(self):
    return tuple((channel, self.GetChannelInfo(channel).branch)
            for channel in BranchUtility.GetAllChannelNames())

  def GetAllVersions(self):
    return tuple(self.GetChannelInfo(channel).version
            for channel in BranchUtility.GetAllChannelNames())

  def GetAllChannelInfo(self):
    return tuple(self.GetChannelInfo(channel)
            for channel in BranchUtility.GetAllChannelNames())


  def GetChannelInfo(self, channel):
    version = self._ExtractFromVersionJson(channel, 'version')
    if version != 'master':
      version = int(version)
    return ChannelInfo(channel,
                       self._ExtractFromVersionJson(channel, 'branch'),
                       version)

  def GetStableChannelInfo(self, version):
    '''Given a |version| corresponding to a 'stable' version of Chrome, returns
    a ChannelInfo object representing that version.
    '''
    branch = self.GetBranchForVersion(version)
    if branch is None:
      return None
    return ChannelInfo('stable', branch, version)

  def _ExtractFromVersionJson(self, channel_name, data_type):
    '''Returns the branch or version number for a channel name.
    '''
    if channel_name == 'master':
      return 'master'

    if data_type == 'branch':
      object_store = self._branch_object_store
    elif data_type == 'version':
      object_store = self._version_object_store

    data = object_store.Get(channel_name).Get()
    if data is not None:
      return data

    try:
      version_json = json.loads(self._fetch_result.Get().content)
    except Exception as e:
      # This can happen if omahaproxy is misbehaving, which we've seen before.
      # Quick hack fix: just serve from master until it's fixed.
      logging.error('Failed to fetch or parse branch from omahaproxy: %s! '
                    'Falling back to "master".' % e)
      return 'master'

    numbers = {}
    for entry in version_json:
      if entry['os'] not in ('win', 'linux', 'mac', 'cros'):
        continue
      for version in entry['versions']:
        if version['channel'] != channel_name:
          continue
        if data_type == 'branch':
          number = version['version'].split('.')[2]
        elif data_type == 'version':
          number = version['version'].split('.')[0]
        if number not in numbers:
          numbers[number] = 0
        else:
          numbers[number] += 1

    sorted_numbers = sorted(numbers.iteritems(),
                            key=operator.itemgetter(1),
                            reverse=True)
    object_store.Set(channel_name, sorted_numbers[0][0])
    return sorted_numbers[0][0]

  def GetBranchForVersion(self, version):
    '''Returns the most recent branch for a given chrome version number using
    data stored on omahaproxy (see url_constants).
    '''
    if version == 'master':
      return 'master'

    branch = self._branch_object_store.Get(str(version)).Get()
    if branch is not None:
      return branch

    version_json = json.loads(self._history_result.Get().content)
    for entry in version_json:
      version_title = entry['version'].split('.')
      # TODO(devlin): Is there a reason we don't cache all these values now?
      if version_title[0] == str(version):
        self._branch_object_store.Set(str(version), version_title[2])
        return version_title[2]

    # This can legitimately happen because
    # https://omahaproxy.appspot.com/history.json?channel=dev&os=win&json=1
    # (where we get the branch data) has a maximum number of results. The
    # assertion is a nasty hack to make sure that we're at least retrieving
    # a reasonable number of version. Unfortunately, this is also doomed to
    # need updating every five years or so.
    # TODO(devlin): Really, this is awful.
    assert(version < 40)
    return None

  def GetChannelForVersion(self, version):
    '''Returns the name of the development channel corresponding to a given
    version number.
    '''
    for channel_info in self.GetAllChannelInfo():
      if channel_info.channel == 'stable' and version <= channel_info.version:
        return channel_info.channel
      if version == channel_info.version:
        return channel_info.channel

  def GetLatestVersionNumber(self):
    '''Returns the most recent version number found using data stored on
    omahaproxy.
    '''
    latest_version = self._version_object_store.Get('latest').Get()
    if latest_version is not None:
      return latest_version

    version_json = json.loads(self._history_result.Get().content)
    latest_version = 0
    for entry in version_json:
      version_title = entry['version'].split('.')
      version = int(version_title[0])
      if version > latest_version:
        latest_version = version

    self._version_object_store.Set('latest', latest_version)
    return latest_version
