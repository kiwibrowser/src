// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TAB_CONTENTS_FAVICON_UTIL_MAC_H_
#define CHROME_BROWSER_UI_COCOA_TAB_CONTENTS_FAVICON_UTIL_MAC_H_

#include "third_party/skia/include/core/SkColor.h"

@class NSImage;

namespace content {
class WebContents;
}

namespace mac {

// Returns an autoreleased favicon for a given WebContents. If |contents|
// is NULL or there's no favicon for the NavigationEntry, this will return the
// default image. The color parameter is only used for the default vector image
// in Material design.
NSImage* FaviconForWebContents(content::WebContents* contents,
                               SkColor color = SK_ColorBLACK);

}  // namespace mac

#endif  // CHROME_BROWSER_UI_COCOA_TAB_CONTENTS_FAVICON_UTIL_MAC_H_
