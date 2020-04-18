# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to merge logs together."""

from __future__ import print_function

import collections
import datetime
import dateutil
import dateutil.parser
import os
import re

from chromite.lib import commandline
from chromite.lib import cros_logging as logging
from chromite.lib import gs


# Default timezone to assume for logs if one is not specified.
DEFAULT_TIMEZONE = 'America/Los_Angeles'


Log = collections.namedtuple('Log', ('filename', 'date', 'log'))

def GetParser():
  """Creates the argparse parser."""
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('files', type=str, nargs='*', action='store',
                      help='Log filenames.')
  parser.add_argument('--filelist', '-f', type=str, action='store',
                      help='File that contains a list of files to view.')
  parser.add_argument('--html', action='store_true',
                      help='Generate HTML.')
  parser.add_argument('--raw', action='store_true',
                      help='Generate raw output.')
  parser.add_argument('--nosort', action='store_false', dest='sort',
                      help='Do not sort the results.')
  parser.add_argument('--base', type=str, action='store',
                      help='Base path to pre-pend to all files.')
  parser.add_argument('--notrim_base', action='store_false', dest='trim_base',
                      help='Do not trim the file path prefixes.')
  parser.add_argument('--trim_path', action='store_true',
                      help='Trim the file path.')
  return parser


def Now():
  """Returns the current datetime.

  Added as separate function to allow it to be mockable during tests.

  Returns:
    a datetime.
  """
  return datetime.datetime.now()


# The following are basic timestamp parsing functions.  All are expected
# to return a timezone, defaulting to DEFAULT_TIMEZONE.  Some of the functions
# are expected to fill in a year (assumes current year) if a year is not
# parseable.

def ParseDate(timestamp):
  """Basic timestamp to datetime parser.

  Args:
    timestamp: a string.

  Returns:
    a datetime.
  """
  naive_dt = dateutil.parser.parse(timestamp)
  dt = naive_dt.replace(tzinfo=naive_dt.tzinfo or
                        dateutil.tz.gettz(DEFAULT_TIMEZONE))
  return dt


def ParseAutoservDate(timestamp):
  """Autoserv log format timestamp to datetime parser.

  Args:
    timestamp: a string.

  Returns:
    a datetime.
  """
  year = str(Now().year)
  naive_dt = dateutil.parser.parse(year + '/' + timestamp)
  dt = naive_dt.replace(tzinfo=naive_dt.tzinfo or
                        dateutil.tz.gettz(DEFAULT_TIMEZONE))
  return dt


def ParseChromeDate(timestamp):
  """Chrome log format timestamp to datetime parser.

  Args:
    timestamp: a string.

  Returns:
    a datetime.
  """
  year = str(Now().year)
  naive_dt = datetime.datetime.strptime(timestamp + ' ' + year,
                                        '%m%d/%H%M%S.%f %Y')
  dt = naive_dt.replace(tzinfo=naive_dt.tzinfo or
                        dateutil.tz.gettz(DEFAULT_TIMEZONE))
  return dt


def ParsePowerdDate(timestamp):
  """Powerd log format timestamp to datetime parser.

  Args:
    timestamp: a string.

  Returns:
    a datetime.
  """
  year = str(Now().year)
  naive_dt = datetime.datetime.strptime(timestamp + ' ' + year,
                                        '%m%d/%H%M%S %Y')
  dt = naive_dt.replace(tzinfo=naive_dt.tzinfo or
                        dateutil.tz.gettz(DEFAULT_TIMEZONE))
  return dt


# The following is all the file/date contents handled.  There is an regexp
# pattern which if it matches, specifies a function to call to parse the date
# and a lambda to extract a timestamp via the regexp.
Pattern = collections.namedtuple('Patern', ('regexp', 'func', 'key'))

# pylint: disable=line-too-long
# [0731/070232:INFO:main.cc(289)] System uptime: 5s
# 2017-07-31 07:05:10.257860139-07:00: Starting arc-removable-media
ARC_DATE_RE = re.compile(
    r'^(\d\d\d\d-\d\d-\d\d \d\d:\d\d:\d\d.\d\d\d\d\d\d)\d\d\d([+-]\d\d:\d\d):')
ARCPatterns = [Pattern(ARC_DATE_RE, ParseDate,
                       lambda m: m.group(1) + m.group(2))]

# 07/31 09:17:27.919 DEBUG|             utils:0212| Running 'test -d /tmp/sysinfo/autoserv-LOMvzK'
AUTOSERV_DATE_RE = re.compile(r'^(\d\d/\d\d \d\d:\d\d:\d\d.\d\d\d) ')
AutoservPatterns = [Pattern(AUTOSERV_DATE_RE, ParseAutoservDate,
                            lambda m: m.group(1))]

