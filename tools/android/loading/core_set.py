# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Compute core set for a page.

This script is a collection of utilities for working with core sets.
"""

import argparse
import glob
import json
import logging
import multiprocessing
import os
import sys

import dependency_graph
import loading_trace
import request_dependencies_lens
import resource_sack


def _Progress(x):
  sys.stderr.write(x + '\n')


def _PageCore(prefix, graph_set_names, output):
  """Compute the page core over sets defined by graph_set_names."""
  assert graph_set_names
  graph_sets = []
  sack = resource_sack.GraphSack()
  for name in graph_set_names:
    name_graphs = []
    _Progress('Processing %s' % name)
    for filename in glob.iglob('-'.join([prefix, name, '*.trace'])):
      _Progress('Reading %s' % filename)
      trace = loading_trace.LoadingTrace.FromJsonFile(filename)
      graph = dependency_graph.RequestDependencyGraph(
          trace.request_track.GetEvents(),
          request_dependencies_lens.RequestDependencyLens(trace))
      sack.ConsumeGraph(graph)
      name_graphs.append(graph)
    graph_sets.append(name_graphs)
  core = sack.CoreSet(*graph_sets)
  json.dump({'page_core': [{'label': b.label,
                            'name': b.name,
                            'count': b.num_nodes}
                           for b in core],
             'non_core': [{'label': b.label,
                           'name': b.name,
                           'count': b.num_nodes}
                          for b in sack.bags if b not in core],
             'threshold': sack.CORE_THRESHOLD},
            output, sort_keys=True, indent=2)
  output.write('\n')


def _DoSite(site, graph_sets, input_dir, output_dir):
  """Compute the appropriate page core for a site.

  Used by _Spawn.
  """
  _Progress('Doing %s on %s' % (site, '/'.join(graph_sets)))
  prefix = os.path.join(input_dir, site)
  with file(os.path.join(output_dir,
                         '%s-%s.json' % (site, '.'.join(graph_sets))),
            'w') as output:
    _PageCore(prefix, graph_sets, output)


def _DoSiteRedirect(t):
  """Unpack arguments for map call.

  Note that multiprocessing.Pool.map cannot use a lambda (as it needs to be
  serialized into the executing process).
  """
  _DoSite(*t)


def _Spawn(site_list_file, graph_sets, input_dir, output_dir, workers):
  """Spool site computation out to a multiprocessing pool."""
  with file(site_list_file) as site_file:
    sites = [l.strip() for l in site_file.readlines()]
  _Progress('Using sites:\n %s' % '\n '.join(sites))
  pool = multiprocessing.Pool(workers, maxtasksperchild=1)
  pool.map(_DoSiteRedirect, [(s, graph_sets, input_dir, output_dir)
                             for s in sites])


def _ReadCoreSet(filename):
  data = json.load(open(filename))
  return set(page['name'] for page in data['page_core'])


def _Compare(a_name, b_name, csv):
  """Compare two core sets."""
  a = _ReadCoreSet(a_name)
  b = _ReadCoreSet(b_name)
  result = (resource_sack.GraphSack.CoreSimilarity(a, b),
            '  Equal' if a == b else 'UnEqual',
            'a<=b' if a <= b else 'a!<b',
            'a>=b' if b <= a else 'a!>b')
  if csv:
    print '%s,%s,%s,%s' % result
  else:
    print '%.2f %s %s %s' % result


if __name__ == '__main__':
  logging.basicConfig(level=logging.ERROR)
  parser = argparse.ArgumentParser()
  subparsers = parser.add_subparsers()

  spawn = subparsers.add_parser(
      'spawn', help=('spawn page core set computation from a sites list.\n'
                     'A core set will be computed for each site by '
                     'combining all run indicies from site traces for each '
                     '--set, then computing the page core over the sets. '
                     'Assumes trace file names in form {input-dir}/'
                     '{site}-{set}-{run index}.trace'))
  spawn.add_argument('--sets', required=True,
                     help='sets to combine, comma-separated')
  spawn.add_argument('--sites', required=True, help='file containing sites')
  spawn.add_argument('--workers', default=8, type=int,
                     help=('number of parallel workers. Each worker seems to '
                           'use about 0.5-1G/trace when processing. Total '
                           'memory usage should be kept less than physical '
                           'memory for the job to run in a reasonable time'))
  spawn.add_argument('--input_dir', required=True,
                     help='trace input directory')
  spawn.add_argument('--output_dir', required=True,
                     help=('core set output directory. Each site will have one '
                           'JSON file generated listing the core set as well '
                           'as some metadata like the threshold used'))
  spawn.set_defaults(executor=lambda args:
                     _Spawn(site_list_file=args.sites,
                            graph_sets=args.sets.split(','),
                            input_dir=args.input_dir,
                            output_dir=args.output_dir,
                            workers=args.workers))

  page_core = subparsers.add_parser(
      'page_core',
      help=('compute page core set for a group of files of form '
            '{--prefix}{set}*.trace over each set in --sets'))
  page_core.add_argument('--sets', required=True,
                       help='sets to combine, comma-separated')
  page_core.add_argument('--prefix', required=True,
                           help='trace file prefix')
  page_core.add_argument('--output', required=True,
                           help='JSON output file name')
  page_core.set_defaults(executor=lambda args:
                         _PageCore(args.prefix, args.sets.split(','),
                                   file(args.output, 'w')))

  compare = subparsers.add_parser(
      'compare',
      help=('compare two core sets (as output by spawn, page_core or '
            'all_cores) using Jaccard index. Outputs on stdout'))
  compare.add_argument('--a', required=True, help='the first core set JSON')
  compare.add_argument('--b', required=True, help='the second core set JSON')
  compare.add_argument('--csv', action='store_true', help='output as CSV')
  compare.set_defaults(
      executor=lambda args:
      _Compare(args.a, args.b, args.csv))

  args = parser.parse_args()
  args.executor(args)
