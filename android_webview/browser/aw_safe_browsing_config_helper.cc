// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_safe_browsing_config_helper.h"

#include "base/android/scoped_java_ref.h"
#include "base/lazy_instance.h"
#include "base/synchronization/lock.h"

namespace {
// g_safebrowsing_enabled_by_manifest can be set and read from different
// threads.
base::LazyInstance<base::Lock>::Leaky g_safebrowsing_enabled_by_manifest_lock =
    LAZY_INSTANCE_INITIALIZER;
bool g_safebrowsing_enabled_by_manifest = false;
}  // namespace

namespace android_webview {

// static
void AwSafeBrowsingConfigHelper::SetSafeBrowsingEnabledByManifest(
    bool enabled) {
  base::AutoLock lock(g_safebrowsing_enabled_by_manifest_lock.Get());
  g_safebrowsing_enabled_by_manifest = enabled;
}

// static
bool AwSafeBrowsingConfigHelper::GetSafeBrowsingEnabledByManifest() {
  base::AutoLock lock(g_safebrowsing_enabled_by_manifest_lock.Get());
  return g_safebrowsing_enabled_by_manifest;
}

}  // namespace android_webview
