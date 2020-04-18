#!/usr/bin/python
#
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

"""
Some common boilerplates and helper functions for source code generation
in files dgen_test_output.py and dgen_decode_output.py.
"""

HEADER_BOILERPLATE ="""/*
 * Copyright 2013 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// DO NOT EDIT: GENERATED CODE
"""

NOT_TCB_BOILERPLATE="""#ifndef NACL_TRUSTED_BUT_NOT_TCB
#error This file is not meant for use in the TCB
#endif
"""

NEWLINE_STR="""
"""

COMMENTED_NEWLINE_STR="""
//"""

"""Adds comment '// ' string after newlines."""
def commented_string(str, indent=''):
  sep = NEWLINE_STR + indent + '//'
  str = str.replace(NEWLINE_STR, sep)
  # This second line is a hack to fix that sometimes newlines are
  # represented as '\n'.
  # TODO(karl) Find the cause of this hack, and fix it.
  return str.replace('\\n', sep)

def ifdef_name(filename):
  """ Generates the ifdef name to use for the given filename"""
  return filename.replace("/", "_").replace(".", "_").upper() + "_"


def GetNumberCodeBlocks(separators):
  """Gets the number of code blocks to break classes into."""
  num_blocks = len(separators) + 1
  assert num_blocks >= 2
  return num_blocks


def FindBlockIndex(filename, format, num_blocks):
  """Returns true if the filename matches the format with an
     index in the range [1, num_blocks]."""
  for block in range(1, num_blocks+1):
    suffix = format % block
    if filename.endswith(suffix):
      return block
  raise Exception("Can't find block index: %s" % filename)


def GetDecodersBlock(n, separators, decoders, name_fcn):
  """Returns the (sorted) list of decoders to include
     in block n, assuming decoders are split using
     the list of separators."""
  num_blocks = GetNumberCodeBlocks(separators)
  assert n > 0 and n <= num_blocks
  return [decoder for decoder in decoders
          if ((n == 1
               or IsPrefixLeDecoder(separators[n-2], decoder, name_fcn)) and
              (n == num_blocks or
               not IsPrefixLeDecoder(separators[n-1], decoder, name_fcn)))]

def IsPrefixLeDecoder(prefix, decoder, name_fcn):
  """Returns true if the prefix is less than or equal to the
     corresponding prefix length of the decoder name."""
  decoder_name = name_fcn(decoder)
  prefix_len = len(prefix)
  decoder_len = len(decoder_name)
  decoder_prefix = (decoder_name[0:prefix_len]
                     if prefix_len < decoder_len
                     else decoder_name)
  return prefix <= decoder_prefix

