# -*- coding: utf-8 -*-
# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helper functions for building graphs with dot."""

from __future__ import print_function

from chromite.lib import cros_build_lib
from chromite.lib import osutils


class Subgraph(object):
  """A subgraph in dot. Contains nodes, arcs, and other subgraphs."""

  _valid_ranks = set(['source', 'sink', 'same', 'min', 'max', None])

  def __init__(self, rank=None):
    self._rank = rank
    self._nodes = []
    self._subgraphs = []
    self._arcs = set()
    self._rank = None

  def AddNode(self, node_id, name=None, color=None, href=None):
    """Adds a node to the subgraph."""
    tags = {}
    if name:
      tags['label'] = name
    if color:
      tags['color'] = color
      tags['fontcolor'] = color
    if href:
      tags['href'] = href
    self._nodes.append({'id': node_id, 'tags': tags})

  def AddSubgraph(self, subgraph):
    """Adds a subgraph to the subgraph."""
    self._subgraphs.append(subgraph)

  def AddNewSubgraph(self, rank=None):
    """Adds a new subgraph to the subgraph. The new subgraph is returned."""
    subgraph = Subgraph(rank)
    self.AddSubgraph(subgraph)
    return subgraph

  def AddArc(self, node_from, node_to):
    """Adds an arc between two nodes."""
    self._arcs.add((node_from, node_to))

  def _GenNodes(self):
    """Generates the code for all the nodes."""
    lines = []
    for node in self._nodes:
      tags = ['%s="%s"' % (k, v) for (k, v) in node['tags'].iteritems()]
      lines.append('"%s" [%s];' % (node['id'], ', '.join(tags)))
    return lines

  def _GenSubgraphs(self):
    """Generates the code for all the subgraphs contained in this subgraph."""
    lines = []
    for subgraph in self._subgraphs:
      lines += subgraph.Gen()
    return lines

  def _GenArcs(self):
    """Generates the code for all the arcs."""
    lines = []
    for node_from, node_to in self._arcs:
      lines.append('"%s" -> "%s";' % (node_from, node_to))
    return lines

  def _GenInner(self):
    """Generates the code for the inner contents of the subgraph."""
    lines = []
    if self._rank:
      lines.append('rank=%s;' % self._rank)
    lines += self._GenSubgraphs()
    lines += self._GenNodes()
    lines += self._GenArcs()
    return lines

  def Gen(self):
    """Generates the code for the subgraph."""
    return ['subgraph {'] + self._GenInner() + ['}']


class Graph(Subgraph):
  """A top-level graph in dot. It's basically a subgraph with a name."""

  def __init__(self, name):
    Subgraph.__init__(self)
    self._name = name

  def Gen(self):
    """Generates the code for the graph."""
    return ['digraph "%s" {' % self._name,
            'graph [name="%s"];' % self._name] + self._GenInner() + ['}']


def GenerateImage(lines, filename, out_format='svg', save_dot_filename=None):
  """Generates the image by calling dot on the input lines."""
  data = '\n'.join(lines)
  cros_build_lib.RunCommand(['dot', '-T%s' % out_format, '-o', filename],
                            input=data)

  if save_dot_filename:
    osutils.WriteFile(save_dot_filename, data)
