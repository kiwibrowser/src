# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
This script takes two warning summary files and reports on the new warnings
and the fixed warnings. The warning summaries are created by
warnings_by_type.py.

A warning is identified by the source file and the warning text, without the
Lines component or any line 'xxx' references within the warning.
All warnings with the same signature are grouped together (duplicates are
assumed to have been removed already).

If a file contains multiple warnings with the same signature then a report
will be generated for each warning when the warning count changes.
"""

import sys
import re

# Some sample warnings:
# sctp_bsd_addr.c(182) : warning C28125: The function
# 'InitializeCriticalSection' must be called from within a try/except block:
#  The requirement might be conditional.
# exception_handler.cc(813) : warning C6387: 'child_thread_handle' could be '0':
#  this does not adhere to the specification for the function 'CloseHandle'. :
# Lines: 773, 774, 775, 776, 777, 784, 802, 804, 809, 813
# unistr.cpp(1823) : warning C28193: 'temporary value' holds a value that must
# be examined.: Lines: 1823, 1824
# Note "line '1428'" in this warning, and 'count went from 3 to 2':
# scheduler.cc(1452) : warning C6246: Local declaration of 'node' hides
# declaration of the same name in outer scope. For additional information, see
# previous declaration at line '1428' of 'scheduler.cc'.: count went from 3 to 2
# Note "line 454" in this warning:
# gurl.cc(449) : warning C28112: A variable (empty_gurl) which is accessed via
# an Interlocked function must always be accessed via an Interlocked function.
# See line 454:  It is not always safe to access a variable which is accessed
# via the Interlocked* family of functions in any other way.


warningsToIgnore = [
  # We assume that memory allocations never fail
  "C6255", # _alloca indicates failure by raising a stack overflow exception.
  "C6308", # 'realloc' might return null pointer, leaking memory
  "C6387", # Param could be '0': this does not adhere to the specification for
  # the function
  # I have yet to see errors caused by passing 'char' to isspace and friends
  "C6330", # 'char' passed as _Param_(1) when 'unsigned char' is required
  # This warning needs to be in clang to make it effective
  "C6262", # Function uses too much stack
  # Triggers on isnan, isinf, and template metaprogramming.
  "C6334", # sizeof operator applied to an expression with an operator might
  # yield unexpected results:
  ]

warningRe = re.compile(r"(.*)\(\d+\) : warning (C\d{4,5}): (.*)")
warningRefLine = re.compile(r"(.*line ')\d+('.*)")
warningRefLine2 = re.compile(r"(.*line )\d+(:.*)")

def RemoveExtraneous(line):
  """
  Remove extraneous data such as the optional 'Lines:' block at the end of some
  warnings, and line ending characters.
  This ensures better matching and makes for less cluttered results.
  """
  linesOffset = line.find(": Lines:")
  if linesOffset >= 0:
    line = line[:linesOffset]
  return line.strip()



def SummarizeWarnings(filename):
  """
  This function reads the file and looks for warning messages. It creates a
  dictionary with the keys being the filename, warning number, and warning text,
  and returns this.
  The warning summary at the end is ignored because it doesn't match the regex
  due to the 'C' being stripped from the warnings, for just this purpose.
  """
  warnings = {}
  for line in open(filename).readlines():
    line = line.replace(r"\chromium\src", r"\analyze_chromium\src")
    line = line.replace(r"\chromium2\src", r"\analyze_chromium\src")
    line = RemoveExtraneous(line)
    match = warningRe.match(line)
    if match:
      file, warningNumber, description = match.groups()
      ignore = False
      if warningNumber in warningsToIgnore:
        ignore = True
      glesTest = "gles2_implementation_unittest"
      if warningNumber == "C6001" and line.count(glesTest) > 0:
        ignore = True  # Many spurious warnings of this form
      if not ignore:
        # See if the description contains line numbers, so that we can
        # remove them.
        matchLine = warningRefLine.match(description)
        if not matchLine:
          matchLine = warningRefLine2.match(description)
        if matchLine:
          # Replace referenced line numbers with #undef so that they don't cause
          # mismatches.
          description = "#undef".join(matchLine.groups())
        # Look for "the readable size is " and "the writable size is " because
        # these are often followed by sizes that vary in uninteresting ways,
        # especially between 32-bit and 64-bit builds.
        readableText = "the readable size is "
        writableText = "the writable size is "
        if description.find(readableText) >= 0:
          description = description[:description.find(readableText)]
        if description.find(writableText) >= 0:
          description = description[:description.find(writableText)]

        key = (file, warningNumber, description)
        if not key in warnings:
          warnings[key] = []
        warnings[key].append(line.strip())
  return warnings



def PrintAdditions(oldResults, newResults, message, invert):
  results = []
  for key in newResults.keys():
    if oldResults.has_key(key):
      # Check to see if the warning count has changed
      old = oldResults[key]
      new = newResults[key]
      if len(new) > len(old):
        # If the warning count has increased then we don't know which ones are
        # new. Sigh... Report the new ones, up to some maximum:
        for warning in newResults[key]:
          if invert:
            results.append(warning + ": count went from %d to %d" % \
                (len(newResults[key]), len(oldResults[key])))
          else:
            results.append(warning + ": count went from %d to %d" % \
                (len(oldResults[key]), len(newResults[key])))
    else:
      # Totally new (or fixed) warning.
      results += newResults[key]
  # This sort is not perfect because it is alphabetic and it needs to switch to
  # numeric when it encounters digits. Later.
  results.sort()
  print "%s (%d total)" % (message, len(results))
  for line in results:
    print line



if len(sys.argv) < 3:
  print "Usage: %s oldsummary.txt newsummary.txt" % sys.argv[0]
  print "Prints the changes in warnings between two /analyze runs."
  sys.exit(0)

oldFilename = sys.argv[1]
newFilename = sys.argv[2]
oldResults = SummarizeWarnings(oldFilename)
newResults = SummarizeWarnings(newFilename)

PrintAdditions(oldResults, newResults, "New warnings", False)
print
print
PrintAdditions(newResults, oldResults, "Fixed warnings", True)
