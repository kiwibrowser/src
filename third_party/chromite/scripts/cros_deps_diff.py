# -*- coding: utf-8 -*-
# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generates dependency graph diffs.

As an input it takes 2 or more dependency graphs output from cros_extract_deps
and it finds all divergent packages (packages whose versions differ between
some of these dependency graphs) and outputs graphs that trace the divergence
in the dependency trees until common packages are found.
"""

from __future__ import print_function

import json
import os

from chromite.lib import commandline
from chromite.lib import dot_helper

NORMAL_COLOR = 'black'
BASE_COLORS = ['red', 'green', 'blue']


def UnversionedName(dep):
  """Returns the name of the package, omitting the version."""
  return '%s/%s' % (dep['category'], dep['name'])


def GetColor(index):
  """Maps index to a color."""
  try:
    return BASE_COLORS[index]
  except IndexError:
    # Generate a color by splicing the bits to generate high contrast colors
    index -= len(BASE_COLORS) - 1
    chars = [0] * 3
    for bit in xrange(0, 24):
      chars[bit % 3] |= ((index >> bit) & 0x1) << (7-bit/3)
    return "#%02x%02x%02x" % tuple(chars)


def GetReverseDependencyClosure(full_name, deps_map, divergent_set):
  """Gets the closure of the reverse dependencies of a node.

  Walks the tree along all the reverse dependency paths to find all the nodes
  of the divergent set that transitively depend on the input node.
  """
  s = set()
  def GetClosure(name):
    node = deps_map[name]
    if UnversionedName(node) in divergent_set:
      s.add(name)
      for dep in node['rev_deps']:
        if dep in s:
          continue
        GetClosure(dep)

  GetClosure(full_name)
  return s


def GetVersionMap(input_deps):
  """Creates the version map for the input data.

  The version map maps an unversioned package name to its corresponding
  versioned name depending on the input dependency graph.

  For every package, it maps the input data index to the full name (versioned)
  of the package in that input data. E.g.
  map['x11-base/xorg-server'] = {0:'x11-base/xorg-server-1.6.5-r203',
                                 1:'x11-base/xorg-server-1.7.6-r8'}
  """
  version_map = {}
  i = 0
  for deps_map in input_deps:
    for full_name, dep in deps_map.iteritems():
      pkg = UnversionedName(dep)
      entry = version_map.setdefault(pkg, {})
      entry[i] = full_name
    i += 1
  return version_map


def GetDivergentSet(version_map, count):
  """Gets the set of divergent packages.

  Divergent packages are those that have a different version among the input
  dependency graphs (or missing version altogether).
  """
  divergent_set = set()
  for pkg, value in version_map.iteritems():
    if len(value.keys()) != count or len(set(value.values())) > 1:
      # The package doesn't exist for at least one ot the input, or there are
      # more than 2 versions.
      divergent_set.add(pkg)
  return divergent_set


def BuildDependencyGraph(pkg, input_deps, version_map, divergent_set):
  graph = dot_helper.Graph(pkg)

  # A subgraph for the divergent package we're considering. Add all its
  # versions as a sink.
  pkg_subgraph = graph.AddNewSubgraph('sink')

  # The outer packages are those that aren't divergent but depend on a
  # divergent package. Add them in their own subgraph, as sources.
  outer_subgraph = graph.AddNewSubgraph('source')

  emitted = set()
  for i in xrange(0, len(input_deps)):
    try:
      pkg_name = version_map[pkg][i]
    except KeyError:
      continue

    color = GetColor(i)

    if pkg_name not in emitted:
      pkg_subgraph.AddNode(pkg_name, pkg_name, color, None)
      emitted.add(pkg_name)

    # Add one subgraph per version for generally better layout.
    subgraph = graph.AddNewSubgraph()

    nodes = GetReverseDependencyClosure(pkg_name, input_deps[i], divergent_set)
    for node_name in nodes:
      if node_name not in emitted:
        subgraph.AddNode(node_name, node_name, color, None)
        emitted.add(node_name)

      # Add outer packages, and all the arcs.
      for dep in input_deps[i][node_name]['rev_deps']:
        dep_node = input_deps[i][dep]
        if (UnversionedName(dep_node) not in divergent_set
            and dep not in emitted):
          outer_subgraph.AddNode(dep, dep, NORMAL_COLOR, None)
          emitted.add(dep)
        graph.AddArc(dep, node_name)

  return graph


def main(argv):
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('-f', '--format', default='svg',
                      help='Dot output format (png, svg, etc.).')
  parser.add_argument('-o', '--output-dir', default='.',
                      help='Output directory.')
  parser.add_argument('-s', '--save-dot', action='store_true',
                      help='Save dot files.')
  parser.add_argument('inputs', nargs='+')
  options = parser.parse_args(argv)

  input_deps = []
  for i in options.inputs:
    with open(i) as handle:
      input_deps.append(json.loads(handle.read()))

  version_map = GetVersionMap(input_deps)
  divergent_set = GetDivergentSet(version_map, len(input_deps))

  # Get all the output directories
  all_dirs = set(os.path.dirname(pkg) for pkg in divergent_set)

  for i in all_dirs:
    try:
      os.makedirs(os.path.join(options.output_dir, i))
    except OSError:
      # The directory already exists.
      pass

  for pkg in divergent_set:
    filename = os.path.join(options.output_dir, pkg) + '.' + options.format

    save_dot_filename = None
    if options.save_dot:
      save_dot_filename = filename + '.dot'

    graph = BuildDependencyGraph(pkg, input_deps, version_map, divergent_set)
    lines = graph.Gen()
    dot_helper.GenerateImage(lines, filename, options.format, save_dot_filename)
