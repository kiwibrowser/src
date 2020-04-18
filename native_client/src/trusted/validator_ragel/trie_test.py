#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import trie


class TrieTest(unittest.TestCase):

  def MakeUncompressedTrie(self):
    uncompressed = trie.Node()
    accept = trie.AcceptInfo(input_rr='%eax', output_rr='%edx')
    trie.AddToUncompressedTrie(uncompressed, ['0', '1', '2'], accept)
    trie.AddToUncompressedTrie(uncompressed, ['0', '1', '2', '3'], accept)
    trie.AddToUncompressedTrie(uncompressed, ['0', '1', '3'], accept)
    trie.AddToUncompressedTrie(uncompressed, ['0', '1', '4'], accept)
    trie.AddToUncompressedTrie(uncompressed, ['0', '1', '5'], accept)
    return uncompressed

  def CheckTrieAccepts(self, accept_sequences):
    accept = trie.AcceptInfo(input_rr='%eax', output_rr='%edx')
    self.assertEquals([(accept, ['0', '1', '2']),
                       (accept, ['0', '1', '2', '3']),
                       (accept, ['0', '1', '3']),
                       (accept, ['0', '1', '4']),
                       (accept, ['0', '1', '5'])],
                      accept_sequences)

  def testTrieAddAndMerge(self):
    uncompressed = self.MakeUncompressedTrie()
    self.CheckTrieAccepts(trie.GetAllAcceptSequences(uncompressed))
    # n0 -0-> n1 -1-> n2 -2-> n3 -3-> n4
    #                  | -3-> n5
    #                  | -4-> n6
    #                  | -5-> n7
    self.assertEquals(8, len(trie.GetAllUniqueNodes(uncompressed)))

    node_cache = trie.NodeCache()
    compressed_trie = node_cache.Merge(node_cache.empty_node, uncompressed)
    self.CheckTrieAccepts(trie.GetAllAcceptSequences(compressed_trie))
    # (n4, n5. n6, n7) can be grouped together from above
    self.assertEquals(5, len(trie.GetAllUniqueNodes(compressed_trie)))

  def testTrieSerializationAndDeserialization(self):
    uncompressed = self.MakeUncompressedTrie()
    node_cache = trie.NodeCache()
    compressed_trie = node_cache.Merge(node_cache.empty_node, uncompressed)
    reconstructed_trie = trie.TrieFromDict(trie.TrieToDict(compressed_trie),
                                           node_cache)
    self.CheckTrieAccepts(trie.GetAllAcceptSequences(reconstructed_trie))
    self.assertEquals(5, len(trie.GetAllUniqueNodes(reconstructed_trie)))

  def testTrieDiff(self):
    trie1 = trie.Node()
    trie2 = trie.Node()
    accept1 = trie.AcceptInfo(input_rr='%eax', output_rr='%edx')
    accept2 = trie.AcceptInfo(input_rr='%eax', output_rr='%ecx')

    trie.AddToUncompressedTrie(trie1, ['0', '1', '2'], accept1)
    trie.AddToUncompressedTrie(trie1, ['0', '1', '3'], accept1)
    trie.AddToUncompressedTrie(trie1, ['0', '1', '4'], accept1)
    trie.AddToUncompressedTrie(trie1, ['0', '1', '5'], accept1)

    trie.AddToUncompressedTrie(trie2, ['0', '1', '2'], accept1)
    trie.AddToUncompressedTrie(trie2, ['0', '1', '3'], accept1)
    trie.AddToUncompressedTrie(trie2, ['0', '1', '4'], accept2)

    node_cache = trie.NodeCache()
    compressed_trie1 = node_cache.Merge(node_cache.empty_node, trie1)
    compressed_trie2 = node_cache.Merge(node_cache.empty_node, trie2)

    diffs = set()
    compressed_diffs = set()

    for diff in trie.DiffTries(trie1, trie2, node_cache.empty_node, ()):
      diffs.add(diff)

    for diff in trie.DiffTries(compressed_trie1, compressed_trie2,
                               node_cache.empty_node, ()):
      compressed_diffs.add(diff)

    self.assertEquals(
        diffs,
        set([(('0', '1', '4'), accept1, accept2),
             (('0', '1', '5'), accept1, None)]))
    self.assertEquals(diffs, compressed_diffs)


if __name__ == '__main__':
  unittest.main()
