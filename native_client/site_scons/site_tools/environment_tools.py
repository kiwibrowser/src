#!/usr/bin/python2.4
# Copyright 2009 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Set up tools for environments for for software construction toolkit.

This module is a SCons tool which should be include in all environments.  It
will automatically be included by the component_setup tool.
"""


import os
import SCons


#------------------------------------------------------------------------------


def FilterOut(self, **kw):
  """Removes values from existing construction variables in an Environment.

  The values to remove should be a list.  For example:

  self.FilterOut(CPPDEFINES=['REMOVE_ME', 'ME_TOO'])

  Args:
    self: Environment to alter.
    kw: (Any other named arguments are values to remove).
  """

  kw = SCons.Environment.copy_non_reserved_keywords(kw)

  for key, val in kw.items():
    if key in self:
      # Filter out the specified values without modifying the original list.
      # This helps isolate us if a list is accidently shared
      # NOTE if self[key] is a UserList, this changes the type into a plain
      # list.  This is OK because SCons also does this in semi_deepcopy
      self[key] = [item for item in self[key] if item not in val]

    # TODO: SCons.Environment.Append() has much more logic to deal with various
    # types of values.  We should handle all those cases in here too.  (If
    # variable is a dict, etc.)

#------------------------------------------------------------------------------


def ApplySConscript(self, sconscript_file):
  """Applies a SConscript to the current environment.

  Args:
    self: Environment to modify.
    sconscript_file: Name of SConscript file to apply.

  Returns:
    The return value from the call to SConscript().

  ApplySConscript() should be used when an existing SConscript which sets up an
  environment gets too large, or when there is common setup between multiple
  environments which can't be reduced into a parent environment which the
  multiple child environments Clone() from.  The latter case is necessary
  because env.Clone() only enables single inheritance for environments.

  ApplySConscript() is NOT intended to replace the Tool() method.  If you need
  to add methods or builders to one or more environments, do that as a tool
  (and write unit tests for them).

  ApplySConscript() is equivalent to the following SCons call:
      SConscript(sconscript_file, exports={'env':self})

  The called SConscript should import the 'env' variable to get access to the
  calling environment:
      Import('env')

  Changes made to env in the called SConscript will be applied to the
  environment calling ApplySConscript() - that is, env in the called SConscript
  is a reference to the calling environment.

  If you need to export multiple variables to the called SConscript, or return
  variables from it, use the existing SConscript() function.
  """
  return self.SConscript(sconscript_file, exports={'env': self})

#------------------------------------------------------------------------------


def BuildSConscript(self, sconscript_file):
  """Builds a SConscript based on the current environment.

  Args:
    self: Environment to clone and pass to the called SConscript.
    sconscript_file: Name of SConscript file to build.  If this is a directory,
        this method will look for sconscript_file+'/build.scons', and if that
        is not found, sconscript_file+'/SConscript'.

  Returns:
    The return value from the call to SConscript().

  BuildSConscript() should be used when an existing SConscript which builds a
  project gets too large, or when a group of SConscripts are logically related
  but should not directly affect each others' environments (for example, a
  library might want to build a number of unit tests which exist in
  subdirectories, but not allow those tests' SConscripts to affect/pollute the
  library's environment.

  BuildSConscript() is NOT intended to replace the Tool() method.  If you need
  to add methods or builders to one or more environments, do that as a tool
  (and write unit tests for them).

  BuildSConscript() is equivalent to the following SCons call:
      SConscript(sconscript_file, exports={'env':self.Clone()})
  or if sconscript_file is a directory:
      SConscript(sconscript_file+'/build.scons', exports={'env':self.Clone()})

  The called SConscript should import the 'env' variable to get access to the
  calling environment:
      Import('env')

  Changes made to env in the called SConscript will NOT be applied to the
  environment calling BuildSConscript() - that is, env in the called SConscript
  is a clone/copy of the calling environment, not a reference to that
  environment.

  If you need to export multiple variables to the called SConscript, or return
  variables from it, use the existing SConscript() function.
  """
  # Need to look for the source node, since by default SCons will look for the
  # entry in the variant_dir, which won't exist (and thus won't be a directory
  # or a file).  This isn't a problem in BuildComponents(), since the variant
  # dir is only set inside its call to SConscript().
  if self.Entry(sconscript_file).srcnode().isdir():
    # Building a subdirectory, so look for build.scons or SConscript
    script_file = sconscript_file + '/build.scons'
    if not self.File(script_file).srcnode().exists():
      script_file = sconscript_file + '/SConscript'
  else:
    script_file = sconscript_file

  self.SConscript(script_file, exports={'env': self.Clone()})

#------------------------------------------------------------------------------


def SubstList2(self, *args):
  """Replacement subst_list designed for flags/parameters, not command lines.

  Args:
    self: Environment context.
    args: One or more strings or lists of strings.

  Returns:
    A flattened, substituted list of strings.

  SCons's built-in subst_list evaluates (substitutes) variables in its
  arguments, and returns a list of lists (one per positional argument).  Since
  it is designed for use in command line expansion, the list items are
  SCons.Subst.CmdStringHolder instances.  These instances can't be passed into
  env.File() (or subsequent calls to env.subst(), either).  The returned
  nested lists also need to be flattened via env.Flatten() before the caller
  can iterate over the contents.

  SubstList2() does a subst_list, flattens the result, then maps the flattened
  list to strings.

  It is better to do:
    for x in env.SubstList2('$MYPARAMS'):
  than to do:
    for x in env.get('MYPARAMS', []):
  and definitely better than:
    for x in env['MYPARAMS']:
  which will throw an exception if MYPARAMS isn't defined.
  """
  return map(str, self.Flatten(self.subst_list(args)))


#------------------------------------------------------------------------------


def RelativePath(self, source, target, sep=os.sep, source_is_file=False):
  """Calculates the relative path from source to target.

  Args:
    self: Environment context.
    source: Source path or node.
    target: Target path or node.
    sep: Path separator to use in returned relative path.
    source_is_file: If true, calculates the relative path from the directory
        containing the source, rather than the source itself.  Note that if
        source is a node, you can pass in source.dir instead, which is shorter.

  Returns:
    The relative path from source to target.
  """
  # Split source and target into list of directories
  source = self.Entry(str(source))
  if source_is_file:
    source = source.dir
  source = source.abspath.split(os.sep)
  target = self.Entry(str(target)).abspath.split(os.sep)

  # Handle source and target identical
  if source == target:
    if source_is_file:
      return source[-1]         # Bare filename
    else:
      return '.'                # Directory pointing to itself

  # TODO: Handle UNC paths and drive letters (fine if they're the same, but if
  # they're different, there IS no relative path)

  # Remove common elements
  while source and target and source[0] == target[0]:
    source.pop(0)
    target.pop(0)
  # Join the remaining elements
  return sep.join(['..'] * len(source) + target)


#------------------------------------------------------------------------------


def generate(env):
  # NOTE: SCons requires the use of this name, which fails gpylint.
  """SCons entry point for this tool."""

  # Add methods to environment
  env.AddMethod(ApplySConscript)
  env.AddMethod(BuildSConscript)
  env.AddMethod(FilterOut)
  env.AddMethod(RelativePath)
  env.AddMethod(SubstList2)
