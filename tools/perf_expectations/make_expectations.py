#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# For instructions see:
# http://www.chromium.org/developers/tree-sheriffs/perf-sheriffs

import hashlib
import math
import optparse
import os
import re
import subprocess
import sys
import time
import urllib2


try:
  import json
except ImportError:
  import simplejson as json


__version__ = '1.0'
EXPECTATIONS_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_CONFIG_FILE = os.path.join(EXPECTATIONS_DIR,
                                   'chromium_perf_expectations.cfg')
DEFAULT_TOLERANCE = 0.05
USAGE = ''


def ReadFile(filename):
  try:
    file = open(filename, 'rb')
  except IOError, e:
    print >> sys.stderr, ('I/O Error reading file %s(%s): %s' %
                          (filename, e.errno, e.strerror))
    raise e
  contents = file.read()
  file.close()
  return contents


def ConvertJsonIntoDict(string):
  """Read a JSON string and convert its contents into a Python datatype."""
  if len(string) == 0:
    print >> sys.stderr, ('Error could not parse empty string')
    raise Exception('JSON data missing')

  try:
    jsondata = json.loads(string)
  except ValueError, e:
    print >> sys.stderr, ('Error parsing string: "%s"' % string)
    raise e
  return jsondata


# Floating point representation of last time we fetched a URL.
last_fetched_at = None
def FetchUrlContents(url):
  global last_fetched_at
  if last_fetched_at and ((time.time() - last_fetched_at) <= 0.5):
    # Sleep for half a second to avoid overloading the server.
    time.sleep(0.5)
  try:
    last_fetched_at = time.time()
    connection = urllib2.urlopen(url)
  except urllib2.HTTPError, e:
    if e.code == 404:
      return None
    raise e
  text = connection.read().strip()
  connection.close()
  return text


def GetRowData(data, key):
  rowdata = []
  # reva and revb always come first.
  for subkey in ['reva', 'revb']:
    if subkey in data[key]:
      rowdata.append('"%s": %s' % (subkey, data[key][subkey]))
  # Strings, like type, come next.
  for subkey in ['type', 'better']:
    if subkey in data[key]:
      rowdata.append('"%s": "%s"' % (subkey, data[key][subkey]))
  # Finally the main numbers come last.
  for subkey in ['improve', 'regress', 'tolerance']:
    if subkey in data[key]:
      rowdata.append('"%s": %s' % (subkey, data[key][subkey]))
  return rowdata


def GetRowDigest(rowdata, key):
  sha1 = hashlib.sha1()
  rowdata = [str(possibly_unicode_string).encode('ascii')
             for possibly_unicode_string in rowdata]
  sha1.update(str(rowdata) + key)
  return sha1.hexdigest()[0:8]


def WriteJson(filename, data, keys, calculate_sha1=True):
  """Write a list of |keys| in |data| to the file specified in |filename|."""
  try:
    file = open(filename, 'wb')
  except IOError, e:
    print >> sys.stderr, ('I/O Error writing file %s(%s): %s' %
                          (filename, e.errno, e.strerror))
    return False
  jsondata = []
  for key in keys:
    rowdata = GetRowData(data, key)
    if calculate_sha1:
      # Include an updated checksum.
      rowdata.append('"sha1": "%s"' % GetRowDigest(rowdata, key))
    else:
      if 'sha1' in data[key]:
        rowdata.append('"sha1": "%s"' % (data[key]['sha1']))
    jsondata.append('"%s": {%s}' % (key, ', '.join(rowdata)))
  jsondata.append('"load": true')
  jsontext = '{%s\n}' % ',\n '.join(jsondata)
  file.write(jsontext + '\n')
  file.close()
  return True


def FloatIsInt(f):
  epsilon = 1.0e-10
  return abs(f - int(f)) <= epsilon


