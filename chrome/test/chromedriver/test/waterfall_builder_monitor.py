#!/usr/bin/env python
# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Waterfall monitoring script.
  This script checks all builders specified in the config file and sends
  status email about any step failures in these builders. This also
  reports a build as failure if the latest build on that builder was built
  2 days back. (Number of days can be configured in the config file)

  This script can be run as cronjob on a linux machine once a day and
  get email notification for any waterfall specified in the config file.

  Sample cronjob entry below. This entry will run the script everyday at 9 AM.
  Include this in the crontab file.
  0 9 * * *  <Path to script> --config <Path to json file>
"""

import datetime
import json
import optparse
import sys
import time
import traceback
import urllib

from datetime import timedelta
from email.mime.text import MIMEText
from subprocess import Popen, PIPE


SUCCESS_SUBJECT = ('[CHROME TESTING]: Builder status %s: PASSED.')
FAILURE_SUBJECT = ('[CHROME TESTING]: Builder status %s: FAILED %d out of %d')
EXCEPTION_SUBJECT = ('Exception occurred running waterfall_builder_monitor.py '
                     'script')


def GetTimeDelta(date, days):
  if isinstance(date, datetime.datetime):
    return date + timedelta(days)


def GetDateFromEpochFormat(epoch_time):
  last_build_date = time.localtime(epoch_time)
  last_build_date = datetime.datetime(int(last_build_date.tm_year),
                                      int(last_build_date.tm_mon),
                                      int(last_build_date.tm_mday),
                                      int(last_build_date.tm_hour),
                                      int(last_build_date.tm_min),
                                      int(last_build_date.tm_sec))
  return last_build_date


def GetJSONData(json_url):
  response = urllib.urlopen(json_url)
  if response.getcode() == 200:
    try:
      data = json.loads(response.read())
    except ValueError:
      print 'ValueError for JSON URL: %s' % json_url
      raise
  else:
    raise Exception('Error from URL: %s' % json_url)
  response.close()
  return data


def SendEmailViaSendmailCommand(sender_email, recipient_emails,
                                subject, email_body):
  msg = MIMEText(email_body)
  msg["From"] = sender_email
  msg["To"] = recipient_emails
  msg["Subject"] = subject
  pipe = Popen(["/usr/sbin/sendmail", "-t"], stdin=PIPE)
  pipe.communicate(msg.as_string())


def SendStatusEmailViaSendmailCommand(consolidated_results,
                                      recipient_emails,
                                      sender_email):
  failure_count = 0
  for result in consolidated_results:
    if result['error'] != 'passed' and not result['build_too_old']:
      failure_count += 1
  today = str(datetime.date.today()).replace('-', '/')[5:]
  if failure_count == 0:
    subject = SUCCESS_SUBJECT % today
  else:
    subject = FAILURE_SUBJECT % (today,
                                 failure_count,
                                 len(consolidated_results))

  email_body = ''
  for result in consolidated_results:
    if result['error'] != 'passed' or result['build_too_old']:
      if result['build_date'] is not None:
        email_body += result['platform'] + ': ' +\
                      result['build_link'] + ' ( Build too old: ' +\
                      result['build_date'] + ' ) ' +'\n\n'
      else:
        email_body += result['platform'] + ': ' +\
                      result['build_link'] + '\n\n'

  SendEmailViaSendmailCommand(sender_email, recipient_emails,
                              subject, email_body)


def SendExceptionEmailViaSendmailCommand(exception_message_lines,
                                         recipient_emails,
                                         sender_email):
  subject = EXCEPTION_SUBJECT
  email_body = ''
  email_body = '\n'.join(exception_message_lines)

  SendEmailViaSendmailCommand(sender_email, recipient_emails,
                              subject, email_body)


class OfficialBuilderParser(object):
  """This class implements basic utility functions on a specified builder."""
  def __init__(self, builder_type, build_info):
    self.platform = builder_type
    self.builder_info = build_info
    self.builder_url = build_info['builder_url']
    self.build_json_url = build_info['json_url']
    self.build = self._GetLatestBuildNumber()

  def _GetLatestBuildNumber(self):
    json_url = self.builder_info['builds_url']
    data = GetJSONData(json_url)
    # Get a sorted list of all the keys in the json data.
    keys = sorted(map(int, data.keys()))
    return self._GetLatestCompletedBuild(keys)

  def _GetLatestCompletedBuild(self, keys):
    reversed_list = keys[::-1]
    for build in reversed_list:
      data = self._GetJSONDataForBuild(build)
      if data is not None:
        if 'text' in data:
          return build
    return None

  def _GetJSONDataForBuild(self, build):
    if build is None:
      return build
    json_url = self.build_json_url % build
    return GetJSONData(json_url)


class GetBuilderStatus(OfficialBuilderParser):
  def __init__(self, builder_type, build_info):
    OfficialBuilderParser.__init__(self, builder_type, build_info)

  def CheckForFailedSteps(self, days):
    if self.build is None:
      return {}
    result = {'platform': self.platform,
              'build_number': self.build,
              'build_link': self.builder_url + str(self.build),
              'build_date': None,
              'build_too_old': False,
              'error': 'unknown'}
    data = self._GetJSONDataForBuild(self.build)
    if data is not None:
      if 'text' in data:
        if 'build' in data['text'] and 'successful' in data['text']:
          result['error'] = 'passed'
        else:
          if 'failed' in data['text'] or\
             'exception' in data['text'] or\
             'interrupted' in data['text']:
            result['error'] = 'failed'
      if 'times' in data:
        old_date = GetTimeDelta(datetime.datetime.now(), days)
        last_build_date = GetDateFromEpochFormat(data['times'][0])
        if last_build_date < old_date:
          result['build_too_old'] = True
          result['build_date'] = str(last_build_date).split(' ')[0]
    else:
      raise Exception('There was some problem getting JSON data '
                      'from URL: %s' % result['build_link'])
    return result

def main():
  parser = optparse.OptionParser()
  parser.add_option('--config', type='str',
                    help='Absolute path to the config file.')

  (options, _) = parser.parse_args()
  if not options.config:
    print 'Error: missing required parameter: --config'
    parser.print_help()
    return 1

  try:
    with open(options.config, 'r') as config_file:
      try:
        json_data = json.loads(config_file.read())
      except ValueError:
        print 'ValueError for loading JSON data from : %s' % options.config
        raise ValueError

    old_build_days = -2
    if 'old_build_days' in json_data:
      old_build_days = - json_data['old_build_days']
    consolidated_results = []
    for key in json_data['build_info'].keys():
      builder_status = GetBuilderStatus(key, json_data['build_info'][key])
      builder_result = builder_status.CheckForFailedSteps(old_build_days)
      consolidated_results.append(builder_result)

    SendStatusEmailViaSendmailCommand(consolidated_results,
                                      json_data['recipient_emails'],
                                      json_data['sender_email'])
    return 0
  except Exception:
    formatted_lines = traceback.format_exc().splitlines()
    SendExceptionEmailViaSendmailCommand(formatted_lines,
                                         json_data['recipient_emails'],
                                         json_data['sender_email'])
    return 1

if __name__ == '__main__':
  sys.exit(main())
