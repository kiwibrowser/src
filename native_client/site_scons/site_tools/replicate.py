#!/usr/bin/python2.4
# Copyright 2009 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Replicate tool for SCons."""


import re


def Replicate(env, target, source, **kw):
  """Replicates (copies) source files/directories to the target directory.

  Much like env.Install(), with the following differences:
     * If the source is a directory, recurses through it and calls
       env.Install() on each source file, rather than copying the entire
       directory at once.  This provides more opportunity for hard linking, and
       also makes the destination files/directories all writable.
     * Can take sources which contain env.Glob()-style wildcards.
     * Can take multiple target directories; will copy to all of them.
     * Handles duplicate requests.

  Args:
    env: Environment in which to operate.
    target: Destination(s) for copy.  Must evaluate to a directory via
        env.Dir(), or a list of directories.  If more than one directory is
        passed, the entire source list will be copied to each target
        directory.
    source: Source file(s) to copy.  May be a string, Node, or a list of
        mixed strings or Nodes.  Strings will be passed through env.Glob() to
        evaluate wildcards.  If a source evaluates to a directory, the entire
        directory will be recursively copied.

  From env:
    REPLICATE_REPLACE: A list of pairs of regex search and replacement strings.
        Each full destination path has substitution performed on each pair
        (search_regex, replacement) in order.

        env.Replicate('destdir', ['footxt.txt'], REPLICATE_REPLACE = [
            ('\\.txt', '.bar'), ('est', 'ist')])
        will copy to 'distdir/footxt.bar'

        In the example above, note the use of \\ to escape the '.' character,
        so that it doesn't act like the regexp '.' and match any character.

  Returns:
    A list of the destination nodes from the calls to env.Install().
  """
  replace_list = kw.get('REPLICATE_REPLACE', env.get('REPLICATE_REPLACE', []))

  dest_nodes = []
  for target_entry in env.Flatten(target):
    for source_entry in env.Flatten(source):
      if type(source_entry) == str:
        # Search for matches for each source entry
        source_nodes = env.Glob(source_entry)
      else:
        # Source entry is already a file or directory node; no need to glob it
        source_nodes = [source_entry]
      for s in source_nodes:
        target_name = env.Dir(target_entry).abspath + '/' + s.name
        # We need to use the following incantation rather than s.isdir() in
        # order to handle chained replicates (A -> B -> C). The isdir()
        # function is not properly defined in all of the Node type classes in
        # SCons. This change is particularly crucial if hardlinks are present,
        # in which case using isdir() can cause files to be unintentionally
        # deleted.
        # TODO: Look into fixing the innards of SCons so this isn't needed.
        if str(s.__class__) == 'SCons.Node.FS.Dir':
          # Recursively copy all files in subdir.  Since glob('*') doesn't
          # match dot files, also glob('.*').
          dest_nodes += env.Replicate(
              target_name, [s.abspath + '/*', s.abspath + '/.*'],
              REPLICATE_REPLACE=replace_list)
        else:
          # Apply replacement strings, if any
          for r in replace_list:
            target_name = re.sub(r[0], r[1], target_name)
          target = env.File(target_name)
          if (target.has_builder()
              and hasattr(target.get_builder(), 'name')
              and target.get_builder().name == 'InstallBuilder'
              and target.sources == [s]):
            # Already installed that file, so pass through the destination node
            # TODO: Is there a better way to determine if this is a duplicate
            # call to install?
            dest_nodes += [target]
          else:
            dest_nodes += env.InstallAs(target_name, s)

  # Return list of destination nodes
  return dest_nodes


def generate(env):
  # NOTE: SCons requires the use of this name, which fails gpylint.
  """SCons entry point for this tool."""

  env.AddMethod(Replicate)
