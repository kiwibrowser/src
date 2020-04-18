// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_WEB_MANIFEST_CHECKER_H_
#define CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_WEB_MANIFEST_CHECKER_H_

namespace blink {
struct Manifest;
}

// Returns whether the format of the URLs in the Web Manifest is WebAPK
// compatible.
bool AreWebManifestUrlsWebApkCompatible(const blink::Manifest& manifest);

#endif  // CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_WEB_MANIFEST_CHECKER_H_
