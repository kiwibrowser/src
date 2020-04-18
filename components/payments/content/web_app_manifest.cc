// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/web_app_manifest.h"

namespace payments {

WebAppManifestSection::WebAppManifestSection() = default;

WebAppManifestSection::WebAppManifestSection(
    const WebAppManifestSection& param) = default;

WebAppManifestSection::~WebAppManifestSection() = default;

WebAppInstallationInfo::WebAppInstallationInfo() = default;
WebAppInstallationInfo::~WebAppInstallationInfo() = default;

}  // namespace payments
