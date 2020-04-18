// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Redirect WebRTC's winsock initialization activity into Chromium's
// singleton object that managest precisely that for the browser.

#include "third_party/webrtc/rtc_base/win32socketinit.h"

#include "net/base/winsock_init.h"

#if !defined(WEBRTC_WIN)
#error "Only compile this on Windows"
#endif

namespace rtc {

void EnsureWinsockInit() {
  net::EnsureWinsockInit();
}

}  // namespace rtc
