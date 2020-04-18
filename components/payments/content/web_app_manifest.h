// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CONTENT_WEB_APP_MANIFEST_H_
#define COMPONENTS_PAYMENTS_CONTENT_WEB_APP_MANIFEST_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "third_party/skia/include/core/SkBitmap.h"

namespace payments {

struct WebAppManifestSection {
  WebAppManifestSection();
  explicit WebAppManifestSection(const WebAppManifestSection& param);
  ~WebAppManifestSection();

  // The package name of the app.
  std::string id;

  // Minimum version number of the app.
  int64_t min_version = 0;

  // The result of SHA256(signing certificate bytes) for each certificate in the
  // app.
  std::vector<std::vector<uint8_t>> fingerprints;
};

struct WebAppInstallationInfo {
  WebAppInstallationInfo();
  ~WebAppInstallationInfo();

  std::unique_ptr<SkBitmap> icon;
  std::string name;
  std::string sw_js_url;
  std::string sw_scope;
  bool sw_use_cache;
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CONTENT_WEB_APP_MANIFEST_H_