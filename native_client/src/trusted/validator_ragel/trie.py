# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module for building and compressing tries.

Note that during the compression procses, we merge nodes which have identical
behavior, even if they have different prefixes which lead to them, which
effectively turns the trie into a DAG. However, traversing the trie to leaf
nodes and traversing the compressed trie/DAG will behave identically.
"""

import json
import weakref


class AcceptInfo(object):
  """Information to hold inside an accepting trie node."""

  __slots__ = ('input_rr', 'output_rr')

  def __init__(self, input_rr='', output_rr=''):
    # A string representation of input restricted registers for x86_64.
    # If this is an accepting state, then the state is actually only
    # accepted if the previous instruction left the register in a restricted
    # state. This variable can codify that a specific register must be
    # restricted (e.g. input_rr='r10') or that special registers (rsp, rbp)
    # must not be restricted (e.g. input_rr='any_nonspecial'), or empty
    # in case of x86_32 (e.g. input_rr='').
    # Note a restricted register is one whose most significant 32 bits are 0.
    self.input_rr = input_rr

    # A string representation of output restricted registers for x86_64.
    # If this is an accepting state, then this can codify that the instruction
    # leaves a register in a restricted state (e.g. output_rr='r14'), or that
    # the instruction doesn't produce a restricted register (output_rr='None'),
    # or empty in case of x86_32. (e.g. output_rr='').
    self.output_rr = output_rr

  def __eq__(self, other):
    return (self.input_rr == other.input_rr and
            self.output_rr == other.output_rr)

  def __hash__(self):
    return hash((self.input_rr, self.output_rr))

  def __repr__(self):
    return str((self.input_rr, self.output_rr))


def CombineAcceptInfo(accept_info1, accept_info2):
  """Merge the state held in two dfa nodes.

  When merging two independently built tries during incremental trie buildup,
  we need to merge the state held in the two nodes from the different tries
  (of the same prefix). If either of the dfa nodes are accepting, then the
  merged node should be accepting. This is because in one of the tries, the
  node might be purely intermediary while in the other trie, it might be a final
  accepting node. However, the input_rr and output_rr from the two nodes
  can not be inconsistent. This would mean that the same sequence of bytes
  could be accepted with different pre and postconditions.

  Args:
   accept_info1: state stored in the first trie (of some prefix).
   accept_info2: state stored in the second trie (of the same prefix).
  Returns:
   a state info3: state to store in a combined trie (of the same prefix).
  Raises:
   ValueError: on inconsistent state info pair.
  """

  # input_rr and output_rr are not allowed to differ between the two dfa
  # nodes (unless one of them is empty). If one of them is empty, we pick
  # the other one.
  if accept_info1 is None:
    return accept_info2
  elif accept_info2 is None:
    return accept_info1
  else:
    if accept_info1 != accept_info2:
      raise ValueError('Inconsistent preconditions/postconditions: ',
                       accept_info1, accept_info2)
    return accept_info2


class Node(object):
  """A Trie node."""

  def __init__(self, accept_info=None):
    self.accept_info = accept_info
    self.frozen = False  # Children shouldn't be modified after being frozen.
    self.children = {}  # mapping of keys to other Trie Nodes.


def AddToUncompressedTrie(root, byte_list, accept_info):
  """Update an uncompressed trie.

  Args:
    root: the trie to be updated starts at root.
    byte_list: the sequence of bytes to store information about.
    accept_info: the information to store at the final trie node.
  Returns:
    None
  Raises:
    ValueError: if bytes have already been added with different accept info.
  """
  node = root
  for byte in byte_list:
    if byte not in node.children:
      node.children[byte] = Node()
    node = node.children[byte]
    assert not node.frozen
  if node.accept_info is not None:
    raise ValueError('Same byte sequence added twice',
                     byte_list, accept_info, node.accept_info)
  node.accept_info = accept_info


def GetUnionOfChildKeys(n1, n2):
  """Returns the set of child keys that are present in either node."""
  keys = set(n1.children.iterkeys())
  keys.update(n2.children.iterkeys())
  return keys


class NodeCache(object):
  """A cache of trie nodes.

  This allows us to save memory when two nodes have the same children and when
  they store the same final state, by reusing the same node at both points in
  the trie even though the nodes may have different prefixes/byte sequences
  leading to them.
  """

  def __init__(self):
    self._cache = weakref.WeakValueDictionary()
    self.empty_node = self.GetOrMakeNode({}, None)

  def GetOrMakeNode(self, children, accept_info):
    """Create a new node (and cache it) or return an existing node.

    Note that the invariant is that once a node is added to a cache,
    it is expected that the node is never updated with new children etc.
    If new children need to be added to a potentially cached node, a new node
    needs to be created. This is because many nodes at different points in the
    tree might share this node.

    Args:
      children: map of bytes to child nodes.
      accept_info: information stored in the node.
    Returns:
      node: a node that resides in the cache.
    """
    key = (accept_info, tuple(sorted(children.iteritems())))
    node = self._cache.get(key)
    if node is None:
      node = Node(accept_info)
      node.children = children
      self._cache[key] = node
      node.frozen = True
    return node

  def Merge(self, node1, node2):
    """Merge in and compress a trie that was independently built.

    From the leaves up, we combine nodes of the same prefix in the two tries
    by combining the children, and combining the state information stored in
    each node.

    Args:
      node1: a compressed trie where all the nodes reside inside the node cache.
      node2: an independently built (potentially uncached) trie to be merged in.
    Returns:
      node: a combined compressed trie that resides fully in the cache.
    """
    if node2 == self.empty_node:
      return node1

    children = {}
    keys = GetUnionOfChildKeys(node1, node2)
    for key in keys:
      c1 = node1.children.get(key, self.empty_node)
      c2 = node2.children.get(key, self.empty_node)
      children[key] = self.Merge(c1, c2)
      assert children[key].frozen
    return self.GetOrMakeNode(children, CombineAcceptInfo(node1.accept_info,
                                                          node2.accept_info))


def GetAllAcceptSequences(root):
  """Return all the byte sequences that are accepted by a trie.

  Args:
    root: the root of the trie to generate accepting sequences for.
  Returns:
    sequences: a list of tuples containing information about accepts.
    Each tuple is a pair of AcceptInfo and a list of the bytes that is accepted.
  """
  accept_sequences = []
  def AddAcceptSequences(node, context):
    if node.accept_info is None and not node.children:
      raise ValueError('Node has no children but is not accepting', context)
    if node.accept_info is not None:
      accept_sequences.append((node.accept_info, context))
    for key, value in sorted(node.children.iteritems()):
      AddAcceptSequences(value, context + [key])
  AddAcceptSequences(root, [])
  return accept_sequences


def GetAllUniqueNodes(root):
  """Return list of unique nodes in trie."""
  node_list = []
  def AddNodes(node, node_set):
    """Add unique nodes to node_list."""
    if node not in node_set:
      node_list.append(node)
      node_set.add(node)
      for _, child in sorted(node.children.iteritems()):
        AddNodes(child, node_set)
  AddNodes(root, set())
  return node_list


def TrieToDict(root):
  """Return a representation of the trie suitable for a json dump.

  Flattens the nodes of the trie and assigns ids to them.
  Dumps a map of edges, as well as AcceptInfo associated with the node.

  Args:
    root: the root of the trie to dump.
  Returns:
    A dictionary representation of the trie (suitable for a json dump
    and suitable for consumption by DictToTrie).
  """
  node_list = GetAllUniqueNodes(root)
  # We stringify the IDs because JSON requires dict keys to be strings.
  # We save space by not having an explicit node structure. Nodes which
  # have input_rr/output_rr specified are implicitly the accepting nodes.
  node_to_id = dict((node, str(index)) for index, node in enumerate(node_list))
  return {'start': node_to_id[root],
          'map': dict((node_to_id[node],
                       dict((key, node_to_id[dest])
                            for key, dest in node.children.iteritems()))
                      for node in node_list),
          'input_rr': dict((node_to_id[node], node.accept_info.input_rr)
                           for node in node_list if node.accept_info),
          'output_rr': dict((node_to_id[node], node.accept_info.output_rr)
                            for node in node_list if node.accept_info)}


def WriteToFile(output_filename, root):
  """Write the trie as a json dump."""
  fh = open(output_filename, 'w')
  json.dump(TrieToDict(root), fh, sort_keys=True)
  fh.close()


def TrieFromDict(trie_data, node_cache):
  """Return a reconstructed trie from a dictionary representation.

  Reconstructs a trie from a dictionary that was generated by
  TrieToDict.

  Args:
    trie_data: The dictionary representation.
    node_cache: The NodeCache instance to use for making new nodes.
  Returns:
    A compressed/cached trie.
  """
  # Note that we use a local cache indexed off ids instead of the shared
  # node_cache because node_cache may be shared across multiple tries
  # (which share IDs).
  id_cache = {}
  def MakeNode(node_id):
    if node_id in id_cache:
      return id_cache[node_id]
    children = dict(
        (key, MakeNode(child_id))
        for key, child_id in trie_data['map'][node_id].iteritems())
    accept_info = None
    input_rr = trie_data['input_rr'].get(node_id, None)
    output_rr = trie_data['output_rr'].get(node_id, None)
    if (input_rr is None) != (output_rr is None):
      raise ValueError('Node has inconsistent input_rr/output_rr',
                       node_id, input_rr, output_rr)
    if input_rr is not None:
      accept_info = AcceptInfo(input_rr=input_rr, output_rr=output_rr)
    node = node_cache.GetOrMakeNode(children, accept_info)
    id_cache[node_id] = node
    return node

  return MakeNode(trie_data['start'])


def ReadFromFile(filename, node_cache):
  """Generate a compressed/cached trie from a json dump."""
  fh = open(filename, 'r')
  trie_data = json.load(fh)
  fh.close()
  return TrieFromDict(trie_data, node_cache)


def DiffTrieFiles(file_a, file_b):
  """Diff two trie files.

  Args:
    file_a: File path of first trie.
    file_b: File path of second trie.
  Returns:
    iterator of difference tuples (bytes, state_from_trie_a, state_from_trie_b)
  """
  cache = NodeCache()
  trie_a = ReadFromFile(file_a, cache)
  trie_b = ReadFromFile(file_b, cache)
  return DiffTries(trie_a, trie_b, cache.empty_node, ())


def DiffTries(node1, node2, empty_node, context):
  """Diff two tries.

  Args:
    node1: Node in first trie.
    node2: Node in second trie.
    empty_node: The empty node (for use when one trie doesn't have the prefix).
    context: The prefix of bytes traversed so far.
  Yields:
    difference tuples (bytes, state_from_trie_a, state_from_trie_b)
  """
  if node1 == node2:
    return

  if node1.accept_info != node2.accept_info:
    yield (context, node1.accept_info, node2.accept_info)

  keys = GetUnionOfChildKeys(node1, node2)
  for key in sorted(keys):
    for diff in DiffTries(node1.children.get(key, empty_node),
                          node2.children.get(key, empty_node),
                          empty_node,
                          context + (key,)):
      yield diff
