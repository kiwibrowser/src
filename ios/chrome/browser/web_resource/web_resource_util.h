// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_RESOURCE_WEB_RESOURCE_UTIL_H_
#define IOS_CHROME_BROWSER_WEB_RESOURCE_WEB_RESOURCE_UTIL_H_

#include "components/web_resource/web_resource_service.h"

namespace web_resource {

// Returns a ParseJSONCallback that parses JSON on a background thread.
// Generates an error if the data is not a dictionary.
WebResourceService::ParseJSONCallback GetIOSChromeParseJSONCallback();

}  // namespace web_resource

#endif  // IOS_CHROME_BROWSER_WEB_RESOURCE_WEB_RESOURCE_UTIL_H_
