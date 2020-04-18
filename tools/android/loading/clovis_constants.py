# Copyright (c) 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provides common constants for Clovis."""

QUEUING_CATEGORY = 'disabled-by-default-loading.resource'

# Categories to enable or disable for all traces collected. Disabled categories
# are prefixed with '-'.
DEFAULT_CATEGORIES = [
  QUEUING_CATEGORY,
  'blink',
  'blink.net',
  'blink.user_timing',
  'devtools.timeline',
  'disabled-by-default-blink.debug.layout',
  'toplevel',
  'v8',
  '-cc',  # Contains a lot of events, none of which we use.
]
