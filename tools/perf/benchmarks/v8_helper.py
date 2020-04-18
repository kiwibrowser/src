# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

_JS_FLAGS_SWITCH = '--js-flags='

def AppendJSFlags(options, js_flags):
  existing_js_flags = ''
  # There should be only one occurence of --js-flags in the browser flags. When
  # there are multiple flags, only one of them would be used. Append any
  # additional js_flags to the existing flags (if present).
  for extra_arg in options.extra_browser_args:
    if extra_arg.startswith(_JS_FLAGS_SWITCH):
      # Find and remove the set of existing js_flags.
      existing_js_flags = extra_arg[len(_JS_FLAGS_SWITCH):]
      options.extra_browser_args.remove(extra_arg)
      break

  options.AppendExtraBrowserArgs([
      # Add a new --js-flags which includes previous flags.
      '%s%s %s' %  (_JS_FLAGS_SWITCH, js_flags, existing_js_flags)
  ])
