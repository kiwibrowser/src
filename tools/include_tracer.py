#!/usr/bin/env python
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# based on an almost identical script by: jyrki@google.com (Jyrki Alakuijala)

"""Prints out include dependencies in chrome.

Since it ignores defines, it gives just a rough estimation of file size.

Usage:
  tools/include_tracer.py chrome/browser/ui/browser.h
"""

import os
import sys

# Created by copying the command line for prerender_browsertest.cc, replacing
# spaces with newlines, and dropping everything except -F and -I switches.
# TODO(port): Add windows, linux directories.
INCLUDE_PATHS = [
  '',
  'gpu',
  'skia/config',
  'skia/ext',
  'testing/gmock/include',
  'testing/gtest/include',
  'third_party/blink/renderer',
  'third_party/blink/renderer/core',
  'third_party/blink/renderer/core/accessibility',
  'third_party/blink/renderer/core/accessibility/chromium',
  'third_party/blink/renderer/core/bindings',
  'third_party/blink/renderer/core/bindings/generic',
  'third_party/blink/renderer/core/bindings/v8',
  'third_party/blink/renderer/core/bindings/v8/custom',
  'third_party/blink/renderer/core/bindings/v8/specialization',
  'third_party/blink/renderer/core/bridge',
  'third_party/blink/renderer/core/bridge/jni',
  'third_party/blink/renderer/core/bridge/jni/v8',
  'third_party/blink/renderer/core/css',
  'third_party/blink/renderer/core/dom',
  'third_party/blink/renderer/core/dom/default',
  'third_party/blink/renderer/core/editing',
  'third_party/blink/renderer/core/fileapi',
  'third_party/blink/renderer/core/history',
  'third_party/blink/renderer/core/html',
  'third_party/blink/renderer/core/html/canvas',
  'third_party/blink/renderer/core/html/parser',
  'third_party/blink/renderer/core/html/shadow',
  'third_party/blink/renderer/core/inspector',
  'third_party/blink/renderer/core/loader',
  'third_party/blink/renderer/core/loader/appcache',
  'third_party/blink/renderer/core/loader/archive',
  'third_party/blink/renderer/core/loader/cache',
  'third_party/blink/renderer/core/loader/icon',
  'third_party/blink/renderer/core/mathml',
  'third_party/blink/renderer/core/notifications',
  'third_party/blink/renderer/core/page',
  'third_party/blink/renderer/core/page/animation',
  'third_party/blink/renderer/core/page/chromium',
  'third_party/blink/renderer/core/platform',
  'third_party/blink/renderer/core/platform/animation',
  'third_party/blink/renderer/core/platform/audio',
  'third_party/blink/renderer/core/platform/audio/chromium',
  'third_party/blink/renderer/core/platform/audio/mac',
  'third_party/blink/renderer/core/platform/chromium',
  'third_party/blink/renderer/core/platform/cocoa',
  'third_party/blink/renderer/core/platform/graphics',
  'third_party/blink/renderer/core/platform/graphics/cg',
  'third_party/blink/renderer/core/platform/graphics/chromium',
  'third_party/blink/renderer/core/platform/graphics/cocoa',
  'third_party/blink/renderer/core/platform/graphics/filters',
  'third_party/blink/renderer/core/platform/graphics/gpu',
  'third_party/blink/renderer/core/platform/graphics/mac',
  'third_party/blink/renderer/core/platform/graphics/opentype',
  'third_party/blink/renderer/core/platform/graphics/skia',
  'third_party/blink/renderer/core/platform/graphics/transforms',
  'third_party/blink/renderer/core/platform/image-decoders',
  'third_party/blink/renderer/core/platform/image-decoders/bmp',
  'third_party/blink/renderer/core/platform/image-decoders/gif',
  'third_party/blink/renderer/core/platform/image-decoders/ico',
  'third_party/blink/renderer/core/platform/image-decoders/jpeg',
  'third_party/blink/renderer/core/platform/image-decoders/png',
  'third_party/blink/renderer/core/platform/image-decoders/skia',
  'third_party/blink/renderer/core/platform/image-decoders/webp',
  'third_party/blink/renderer/core/platform/image-decoders/xbm',
  'third_party/blink/renderer/core/platform/image-encoders/skia',
  'third_party/blink/renderer/core/platform/mac',
  'third_party/blink/renderer/core/platform/mock',
  'third_party/blink/renderer/core/platform/network',
  'third_party/blink/renderer/core/platform/network/chromium',
  'third_party/blink/renderer/core/platform/sql',
  'third_party/blink/renderer/core/platform/text',
  'third_party/blink/renderer/core/platform/text/mac',
  'third_party/blink/renderer/core/platform/text/transcoder',
  'third_party/blink/renderer/core/plugins',
  'third_party/blink/renderer/core/plugins/chromium',
  'third_party/blink/renderer/core/rendering',
  'third_party/blink/renderer/core/rendering/style',
  'third_party/blink/renderer/core/rendering/svg',
  'third_party/blink/renderer/core/storage',
  'third_party/blink/renderer/core/storage/chromium',
  'third_party/blink/renderer/core/svg',
  'third_party/blink/renderer/core/svg/animation',
  'third_party/blink/renderer/core/svg/graphics',
  'third_party/blink/renderer/core/svg/graphics/filters',
  'third_party/blink/renderer/core/svg/properties',
  'third_party/blink/renderer/core/webaudio',
  'third_party/blink/renderer/core/websockets',
  'third_party/blink/renderer/core/workers',
  'third_party/blink/renderer/core/xml',
  'third_party/blink/renderer/public',
  'third_party/blink/renderer/web',
  'third_party/blink/renderer/wtf',
  'third_party/google_toolbox_for_mac/src',
  'third_party/icu/public/common',
  'third_party/icu/public/i18n',
  'third_party/npapi',
  'third_party/npapi/bindings',
  'third_party/protobuf',
  'third_party/protobuf/src',
  'third_party/skia/gpu/include',
  'third_party/skia/include/config',
  'third_party/skia/include/core',
  'third_party/skia/include/effects',
  'third_party/skia/include/gpu',
  'third_party/skia/include/pdf',
  'third_party/skia/include/ports',
  'v8/include',
  'xcodebuild/Debug/include',
  'xcodebuild/DerivedSources/Debug/chrome',
  'xcodebuild/DerivedSources/Debug/policy',
  'xcodebuild/DerivedSources/Debug/protoc_out',
  'xcodebuild/DerivedSources/Debug/webkit',
  'xcodebuild/DerivedSources/Debug/webkit/bindings',
]


