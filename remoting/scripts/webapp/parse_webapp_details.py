#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import json
import sys

DESCRIPTION = '''This tools reads in a GYP file, parses it as JSON, grabs the
    requested object(s) using the passed in |key_name| and |key_value| and then
    builds up a space delimited string using the |return_key| given.  The built
    up string is then output to STDOUT which can be read and tokenized in
    bash or python scripts.'''
FILE_HELP = '''The GYP file to parse'''
APP_KEY_NAME_HELP = '''Key name used to identify relevant webapp details'''
APP_KEY_VALUE_HELP = '''Key value used to identify relevant webapp details,
    multiple values can be passed in'''
TARGET_KEY_VALUE_HELP = '''The key name used to output the targeted value'''
DEBUG_HELP = '''Turns on verbose debugging output'''

# Cleans up the text in the passed in GYP file, updates it to make it valid JSON
# and returns the valid json string.
def gyp_file_to_json_string(gyp_file):
  # First, read in each line and discard comments and whitespace.
  line_data = ''
  for line in gyp_file:
    lines = line.split("#")
    line_data += lines[0].strip()

  # Trailing commas are valid in GYP files but invalid in JSON, so remove them
  # here.  Also convert double quotes to single quotes and those throw off the
  # python json parser.
  line_data = line_data.replace(',}', '}')
  line_data = line_data.replace(',]', ']')
  line_data = line_data.replace('\'', '\"')

  return line_data


# Finds the matching app detail sections in |data| and generates a space
# delimited string with the |return_key|'s value.  If we found at least one
# matching app and created a string, we output it to STDOUT.
def print_details(data, key_name, key_values, target_key):
  output_string = ''
  for target in data['targets']:
    if target[key_name] in key_values:
      if output_string:
        output_string += " " + target[target_key]
      else:
        output_string += target[target_key]

  if output_string:
    print output_string


def main():
  parser = argparse.ArgumentParser(description = DESCRIPTION)
  parser.add_argument('file', nargs = '+', help = FILE_HELP)
  parser.add_argument('-k', '--key_name', help = APP_KEY_NAME_HELP,
      nargs = '?', required=True)
  parser.add_argument('-v', '--key_value', help = APP_KEY_VALUE_HELP,
      nargs = '+', required=True)
  parser.add_argument('-t', '--target_key', help = TARGET_KEY_VALUE_HELP,
      nargs = '?', required=True)
  parser.add_argument('-d', '--debug', help = DEBUG_HELP,
      action='store_true')
  options = parser.parse_args()

  if options.debug:
    print 'Reading from file \"' + options.file[0] + '\"'

  gyp_file = open(options.file[0])
  json_string = gyp_file_to_json_string(gyp_file)

  json_object = json.loads(json_string)

  if options.debug:
    print 'The following app details were found:'
    for target in json_object['targets']:
      print target['target_name'], target['app_id'], target['app_name']

  print_details(json_object,
                options.key_name,
                options.key_value,
                options.target_key)

  return 0

if __name__ == '__main__':
  sys.exit(main())
