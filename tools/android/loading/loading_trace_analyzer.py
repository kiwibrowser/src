#! /usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import json
import re
import sys

from loading_trace import LoadingTrace
import request_track


def _ArgumentParser():
  """Builds a command line argument's parser.
  """
  parser = argparse.ArgumentParser()
  subparsers = parser.add_subparsers(dest='subcommand', help='subcommand line')

  # requests listing subcommand.
  requests_parser = subparsers.add_parser('requests',
      help='Lists all request from the loading trace.')
  requests_parser.add_argument('loading_trace', type=str,
      help='Input loading trace to see the cache usage from.')
  requests_parser.add_argument('--output',
      type=argparse.FileType('w'),
      default=sys.stdout,
      help='Output destination path if different from stdout.')
  requests_parser.add_argument('--output-format', type=str, default='{url}',
      help='Output line format (Default to "{url}")')
  requests_parser.add_argument('--where',
      dest='where_statement', type=str,
      nargs=2, metavar=('FORMAT', 'REGEX'), default=[],
      help='Where statement to filter such as: --where "{protocol}" "https?"')

  # requests listing subcommand.
  prune_parser = subparsers.add_parser('prune',
      help='Prunes some stuff from traces to make them small.')
  prune_parser.add_argument('loading_trace', type=file,
      help='Input path of the loading trace.')
  prune_parser.add_argument('-t', '--trace-filters',
      type=str, nargs='+', metavar='REGEX', default=[],
      help='Regex filters to whitelist trace events.')
  prune_parser.add_argument('-r', '--request-member-filter',
      type=str, nargs='+', metavar='REGEX', default=[],
      help='Regex filters to whitelist requests\' members.')
  prune_parser.add_argument('-i', '--indent', type=int, default=2,
      help='Number of space to indent the output.')
  prune_parser.add_argument('-o', '--output',
      type=argparse.FileType('w'), default=sys.stdout,
      help='Output destination path if different from stdout.')
  return parser


def ListRequests(loading_trace_path,
                 output_format='{url}',
                 where_format='{url}',
                 where_statement=None):
  """`loading_trace_analyzer.py requests` Command line tool entry point.

  Args:
    loading_trace_path: Path of the loading trace.
    output_format: Output format of the generated strings.
    where_format: String formated to be regex tested with <where_statement>
    where_statement: Regex for selecting request event.

  Yields:
    Formated string of the selected request event.

  Example:
    Lists all request with timing:
      ... requests --output-format "{timing} {url}"

    Lists  HTTP/HTTPS requests that have used the cache:
      ... requests --where "{protocol} {from_disk_cache}" "https?\S* True"
  """
  if where_statement:
    where_statement = re.compile(where_statement)
  loading_trace = LoadingTrace.FromJsonFile(loading_trace_path)
  for request_event in loading_trace.request_track.GetEvents():
    request_event_json = request_event.ToJsonDict()
    if where_statement != None:
      where_in = where_format.format(**request_event_json)
      if not where_statement.match(where_in):
        continue
    yield output_format.format(**request_event_json)


def _PruneMain(args):
  """`loading_trace_analyzer.py requests` Command line tool entry point.

  Args:
    args: Command line parsed arguments.

  Example:
    Keep only blink.net trace event category:
      ... prune -t "blink.net"

    Keep only requestStart trace events:
      ... prune -t "requestStart"

    Keep only requestStart trace events of the blink.user_timing category:
      ... prune -t "blink.user_timing:requestStart"

    Keep only all blink trace event categories:
      ... prune -t "^blink\.*"

    Keep only requests' url member:
      ... prune -r "^url$"

    Keep only requests' url and document_url members:
      ... prune -r "^./url$"

    Keep only requests' url, document_url and initiator members:
      ... prune -r "^./url$" "initiator"
  """
  trace_json = json.load(args.loading_trace)

  # Filter trace events.
  regexes = [re.compile(f) for f in args.trace_filters]
  events = []
  for event in trace_json['tracing_track']['events']:
    prune = True
    for cat in event['cat'].split(','):
      event_name = cat + ':' + event['name']
      for regex in regexes:
        if regex.search(event_name):
          prune = False
          break
      if not prune:
        events.append(event)
        break
  trace_json['tracing_track']['events'] = events

  # Filter members of requests.
  regexes = [re.compile(f) for f in args.request_member_filter]
  for request in trace_json['request_track']['events']:
    for key in request.keys():
      prune = True
      for regex in regexes:
        if regex.search(key):
          prune = False
          break
      if prune:
        del request[key]

  json.dump(trace_json, args.output, indent=args.indent)
  return 0


def main(command_line_args):
  """Command line tool entry point."""
  args = _ArgumentParser().parse_args(command_line_args)
  if args.subcommand == 'requests':
    try:
      where_format = None
      where_statement = None
      if args.where_statement:
        where_format = args.where_statement[0]
        where_statement = args.where_statement[1]
      for output_line in ListRequests(loading_trace_path=args.loading_trace,
                                      output_format=args.output_format,
                                      where_format=where_format,
                                      where_statement=where_statement):
        args.output.write(output_line + '\n')
      return 0
    except re.error as e:
      sys.stderr.write("Invalid where statement REGEX: {}\n{}\n".format(
          where_statement[1], str(e)))
    return 1
  elif args.subcommand == 'prune':
    return _PruneMain(args)
  assert False


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
