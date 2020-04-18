# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Visualize a loading_graph_view.LoadingGraphView.

When executed as a script, takes a loading trace and generates a png of the
loading graph."""

import activity_lens
import request_track


class LoadingGraphViewVisualization(object):
  """Manipulate visual representations of a request graph.

  Currently only DOT output is supported.
  """
  _LONG_EDGE_THRESHOLD_MS = 2000  # Time in milliseconds.

  _CONTENT_KIND_TO_COLOR = {
      'application':     'blue',      # Scripts.
      'font':            'grey70',
      'image':           'orange',    # This probably catches gifs?
      'video':           'hotpink1',
      'audio':           'hotpink2',
      }

  _CONTENT_TYPE_TO_COLOR = {
      'html':            'red',
      'css':             'green',
      'script':          'blue',
      'javascript':      'blue',
      'json':            'purple',
      'gif':             'grey',
      'image':           'orange',
      'jpeg':            'orange',
      'ping':            'cyan',  # Empty response
      'redirect':        'forestgreen',
      'png':             'orange',
      'plain':           'brown3',
      'octet-stream':    'brown3',
      'other':           'white',
      }

  _EDGE_REASON_TO_COLOR = {
    'redirect': 'black',
    'parser': 'red',
    'script': 'blue',
    'script_inferred': 'purple',
  }

  _ACTIVITY_TYPE_LABEL = (
      ('idle', 'I'), ('unrelated_work', 'W'), ('script', 'S'),
      ('parsing', 'P'), ('other_url', 'O'), ('unknown_url', 'U'))

  def __init__(self, graph_view):
    """Initialize.

    Args:
      graph_view: (loading_graph_view.LoadingGraphView) the graph to visualize.
    """
    self._graph_view = graph_view
    self._global_start = None

  def OutputDot(self, output):
    """Output DOT (graphviz) representation.

    Args:
      output: a file-like output stream to receive the dot file.
    """
    nodes = self._graph_view.deps_graph.graph.Nodes()
    self._global_start = min(n.request.start_msec for n in nodes)
    g = self._graph_view.deps_graph.graph

    output.write("""digraph dependencies {
    rankdir = LR;
    """)

    isolated_nodes = [
        n for n in nodes if (
            len(g.InEdges(n)) == 0 and len(g.OutEdges(n)) == 0)]
    if isolated_nodes:
      output.write("""subgraph cluster_isolated {
                        color=black;
                        label="Isolated Nodes";
                   """)
      for n in isolated_nodes:
        output.write(self._DotNode(n))
      output.write('}\n')

    output.write("""subgraph cluster_nodes {
                      color=invis;
                 """)
    for n in nodes:
      if n in isolated_nodes:
        continue
      output.write(self._DotNode(n))

    edges = g.Edges()
    for edge in edges:
      output.write(self._DotEdge(edge))

    output.write('}\n')
    output.write('}\n')

  def _ContentTypeToColor(self, content_type):
    if not content_type:
      type_str = 'other'
    elif '/' in content_type:
      kind, type_str = content_type.split('/', 1)
      if kind in self._CONTENT_KIND_TO_COLOR:
        return self._CONTENT_KIND_TO_COLOR[kind]
    else:
      type_str = content_type
    return self._CONTENT_TYPE_TO_COLOR[type_str]

  def _DotNode(self, node):
    """Returns a graphviz node description for a given node.

    Args:
      node: (RequestNode)

    Returns:
      A string describing the resource in graphviz format.
      The resource is color-coded according to its content type, and its shape
      is oval if its max-age is less than 300s (or if it's not cacheable).
    """
    color = self._ContentTypeToColor(node.request.GetContentType())
    request = node.request
    max_age = request.MaxAge()
    shape = 'polygon' if max_age > 300 else 'oval'
    styles = ['filled']
    if node.is_ad or node.is_tracking:
      styles += ['bold', 'diagonals']
    return ('"%s" [label = "%s\\n%.2f->%.2f (%.2f)"; style = "%s"; '
            'fillcolor = %s; shape = %s];\n'
            % (request.request_id, request_track.ShortName(request.url),
               request.start_msec - self._global_start,
               request.end_msec - self._global_start,
               request.end_msec - request.start_msec,
               ','.join(styles), color, shape))

  def _DotEdge(self, edge):
    """Returns a graphviz edge description for a given edge.

    Args:
      edge: (Edge)

    Returns:
      A string encoding the graphviz representation of the edge.
    """
    style = {'color': 'orange'}
    label = '%.02f' % edge.cost
    if edge.is_timing:
      style['style'] = 'dashed'
    style['color'] = self._EDGE_REASON_TO_COLOR[edge.reason]
    if edge.cost > self._LONG_EDGE_THRESHOLD_MS:
      style['penwidth'] = '5'
      style['weight'] = '2'
    style_str = '; '.join('%s=%s' % (k, v) for (k, v) in style.items())

    label = '%.02f' % edge.cost
    if edge.activity:
      separator = ' - '
      for activity_type, activity_label in self._ACTIVITY_TYPE_LABEL:
        label += '%s%s:%.02f ' % (
            separator, activity_label, edge.activity[activity_type])
        separator = ' '
    arrow = '[%s; label="%s"]' % (style_str, label)
    from_request_id = edge.from_node.request.request_id
    to_request_id = edge.to_node.request.request_id
    return '"%s" -> "%s" %s;\n' % (from_request_id, to_request_id, arrow)

def main(trace_file):
  import subprocess

  import loading_graph_view
  import loading_trace
  import request_dependencies_lens

  trace = loading_trace.LoadingTrace.FromJsonFile(trace_file)
  dependencies_lens = request_dependencies_lens.RequestDependencyLens(trace)
  activity = activity_lens.ActivityLens(trace)
  graph_view = loading_graph_view.LoadingGraphView(trace, dependencies_lens,
                                                   activity=activity)
  visualization = LoadingGraphViewVisualization(graph_view)

  dotfile = trace_file + '.dot'
  pngfile = trace_file + '.png'
  with file(dotfile, 'w') as output:
    visualization.OutputDot(output)
  subprocess.check_call(['dot', '-Tpng', dotfile, '-o', pngfile])


if __name__ == '__main__':
  import sys
  main(sys.argv[1])
