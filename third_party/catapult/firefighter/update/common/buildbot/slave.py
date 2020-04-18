# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cStringIO
import mimetools

from common.buildbot import network


def Slaves(master_name):
  slave_data = network.FetchData(network.BuildUrl(master_name, 'json/slaves'))
  return sorted(Slave(master_name, slave_name, slave_info)
                for slave_name, slave_info in slave_data.iteritems())


class Slave(object):

  def __init__(self, master_name, name, data):
    self._master_name = master_name
    self._name = name

    self._builders = frozenset(data['builders'].keys())
    self._connected = data['connected']

    if data['host']:
      host_data = dict(mimetools.Message(cStringIO.StringIO(data['host'])))
      self._bitness = 64 if '64' in host_data['architecture'] else 32
      self._git_version = host_data['git version']
      self._hardware = host_data['product name']
      self._memory = float(host_data['memory total'].split()[0])
      self._os = _ParseOs(host_data['osfamily'])
      self._os_version = _ParseOsVersion(self._os, host_data['os version'])
      self._processor_count = host_data['processor count']
    else:
      # The information is populated by Puppet. Puppet doesn't run on our GCE
      # instances, so if the info is missing, assume it's in GCE.
      self._bitness = 64
      self._git_version = None
      self._hardware = 'Compute Engine'
      self._memory = None
      self._os = 'linux'
      self._os_version = None
      self._processor_count = None

  def __lt__(self, other):
    return self.name < other.name

  def __str__(self):
    return self.name

  @property
  def master_name(self):
    return self._master_name

  @property
  def name(self):
    return self._name

  @property
  def builders(self):
    return self._builders

  @property
  def bitness(self):
    return self._bitness

  @property
  def git_version(self):
    return self._git_version

  @property
  def hardware(self):
    """Returns the model of the hardware.

    For example, "MacBookPro11,2", "PowerEdge R220", or "Compute Engine".
    """
    return self._hardware

  @property
  def memory(self):
    """Returns the quantity of RAM, in GB, as a float."""
    return self._memory

  @property
  def os(self):
    """Returns the canonical os name string.

    The return value must be in the following list:
    https://chromium.googlesource.com/infra/infra/+/HEAD/doc/users/services/buildbot/builders.pyl.md#os
    """
    return self._os

  @property
  def os_version(self):
    """Returns the canonical major os version name string.

    The return value must be in the following table:
    https://chromium.googlesource.com/infra/infra/+/HEAD/doc/users/services/buildbot/builders.pyl.md#version
    """
    return self._os_version

  @property
  def processor_count(self):
    return self._processor_count


def _ParseOs(os_family):
  return {
      'darwin': 'mac',
      'debian': 'linux',
      'windows': 'win',
  }[os_family.lower()]


def _ParseOsVersion(os, os_version):
  if os == 'mac':
    return '.'.join(os_version.split('.')[:2])
  elif os == 'linux':
    return {
        '12.04': 'precise',
        '14.04': 'trusty',
    }[os_version]
  elif os == 'win':
    return {
        '5.1.2600': 'xp',
        '6.0.6001': 'vista',
        '2008 R2': '2008',  # 2008 R2
        '7': 'win7',
        '6.3.9600': 'win8',  # 8.1
        '10.0.10240': 'win10',
    }[os_version]
  else:
    raise ValueError('"%s" is not a valid os string.' % os)
