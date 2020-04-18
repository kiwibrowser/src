#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Tool for searching for libraries.


import driver_log
import filetype
import pathtools


class LibraryTypes(object):
  """ An enum indicating the type of library that can be searched for. """
  BITCODE = 1
  NATIVE = 2
  ANY = 3


def FindFirstLinkerScriptInput(inputs):
  for i in xrange(len(inputs)):
    f = inputs[i]
    if IsFlag(f):
      continue
    if filetype.IsLinkerScript(f):
      return (i, f)
  return (None, None)

def ExpandLinkerScripts(inputs, searchdirs, static_only):
  while True:
    # Find a ldscript in the input list
    i, path = FindFirstLinkerScriptInput(inputs)

    # If none, we are done.
    if path is None:
      break

    new_inputs = filetype.ParseLinkerScript(path)
    ExpandLibFlags(new_inputs, searchdirs, static_only,
                   LibraryTypes.ANY)
    inputs = inputs[:i] + new_inputs + inputs[i+1:]
  return inputs

def ExpandLibFlags(inputs, searchdirs, static_only, acceptable_types):
  """ Given an input list, expand -lfoo or -l:foo.so
      into a full filename. Returns the new input list """
  for i in xrange(len(inputs)):
    f = inputs[i]
    if IsFlag(f):
      continue
    if IsLib(f):
      inputs[i] = FindLib(f, searchdirs, static_only, acceptable_types)

def IsFlag(arg):
  return arg.startswith('-') and not IsLib(arg)

def IsLib(arg):
  return arg.startswith('-l')

def FindLib(arg, searchdirs, static_only, acceptable_types):
  """Returns the full pathname for the library input.
     For example, name might be "-lc" or "-lm".
  """
  assert(IsLib(arg))
  assert(searchdirs is not None)
  name = arg[len('-l'):]
  if not name:
    driver_log.Log.Fatal("-l missing library name")
  is_whole_name = (name[0] == ':')

  if static_only:
    extensions = [ 'a' ]
  else:
    extensions = [ 'so', 'a' ]

  if is_whole_name:
    label = name
  else:
    label = arg

  searchnames = []
  if is_whole_name:
    # -l:filename  (search for the filename)
    name = name[1:]
    searchnames.append(name)

    # If the real IRT shim is not found, fall back to the dummy shim
    if name == 'libpnacl_irt_shim.a':
      searchnames.append('libpnacl_irt_shim_dummy.a')
  else:
    # -lfoo
    for ext in extensions:
      searchnames.append('lib' + name + '.' + ext)

  foundpath = FindFile(searchnames, searchdirs, acceptable_types)
  if foundpath:
    return foundpath

  # The driver sometimes injects pthread into the input library list
  # when pthread_private should be used. The following is mostly okay
  # because it's only run when the library isn't found, and only does
  # something if pthread_private is found. The SDK doesn't ship with
  # pthread_private, so in practice it won't be found by real SDK users,
  # so it'll just fall through to the log Fatal.
  if name == 'pthread':
    for ext in extensions:
      searchnames.append('lib' + 'pthread_private' + '.' + ext)
    foundpath = FindFile(searchnames, searchdirs, acceptable_types)
    if foundpath:
      return foundpath

  driver_log.Log.Fatal("Cannot find '%s'", label)

def FindFile(search_names, search_dirs, acceptable_types):
  for curdir in search_dirs:
    for name in search_names:
      path = pathtools.join(curdir, name)
      if pathtools.exists(path):
        if acceptable_types == LibraryTypes.ANY:
          return path
        # Linker scripts aren't classified as Native or Bitcode.
        if filetype.IsLinkerScript(path):
          return path
        if (acceptable_types == LibraryTypes.NATIVE and
            filetype.IsNative(path)):
          return path
        if (acceptable_types == LibraryTypes.BITCODE and
            (filetype.IsLLVMBitcode(path) or
             filetype.IsBitcodeArchive(path))):
          return path
  return None

def ExpandInputs(inputs, searchdirs, static_only, acceptable_types):
  # Expand all -l flags into filenames
  ExpandLibFlags(inputs, searchdirs, static_only, acceptable_types)

  # Expand input files which are linker scripts
  inputs = ExpandLinkerScripts(inputs, searchdirs, static_only)
  return inputs