last_key_printed = None
def Main(args):
  def OutputMessage(message, verbose_message=True):
    global last_key_printed
    if not options.verbose and verbose_message:
      return

    if key != last_key_printed:
      last_key_printed = key
      print '\n' + key + ':'
    print '  %s' % message

  parser = optparse.OptionParser(usage=USAGE, version=__version__)
  parser.add_option('-v', '--verbose', action='store_true', default=False,
                    help='enable verbose output')
  parser.add_option('-s', '--checksum', action='store_true',
                    help='test if any changes are pending')
  parser.add_option('-c', '--config', dest='config_file',
                    default=DEFAULT_CONFIG_FILE,
                    help='set the config file to FILE', metavar='FILE')
  options, args = parser.parse_args(args)

  if options.verbose:
    print 'Verbose output enabled.'

  config = ConvertJsonIntoDict(ReadFile(options.config_file))

  # Get the list of summaries for a test.
  base_url = config['base_url']
  # Make the perf expectations file relative to the path of the config file.
  perf_file = os.path.join(
    os.path.dirname(options.config_file), config['perf_file'])
  perf = ConvertJsonIntoDict(ReadFile(perf_file))

  # Fetch graphs.dat for this combination.
  perfkeys = perf.keys()
  # In perf_expectations.json, ignore the 'load' key.
  perfkeys.remove('load')
  perfkeys.sort()

  write_new_expectations = False
  found_checksum_mismatch = False
  for key in perfkeys:
    value = perf[key]
    tolerance = value.get('tolerance', DEFAULT_TOLERANCE)
    better = value.get('better', None)

    # Verify the checksum.
    original_checksum = value.get('sha1', '')
    if 'sha1' in value:
      del value['sha1']
    rowdata = GetRowData(perf, key)
    computed_checksum = GetRowDigest(rowdata, key)
    if original_checksum == computed_checksum:
      OutputMessage('checksum matches, skipping')
      continue
    elif options.checksum:
      OutputMessage('checksum mismatch, original = %s, computed = %s' %
                    (original_checksum, computed_checksum))
      found_checksum_mismatch = True
      continue

    # Skip expectations that are missing a reva or revb.  We can't generate
    # expectations for those.
    if not(value.has_key('reva') and value.has_key('revb')):
      OutputMessage('missing revision range, skipping')
      continue
    revb = int(value['revb'])
    reva = int(value['reva'])

    # Ensure that reva is less than revb.
    if reva > revb:
      temp = reva
      reva = revb
      revb = temp

    # Get the system/test/graph/tracename and reftracename for the current key.
    matchData = re.match(r'^([^/]+)\/([^/]+)\/([^/]+)\/([^/]+)$', key)
    if not matchData:
      OutputMessage('cannot parse key, skipping')
      continue
    system = matchData.group(1)
    test = matchData.group(2)
    graph = matchData.group(3)
    tracename = matchData.group(4)
    reftracename = tracename + '_ref'

    # Create the summary_url and get the json data for that URL.
    # FetchUrlContents() may sleep to avoid overloading the server with
    # requests.
    summary_url = '%s/%s/%s/%s-summary.dat' % (base_url, system, test, graph)
    summaryjson = FetchUrlContents(summary_url)
    if not summaryjson:
      OutputMessage('ERROR: cannot find json data, please verify',
                    verbose_message=False)
      return 0

    # Set value's type to 'relative' by default.
    value_type = value.get('type', 'relative')

    summarylist = summaryjson.split('\n')
    trace_values = {}
    traces = [tracename]
    if value_type == 'relative':
      traces += [reftracename]
    for trace in traces:
      trace_values.setdefault(trace, {})

    # Find the high and low values for each of the traces.
    scanning = False
    for line in summarylist:
      jsondata = ConvertJsonIntoDict(line)
      try:
        rev = int(jsondata['rev'])
      except ValueError:
        print ('Warning: skipping rev %r because could not be parsed '
               'as an integer.' % jsondata['rev'])
        continue
      if rev <= revb:
        scanning = True
      if rev < reva:
        break

      # We found the upper revision in the range.  Scan for trace data until we
      # find the lower revision in the range.
      if scanning:
        for trace in traces:
          if trace not in jsondata['traces']:
            OutputMessage('trace %s missing' % trace)
            continue
          if type(jsondata['traces'][trace]) != type([]):
            OutputMessage('trace %s format not recognized' % trace)
            continue
          try:
            tracevalue = float(jsondata['traces'][trace][0])
          except ValueError:
            OutputMessage('trace %s value error: %s' % (
                trace, str(jsondata['traces'][trace][0])))
            continue

          for bound in ['high', 'low']:
            trace_values[trace].setdefault(bound, tracevalue)

          trace_values[trace]['high'] = max(trace_values[trace]['high'],
                                            tracevalue)
          trace_values[trace]['low'] = min(trace_values[trace]['low'],
                                           tracevalue)

    if 'high' not in trace_values[tracename]:
      OutputMessage('no suitable traces matched, skipping')
      continue

    if value_type == 'relative':
      # Calculate assuming high deltas are regressions and low deltas are
      # improvements.
      regress = (float(trace_values[tracename]['high']) -
                 float(trace_values[reftracename]['low']))
      improve = (float(trace_values[tracename]['low']) -
                 float(trace_values[reftracename]['high']))
    elif value_type == 'absolute':
      # Calculate assuming high absolutes are regressions and low absolutes are
      # improvements.
      regress = float(trace_values[tracename]['high'])
      improve = float(trace_values[tracename]['low'])

    # So far we've assumed better is lower (regress > improve).  If the actual
    # values for regress and improve are equal, though, and better was not
    # specified, alert the user so we don't let them create a new file with
    # ambiguous rules.
    if better == None and regress == improve:
      OutputMessage('regress (%s) is equal to improve (%s), and "better" is '
                    'unspecified, please fix by setting "better": "lower" or '
                    '"better": "higher" in this perf trace\'s expectation' % (
                    regress, improve), verbose_message=False)
      return 1

    # If the existing values assume regressions are low deltas relative to
    # improvements, swap our regress and improve.  This value must be a
    # scores-like result.
    if 'regress' in perf[key] and 'improve' in perf[key]:
      if perf[key]['regress'] < perf[key]['improve']:
        assert(better != 'lower')
        better = 'higher'
        temp = regress
        regress = improve
        improve = temp
      else:
        # Sometimes values are equal, e.g., when they are both 0,
        # 'better' may still be set to 'higher'.
        assert(better != 'higher' or
               perf[key]['regress'] == perf[key]['improve'])
        better = 'lower'

    # If both were ints keep as int, otherwise use the float version.
    originally_ints = False
    if FloatIsInt(regress) and FloatIsInt(improve):
      originally_ints = True

    if better == 'higher':
      if originally_ints:
        regress = int(math.floor(regress - abs(regress*tolerance)))
        improve = int(math.ceil(improve + abs(improve*tolerance)))
      else:
        regress = regress - abs(regress*tolerance)
        improve = improve + abs(improve*tolerance)
    else:
      if originally_ints:
        improve = int(math.floor(improve - abs(improve*tolerance)))
        regress = int(math.ceil(regress + abs(regress*tolerance)))
      else:
        improve = improve - abs(improve*tolerance)
        regress = regress + abs(regress*tolerance)

    # Calculate the new checksum to test if this is the only thing that may have
    # changed.
    checksum_rowdata = GetRowData(perf, key)
    new_checksum = GetRowDigest(checksum_rowdata, key)

    if ('regress' in perf[key] and 'improve' in perf[key] and
        perf[key]['regress'] == regress and perf[key]['improve'] == improve and
        original_checksum == new_checksum):
      OutputMessage('no change')
      continue

    write_new_expectations = True
    OutputMessage('traces: %s' % trace_values, verbose_message=False)
    OutputMessage('before: %s' % perf[key], verbose_message=False)
    perf[key]['regress'] = regress
    perf[key]['improve'] = improve
    OutputMessage('after: %s' % perf[key], verbose_message=False)

  if options.checksum:
    if found_checksum_mismatch:
      return 1
    else:
      return 0

  if write_new_expectations:
    print '\nWriting expectations... ',
    WriteJson(perf_file, perf, perfkeys)
    print 'done'
  else:
    if options.verbose:
      print ''
    print 'No changes.'
  return 0


if __name__ == '__main__':
  sys.exit(Main(sys.argv))
