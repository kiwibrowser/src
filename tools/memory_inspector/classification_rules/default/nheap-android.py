# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This is a generic rule-tree for classifying native heaps on Android. It is a
# simple hierarchical python data structure (list of dictionaries). Some rules:
# - Order matters: what is caught by a node is not caught by its siblings.
# - Hierarchy matters: what is caught by a node is propagated to its children
#   (if any). Only one of its children, though, will get the data.
# - Non leaf nodes have an extra implicit node called {node-name}-other: if
#   something is caught by a non leaf node, but none of its children, it is
#   appended to implicit {node-name}-other catch-all children.
#
# See memory_inspector/classification/native_heap_classifier.py for more docs.

# TODO(primiano): This is just a quick sample. Enrich looking at DMP.
[
{
  'name': 'Blink',
  'stacktrace':  r'WTF::|WebCore::|WebKit::',
  'children': [
    {
      'name': 'SharedBuffer',
      'stacktrace': r'WebCore::SharedBuffer',
    },
    {
      'name': 'XMLHttpRequest',
      'stacktrace': r'WebCore::XMLHttpRequest',
    },
    {
      'name': 'DocumentWriter',
      'stacktrace': r'WebCore::DocumentWriter',
    },
    {
      'name': 'Node_Docs',
      'stacktrace': r'WebCore::\w+::create',
    },
  ],
},

{
  'name': 'Skia',
  'stacktrace':  r'sk\w+::',
},
{
  'name': 'V8',
  'stacktrace':  r'v8::',
  'children': [
    {
      'name': 'heap-newspace',
      'stacktrace': r'v8::internal::NewSpace',
    },
    {
      'name': 'heap-coderange',
      'stacktrace': r'v8::internal::CodeRange',
    },
    {
      'name': 'heap-pagedspace',
      'stacktrace': r'v8::internal::PagedSpace',
    },
  ],
},
]
