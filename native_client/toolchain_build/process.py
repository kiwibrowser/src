#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Recipes for NativeClient toolchain packages.

The real entry plumbing is in toolchain_main.py.
"""

import os
import StringIO
import subprocess
import sys
import tempfile

class Process(object):
  def __init__(self, args, cwd=None, shell=False, env=None, pipe=True):
    print 'IN CWD=%s' % str(cwd)
    self.args = args
    self.cwd = cwd or os.getcwd()
    self.shell = shell
    self.env = env
    print 'CWD=' + self.cwd
    try:
      if not pipe:
        self.proc = subprocess.Popen(self.args,
                                     shell=self.shell,
                                     cwd=self.cwd,
                                     env=self.env)
      else:
        self.proc = subprocess.Popen(self.args,
                                     shell=self.shell,
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE,
                                     cwd=self.cwd,
                                     env=self.env)
    except:
      print 'Failed to run:\n\t%s> %s' % (self.cwd, ' '.join(self.args))
      raise

  def Run(self, outfile=None, verbose=True):
    if self.proc:
      out, err = self.proc.communicate()
      if outfile:
        if verbose:
          outfile.write(' '.join(self.args) + '\n')
        if out:
          outfile.write(out)
        if err:
          outfile.write(err)
      return self.proc.returncode
    return -1


def Start(args, cwd=None, shell=False, env=None):
  return Process(args, cwd, sell, env)


def Run(args, cwd=None, shell=False, env=None, outfile=None, verbose=True):
  return Process(args, cwd, shell, env, outfile).Run(outfile, verbose)


def WaitForAll(procs, outfile=None):
  for proc in procs:
    proc.Run(outfile)