# [8525:8525:0731/072833.409065:VERBOSE1:gaia_screen_handler.cc(480)] OnPortalDetectionCompleted Online
CHROME_DATE_RE = re.compile(r'^\[\d+:\d+:(\d{4}/\d{6}.\d{6}):')
ChromePatterns = [Pattern(CHROME_DATE_RE, ParseChromeDate,
                          lambda m: m.group(1))]

# 2017/07/31 09:18:08.871 DEBUG|     remote_access:0659| The temporary working directory on the device is /mnt/stateful_partition/unencrypted/preserve/cros-update/tmp.S7y5vF3xQE
CROS_DATE_RE = re.compile(r'^(\d\d\d\d/\d\d/\d\d \d\d:\d\d:\d\d.\d\d\d) ')
CrOSPatterns = [Pattern(CROS_DATE_RE, ParseDate, lambda m: m.group(1))]

# [0731/070232:INFO:main.cc(289)] System uptime: 5s
POWERD_DATE_RE = re.compile(r'^\[(\d{4}/\d{6}):')
PowerdPatterns = [Pattern(POWERD_DATE_RE, ParsePowerdDate,
                          lambda m: m.group(1))]

# 2017-07-31 07:00:46,650 - DEBUG - Running hook: /usr/local/bin/hooks/check_ethernet.hook
RECOVER_DUTS_DATE_RE = re.compile(
    r'^(\d\d\d\d-\d\d-\d\d \d\d:\d\d:\d\d),(\d\d\d) ')
RecoverDutsPatterns = [Pattern(RECOVER_DUTS_DATE_RE, ParseDate,
                               lambda m: m.group(1) + '.' + m.group(2))]

#         START   provision_AutoUpdate    provision_AutoUpdate    timestamp=1501517822    localtime=Jul 31 09:17:02
STATUS_LOG_DATE_RE = re.compile(r'.*localtime=(... \d\d \d\d:\d\d:\d\d)')
StatusLogPatterns = [Pattern(STATUS_LOG_DATE_RE, ParseDate,
                             lambda m: m.group(1))]

# 2017-07-31T09:17:25.907285-07:00 NOTICE ag[9829]: autotest server[stack::get_tmp_dir|run|wrapper] -> ssh_run(mktemp -d /tmp/autoserv-XXXXXX)
SYSINFO_DATE_RE = re.compile(
    r'^(\d\d\d\d-\d\d-\d\dT\d\d:\d\d:\d\d.\d\d\d\d\d\d[+-]\d\d:\d\d) ')
SysinfoPatterns = [Pattern(SYSINFO_DATE_RE, ParseDate, lambda m: m.group(1))]

AllPatterns = (ARCPatterns + AutoservPatterns + ChromePatterns +
               CrOSPatterns + PowerdPatterns + RecoverDutsPatterns +
               StatusLogPatterns + SysinfoPatterns)


class LogParser(object):
  """Line-based log parsing class.

  Allows sub-typing via the patterns field to handle specific file types
  without looping through all patterns for each file.
  """
  def __init__(self, filename, patterns=None):
    """LogParser constructor.

    Args:
      filename: a string of the file to be parsed.
      patterns: a list of Pattern namedtuples
    """
    self.filename = filename
    self.patterns = patterns or AllPatterns

  def ParseDate(self, line):
    """Parse the date out of a single log line.

    Args:
      line: a string of the log line.

    Returns:
      a datetime if one can be parsed, None otherwise.
    """
    for pattern, func, key in self.patterns:
      m = pattern.match(line)
      if m:
        dt = func(key(m))
        return dt
    return None

  def ParseLine(self, line, previous_dt=None):
    """Parse a single log line.

    Args:
      line: a string of the log line.
      previous_dt: a datetime of the previous log to use if one cannot be found.

    Returns:
      a Log namedtuple.
    """
    dt = self.ParseDate(line)
    if dt is None:
      dt = previous_dt
    return Log(filename=self.filename, date=dt, log=line)


# Patterns to match filename to date parsing patterns.
FILE_PATTERNS = [
    (re.compile(p), dp) for p, dp in [
        (r'(.*/|)arc.*.log', ARCPatterns),
        (r'(.*/|)debug/client.*', AutoservPatterns),
        (r'(.*/|)debug/autoserv.*', AutoservPatterns),
        (r'(.*/|)[^/]+.(DEBUG|INFO|WARNING|ERROR)', AutoservPatterns),
        (r'(.*/|)CrOS_update_[^/]*log', CrOSPatterns),
        (r'(.*/|)powerd', PowerdPatterns),
        (r'(.*/|)recover_duts.log', RecoverDutsPatterns),
        (r'(.*/|)status(.log)?', StatusLogPatterns),
        (r'(.*/|)(messages|secure|tlsdate.log)', SysinfoPatterns),
        # chrome will match directory components, so leave it last.
        (r'(.*/|)(chrome|ui)(\.[^/]*)?$', ChromePatterns),
    ]
]


