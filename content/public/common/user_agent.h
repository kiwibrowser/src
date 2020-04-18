// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_USER_AGENT_H_
#define CONTENT_PUBLIC_COMMON_USER_AGENT_H_

#include <string>

#include "content/common/content_export.h"

namespace content {

// Returns the WebKit version, in the form "major.minor (branch@revision)".
CONTENT_EXPORT std::string GetWebKitVersion();

CONTENT_EXPORT std::string GetWebKitRevision();

// Builds a User-agent compatible string that describes the OS and CPU type.
CONTENT_EXPORT std::string BuildOSCpuInfo();

// Helper function to generate a full user agent string from a short
// product name.
CONTENT_EXPORT std::string BuildUserAgentFromProduct(
    const std::string& product);

// Helper function to generate a full user agent string given a short
// product name and some extra text to be added to the OS info.
CONTENT_EXPORT std::string BuildUserAgentFromProductAndExtraOSInfo(
        const std::string& product,
        const std::string& extra_os_info);

// Builds a full user agent string given a string describing the OS and a
// product name.
CONTENT_EXPORT std::string BuildUserAgentFromOSAndProduct(
    const std::string& os_info,
    const std::string& product);

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_USER_AGENT_H_
