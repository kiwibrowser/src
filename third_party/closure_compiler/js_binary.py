# Copyright 2017 The Chromium Authors.  All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Used by a js_binary action to compile javascript files.

This script takes in a list of sources and dependencies and compiles them all
together into a single compiled .js file.  The dependencies are ordered in a
post-order, left-to-right traversal order.  If multiple instances of the same
source file are read, only the first is kept. The script can also take in
optional --flags argument which will add custom flags to the compiler.  Any
extern files can also be passed in using the --extern flag.
"""

import argparse
import os
import sys

import compile2


def ParseDepList(dep):
  """Parses a dependency list, returns |sources, deps, externs|."""
  assert os.path.isfile(dep), (dep +
                               ' is not a js_library target')
  with open(dep, 'r') as dep_list:
    lines = dep_list.read().splitlines()
  assert 'deps:' in lines, dep + ' is not formated correctly, missing "deps:"'
  deps_start = lines.index('deps:')
  assert 'externs:' in lines, dep + ' is not formated correctly, missing "externs:"'
  externs_start = lines.index('externs:')

  return (lines[1:deps_start],
          lines[deps_start+1:externs_start],
          lines[externs_start+1:])


def CrawlDepsTree(deps, sources, externs):
  """Parses the dependency tree creating a post-order listing of sources."""
  for dep in deps:
    cur_sources, cur_deps, cur_externs = ParseDepList(dep)

    child_sources, child_externs = CrawlDepsTree(
      cur_deps, cur_sources, cur_externs)

    # Add child dependencies of this node first.
    new_sources = child_sources

    # Add the current node's sources and dedupe.
    new_sources += [s for s in cur_sources if s not in new_sources]

    # Add the original sources, none of which will be dependencies of this node,
    # and dedupe.
    new_sources += [s for s in sources if s not in new_sources]
    sources = new_sources

    externs += [e for e in cur_externs if e not in externs]
  return sources, externs


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('-c', '--compiler', required=True,
                      help='Path to compiler')
  parser.add_argument('-s', '--sources', nargs='*', default=[],
                      help='List of js source files')
  parser.add_argument('-o', '--output', required=True,
                      help='Compile to output')
  parser.add_argument('-d', '--deps', nargs='*', default=[],
                      help='List of js_libarary dependencies')
  parser.add_argument('-b', '--bootstrap',
                      help='A file to include before all others')
  parser.add_argument('-cf', '--config', nargs='*', default=[],
                      help='A list of files to include after bootstrap and '
                      'before all others')
  parser.add_argument('-f', '--flags', nargs='*', default=[],
                      help='A list of custom flags to pass to the compiler. '
                      'Do not include leading dashes')
  parser.add_argument('-e', '--externs', nargs='*', default=[],
                      help='A list of extern files to pass to the compiler')

  args = parser.parse_args()
  sources, externs = CrawlDepsTree(args.deps, args.sources, args.externs)

  compiler_args = ['--%s' % flag for flag in args.flags]
  compiler_args += ['--externs=%s' % e for e in args.externs]
  compiler_args += [
      '--js_output_file',
      args.output,
      '--js',
  ]
  if args.bootstrap:
    compiler_args += [args.bootstrap]
  compiler_args += args.config
  compiler_args += sources

  returncode, errors = compile2.Checker().run_jar(args.compiler, compiler_args)
  if returncode != 0:
    print errors

  return returncode


if __name__ == '__main__':
  sys.exit(main())
