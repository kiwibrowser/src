# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
import time


_INTEGER_PARAMETERS = (
    'build',
    'device_shard',
    'host_shard',
    'status',
)


# TODO(dtu): Pull these from table.
_STRING_PARAMETERS = (
    'benchmark',
    'builder',
    'configuration',
    'device_id',
    'hostname',
    'master',
    'os',
    'os_version',
    'role',
)


def Filters(request):
  filters = {}

  for parameter_name in _INTEGER_PARAMETERS:
    parameter_values = request.get_all(parameter_name)
    if parameter_values:
      filters[parameter_name] = map(int, parameter_values)

  for parameter_name in _STRING_PARAMETERS:
    parameter_values = request.get_all(parameter_name)
    if parameter_values:
      for parameter_value in parameter_values:
        if re.search(r'[^A-Za-z0-9\(\)-_. ]', parameter_value):
          raise ValueError('invalid %s: "%s"' %
                           (parameter_name, parameter_value))
      filters[parameter_name] = parameter_values

  start_time = request.get('start_time')
  if start_time:
    filters['start_time'] = _ParseTime(start_time)

  end_time = request.get('end_time')
  if end_time:
    filters['end_time'] = _ParseTime(end_time)

  return filters


def _ParseTime(time_parameter):
  units = {
      's': 1,
      'm': 60,
      'h': 60 * 60,
      'd': 60 * 60 * 24,
      'w': 60 * 60 * 24 * 7,
  }
  unit = time_parameter[-1]
  if unit in units:
    time_delta = -abs(float(time_parameter[:-1]))
    time_parameter = time_delta * units[unit]
  else:
    time_parameter = float(time_parameter)

  if time_parameter < 0:
    time_parameter = time.time() + time_parameter
  return time_parameter
