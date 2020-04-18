#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""TPLog Manipulation"""


import getopt
import logging
import os
import simplejson as json
import sys

from operator import ge, lt


class TPLog:
  """TPLog Manipulation"""
  # Constants for entry type
  CALLBACK_REQUEST = 'callbackRequest'
  GESTURE = 'gesture'
  HARDWARE_STATE = 'hardwareState'
  PROPERTY_CHANGE = 'propertyChange'
  TIMER_CALLBACK = 'timerCallback'

  # Constants for log keys
  DESCRIPTION = 'description'
  ENTRIES = 'entries'
  GESTURES_VERSION = 'gesturesVersion'
  HARDWARE_PROPERTIES = 'hardwareProperties'
  PROPERTIES = 'properties'
  VERSION = 'version'

  # Constants for entry keys
  END_TIME = 'endTime'
  START_TIME = 'startTime'
  TIMESTAMP = 'timestamp'
  TYPE = 'type'

  def __init__(self, log_file):
    self._load_log(log_file)
    self._setup_get_time_functions()

  def _load_log(self, log_file):
    """Load the json file."""
    with open(log_file) as f:
      self.log = json.load(f)
    self.shrunk_log = {}
    # Build a new shrunk log from the original log so that we could
    # modify the entries and properties, and add description later.
    self.shrunk_log = self.log.copy()
    self.entries = self.log[self.ENTRIES]

  def _setup_get_time_functions(self):
    """Set up get time functions for hardware state, gesture,
    and timer callback."""
    self._get_time = {self.HARDWARE_STATE: self._get_hwstate_time,
                      self.GESTURE: self._get_gesture_end_time,
                      self.TIMER_CALLBACK: self._get_timercb_time}

  def _get_hwstate_time(self, entry):
    """Get the timestamp of a hardware state entry."""
    if entry[self.TYPE] == self.HARDWARE_STATE:
      return entry[self.TIMESTAMP]
    else:
      return None

  def _get_gesture_end_time(self, entry):
    """Get the end timestamp of a gesture entry."""
    if entry[self.TYPE] == self.GESTURE:
      return entry[self.END_TIME]
    else:
      return None

  def _get_timercb_time(self, entry):
    """Get the timestamp of a timer callback entry."""
    if entry[self.TYPE] == self.TIMER_CALLBACK:
      return entry['now']
    else:
      return None

  def _get_entry_time(self, entry):
    """Get the timestamp of the given entry."""
    e_type = entry[self.TYPE]
    if self._get_time.get(e_type):
      return self._get_time[e_type](entry)
    return None

  def _compare_entry_time(self, entry, timestamp, op):
    """Compare entry time with a given timestamp using the operator op."""
    e_time = self._get_entry_time(entry)
    return e_time and op(e_time, timestamp)

  def _get_begin_hwstate(self, timestamp):
    """Get the hardwareState entry after the specified timestamp."""
    for index, e in enumerate(self.entries):
      if (e[self.TYPE] == self.HARDWARE_STATE and
          self._get_hwstate_time(e) >= timestamp):
        return index
    return None

  def _get_end_entry(self, timestamp):
    """Get the entry after the specified timestamp."""
    for index, e in enumerate(self.entries):
      if self._compare_entry_time(e, timestamp, ge):
        return index
    return None

  def _get_end_gesture(self, timestamp):
    """Get the gesture entry after the specified timestamp."""
    end_entry = None
    entry_len = len(self.entries)
    for index, e in enumerate(reversed(self.entries)):
      # Try to find the last gesture entry resulted from the events within the
      # timestamp.
      if (e[self.TYPE] == self.GESTURE and
          self._get_gesture_end_time(e) <= timestamp):
        return entry_len - index - 1
      # Keep the entry with timestamp >= the specified timestamp
      elif self._compare_entry_time(e, timestamp, ge):
        end_entry = entry_len - index - 1
      elif self._compare_entry_time(e, timestamp, lt):
        return end_entry
    return end_entry

  def shrink(self, bgn_time=None, end_time=None, end_gesture_flag=True):
    """Shrink the log according to the begin time and end time.

    end_gesture_flag:
      When set to True, the shrunk log will contain the gestures resulted from
      the activities within the time range.
      When set to False, the shrunk log will have a hard cut at the entry
      with the smallest timestamp greater than or equal to the specified
      end_time.
    """
    if bgn_time is not None:
      self.bgn_entry_index = self._get_begin_hwstate(bgn_time)
    else:
      self.bgn_entry_index = 0

    if end_time is not None:
      if end_gesture_flag:
        self.end_entry_index = self._get_end_gesture(end_time)
      else:
        self.end_entry_index = self._get_end_entry(end_time)
    else:
      self.end_entry_index = len(self.entries) - 1

    if self.bgn_entry_index is None:
      logging.error('Error: fail to shrink the log baed on begin time: %f' %
                    bgn_time)
    if self.end_entry_index is None:
      logging.error('Error: fail to shrink the log baed on end time: %f' %
                    end_time)
    if self.bgn_entry_index is None or self.end_entry_index is None:
      exit(1)

    self.shrunk_log[self.ENTRIES] = self.entries[self.bgn_entry_index :
                                                 self.end_entry_index + 1]
    logging.info('  bgn_entry_index (%d):  %s' %
                 (self.bgn_entry_index, self.entries[self.bgn_entry_index]))
    logging.info('  end_entry_index (%d):  %s' %
                 (self.end_entry_index, self.entries[self.end_entry_index]))

  def replace_properties(self, prop_file):
    """Replace properties with those in the given file."""
    if not prop_file:
      return
    with open(prop_file) as f:
      prop = json.load(f)
    properties = prop.get(self.PROPERTIES)
    if properties:
        self.shrunk_log[self.PROPERTIES] = properties

  def add_description(self, description):
    """Add description to the shrunk log."""
    if description:
      self.shrunk_log[self.DESCRIPTION] = description

  def dump_json(self, output_file):
    """Dump the new log object to a jason file."""
    with open(output_file, 'w') as f:
      json.dump(self.shrunk_log, f, indent=3, separators=(',', ': '),
                sort_keys=True)

  def run(self, options):
    """Run the operations on the log.

    The operations include shrinking the log and replacing the properties.
    """
    logging.info('Log file: %s' % options['log'])
    self.shrink(bgn_time=options['bgn_time'], end_time=options['end_time'],
                end_gesture_flag=options['end_gesture'])
    self.replace_properties(options['prop'])
    self.add_description(options['description'])
    self.dump_json(options['output'])


