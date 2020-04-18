#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Get rietveld stats about the review you done, or forgot to do.

Example:
  - my_reviews.py -r me@chromium.org -Q  for stats for last quarter.
"""
import datetime
import math
import optparse
import os
import sys

import auth
import rietveld

try:
  import dateutil  # pylint: disable=import-error
  import dateutil.parser
  from dateutil.relativedelta import relativedelta
except ImportError:
  print 'python-dateutil package required'
  exit(1)


def username(email):
  """Keeps the username of an email address."""
  return email.split('@', 1)[0]


def to_datetime(string):
  """Load UTC time as a string into a datetime object."""
  try:
    # Format is 2011-07-05 01:26:12.084316
    return datetime.datetime.strptime(
        string.split('.', 1)[0], '%Y-%m-%d %H:%M:%S')
  except ValueError:
    return datetime.datetime.strptime(string, '%Y-%m-%d')


def to_time(seconds):
  """Convert a number of seconds into human readable compact string."""
  prefix = ''
  if seconds < 0:
    prefix = '-'
    seconds *= -1
  minutes = math.floor(seconds / 60)
  seconds -= minutes * 60
  hours = math.floor(minutes / 60)
  minutes -= hours * 60
  days = math.floor(hours / 24)
  hours -= days * 24
  out = []
  if days > 0:
    out.append('%dd' % days)
  if hours > 0 or days > 0:
    out.append('%02dh' % hours)
  if minutes > 0 or hours > 0 or days > 0:
    out.append('%02dm' % minutes)
  if seconds > 0 and not out:
    # Skip seconds unless there's only seconds.
    out.append('%02ds' % seconds)
  return prefix + ''.join(out)


class Stats(object):
  def __init__(self):
    self.total = 0
    self.actually_reviewed = 0
    self.latencies = []
    self.lgtms = 0
    self.multiple_lgtms = 0
    self.drive_by = 0
    self.not_requested = 0
    self.self_review = 0

    self.percent_lgtm = 0.
    self.percent_drive_by = 0.
    self.percent_not_requested = 0.
    self.days = 0

  @property
  def average_latency(self):
    if not self.latencies:
      return 0
    return sum(self.latencies) / float(len(self.latencies))

  @property
  def median_latency(self):
    if not self.latencies:
      return 0
    length = len(self.latencies)
    latencies = sorted(self.latencies)
    if (length & 1) == 0:
      return (latencies[length/2] + latencies[length/2-1]) / 2.
    else:
      return latencies[length/2]

  @property
  def percent_done(self):
    if not self.total:
      return 0
    return self.actually_reviewed * 100. / self.total

  @property
  def review_per_day(self):
    if not self.days:
      return 0
    return self.total * 1. / self.days

  @property
  def review_done_per_day(self):
    if not self.days:
      return 0
    return self.actually_reviewed * 1. / self.days

  def finalize(self, first_day, last_day):
    if self.actually_reviewed:
      assert self.actually_reviewed > 0
      self.percent_lgtm = (self.lgtms * 100. / self.actually_reviewed)
      self.percent_drive_by = (self.drive_by * 100. / self.actually_reviewed)
      self.percent_not_requested = (
          self.not_requested * 100. / self.actually_reviewed)
    assert bool(first_day) == bool(last_day)
    if first_day and last_day:
      assert first_day <= last_day
      self.days = (to_datetime(last_day) - to_datetime(first_day)).days + 1
      assert self.days > 0


def _process_issue_lgtms(issue, reviewer, stats):
  """Calculates LGTMs stats."""
  stats.actually_reviewed += 1
  reviewer_lgtms = len([
    msg for msg in issue['messages']
    if msg['approval'] and msg['sender'] == reviewer])
  if reviewer_lgtms > 1:
    stats.multiple_lgtms += 1
    return ' X '
  if reviewer_lgtms:
    stats.lgtms += 1
    return ' x '
  else:
    return ' o '


def _process_issue_latency(issue, reviewer, stats):
  """Calculates latency for an issue that was actually reviewed."""
  from_owner = [
    msg for msg in issue['messages'] if msg['sender'] == issue['owner_email']
  ]
  if not from_owner:
    # Probably requested by email.
    stats.not_requested += 1
    return '<no rqst sent>'

  first_msg_from_owner = None
  latency = None
  received = False
  for index, msg in enumerate(issue['messages']):
    if not first_msg_from_owner and msg['sender'] == issue['owner_email']:
      first_msg_from_owner = msg
    if index and not received and msg['sender'] == reviewer:
      # Not first email, reviewer never received one, reviewer sent a mesage.
      stats.drive_by += 1
      return '<drive-by>'
    received |= reviewer in msg['recipients']

    if first_msg_from_owner and msg['sender'] == reviewer:
      delta = msg['date'] - first_msg_from_owner['date']
      latency = delta.seconds + delta.days * 24 * 3600
      break

  if latency is None:
    stats.not_requested += 1
    return '<no rqst sent>'
  if latency > 0:
    stats.latencies.append(latency)
  else:
    stats.not_requested += 1
  return to_time(latency)


def _process_issue(issue):
  """Preprocesses the issue to simplify the remaining code."""
  issue['owner_email'] = username(issue['owner_email'])
  issue['reviewers'] = set(username(r) for r in issue['reviewers'])
  # By default, hide commit-bot.
  issue['reviewers'] -= set(['commit-bot'])
  for msg in issue['messages']:
    msg['sender'] = username(msg['sender'])
    msg['recipients'] = [username(r) for r in msg['recipients']]
    # Convert all times to datetime instances.
    msg['date'] = to_datetime(msg['date'])
  issue['messages'].sort(key=lambda x: x['date'])


def print_issue(issue, reviewer, stats):
  """Process an issue and prints stats about it."""
  stats.total += 1
  _process_issue(issue)
  if issue['owner_email'] == reviewer:
    stats.self_review += 1
    latency = '<self review>'
    reviewed = ''
  elif any(msg['sender'] == reviewer for msg in issue['messages']):
    reviewed = _process_issue_lgtms(issue, reviewer, stats)
    latency = _process_issue_latency(issue, reviewer, stats)
  else:
    latency = 'N/A'
    reviewed = ''

  # More information is available, print issue.keys() to see them.
  print '%7d %10s %3s %14s %-15s  %s' % (
      issue['issue'],
      issue['created'][:10],
      reviewed,
      latency,
      issue['owner_email'],
      ', '.join(sorted(issue['reviewers'])))


def print_reviews(
    reviewer, created_after, created_before, instance_url, auth_config):
  """Prints issues |reviewer| received and potentially reviewed."""
  remote = rietveld.Rietveld(instance_url, auth_config)

  # The stats we gather. Feel free to send me a CL to get more stats.
  stats = Stats()

  # Column sizes need to match print_issue() output.
  print >> sys.stderr, (
      'Issue   Creation   Did         Latency Owner           Reviewers')

  # See def search() in rietveld.py to see all the filters you can use.
  issues = []
  for issue in remote.search(
      reviewer=reviewer,
      created_after=created_after,
      created_before=created_before,
      with_messages=True):
    issues.append(issue)
    print_issue(issue, username(reviewer), stats)

  issues.sort(key=lambda x: x['created'])
  first_day = None
  last_day = None
  if issues:
    first_day = issues[0]['created'][:10]
    last_day = issues[-1]['created'][:10]
  stats.finalize(first_day, last_day)

  print >> sys.stderr, (
      '%s reviewed %d issues out of %d (%1.1f%%). %d were self-review.' %
      (reviewer, stats.actually_reviewed, stats.total, stats.percent_done,
        stats.self_review))
  print >> sys.stderr, (
      '%4.1f review request/day during %3d days   (%4.1f r/d done).' % (
      stats.review_per_day, stats.days, stats.review_done_per_day))
  print >> sys.stderr, (
      '%4d were drive-bys                       (%5.1f%% of reviews done).' % (
        stats.drive_by, stats.percent_drive_by))
  print >> sys.stderr, (
      '%4d were requested over IM or irc        (%5.1f%% of reviews done).' % (
        stats.not_requested, stats.percent_not_requested))
  print >> sys.stderr, (
      ('%4d issues LGTM\'d                        (%5.1f%% of reviews done),'
       ' gave multiple LGTMs on %d issues.') % (
      stats.lgtms, stats.percent_lgtm, stats.multiple_lgtms))
  print >> sys.stderr, (
      'Average latency from request to first comment is %s.' %
      to_time(stats.average_latency))
  print >> sys.stderr, (
      'Median latency from request to first comment is %s.' %
      to_time(stats.median_latency))


def print_count(
    reviewer, created_after, created_before, instance_url, auth_config):
  remote = rietveld.Rietveld(instance_url, auth_config)
  print len(list(remote.search(
      reviewer=reviewer,
      created_after=created_after,
      created_before=created_before,
      keys_only=True)))


def get_previous_quarter(today):
  """There are four quarters, 01-03, 04-06, 07-09, 10-12.

  If today is in the last month of a quarter, assume it's the current quarter
  that is requested.
  """
  end_year = today.year
  end_month = today.month - (today.month % 3) + 1
  if end_month <= 0:
    end_year -= 1
    end_month += 12
  if end_month > 12:
    end_year += 1
    end_month -= 12
  end = '%d-%02d-01' % (end_year, end_month)
  begin_year = end_year
  begin_month = end_month - 3
  if begin_month <= 0:
    begin_year -= 1
    begin_month += 12
  begin = '%d-%02d-01' % (begin_year, begin_month)
  return begin, end


def main():
  # Silence upload.py.
  rietveld.upload.verbosity = 0
  today = datetime.date.today()
  begin, end = get_previous_quarter(today)
  default_email = os.environ.get('EMAIL_ADDRESS')
  if not default_email:
    user = os.environ.get('USER')
    if user:
      default_email = user + '@chromium.org'

  parser = optparse.OptionParser(description=__doc__)
  parser.add_option(
      '--count', action='store_true',
      help='Just count instead of printing individual issues')
  parser.add_option(
      '-r', '--reviewer', metavar='<email>', default=default_email,
      help='Filter on issue reviewer, default=%default')
  parser.add_option(
      '-b', '--begin', metavar='<date>',
      help='Filter issues created after the date')
  parser.add_option(
      '-e', '--end', metavar='<date>',
      help='Filter issues created before the date')
  parser.add_option(
      '-Q', '--last_quarter', action='store_true',
      help='Use last quarter\'s dates, e.g. %s to %s' % (begin, end))
  parser.add_option(
      '-i', '--instance_url', metavar='<host>',
      default='http://codereview.chromium.org',
      help='Host to use, default is %default')
  auth.add_auth_options(parser)
  # Remove description formatting
  parser.format_description = (
      lambda _: parser.description)  # pylint: disable=no-member
  options, args = parser.parse_args()
  auth_config = auth.extract_auth_config_from_options(options)
  if args:
    parser.error('Args unsupported')
  if options.reviewer is None:
    parser.error('$EMAIL_ADDRESS and $USER are not set, please use -r')

  print >> sys.stderr, 'Searching for reviews by %s' % options.reviewer
  if options.last_quarter:
    options.begin = begin
    options.end = end
    print >> sys.stderr, 'Using range %s to %s' % (
        options.begin, options.end)
  else:
    if options.begin is None or options.end is None:
      parser.error('Please specify either --last_quarter or --begin and --end')

  # Validate dates.
  try:
    options.begin = dateutil.parser.parse(options.begin).strftime('%Y-%m-%d')
    options.end = dateutil.parser.parse(options.end).strftime('%Y-%m-%d')
  except ValueError as e:
    parser.error('%s: %s - %s' % (e, options.begin, options.end))

  if options.count:
    print_count(
        options.reviewer,
        options.begin,
        options.end,
        options.instance_url,
        auth_config)
  else:
    print_reviews(
        options.reviewer,
        options.begin,
        options.end,
        options.instance_url,
        auth_config)
  return 0


if __name__ == '__main__':
  try:
    sys.exit(main())
  except KeyboardInterrupt:
    sys.stderr.write('interrupted\n')
    sys.exit(1)
