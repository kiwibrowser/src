// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/web_capabilities.h"

namespace web {

bool IsAutoDetectEncodingSupported() {
  // TODO(crbug.com/600765): WKWebView does not provide API for auto-detection
  // of the page encoding. Revisit this issue with the next major of iOS.
  return false;
}

bool IsDoNotTrackSupported() {
  // TODO(crbug.com/493368): WKWebView does not provide API for adding DNT
  // headers on iOS9. Revisit this issue with the next major release of iOS.
  return false;
}

}  // namespace web
