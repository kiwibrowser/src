// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/common/safebrowsing_constants.h"

namespace safe_browsing {

const base::FilePath::CharType kSafeBrowsingBaseFilename[] =
    FILE_PATH_LITERAL("Safe Browsing");

const base::FilePath::CharType kCookiesFile[] = FILE_PATH_LITERAL(" Cookies");
const base::FilePath::CharType kChannelIDFile[] =
    FILE_PATH_LITERAL(" Channel IDs");

// The default URL prefix where browser fetches chunk updates, hashes,
// and reports safe browsing hits and malware details.
const char kSbDefaultURLPrefix[] =
    "https://safebrowsing.google.com/safebrowsing";

// The backup URL prefix used when there are issues establishing a connection
// with the server at the primary URL.
const char kSbBackupConnectErrorURLPrefix[] =
    "https://alt1-safebrowsing.google.com/safebrowsing";

// The backup URL prefix used when there are HTTP-specific issues with the
// server at the primary URL.
const char kSbBackupHttpErrorURLPrefix[] =
    "https://alt2-safebrowsing.google.com/safebrowsing";

// The backup URL prefix used when there are local network specific issues.
const char kSbBackupNetworkErrorURLPrefix[] =
    "https://alt3-safebrowsing.google.com/safebrowsing";

const char kCustomCancelReasonForURLLoader[] = "SafeBrowsing";

}  // namespace safe_browsing
