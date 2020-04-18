#! /usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import logging
import os
import sys

from processing import (SitesFromDir, WarmGraph, ColdGraph)


def main():
  logging.basicConfig(level=logging.ERROR)
  parser = argparse.ArgumentParser(
      description=('Convert a directory created by ./analyze.py fetch '
                   'to a node cost CSV which compares cold and warm total '
                   'node costs.'))
  parser.add_argument('--datadir', required=True)
  parser.add_argument('--csv', required=True)
  parser.add_argument('--noads', action='store_true')
  args = parser.parse_args()
  sites = SitesFromDir(args.datadir)
  with open(args.csv, 'w') as output:
    output.write('site,cold.total,warm.total,cold.common,warm.common,'
                 'cold.node.count,common.cold.node.count,'
                 'cold.all.edges,warm.all.edges,'
                 'cold.common.edges,warm.common.edges,'
                 'cold.edge.fraction,common.cold.edge.fraction\n')
    for site in sites:
      print site
      warm = WarmGraph(args.datadir, site)
      if args.noads:
        warm.Set(node_filter=warm.FilterAds)
      cold = ColdGraph(args.datadir, site)
      if args.noads:
        cold.Set(node_filter=cold.FilterAds)
      common = [p for p in cold.Intersect(warm.Nodes())]
      common_cold = set([c.Node() for c, w in common])
      common_warm = set([w.Node() for c, w in common])
      output.write(','.join([str(s) for s in [
          site,
          sum((n.NodeCost() for n in cold.Nodes())),
          sum((n.NodeCost() for n in warm.Nodes())),
          sum((c.NodeCost() for c, w in common)),
          sum((w.NodeCost() for c, w in common)),
          sum((1 for n in cold.Nodes())),
          len(common),
          cold.EdgeCosts(), warm.EdgeCosts(),
          cold.EdgeCosts(lambda n: n in common_cold),
          warm.EdgeCosts(lambda n: n in common_warm),
          (cold.EdgeCosts() /
           sum((n.NodeCost() for n in cold.Nodes()))),
          (cold.EdgeCosts(lambda n: n in common_cold) /
           sum((c.NodeCost() for c, w in common)))
          ]]) + '\n')


if __name__ == '__main__':
  main()
