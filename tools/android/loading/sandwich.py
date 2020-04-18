#! /usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Instructs Chrome to load series of web pages and reports results.

When running Chrome is sandwiched between preprocessed disk caches and
WepPageReplay serving all connections.
"""

import argparse
import csv
import json
import logging
import os
import re
import sys
from urlparse import urlparse
import yaml

_SRC_DIR = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..', '..'))

sys.path.append(os.path.join(_SRC_DIR, 'third_party', 'catapult', 'devil'))
from devil.android import device_utils

sys.path.append(os.path.join(_SRC_DIR, 'build', 'android'))
from pylib import constants
import devil_chromium

import csv_util
import device_setup
import options
import sandwich_prefetch
import sandwich_swr
import sandwich_utils
import task_manager


# Use options layer to access constants.
OPTIONS = options.OPTIONS

_SPEED_INDEX_MEASUREMENT = 'speed-index'
_MEMORY_MEASUREMENT = 'memory'
_TTFMP_MEASUREMENT = 'ttfmp'
_CORPUS_DIR = 'sandwich_corpuses'
_SANDWICH_SETUP_FILENAME = 'sandwich_setup.yaml'

_MAIN_TRANSFORMER_LIST_NAME = 'no-network-emulation'


def ReadUrlsFromCorpus(corpus_path):
  """Retrieves the list of URLs associated with the corpus name."""
  try:
    # Attempt to read by regular file name.
    json_file_name = corpus_path
    with open(json_file_name) as f:
      json_data = json.load(f)
  except IOError:
    # Extra sugar: attempt to load from _CORPUS_DIR.
    json_file_name = os.path.join(
        os.path.dirname(__file__), _CORPUS_DIR, corpus_path)
    with open(json_file_name) as f:
      json_data = json.load(f)

  key = 'urls'
  if json_data and key in json_data:
    url_list = json_data[key]
    if isinstance(url_list, list) and len(url_list) > 0:
      return [str(u) for u in url_list]
  raise Exception(
      'File {} does not define a list named "urls"'.format(json_file_name))


def _GenerateUrlDirectoryMap(urls):
  domain_times_encountered_per_domain = {}
  url_directories = {}
  for url in urls:
    domain = '.'.join(urlparse(url).netloc.split('.')[-2:])
    domain_times_encountered = domain_times_encountered_per_domain.get(
        domain, 0)
    output_subdirectory = '{}.{}'.format(domain, domain_times_encountered)
    domain_times_encountered_per_domain[domain] = domain_times_encountered + 1
    url_directories[output_subdirectory] = url
  return url_directories


def _ArgumentParser():
  """Build a command line argument's parser."""
  # Command line parser when dealing with _SetupBenchmarkMain.
  sandwich_setup_parser = argparse.ArgumentParser(add_help=False)
  sandwich_setup_parser.add_argument('--android', default=None, type=str,
      dest='android_device_serial', help='Android device\'s serial to use.')
  sandwich_setup_parser.add_argument('-c', '--corpus', required=True,
      help='Path to a JSON file with a corpus such as in %s/.' % _CORPUS_DIR)
  sandwich_setup_parser.add_argument('-m', '--measure', default=[], nargs='+',
      choices=[_SPEED_INDEX_MEASUREMENT,
               _MEMORY_MEASUREMENT,
               _TTFMP_MEASUREMENT],
      dest='optional_measures', help='Enable optional measurements.')
  sandwich_setup_parser.add_argument('-o', '--output', type=str, required=True,
      help='Path of the output directory to setup.')
  sandwich_setup_parser.add_argument('-r', '--url-repeat', default=1, type=int,
      help='How many times to repeat the urls.')

  # Plumbing parser to configure OPTIONS.
  plumbing_parser = OPTIONS.GetParentParser('plumbing options')

  # Main parser
  parser = argparse.ArgumentParser(parents=[plumbing_parser],
      fromfile_prefix_chars=task_manager.FROMFILE_PREFIX_CHARS)
  subparsers = parser.add_subparsers(dest='subcommand', help='subcommand line')

  # Setup NoState-Prefetch benchmarks subcommand.
  subparsers.add_parser('setup-prefetch', parents=[sandwich_setup_parser],
      help='Setup all NoState-Prefetch benchmarks.')

  # Setup Stale-While-Revalidate benchmarks subcommand.
  swr_setup_parser = subparsers.add_parser('setup-swr',
      parents=[sandwich_setup_parser],
      help='Setup all Stale-While-Revalidate benchmarks.')
  swr_setup_parser.add_argument('-d', '--domains-csv',
      type=argparse.FileType('r'), required=True,
      help='Path of the CSV containing the pattern of domains in a '
           '`domain-patterns` column and a `usage` column in percent in how '
           'likely they are in a page load.')

  # Run benchmarks subcommand (used in _RunBenchmarkMain).
  subparsers.add_parser('run', parents=[task_manager.CommandLineParser()],
      help='Run benchmarks steps using the task manager infrastructure.')

  # Collect subcommand.
  collect_csv_parser = subparsers.add_parser('collect-csv',
      help='Collects all CSVs from Sandwich output directory into a single '
           'CSV.')
  collect_csv_parser.add_argument('output_dir', type=str,
                                  help='Path to the run output directory.')
  collect_csv_parser.add_argument('output_csv', type=argparse.FileType('w'),
                                  help='Path to the output CSV.')

  return parser


