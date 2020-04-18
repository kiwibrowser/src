// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_WEB_CAPABILITIES_H_
#define IOS_WEB_PUBLIC_WEB_CAPABILITIES_H_

namespace web {

// Returns true if auto-detection of page encoding is supported by //web.
bool IsAutoDetectEncodingSupported();

// Returns true if "Do-Not-Track" is supported by //web API.
bool IsDoNotTrackSupported();

}  // namespace web

#endif  // IOS_WEB_PUBLIC_WEB_CAPABILITIES_H_
