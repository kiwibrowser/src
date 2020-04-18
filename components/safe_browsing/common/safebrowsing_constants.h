// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SAFE_BROWSING_COMMON_SAFEBROWSING_CONSTANTS_H_
#define COMPONENTS_SAFE_BROWSING_COMMON_SAFEBROWSING_CONSTANTS_H_

#include "base/files/file_path.h"

namespace safe_browsing {

extern const base::FilePath::CharType kSafeBrowsingBaseFilename[];

// Filename suffix for the cookie database.
extern const base::FilePath::CharType kCookiesFile[];
extern const base::FilePath::CharType kChannelIDFile[];

// The default URL prefix where browser fetches chunk updates, hashes,
// and reports safe browsing hits and malware details.
extern const char kSbDefaultURLPrefix[];

// The backup URL prefix used when there are issues establishing a connection
// with the server at the primary URL.
extern const char kSbBackupConnectErrorURLPrefix[];

// The backup URL prefix used when there are HTTP-specific issues with the
// server at the primary URL.
extern const char kSbBackupHttpErrorURLPrefix[];

// The backup URL prefix used when there are local network specific issues.
extern const char kSbBackupNetworkErrorURLPrefix[];

// When a network::mojom::URLLoader is cancelled because of SafeBrowsing, this
// custom cancellation reason could be used to notify the implementation side.
// Please see network::mojom::URLLoader::kClientDisconnectReason for more
// details.
extern const char kCustomCancelReasonForURLLoader[];
}

#endif  // COMPONENTS_SAFE_BROWSING_COMMON_SAFEBROWSING_CONSTANTS_H_
