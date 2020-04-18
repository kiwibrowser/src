// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_CAPTIVE_PORTAL_HELPER_H_
#define CHROME_BROWSER_SSL_CAPTIVE_PORTAL_HELPER_H_

namespace chrome {

// Returns true if the OS reports that the device is behind a captive portal.
bool IsBehindCaptivePortal();

}  // namespace chrome

#endif  // CHROME_BROWSER_SSL_CAPTIVE_PORTAL_HELPER_H_