def _usage():
  """Print the usage of this program."""
  logging.info('Usage: $ %s [options]\n' % sys.argv[0])
  logging.info('options:')
  logging.info('  -b, --begin=<event_begin_time>')
  logging.info('        the begin timestamp to shrink the log.')
  logging.info('  -d, --description=<log_description>')
  logging.info('        Description of the log, e.g., "crosbug.com/12345"')
  logging.info('  -e, --end=<event_end_time>')
  logging.info('        the end timestamp to shrink the log.')
  logging.info('  -g, --end_gesture')
  logging.info('        When this flag is set, the shrunk log will contain\n'
               '        the gestures resulted from the activities within the\n'
               '        time range. Otherwise, the shrunk log will have a\n'
               '        hard cut at the entry with the smallest timestamp\n'
               '        greater than or equal to the specified end_time.')
  logging.info('  -h, --help: show this help')
  logging.info('  -l, --log=<activity_log> (required)')
  logging.info('  -o, --output=<output_file> (required)')
  logging.info('  -p, --prop=<new_property_file>')
  logging.info('        If a new property file is specified, it will be used\n'
               '        to replace the original properties in the log.')
  logging.info('')


def _parse_options():
  """Parse the command line options."""
  try:
    short_opt = 'b:d:e:ghl:o:p:'
    long_opt = ['begin=', 'description', 'end=', 'end_gesture', 'help',
                'log=', 'output=', 'prop=']
    opts, _ = getopt.getopt(sys.argv[1:], short_opt, long_opt)
  except getopt.GetoptError, err:
    logging.error('Error: %s' % str(err))
    _usage()
    sys.exit(1)

  options = {}
  options['end_gesture'] = False
  options['bgn_time'] = None
  options['description'] = None
  options['end_time'] = None
  options['prop'] = None
  for opt, arg in opts:
    if opt in ('-h', '--help'):
      _usage()
      sys.exit()
    elif opt in ('-b', '--begin'):
      options['bgn_time'] = float(arg)
    elif opt in ('-d', '--description'):
      options['description'] = arg
    elif opt in ('-e', '--end'):
      options['end_time'] = float(arg)
    elif opt in ('-g', '--end_gesture'):
      options['end_gesture'] = True
    elif opt in ('-l', '--log'):
      if os.path.isfile(arg):
        options['log'] = arg
      else:
        logging.error('Error: the log file does not exist: %s.' % arg)
        sys.exit(1)
    elif opt in ('-o', '--output'):
      options['output'] = arg
    elif opt in ('-p', '--prop'):
      if os.path.isfile(arg):
        options['prop'] = arg
      else:
        logging.error('Error: the properties file does not exist: %s.' % arg)
        sys.exit(1)
    else:
      logging.error('Error: This option %s is not handled in program.' % opt)
      _usage()
      sys.exit(1)

  if not options.get('log') or not options.get('output'):
    logging.error('Error: You need to specify both --log and --output.')
    _usage()
    sys.exit(1)
  return options


if __name__ == '__main__':
  logging.basicConfig(format='', level=logging.INFO)
  options = _parse_options()
  tplog = TPLog(options['log'])
  tplog.run(options)
