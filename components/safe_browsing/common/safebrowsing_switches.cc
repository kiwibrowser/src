// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/common/safebrowsing_switches.h"

namespace safe_browsing {
namespace switches {

// TODO(lzheng): Remove this flag once the feature works fine
// (http://crbug.com/74848).
//
// Disables safebrowsing feature that checks download url and downloads
// content's hash to make sure the content are not malicious.
const char kSbDisableDownloadProtection[] =
    "safebrowsing-disable-download-protection";

// Disables safebrowsing feature that checks for blacklisted extensions.
const char kSbDisableExtensionBlacklist[] =
    "safebrowsing-disable-extension-blacklist";

// List of comma-separated sha256 hashes of executable files which the
// download-protection service should treat as "dangerous."  For a file to
// show a warning, it also must be considered a dangerous filetype and not
// be whitelisted otherwise (by signature or URL) and must be on a supported
// OS. Hashes are in hex. This is used for manual testing when looking
// for ways to by-pass download protection.
const char kSbManualDownloadBlacklist[] =
    "safebrowsing-manual-download-blacklist";

}  // namespace switches
}  // namespace safe_browsing
