// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_ARCHIVE_INFO_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_ARCHIVE_INFO_H_

#include "base/time/time.h"
#include "third_party/blink/public/platform/web_url.h"

namespace blink {

// Basic attributes of an archive resource that are derived from the parsed
// headers and content contained within.
struct WebArchiveInfo {
  // Main resource URL, the parser chooses the first appropriate resource from
  // within the MTHML file.  This is the Content-Location header from that
  // resource.
  WebURL url;

  // Date as reported in the Date: header from the MHTML header section
  base::Time date;
};

}  // namespace blink

#endif
