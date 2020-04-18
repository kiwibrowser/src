// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/web_application_info.h"

WebApplicationInfo::IconInfo::IconInfo() : width(0), height(0) {
}

WebApplicationInfo::IconInfo::~IconInfo() {
}

WebApplicationInfo::WebApplicationInfo()
    : mobile_capable(MOBILE_CAPABLE_UNSPECIFIED),
      generated_icon_color(SK_ColorTRANSPARENT),
      open_as_window(false) {}

WebApplicationInfo::WebApplicationInfo(const WebApplicationInfo& other) =
    default;

WebApplicationInfo::~WebApplicationInfo() {
}
