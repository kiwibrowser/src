#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import optparse
import sys

import bot_update  # pylint: disable=relative-import


if __name__ == '__main__':
  parse = optparse.OptionParser()

  parse.add_option('--gerrit_repo',
                   help='Gerrit repository to pull the ref from.')
  parse.add_option('--gerrit_ref', help='Gerrit ref to apply.')
  parse.add_option('--root', help='The location of the checkout.')
  parse.add_option('--gerrit_no_reset', action='store_true',
                   help='Bypass calling reset after applying a gerrit ref.')
  parse.add_option('--gerrit_no_rebase_patch_ref', action='store_true',
                   help='Bypass rebase of Gerrit patch ref after checkout.')

  options, _ = parse.parse_args()

  sys.exit(
      bot_update.apply_gerrit_ref(
          options.gerrit_repo,
          options.gerrit_ref,
          options.root,
          not options.gerrit_no_reset,
          not options.gerrit_no_rebase_patch_ref)
  )
