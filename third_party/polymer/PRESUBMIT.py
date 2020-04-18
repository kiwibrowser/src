# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

def PostUploadHook(cl, change, output_api):
  return output_api.EnsureCQIncludeTrybotsAreAdded(
    cl,
    [
      'master.tryserver.chromium.linux:closure_compilation',
    ],
    'Automatically added optional Closure bots to run on CQ.')