def _SetupNoStatePrefetchBenchmark(args):
  del args # unused.
  return {
    'network_conditions': ['Regular4G', 'Regular3G', 'Regular2G'],
    'subresource_discoverers': [
        e for e in sandwich_prefetch.SUBRESOURCE_DISCOVERERS
            if e != sandwich_prefetch.Discoverer.FullCache]
  }


def _GenerateNoStatePrefetchBenchmarkTasks(
    common_builder, main_transformer, benchmark_setup):
  builder = sandwich_prefetch.PrefetchBenchmarkBuilder(common_builder)
  builder.PopulateLoadBenchmark(sandwich_prefetch.Discoverer.EmptyCache,
                                _MAIN_TRANSFORMER_LIST_NAME,
                                transformer_list=[main_transformer])
  builder.PopulateLoadBenchmark(sandwich_prefetch.Discoverer.FullCache,
                                _MAIN_TRANSFORMER_LIST_NAME,
                                transformer_list=[main_transformer])
  for network_condition in benchmark_setup['network_conditions']:
    transformer_list_name = network_condition.lower()
    network_transformer = \
        sandwich_utils.NetworkSimulationTransformer(network_condition)
    transformer_list = [main_transformer, network_transformer]
    for subresource_discoverer in benchmark_setup['subresource_discoverers']:
      builder.PopulateLoadBenchmark(
          subresource_discoverer, transformer_list_name, transformer_list)


def _SetupStaleWhileRevalidateBenchmark(args):
  domain_regexes = []
  for row in csv.DictReader(args.domains_csv):
    domain_patterns = json.loads('[{}]'.format(row['domain-patterns']))
    for domain_pattern in domain_patterns:
      domain_pattern_escaped = r'(\.|^){}$'.format(re.escape(domain_pattern))
      domain_regexes.append({
          'usage': float(row['usage']),
          'domain_regex': domain_pattern_escaped.replace(r'\?', r'\w*')})
  return {
    'domain_regexes': domain_regexes,
    'network_conditions': ['Regular3G', 'Regular2G'],
    'usage_thresholds': [1, 3, 5, 10]
  }


def _GenerateStaleWhileRevalidateBenchmarkTasks(
    common_builder, main_transformer, benchmark_setup):
  # Compile domain regexes.
  domain_regexes = []
  for e in benchmark_setup['domain_regexes']:
     domain_regexes.append({
        'usage': e['usage'],
        'domain_regex': re.compile(e['domain_regex'])})

  # Build tasks.
  builder = sandwich_swr.StaleWhileRevalidateBenchmarkBuilder(common_builder)
  for network_condition in benchmark_setup['network_conditions']:
    transformer_list_name = network_condition.lower()
    network_transformer = \
        sandwich_utils.NetworkSimulationTransformer(network_condition)
    transformer_list = [main_transformer, network_transformer]
    builder.PopulateBenchmark(
        'no-swr', [], transformer_list_name, transformer_list)
    for usage_threshold in benchmark_setup['usage_thresholds']:
      benchmark_name = 'threshold{}'.format(usage_threshold)
      selected_domain_regexes = [e['domain_regex'] for e in domain_regexes
          if e['usage'] > usage_threshold]
      builder.PopulateBenchmark(
          benchmark_name, selected_domain_regexes,
          transformer_list_name, transformer_list)


