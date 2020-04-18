# Copyright (c) 2013 Mark Dickinson
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

def StronglyConnectedComponents(vertices, edges):
  """Find the strongly connected components of a directed graph.

  Uses a non-recursive version of Gabow's linear-time algorithm [1] to find
  all strongly connected components of a directed graph.

  A "strongly connected component" of a directed graph is a maximal subgraph
  such that any vertex in the subgraph is reachable from any other; any
  directed graph can be decomposed into its strongly connected components.

  Written by Mark Dickinson and licensed under the MIT license [2].

  [1] Harold N. Gabow, "Path-based depth-first search for strong and
      biconnected components," Inf. Process. Lett. 74 (2000) 107--114.
  [2] From code.activestate.com: http://goo.gl/X0z4C

  Args:
    vertices: A list of vertices. Each vertex should be hashable.
    edges: Dictionary that maps each vertex v to a set of the vertices w
           that are linked to v by a directed edge (v, w).

  Returns:
    A list of sets of vertices.
  """
  identified = set()
  stack = []
  index = {}
  boundaries = []

  for v in vertices:
    if v not in index:
      to_do = [('VISIT', v)]
      while to_do:
        operation_type, v = to_do.pop()
        if operation_type == 'VISIT':
          index[v] = len(stack)
          stack.append(v)
          boundaries.append(index[v])
          to_do.append(('POSTVISIT', v))
          to_do.extend([('VISITEDGE', w) for w in edges[v]])
        elif operation_type == 'VISITEDGE':
          if v not in index:
            to_do.append(('VISIT', v))
          elif v not in identified:
            while index[v] < boundaries[-1]:
              boundaries.pop()
        else:
          # operation_type == 'POSTVISIT'
          if boundaries[-1] == index[v]:
            boundaries.pop()
            scc = set(stack[index[v]:])
            del stack[index[v]:]
            identified.update(scc)
            yield scc
