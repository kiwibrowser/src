#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from driver_log import Log

import types

######################################################################
#
# Shell Utilities
#
######################################################################

class shell(object):

  @staticmethod
  def unescape(s):
    w = shell.split(s)
    if len(w) == 0:
      return ''
    if len(w) == 1:
      return w[0]
    # String was not properly escaped in the first place?
    assert(False)

  # TODO(pdox): Simplify this function by moving more of it into unescape
  @staticmethod
  def split(s):
    """Split a shell-style string up into a list of distinct arguments.
    For example: split('cmd -arg1 -arg2="a b c"')
    Returns ['cmd', '-arg1', '-arg2=a b c']
    """
    assert(isinstance(s, types.StringTypes))
    out = []
    inspace = True
    inquote = False
    buf = ''

    i = 0
    while i < len(s):
      if s[i] == '"':
        inspace = False
        inquote = not inquote
      elif s[i] == ' ' and not inquote:
        if not inspace:
          out.append(buf)
          buf = ''
        inspace = True
      elif s[i] == '\\':
        if not i+1 < len(s):
          Log.Fatal('Unterminated \\ escape sequence')
        inspace = False
        i += 1
        buf += s[i]
      else:
        inspace = False
        buf += s[i]
      i += 1
    if inquote:
      Log.Fatal('Unterminated quote')
    if not inspace:
      out.append(buf)
    return out

  @staticmethod
  def join(args):
    """Turn a list into a shell-style string For example:
       shell.join([ 'a', 'b', 'c d e' ]) = 'a b "c d e"'
    """
    return ' '.join([ shell.escape(a) for a in args ])

  @staticmethod
  def escape(s):
    """Shell-escape special characters in a string
       Surround with quotes if necessary
    """
    s = s.replace('\\', '\\\\')
    s = s.replace('"', '\\"')
    if ' ' in s:
      s = '"' + s + '"'
    return s
