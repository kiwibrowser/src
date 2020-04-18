// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FAVICON_FAVICON_UTILS_H_
#define CHROME_BROWSER_FAVICON_FAVICON_UTILS_H_

#include "components/favicon/content/content_favicon_driver.h"

namespace content {
class WebContents;
}  // namespace content

namespace gfx {
class Image;
}  // namespace gfx

namespace favicon {

// Creates a ContentFaviconDriver and associates it with |web_contents| if none
// exists yet.
//
// This is a helper method for ContentFaviconDriver::CreateForWebContents() that
// gets KeyedService factories from the Profile linked to web_contents.
void CreateContentFaviconDriverForWebContents(
    content::WebContents* web_contents);

// Returns whether the favicon should be displayed. If this returns false, no
// space is provided for the favicon, and the favicon is never displayed.
bool ShouldDisplayFavicon(content::WebContents* web_contents);

// Retrieves the favicon from given WebContents. If contents contain a
// network error, desaturate the favicon.
gfx::Image TabFaviconFromWebContents(content::WebContents* contents);

}  // namespace favicon

#endif  // CHROME_BROWSER_FAVICON_FAVICON_UTILS_H_
