// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/webapk/webapk_web_manifest_checker.h"

#include "third_party/blink/public/common/manifest/manifest.h"
#include "url/gurl.h"

namespace {

// Returns whether a URL in the Web Manifest is WebAPK compatible.
bool IsUrlWebApkCompatible(const GURL& url) {
  // WebAPK web manifests are stored on the Chrome WebAPK server. Do not
  // generate WebAPKs for Web Manifests with URLs with a user name or password
  // in order to avoid storing user names and passwords on the WebAPK server.
  return !url.has_username() && !url.has_password();
}

}  // anonymous namespace

bool AreWebManifestUrlsWebApkCompatible(const blink::Manifest& manifest) {
  for (const blink::Manifest::Icon& icon : manifest.icons) {
    if (!IsUrlWebApkCompatible(icon.src))
      return false;
  }

  // Do not check "related_applications" URLs because they are not used by
  // WebAPKs.

  return IsUrlWebApkCompatible(manifest.start_url) &&
      IsUrlWebApkCompatible(manifest.scope);
}
