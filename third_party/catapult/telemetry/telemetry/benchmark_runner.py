# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Parses the command line, discovers the appropriate benchmarks, and runs them.

Handles benchmark configuration, but all the logic for
actually running the benchmark is in Benchmark and StoryRunner."""

import argparse
import json
import logging
import optparse
import os
import sys

from telemetry import benchmark
from telemetry import decorators
from telemetry.internal.browser import browser_finder
from telemetry.internal.browser import browser_options
from telemetry.internal.util import binary_manager
from telemetry.internal.util import command_line
from telemetry.internal.util import ps_util
from telemetry.util import matching

from py_utils import discover


DEFAULT_LOG_FORMAT = (
    '(%(levelname)s) %(asctime)s %(module)s.%(funcName)s:%(lineno)d  '
    '%(message)s')

def _SetExpectations(bench, path):
  if path and os.path.exists(path):
    with open(path) as fp:
      bench.AugmentExpectationsWithParser(fp.read())
  return bench.expectations


def _IsBenchmarkEnabled(bench, possible_browser, expectations_file):
  b = bench()
  expectations = _SetExpectations(b, expectations_file)
  return (
      # Test that the current platform is supported.
      any(t.ShouldDisable(possible_browser.platform, possible_browser)
          for t in b.SUPPORTED_PLATFORMS) and
      # Test that expectations say it is enabled.
      not expectations.IsBenchmarkDisabled(possible_browser.platform,
                                           possible_browser))


def _GetStoryTags(b):
  """Finds story tags given a benchmark.

  Args:
    b: a subclass of benchmark.Benchmark
  Returns:
    A list of story tags as strings.
  """
  # Create a options object which hold default values that are expected
  # by Benchmark.CreateStorySet(options) method.
  parser = optparse.OptionParser()
  b.AddBenchmarkCommandLineArgs(parser)
  options, _ = parser.parse_args([])

  # Some benchmarks require special options, such as *.cluster_telemetry.
  # Just ignore them for now.
  try:
    story_set = b().CreateStorySet(options)
  # pylint: disable=broad-except
  except Exception as e:
    logging.warning('Unable to get story tags for %s due to "%s"', b.Name(), e)
    story_set = []

  story_tags = set()
  for s in story_set:
    story_tags.update(s.tags)
  return sorted(story_tags)


def PrintBenchmarkList(
    benchmarks, possible_browser, expectations_file, output_pipe=sys.stdout,
    json_pipe=None):
  """ Print benchmarks that are not filtered in the same order of benchmarks in
  the |benchmarks| list.

  If json_pipe is available, a json file with the following contents will be
  written:
  [
      {
          "name": <string>,
          "description": <string>,
          "enabled": <boolean>,
          "story_tags": [
              <string>,
              ...
          ]
          ...
      },
      ...
  ]

  Args:
    benchmarks: the list of benchmarks to be printed (in the same order of the
      list).
    possible_browser: the possible_browser instance that's used for checking
      which benchmarks are enabled.
    output_pipe: the stream in which benchmarks are printed on.
    json_pipe: if available, also serialize the list into json_pipe.
  """
  if not benchmarks:
    print >> output_pipe, 'No benchmarks found!'
    if json_pipe:
      print >> json_pipe, '[]'
    return

  bad_benchmark = next((b for b in benchmarks
                        if not issubclass(b, benchmark.Benchmark)), None)
  assert bad_benchmark is None, (
      '|benchmarks| param contains non benchmark class: %s' % bad_benchmark)

  all_benchmark_info = []
  for b in benchmarks:
    benchmark_info = {'name': b.Name(), 'description': b.Description()}
    benchmark_info['enabled'] = (
        not possible_browser or
        _IsBenchmarkEnabled(b, possible_browser, expectations_file))
    benchmark_info['story_tags'] = _GetStoryTags(b)
    all_benchmark_info.append(benchmark_info)

  # Align the benchmark names to the longest one.
  format_string = '  %%-%ds %%s' % max(len(b['name'])
                                       for b in all_benchmark_info)

  # Sort the benchmarks by benchmark name.
  all_benchmark_info.sort(key=lambda b: b['name'])

  enabled = [b for b in all_benchmark_info if b['enabled']]
  if enabled:
    print >> output_pipe, 'Available benchmarks %sare:' % (
        'for %s ' % possible_browser.browser_type if possible_browser else '')
    for b in enabled:
      print >> output_pipe, format_string % (b['name'], b['description'])

  disabled = [b for b in all_benchmark_info if not b['enabled']]
  if disabled:
    print >> output_pipe, (
        '\nDisabled benchmarks for %s are (force run with -d):' %
        possible_browser.browser_type)
    for b in disabled:
      print >> output_pipe, format_string % (b['name'], b['description'])

  print >> output_pipe, (
      'Pass --browser to list benchmarks for another browser.\n')

  if json_pipe:
    print >> json_pipe, json.dumps(all_benchmark_info, indent=4,
                                   sort_keys=True, separators=(',', ': ')),


class Help(command_line.OptparseCommand):
  """Display help information about a command"""

  usage = '[command]'

  def __init__(self, commands):
    self._all_commands = commands

  def Run(self, args):
    if len(args.positional_args) == 1:
      commands = _MatchingCommands(args.positional_args[0], self._all_commands)
      if len(commands) == 1:
        command = commands[0]
        parser = command.CreateParser()
        command.AddCommandLineArgs(parser, None)
        parser.print_help()
        return 0

    print >> sys.stderr, ('usage: %s [command] [<options>]' % _ScriptName())
    print >> sys.stderr, 'Available commands are:'
    for command in self._all_commands:
      print >> sys.stderr, '  %-10s %s' % (command.Name(),
                                           command.Description())
    print >> sys.stderr, ('"%s help <command>" to see usage information '
                          'for a specific command.' % _ScriptName())
    return 0


class List(command_line.OptparseCommand):
  """Lists the available benchmarks"""

  usage = '[benchmark_name] [<options>]'

  @classmethod
  def AddCommandLineArgs(cls, parser, _):
    parser.add_option('--json', action='store', dest='json_filename',
                      help='Output the list in JSON')

  @classmethod
  def CreateParser(cls):
    options = browser_options.BrowserFinderOptions()
    parser = options.CreateParser('%%prog %s %s' % (cls.Name(), cls.usage))
    return parser

  @classmethod
  def ProcessCommandLineArgs(cls, parser, args, environment):
    if not args.positional_args:
      args.benchmarks = _Benchmarks(environment)
    elif len(args.positional_args) == 1:
      args.benchmarks = _MatchBenchmarkName(
          args.positional_args[0], environment, exact_matches=False)
    else:
      parser.error('Must provide at most one benchmark name.')
    cls._expectations_file = environment.expectations_file

  def Run(self, args):
    # Set at least log info level for List command.
    # TODO(nedn): remove this once crbug.com/656224 is resolved. The recipe
    # should be change to use verbose logging instead.
    logging.getLogger().setLevel(logging.INFO)
    possible_browser = browser_finder.FindBrowser(args)
    if args.json_filename:
      with open(args.json_filename, 'w') as json_out:
        PrintBenchmarkList(args.benchmarks, possible_browser,
                           self._expectations_file,
                           json_pipe=json_out)
    else:
      PrintBenchmarkList(args.benchmarks, possible_browser,
                         self._expectations_file)
    return 0


class Run(command_line.OptparseCommand):
  """Run one or more benchmarks (default)"""

  usage = 'benchmark_name [<options>]'

  @classmethod
  def CreateParser(cls):
    options = browser_options.BrowserFinderOptions()
    parser = options.CreateParser('%%prog %s %s' % (cls.Name(), cls.usage))
    return parser

  @classmethod
  def AddCommandLineArgs(cls, parser, environment):
    benchmark.AddCommandLineArgs(parser)

    # Allow benchmarks to add their own command line options.
    matching_benchmarks = []
    for arg in sys.argv[1:]:
      matching_benchmarks += _MatchBenchmarkName(arg, environment)

    if matching_benchmarks:
      # TODO(dtu): After move to argparse, add command-line args for all
      # benchmarks to subparser. Using subparsers will avoid duplicate
      # arguments.
      matching_benchmark = matching_benchmarks.pop()
      matching_benchmark.AddCommandLineArgs(parser)
      # The benchmark's options override the defaults!
      matching_benchmark.SetArgumentDefaults(parser)

  @classmethod
  def ProcessCommandLineArgs(cls, parser, args, environment):
    all_benchmarks = _Benchmarks(environment)
    if not args.positional_args:
      possible_browser = (browser_finder.FindBrowser(args)
                          if args.browser_type else None)
      PrintBenchmarkList(
          all_benchmarks, possible_browser, environment.expectations_file)
      sys.exit(-1)

    input_benchmark_name = args.positional_args[0]
    matching_benchmarks = _MatchBenchmarkName(input_benchmark_name, environment)
    if not matching_benchmarks:
      print >> sys.stderr, 'No benchmark named "%s".' % input_benchmark_name
      print >> sys.stderr
      most_likely_matched_benchmarks = matching.GetMostLikelyMatchedObject(
          all_benchmarks, input_benchmark_name, lambda x: x.Name())
      if most_likely_matched_benchmarks:
        print >> sys.stderr, 'Do you mean any of those benchmarks below?'
        PrintBenchmarkList(most_likely_matched_benchmarks, None,
                           environment.expectations_file, sys.stderr)
      sys.exit(-1)

    if len(matching_benchmarks) > 1:
      print >> sys.stderr, (
          'Multiple benchmarks named "%s".' % input_benchmark_name)
      print >> sys.stderr, 'Did you mean one of these?'
      print >> sys.stderr
      PrintBenchmarkList(matching_benchmarks, None,
                         environment.expectations_file, sys.stderr)
      sys.exit(-1)

    benchmark_class = matching_benchmarks.pop()
    if len(args.positional_args) > 1:
      parser.error('Too many arguments.')

    assert issubclass(benchmark_class,
                      benchmark.Benchmark), ('Trying to run a non-Benchmark?!')

    benchmark.ProcessCommandLineArgs(parser, args)
    benchmark_class.ProcessCommandLineArgs(parser, args)

    cls._benchmark = benchmark_class
    cls._expectations_path = environment.expectations_file

  def Run(self, args):
    b = self._benchmark()
    _SetExpectations(b, self._expectations_path)
    return min(255, b.Run(args))


def _ScriptName():
  return os.path.basename(sys.argv[0])


def _MatchingCommands(string, commands):
  return [command for command in commands if command.Name().startswith(string)]


@decorators.Cache
def _Benchmarks(environment):
  benchmarks = []
  for search_dir in environment.benchmark_dirs:
    benchmarks += discover.DiscoverClasses(
        search_dir,
        environment.top_level_dir,
        benchmark.Benchmark,
        index_by_class_name=True).values()
  return benchmarks


def _MatchBenchmarkName(input_benchmark_name, environment, exact_matches=True):

  def _Matches(input_string, search_string):
    if search_string.startswith(input_string):
      return True
    for part in search_string.split('.'):
      if part.startswith(input_string):
        return True
    return False

  # Exact matching.
  if exact_matches:
    # Don't add aliases to search dict, only allow exact matching for them.
    if input_benchmark_name in environment.benchmark_aliases:
      exact_match = environment.benchmark_aliases[input_benchmark_name]
    else:
      exact_match = input_benchmark_name

    for benchmark_class in _Benchmarks(environment):
      if exact_match == benchmark_class.Name():
        return [benchmark_class]
    return []

  # Fuzzy matching.
  return [
      benchmark_class for benchmark_class in _Benchmarks(environment)
      if _Matches(input_benchmark_name, benchmark_class.Name())
  ]


def GetBenchmarkByName(name, environment):
  matched = _MatchBenchmarkName(name, environment, exact_matches=True)
  # With exact_matches, len(matched) is either 0 or 1.
  if len(matched) == 0:
    return None
  return matched[0]


def main(environment, extra_commands=None, **log_config_kwargs):
  # The log level is set in browser_options.
  # Clear the log handlers to ensure we can set up logging properly here.
  logging.getLogger().handlers = []
  log_config_kwargs.pop('level', None)
  log_config_kwargs.setdefault('format', DEFAULT_LOG_FORMAT)
  logging.basicConfig(**log_config_kwargs)

  ps_util.EnableListingStrayProcessesUponExitHook()

  # Get the command name from the command line.
  if len(sys.argv) > 1 and sys.argv[1] == '--help':
    sys.argv[1] = 'help'

  command_name = 'run'
  for arg in sys.argv[1:]:
    if not arg.startswith('-'):
      command_name = arg
      break

  # TODO(eakuefner): Remove this hack after we port to argparse.
  if command_name == 'help' and len(sys.argv) > 2 and sys.argv[2] == 'run':
    command_name = 'run'
    sys.argv[2] = '--help'

  if extra_commands is None:
    extra_commands = []
  all_commands = [Help, List, Run] + extra_commands

  # Validate and interpret the command name.
  commands = _MatchingCommands(command_name, all_commands)
  if len(commands) > 1:
    print >> sys.stderr, (
        '"%s" is not a %s command. Did you mean one of these?' %
        (command_name, _ScriptName()))
    for command in commands:
      print >> sys.stderr, '  %-10s %s' % (command.Name(),
                                           command.Description())
    return 1
  if commands:
    command = commands[0]
  else:
    command = Run

  binary_manager.InitDependencyManager(environment.client_configs)

  # Parse and run the command.
  parser = command.CreateParser()
  command.AddCommandLineArgs(parser, environment)

  # Set the default chrome root variable.
  parser.set_defaults(chrome_root=environment.default_chrome_root)

  if isinstance(parser, argparse.ArgumentParser):
    commandline_args = sys.argv[1:]
    options, args = parser.parse_known_args(commandline_args[1:])
    command.ProcessCommandLineArgs(parser, options, args, environment)
  else:
    options, args = parser.parse_args()
    if commands:
      args = args[1:]
    options.positional_args = args
    command.ProcessCommandLineArgs(parser, options, environment)

  if command == Help:
    command_instance = command(all_commands)
  else:
    command_instance = command()
  if isinstance(command_instance, command_line.OptparseCommand):
    return command_instance.Run(options)
  else:
    return command_instance.Run(options, args)
