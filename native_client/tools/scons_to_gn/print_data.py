# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

def GetItem(data, depth=0):
  if isinstance(data, str):
    return  '  ' * depth + '"%s"' % data
  return '  ' * depth + str(data)

def PrintData(data, depth=0):
  if isinstance(data, dict):
    print '  ' * depth + '{'
    for key in data:
      print GetItem(key, depth + 1) + ':'
      PrintData(data[key], depth + 2)
    print '  ' * depth + '}'
    return
  if isinstance(data, list):
    print '  ' * depth + '['
    for item in data:
      PrintData(item, depth + 1)
    print '  ' * depth + ']'
    return
  print GetItem(data, depth)
