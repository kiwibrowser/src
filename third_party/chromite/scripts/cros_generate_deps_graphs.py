# -*- coding: utf-8 -*-
# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generates pretty dependency graphs for Chrome OS packages."""

from __future__ import print_function

import json
import os
import sys

from chromite.lib import commandline
from chromite.lib import dot_helper


NORMAL_COLOR = 'black'
TARGET_COLOR = 'red'
SEED_COLOR = 'green'
CHILD_COLOR = 'grey'


def GetReverseDependencyClosure(full_name, deps_map):
  """Gets the closure of the reverse dependencies of a node.

  Walks the tree along all the reverse dependency paths to find all the nodes
  that transitively depend on the input node.
  """
  s = set()
  def GetClosure(name):
    s.add(name)
    node = deps_map[name]
    for dep in node['rev_deps']:
      if dep in s:
        continue
      GetClosure(dep)

  GetClosure(full_name)
  return s


def GetOutputBaseName(node, options):
  """Gets the basename of the output file for a node."""
  return '%s_%s-%s.%s' % (node['category'], node['name'], node['version'],
                          options.format)


def AddNodeToSubgraph(subgraph, node, options, color):
  """Gets the dot definition for a node."""
  name = node['full_name']
  href = None
  if options.link:
    filename = GetOutputBaseName(node, options)
    href = '%s%s' % (options.base_url, filename)
  subgraph.AddNode(name, name, color, href)



def GenerateDotGraph(package, deps_map, options):
  """Generates the dot source for the dependency graph leading to a node.

  The output is a list of lines.
  """
  deps = GetReverseDependencyClosure(package, deps_map)
  node = deps_map[package]

  # Keep track of all the emitted nodes so that we don't issue multiple
  # definitions
  emitted = set()

  graph = dot_helper.Graph(package)

  # Add all the children if we want them, all of them in their own subgraph,
  # as a sink. Keep the arcs outside of the subgraph though (it generates
  # better layout).
  children_subgraph = None
  if options.children and node['deps']:
    children_subgraph = graph.AddNewSubgraph('sink')
    for child in node['deps']:
      child_node = deps_map[child]
      AddNodeToSubgraph(children_subgraph, child_node, options, CHILD_COLOR)
      emitted.add(child)
      graph.AddArc(package, child)

  # Add the package in its own subgraph. If we didn't have children, make it
  # a sink
  if children_subgraph:
    rank = 'same'
  else:
    rank = 'sink'
  package_subgraph = graph.AddNewSubgraph(rank)
  AddNodeToSubgraph(package_subgraph, node, options, TARGET_COLOR)
  emitted.add(package)

  # Add all the other nodes, as well as all the arcs.
  for dep in deps:
    dep_node = deps_map[dep]
    if not dep in emitted:
      color = NORMAL_COLOR
      if dep_node['action'] == 'seed':
        color = SEED_COLOR
      AddNodeToSubgraph(graph, dep_node, options, color)
    for j in dep_node['rev_deps']:
      graph.AddArc(j, dep)

  return graph.Gen()


def GenerateImages(data, options):
  """Generate the output images for all the nodes in the input."""
  deps_map = json.loads(data)

  for package in deps_map:
    lines = GenerateDotGraph(package, deps_map, options)

    filename = os.path.join(options.output_dir,
                            GetOutputBaseName(deps_map[package], options))

    save_dot_filename = None
    if options.save_dot:
      save_dot_filename = filename + '.dot'

    dot_helper.GenerateImage(lines, filename, options.format, save_dot_filename)


def GetParser():
  """Return a command line parser."""
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('-f', '--format', default='svg',
                      help='Dot output format (png, svg, etc.).')
  parser.add_argument('-o', '--output-dir', default='.',
                      help='Output directory.')
  parser.add_argument('-c', '--children', action='store_true',
                      help='Also add children.')
  parser.add_argument('-l', '--link', action='store_true',
                      help='Embed links.')
  parser.add_argument('-b', '--base-url', default='',
                      help='Base url for links.')
  parser.add_argument('-s', '--save-dot', action='store_true',
                      help='Save dot files.')
  parser.add_argument('inputs', nargs='*',
                      help='Chromium OS package lists')
  return parser


def main(argv):
  parser = GetParser()
  options = parser.parse_args(argv)
  options.Freeze()

  try:
    os.makedirs(options.output_dir)
  except OSError:
    # The directory already exists.
    pass

  if not options.inputs:
    GenerateImages(sys.stdin.read(), options)
  else:
    for i in options.inputs:
      with open(i) as handle:
        GenerateImages(handle.read(), options)