_TASK_GENERATORS = {
  'prefetch': _GenerateNoStatePrefetchBenchmarkTasks,
  'swr': _GenerateStaleWhileRevalidateBenchmarkTasks
}


def _SetupBenchmarkMain(args, benchmark_type, benchmark_specific_handler):
  assert benchmark_type in _TASK_GENERATORS
  urls = ReadUrlsFromCorpus(args.corpus)
  setup = {
    'benchmark_type': benchmark_type,
    'benchmark_setup': benchmark_specific_handler(args),
    'sandwich_runner': {
      'record_video': _SPEED_INDEX_MEASUREMENT in args.optional_measures,
      'record_memory_dumps': _MEMORY_MEASUREMENT in args.optional_measures,
      'record_first_meaningful_paint': (
          _TTFMP_MEASUREMENT in args.optional_measures),
      'repeat': args.url_repeat,
      'android_device_serial': args.android_device_serial
    },
    'urls': _GenerateUrlDirectoryMap(urls)
  }
  if not os.path.isdir(args.output):
    os.makedirs(args.output)
  setup_path = os.path.join(args.output, _SANDWICH_SETUP_FILENAME)
  with open(setup_path, 'w') as file_output:
    yaml.dump(setup, file_output, default_flow_style=False)


def _RunBenchmarkMain(args):
  setup_path = os.path.join(args.output, _SANDWICH_SETUP_FILENAME)
  with open(setup_path) as file_input:
    setup = yaml.load(file_input)
  android_device = None
  if setup['sandwich_runner']['android_device_serial']:
    android_device = device_setup.GetDeviceFromSerial(
        setup['sandwich_runner']['android_device_serial'])
  task_generator = _TASK_GENERATORS[setup['benchmark_type']]

  def MainTransformer(runner):
    runner.record_video = setup['sandwich_runner']['record_video']
    runner.record_memory_dumps = setup['sandwich_runner']['record_memory_dumps']
    runner.record_first_meaningful_paint = (
        setup['sandwich_runner']['record_first_meaningful_paint'])
    runner.repeat = setup['sandwich_runner']['repeat']

  default_final_tasks = []
  for output_subdirectory, url in setup['urls'].iteritems():
    common_builder = sandwich_utils.SandwichCommonBuilder(
        android_device=android_device,
        url=url,
        output_directory=args.output,
        output_subdirectory=output_subdirectory)
    common_builder.PopulateWprRecordingTask()
    task_generator(common_builder, MainTransformer, setup['benchmark_setup'])
    default_final_tasks.extend(common_builder.default_final_tasks)
  return task_manager.ExecuteWithCommandLine(args, default_final_tasks)


def main(command_line_args):
  logging.basicConfig(level=logging.INFO)
  devil_chromium.Initialize()

  args = _ArgumentParser().parse_args(command_line_args)
  OPTIONS.SetParsedArgs(args)

  if args.subcommand == 'setup-prefetch':
    return _SetupBenchmarkMain(
        args, 'prefetch', _SetupNoStatePrefetchBenchmark)
  if args.subcommand == 'setup-swr':
    return _SetupBenchmarkMain(
        args, 'swr', _SetupStaleWhileRevalidateBenchmark)
  if args.subcommand == 'run':
    return _RunBenchmarkMain(args)
  if args.subcommand == 'collect-csv':
    with args.output_csv as output_file:
      if not csv_util.CollectCSVsFromDirectory(args.output_dir, output_file):
        return 1
    return 0
  assert False


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
