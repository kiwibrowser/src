# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import pprint
import sys

import properties


TEST_EMPTY = {
  'in': {
    'EMPTY_LIST': [],
    'NONE': None,
    'NONE_LIST': [None],
    'STRING' : '',
    'STRING_LIST' : [''],
  },
  'out': {
    'EMPTY_LIST': [],
    'NONE': [None],
    'NONE_LIST': [None],
    'STRING' : [''],
    'STRING_LIST' : [''],
  }
}

TEST_CFLAGS = {
  'in': {
    'CFLAGS': ['-DDEFINE1', '-Iinclude', '-DDEFINE2=2', '-fPIC',],
    'CCFLAGS': None,
    'OTHER_NONE': None,
    'OTHER_SOME': 'Some',
    'CXXFLAGS': [],
  },
  'out': {
    'cflags' : ['-fPIC'],
    'defines' : ['DEFINE1', 'DEFINE2=2'],
    'include_dirs' : ['include'],
    'cflags_c' : [],
    'cflags_cc': [],
    'OTHER_NONE': [None],
    'OTHER_SOME': ['Some']
  }
}


def TestMap(table, name):
  out = properties.ParsePropertyTable(table['in'])
  pp = pprint.PrettyPrinter(indent=4)
  if out != table['out']:
    print '*** Conversion mismatch! ***'
    print 'ORIGINAL:'
    pp.pprint(table['in'])
    print 'CONVERTED:'
    pp.pprint(dict(out))
    print 'EXPECTED:'
    pp.pprint(table['out'])
    print '\n\n'
    return 1
  print 'PASSED ' + name
  return 0


if __name__ == '__main__':
  retval = 0
  retval += TestMap(TEST_EMPTY, 'Empty')
  retval += TestMap(TEST_CFLAGS, 'C flags')
  sys.exit(retval)
