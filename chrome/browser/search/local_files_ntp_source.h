// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_LOCAL_FILES_NTP_SOURCE_H_
#define CHROME_BROWSER_SEARCH_LOCAL_FILES_NTP_SOURCE_H_

#include <string>

#include "build/build_config.h"
#include "content/public/browser/url_data_source.h"

#if defined(OS_ANDROID)
#error "Instant is only used on desktop";
#endif

#if !defined(GOOGLE_CHROME_BUILD)

namespace local_ntp {

// Sends the content of |path| to |callback|, reading |path| as a local file.
// This function is only used for dev builds.
void SendLocalFileResource(
    const std::string& path,
    const content::URLDataSource::GotDataCallback& callback);

// Sends the content of |path| to |callback|, reading |path| as a local file.
// It also replaces the first occurrence of {{ORIGIN}} with |origin|.
// This function is only used for dev builds.
void SendLocalFileResourceWithOrigin(
    const std::string& path,
    const std::string& origin,
    const content::URLDataSource::GotDataCallback& callback);

}  // namespace local_ntp

#endif  // !defined(GOOGLE_CHROME_BUILD)
#endif  // CHROME_BROWSER_SEARCH_LOCAL_FILES_NTP_SOURCE_H_
