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
                   'to a CSV.'))
  parser.add_argument('--datadir', required=True)
  parser.add_argument('--csv', required=True)
  parser.add_argument('--noads', action='store_true')
  args = parser.parse_args()
  sites = SitesFromDir(args.datadir)
  with open(args.csv, 'w') as output:
    output.write('site,kind,cost\n')
    for site in sites:
      print site
      warm = WarmGraph(args.datadir, site)
      if args.noads:
        warm.Set(node_filter=warm.FilterAds)
      cold = ColdGraph(args.datadir, site)
      if args.noads:
        cold.Set(node_filter=cold.FilterAds)
      output.write('%s,%s,%s\n' % (site, 'warm', warm.Cost()))
      warm.Set(cache_all=True)
      output.write('%s,%s,%s\n' % (site, 'warm-cache', warm.Cost()))
      output.write('%s,%s,%s\n' % (site, 'cold', cold.Cost()))
      cold.Set(cache_all=True)
      output.write('%s,%s,%s\n' % (site, 'cold-cache', cold.Cost()))


if __name__ == '__main__':
  main()
