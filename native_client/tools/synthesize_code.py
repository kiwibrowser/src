#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""This tool synthesizes a very large C program based on a single
integer argument passed on the command line.
"""
import sys

# The genareted code alternated between these operators
OPERATORS = ["+", "-", "*", "/"]
# number of function calls made by each function
# A fairly high number here will disuade the inliner from doing anything
# because function bodies will be large and the number of callers will be large.
NUM_CALLS = 100

def EmitProlog(n, out):
  out.write("/* n == %d */\n" % n)

  for i in range(n):
    out.write("int func_%d(int x);\n" % i)

def EmitFunctions(n, out):
  for i in range(n):
    out.write("\n")
    out.write("int func_%d(int x) {\n" % i)
    out.write("  int result = %d;\n" % i)
    op = OPERATORS[i % len(OPERATORS)]
    fun_indices = [(i + x) % n for x in range(1, NUM_CALLS + 1)]
    for f in fun_indices:
      out.write("  if (result != %d) result %s= func_%d(result);\n" %
                (f, op, f))
    out.write("  return result;\n")
    out.write("}\n")

def EmitMain(n, out):
  out.write("\n")
  # Note, use of argc is intended to thwart optimization
  out.write("int main(int argc, char* argv[]) { return func_0(argc); }\n")

def main(argv):
  if len(argv) != 2:
    print "Expecting integer parameter"
    return 1
  n = int(argv[1])
  out = sys.stdout
  out.write("/* This code was generated using %s */\n" % repr(argv))
  EmitProlog(n, out)
  EmitFunctions(n, out)
  EmitMain(n, out)

if __name__ == '__main__':
  sys.exit(main(sys.argv))