def Walk(seen, filename, parent, indent):
  """Returns the size of |filename| plus the size of all files included by
  |filename| and prints the include tree of |filename| to stdout. Every file
  is visited at most once.
  """
  total_bytes = 0

  # .proto(devel) filename translation
  if filename.endswith('.pb.h'):
    basename = filename[:-5]
    if os.path.exists(basename + '.proto'):
      filename = basename + '.proto'
    else:
      print 'could not find ', filename

  # Show and count files only once.
  if filename in seen:
    return total_bytes
  seen.add(filename)

  # Display the paths.
  print ' ' * indent + filename

  # Skip system includes.
  if filename[0] == '<':
    return total_bytes

  # Find file in all include paths.
  resolved_filename = filename
  for root in INCLUDE_PATHS + [os.path.dirname(parent)]:
    if os.path.exists(os.path.join(root, filename)):
      resolved_filename = os.path.join(root, filename)
      break

  # Recurse.
  if os.path.exists(resolved_filename):
    lines = open(resolved_filename).readlines()
  else:
    print ' ' * (indent + 2) + "-- not found"
    lines = []
  for line in lines:
    line = line.strip()
    if line.startswith('#include "'):
      total_bytes += Walk(
          seen, line.split('"')[1], resolved_filename, indent + 2)
    elif line.startswith('#include '):
      include = '<' + line.split('<')[1].split('>')[0] + '>'
      total_bytes += Walk(
          seen, include, resolved_filename, indent + 2)
    elif line.startswith('import '):
      total_bytes += Walk(
          seen, line.split('"')[1], resolved_filename, indent + 2)
  return total_bytes + len("".join(lines))


def main():
  bytes = Walk(set(), sys.argv[1], '', 0)
  print
  print float(bytes) / (1 << 20), "megabytes of chrome source"


if __name__ == '__main__':
  sys.exit(main())
