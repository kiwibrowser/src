# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import subprocess
import sys

def main():
  args = parseInput()

  assert validateHeaderInput(args.header), \
         "Error: '%s' is not a valid .h file" % args.header
  assert validateCodeInput(args.cc), \
         "Error: '%s' is not a valid .cc file" % args.cc
  assert validatePathInput(args.gen_dir), \
         "Error: '%s' is not a valid output directory" % args.gen_dir
  assert validateCddlInput(args.file), \
         "Error: '%s' is not a valid CDDL file" % args.file

  if args.log:
    logPath = os.path.join(args.gen_dir, args.log)
    log = open(logPath, "w")
    log.write("OUTPUT FOR CDDL CODE GENERATION TOOL:\n\n")
    log = open(logPath, "a")

    if (args.verbose):
      print("Logging to %s" % logPath)
  else:
    log = None

  if (args.verbose):
    print('Creating C++ files from provided CDDL file...')
  echoAndRunCommand(['./cddl', "--header", args.header, "--cc", args.cc,
                     "--gen-dir", args.gen_dir, args.file],
                     False, log, args.verbose)

  clangFormatLocation = findClangFormat()
  if not clangFormatLocation:
    if args.verbose:
      print("WARNING: clang-format could not be found")
    return

  for filename in [args.header, args.cc]:
    echoAndRunCommand([clangFormatLocation + 'clang-format', "-i",
                       os.path.join(args.gen_dir, filename)],
                       True, verbose=args.verbose)

def parseInput():
  parser = argparse.ArgumentParser()
  parser.add_argument("--header", help="Specify the filename of the output \
     header file. This is also the name that will be used for the include \
     guard and as the include path in the source file.")
  parser.add_argument("--cc", help="Specify the filename of the output \
     source file")
  parser.add_argument("--gen-dir", help="Specify the directory prefix that \
     should be added to the output header and source file.")
  parser.add_argument("--log", help="Specify the file to which stdout should \
     be redirected.")
  parser.add_argument("--verbose", help="Specify that we should log info \
     messages to stdout")
  parser.add_argument("file", help="the input file which contains the spec")
  return parser.parse_args()

def validateHeaderInput(headerFile):
  return headerFile and headerFile.endswith('.h')

def validateCodeInput(ccFile):
  return ccFile and ccFile.endswith('.cc')

def validatePathInput(dirPath):
  return dirPath and os.path.isdir(dirPath)

def validateCddlInput(cddlFile):
  return cddlFile and os.path.isfile(cddlFile)

def echoAndRunCommand(commandArray, allowFailure,
                      logfile = None, verbose = False):
  if verbose:
    print("\tExecuting Command: '%s'" % " ".join(commandArray))

  if logfile != None:
    process = subprocess.Popen(commandArray, stdout=logfile, stderr=logfile)
    process.wait()
    logfile.flush()
  else:
    process = subprocess.Popen(commandArray)
    process.wait()

  returncode = process.returncode
  if returncode != None and returncode != 0:
    if not allowFailure:
      sys.exit("\t\tERROR: Command failed with error code: '%i'!" % returncode)
    elif verbose:
      print("\t\tWARNING: Command failed with error code: '%i'!" % returncode)

def findClangFormat():
  executable = "clang-format"

  # Try and run from the environment variable
  for directory in os.environ["PATH"].split(os.pathsep):
    fullPath = os.path.join(directory, executable)
    if os.path.isfile(fullPath):
      return ""

  # Check 2 levels up since this should be correct on the build machine
  path = "../../"
  fullPath = os.path.join(path, executable)
  if os.path.isfile(fullPath):
    return path

  return None

if __name__ == "__main__":
  main()
