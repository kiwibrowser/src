# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
Run this script to summarize VC++ warnings and errors. This is normally used
to summarize the results of Chrome's /analyze runs. Just pass the name of
a file containing build output -- typically with a _full.txt ending -- and
a *_summary.txt file will be created. The warnings are grouped
by warning number, sorted by source file/line, and uniquified.
In addition a summary is created at the end that records how many unique
warnings of each type there are, and which warning numbers were the noisiest.

If you pass -codesnippets as the final argument then a few lines of code will
be extracted for each warning. This is useful for creating summaries of fixed
warnings.
"""

import re
import sys
import os
from collections import defaultdict

grabCodeSnippets = False

if len(sys.argv) < 2:
  print "Missing input filename."
  sys.exit(10)
inputName = sys.argv[1]
outputName = inputName.replace("_full", "_summary")
if inputName == outputName:
  outputName += "_summary.txt"

if len(sys.argv) > 2:
  if sys.argv[2] == "-codesnippets":
    grabCodeSnippets = True
    snippetExtent = 3 # How many lines of context to grap, +/-
  else:
    print "Unsupported command line option."
    sys.exit(10)

# Code snippets, typically used for filing bugs or demonstrating issues,
# don't need detailed line information, so strip it out.
stripLines = grabCodeSnippets

# Typical warning and error patterns:
# wspiapi.h(933) : warning C6102: Using 'SystemDir'
# exception_handler.cc(813) : warning C6387: 'child_thread_handle' could be '0':
# unistr.cpp(1823) : warning C28193: 'temporary value' holds a value that must
# be examined.: Lines: 1823, 1824
# LINK : warning LNK4014: cannot find member object nativec\\malloc.obj
# hash_set(17): error C2338: <hash_set> is deprecated and will be REMOVED
# Regex to extract warning/error number for processing warnings.
# Note that in VS 2015 there is no space before the colon before 'warning'
warningRe = re.compile(r".*: (warning|error) (C\d{4,5}|LNK\d{4,5}):")
# Regex to extract file/line/"warning" and line number. This is used when
# grabbing a snippet of nearby code.
codeRe = re.compile(r"(.*)\((\d*)\) : .*")

failedBuild = False

# For each warning ID we will have a dictionary of uniquified warning lines
warnByID = defaultdict(dict)
# We will also count how many times warnings show up in the raw output, to make
# it easier to address the really noisy ones.
warnCountByID = defaultdict(int)

output = open(outputName, "wt")
# Scan the input building up a database of warnings, discarding duplicates.
for line in open(inputName).readlines():
  # Detect and warn on failed builds since their results will be incomplete.
  if line.count("subcommand failed") > 0:
    failedBuild = True
  # Ignore lines without warnings
  if line.count(": warning ") == 0 and line.count(": error ") == 0:
    continue
  # Ignore "Command line warning D9025 : overriding '/WX' with '/WX-'" and
  # warnings from depot_tools header files
  if line.count("D9025") > 0 or line.count("depot_tools") > 0:
    continue
  # Ignore warnings from unit tests -- some are intentional
  if line.count("_unittest.cc") > 0:
    continue
  match = warningRe.match(line)
  if match:
    warningID = match.groups()[1]
  else:
    warningID = " " # A few warnings lack a warning ID
  warnCountByID[warningID] += 1
  # Insert warnings (warning ID and text) into a dictionary to automatically
  # purge duplicates.
  if stripLines:
    linesIndex = line.find(": Lines:")
    if linesIndex >= 0:
      line = line[:linesIndex]
  warnByID[warningID][line.strip()] = True

if failedBuild:
  print >>output, "Build did not entirely succeed!"

warnIDsByCount = warnByID.keys()
# Sort by (post uniquification) warning frequency, least frequent first
# Sort first by ID, and then by ID frequency. Sort is stable so this gives us
# consistent results within warning IDs with the same frequency
warnIDsByCount.sort(lambda x, y: cmp(x, y))
warnIDsByCount.sort(lambda x, y: cmp(len(warnByID[x]), len(warnByID[y])))

# Print all the warnings, grouped by warning ID and sorted by frequency,
# then filename
totalWarnings = 0
print >>output, "All warnings by type, sorted by count:"
for warnID in warnIDsByCount:
  warningLines = warnByID[warnID].keys()
  totalWarnings += len(warningLines)
  warningLines.sort() # Sort by file name
  for warningText in warningLines:
    print >>output, warningText
    if grabCodeSnippets:
      codeMatch = codeRe.match(warningText)
      if codeMatch:
        try:
          file, line = codeMatch.groups()
          line = int(line)
          lines = open(file).readlines()
          lines = lines[line-snippetExtent-1:line+snippetExtent]
          for line in lines:
            print >>output, line,
        except:
          pass
  print >>output # Blank separator line between warning types

print >>output, "Warning counts by type, sorted by count:"
for warnID in warnIDsByCount:
  warningLines = warnByID[warnID].keys()
  # Get a sample of this type of warning
  warningExemplar = warningLines[0]
  # Clean up the warning exemplar
  linesIndex = warningExemplar.find(": Lines:")
  if linesIndex >= 0:
    warningExemplar = warningExemplar[:linesIndex]
  warnIDIndex = warningExemplar.find(warnID)
  if warnIDIndex >= 0:
    warningExemplar = warningExemplar[warnIDIndex + len(warnID) + 2:]
  # Print the warning count and warning number -- omitting the leading 'C' or
  # 'LNK' so that searching on C6001 won't find the summary.
  count = len(warnByID[warnID])
  while warnID[0].isalpha():
    warnID = warnID[1:]
  print >>output, "%4d: %5s, eg. %s" % (count, warnID, warningExemplar)

print >>output
print >>output, "%d warnings of %d types" % (totalWarnings, len(warnIDsByCount))

print >>output
print >>output, "Noisy warning counts in raw output:"
totalRawCount = 0
for warnID in warnCountByID.keys():
  if warnCountByID[warnID] > 500:
    print >>output, "%5s: %6d" % (warnID[1:], warnCountByID[warnID])
  totalRawCount += warnCountByID[warnID]
print >>output, "Total: %6d" % totalRawCount
