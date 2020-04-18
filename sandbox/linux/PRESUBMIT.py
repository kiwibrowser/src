# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

def PostUploadHook(cl, change, output_api):
  """git cl upload will call this hook after the issue is created/modified.

  This will add extra trybot coverage for non-default Android architectures
  that have a history of breaking with Seccomp changes.
  """
  def affects_seccomp(f):
    seccomp_paths = [
        'bpf_dsl/',
        'seccomp-bpf/',
        'seccomp-bpf-helpers/',
        'system_headers/',
        'tests/'
        ]
    # If the file path contains any of the above fragments, it affects
    # the Seccomp implementation.
    affected_any = map(lambda sp: sp in f.LocalPath(), seccomp_paths)
    return reduce(lambda a, b: a or b, affected_any)

  if not change.AffectedFiles(file_filter=affects_seccomp):
    return []

  return output_api.EnsureCQIncludeTrybotsAreAdded(
      cl,
      [
        'master.tryserver.chromium.android:android_arm64_dbg_recipe',
        'master.tryserver.chromium.android:android_compile_x64_dbg',
        'master.tryserver.chromium.android:android_compile_x86_dbg',
      ],
      'Automatically added Android multi-arch compile bots to run on CQ.')
