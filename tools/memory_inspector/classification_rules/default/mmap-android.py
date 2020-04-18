# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This is a generic rule-tree for classifying memory maps on Android. It is a
# simple hierarchical python data structure (list of dictionaries). Some rules:
# - Order matters: what is caught by a node is not caught by its siblings.
# - Hierarchy matters: what is caught by a node is propagated to its children
#   (if any). Only one of its children, though, will get the data.
# - Non leaf nodes have an extra implicit node called {node-name}-other: if
#   something is caught by a non leaf node, but none of its children, it is
#   appended to implicit {node-name}-other catch-all children.
#
# See memory_inspector/classification/mmap_classifier.py for more docs.

[
{
  'name': 'Anon',
  'mmap_file': r'(^$)|(^\[)',
  'children': [
    {
      'name': 'stack',
      'mmap_file': r'\[stack',
    },
    {
      'name': 'libc malloc',
      'mmap_file': 'libc_malloc',
    },
    {
      'name': 'JIT',
      'mmap_prot': 'r.x',
    },
  ],
},
{
  'name': 'Ashmem',
  'mmap_file': r'^/dev/ashmem',
  'children': [
    {
      'name': 'Dalvik',
      'mmap_file': r'^/dev/ashmem/dalvik',
      'children': [
        {
          'name': 'Java Heap',
          'mmap_file': r'dalvik-heap',
        },
        {
          'name': 'JIT',
          'mmap_prot': 'r.x',
        },
      ],
    },
  ],
},
{
  'name': 'Libs',
  'mmap_file': r'(\.so)|(\.apk)|(\.jar)',
  'children': [
    {
      'name': 'Native',
      'mmap_file': r'\.so',
    },
    {
      'name': 'APKs',
      'mmap_file': r'\.apk',
    },
    {
      'name': 'JARs',
      'mmap_file': r'\.jar',
    },
  ],
},
{
  'name': 'Devices',
  'mmap_file': r'^/dev/',
  'children': [
    {
      'name': 'GPU',
      'mmap_file': r'(nv)|(mali)|(kgsl)',
    },
  ],
},
]
