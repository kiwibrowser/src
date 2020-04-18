# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""System metrics."""

from __future__ import absolute_import
from __future__ import print_function

import collections
import platform
import sys

from chromite.lib import metrics

_os_name_metric = metrics.StringMetric(
    'proc/os/name',
    description='OS name on the machine')

_os_version_metric = metrics.StringMetric(
    'proc/os/version',
    description='OS version on the machine')

_os_arch_metric = metrics.StringMetric(
    'proc/os/arch',
    description='OS architecture on this machine')

_python_arch_metric = metrics.StringMetric(
    'proc/python/arch',
    description='python userland architecture on this machine')


def collect_os_info():
  os_info = _get_osinfo()
  _os_name_metric.set(os_info.name)
  _os_version_metric.set(os_info.version)
  _os_arch_metric.set(platform.machine())
  _python_arch_metric.set(_get_python_arch())


class OSInfo(collections.namedtuple('OSInfo', 'name,version')):
  """namedtuple representing OS info (all fields lowercased)."""

  def __new__(cls, name, version):
    return super(OSInfo, cls).__new__(cls, name.lower(), version.lower())


def _get_osinfo():
  """Get OS name and version.

  Returns:
    OSInfo instance
  """
  os_name = platform.system()
  if os_name == 'Linux':
    return _get_linux_osinfo()
  else:
    return OSInfo(name='', version='')


def _get_linux_osinfo():
  # will return something like ('Ubuntu', '14.04', 'trusty')
  os_name, os_version, _ = platform.dist()
  return OSInfo(name=os_name, version=os_version)


def _get_python_arch():
  if sys.maxsize > 2**32:
    return '64'
  else:
    return '32'
