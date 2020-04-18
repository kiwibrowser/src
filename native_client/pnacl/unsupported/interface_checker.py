#!/usr/bin/python
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Process ppapi header files, e.g.
../ppapi/c/ppp_*h and
../ppapi/c/ppb_*h
And check whether they could cause pnacl calling convention problems
"""

import sys
import re

# NOTE: there is an extra white space at the end
#       which distinguishes them from pointer types
#      like struct PP_Var*
BAD_TYPES = ["struct PP_CompletionCallback ",
             "struct PP_Var ",
             "struct PP_Point ",
             "struct PP_FloatPoint ",
             "struct PP_Context3DTrustedState ",
             ]


RE_INTERFACE_STRUCT_START = re.compile(r"^struct +PP[PB]_[_0-9A-Za-z]+ +{$")


def ProcessHeader(filename):
  """extract interface structs from headers
  """
  result = []
  found_struct = 0
  for line in open(filename):
    if found_struct:
      result.append(line)
      if line.startswith("};"):
        found_struct = 0
    else:
      if RE_INTERFACE_STRUCT_START.search(line.strip()):
        result.append(line)
        found_struct = 1
  return result


def StripComments(lines):
  """strip comments and empty lines
  """
  result = []
  found_comment = 0
  for line in lines:
    if not line.strip():
      continue
    if found_comment:
      if line.startswith("   */"):
        found_comment = 0
    else:
      if line.startswith("  //"):
        continue
      elif line.startswith("  /*"):
        found_comment = 1
      else:
        result.append(line)
  return result


def MakeSingleLineProtos(lines):
  """move function prototypes into single lines for easier post processing
  """
  result = []
  # we use the presence of the newline in  result[-1] as an indicator
  # whether we should append to that line or start a new one
  for line in lines:
    if line.endswith(",\n") or line.endswith("(\n"):
      line = line.rstrip()
    if not result or result[-1].endswith("\n"):
      result.append(line)
    else:
      result[-1] = result[-1] + line.lstrip()
  return result


def ExtractInterfaces(lines):
  result = []
  for line in lines:
    if RE_INTERFACE_STRUCT_START.search(line.strip()):
      result.append([])
    result[-1].append(line)
  return result
######################################################################
#
######################################################################

def AnalyzeInterface(lines):
  bad_functions = []
  for line in lines[1:-1]:
    # functions look like:
    # void (*PostMessage)(PP_Instance instance, struct PP_Var message);
    result, rest = line.split("(*", 1)
    name, rest = rest.split(")(", 1)
    args, rest = rest.split(");", 1)
    args = args.split(",")
    # print result, name, repr(args)
    result = result.strip()
    for bad in BAD_TYPES:
      bad = bad.strip()
      if result.startswith(bad):
        print "@PROBLEM: [result %s] in:" % result, line.strip()
        bad_functions.append(name)

    for a in args:
      a = a.strip()
      for bad in BAD_TYPES:
        if a.startswith(bad):
          print "@PROBLEM: [%s] in:" % a, line.strip()
          bad_functions.append(name)
  return bad_functions
######################################################################
#
######################################################################
bad_interfaces = []
bad_functions = []

for filename in sys.argv:
  lines = MakeSingleLineProtos(StripComments(ProcessHeader(filename)))

  # a number of files contain multiple interfacess
  for interface in ExtractInterfaces(lines):
    print
    print
    print filename
    print "".join(interface),
    errors = AnalyzeInterface(interface)
    if len(errors) > 0:
      bad_functions += errors
      # the first line looks like:
      # struct PPB_URLLoader {
      tokens = interface[0].split()
      bad_interfaces.append(tokens[1])

print "\nBAD INTERFACES (%d):" % len(bad_interfaces)

for b in bad_interfaces:
  print b

print
print "BAD FUNCTIONS (%d):" % len(bad_functions)
for b in bad_functions:
  print b
