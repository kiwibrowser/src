// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_TRUSTED_CDN_H_
#define CHROME_BROWSER_ANDROID_TRUSTED_CDN_H_

class GURL;

namespace trusted_cdn {

// Returns whether the given URL is hosted by a trusted CDN. This can be turned
// off via a Feature, and the base URL to trust can be set via a command line
// flag for testing.
bool IsTrustedCDN(const GURL& url);

}  // namespace trusted_cdn

#endif  // CHROME_BROWSER_ANDROID_TRUSTED_CDN_H_