def FindParser(filename):
  """Select a parser based on filename.

  Generates a sub-types parser suitable for handling the dates of a given file.

  Args:
    filename: a string of the log file to be parsed.

  Returns:
    LogParser object suitable for parsing that filetype.
  """
  for file_pattern, date_patterns in FILE_PATTERNS:
    if file_pattern.match(filename):
      return LogParser(filename, date_patterns)
  logging.warning('Could not find parser for file: %s', filename)
  return LogParser(filename)


def ParseFileContents(filename, content):
  """Parse the log contents belonging to a single file.

  Args:
    filename: a string of the file's filename to parse.
    content: a list of string of the log lines of the file.

  Returns:
    a list of Log namedtuples.
  """
  parser = FindParser(filename)
  logs = []
  first_dt = None
  lines = content.splitlines()

  previous_dt = None
  for line in lines:
    log = parser.ParseLine(line.rstrip(), previous_dt)
    logs.append(log)
    previous_dt = log.date
    # Keep track of the first non-None datetime that is parsed.
    if first_dt is None and previous_dt is not None:
      first_dt = previous_dt

  # Ensure that all log entries have a time, using now if one cannot be
  # determined.
  if first_dt is None:
    first_dt = Now()
  for i in xrange(len(logs)):
    if logs[i].date is None:
      logs[i].date = first_dt

  return logs


GS_RE = re.compile(r'gs://')


def ParseURL(url):
  """Parse the files specified by a URL or filename.

  If url is a gs:// URL, globbing is supported.

  Args:
    url: a string of a GS URL or a flat filename.

  Returns:
    a list of Log namedtuples.
  """
  logs = []
  if GS_RE.match(url):
    ctx = gs.GSContext()
    try:
      files = ctx.LS(url)
    except gs.GSNoSuchKey:
      files = []
    for filename in files:
      try:
        content = ctx.Cat(filename)
        logs.extend(ParseFileContents(filename, content))
      except gs.GSNoSuchKey:
        logging.warning("Couldn't find file %s for url %s.", filename, url)

  else:
    with open(url) as f:
      content = f.read()
    logs.extend(ParseFileContents(url, content))
  return logs


def PrintLog(log):
  """Prints a log to stdout.

  Args:
    log: a Log namedtuple.
  """
  print('%s: %s' % (log.filename, log.log))


def TrimLogPrefix(log, base):
  """Removes the prefix |base| from |log|'s filename.

  Args:
    log: a Log namedtuple
    base: a string prefix to trim the filenames by.
  """
  fname = log.filename
  if fname.startswith(base):
    fname = fname[len(base):]
  return Log(fname, log.date, log.log)


def TrimLogPath(log):
  """Removes the prefix path from |log|'s filename.

  Args:
    log: a Log namedtuple
  """
  return Log(os.path.basename(log.filename), log.date, log.log)


def PrintHtmlHeader():
  """Prints an HTML header for log output."""
  print('<html>')
  print(' <head>')
  print('  <style>')
  print('   .line {font-family: monospace;}')
  print('   .filename {color: red;}')
  print('   .date {color: blue; display: none;}')
  print('   .log {white-space: pre;}')
  print('  </style>')
  print(' </head>')
  print(' <body>')


def PrintHtmlFooter():
  """Prints an HTML footer for log output."""
  print(' </body>')
  print('</html>')


def PrintHtml(log):
  """Prints a log as HTML.

  Args:
    log: a Log namedtuple.
  """
  def Tag(tag, cls, value):
    return '<%s class="%s">%s</%s>' % (tag, cls, value, tag)
  classes = ['filename', 'date', 'log']
  line = ' '.join([Tag('span', cls, value) for cls, value in zip(classes, log)])
  print(Tag('div', 'line', line))


def main(argv):
  parser = GetParser()
  options = parser.parse_args(argv)

  # Determine list of files to show.
  files = options.files
  if options.filelist:
    with open(options.filelist) as f:
      found = [l.strip() for l in f.readlines()]
    files.extend(filter(bool, found))
  if options.base:
    files = [os.path.join(options.base, f) for f in files]

  # Parse all the files.
  logs = []
  for filename in files:
    logs.extend(ParseURL(filename))

  if options.sort:
    logs.sort(key=lambda log: log.date)

  if options.trim_base and options.base:
    logs = [TrimLogPrefix(log, options.base) for log in logs]

  if options.trim_path:
    logs = [TrimLogPath(log) for log in logs]

  # TODO(davidriley): This should dump JSON as well.
  if options.html:
    PrintHtmlHeader()
    printer = PrintHtml
  elif options.raw:
    printer = print
  else:
    printer = PrintLog

  for l in logs:
    printer(l)

  if options.html:
    PrintHtmlFooter()
