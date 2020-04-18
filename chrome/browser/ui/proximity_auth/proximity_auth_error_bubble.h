// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PROXIMITY_AUTH_PROXIMITY_AUTH_ERROR_BUBBLE_H_
#define CHROME_BROWSER_UI_PROXIMITY_AUTH_PROXIMITY_AUTH_ERROR_BUBBLE_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}

namespace gfx {
class Range;
class Rect;
}

// Shows an error bubble with the given |message|, with an arrow pointing to the
// |anchor_rect|. If the |link_range| is non-empty, then that range of the
// |message| is drawn as a link with |link_url| as the target. When the link is
// clicked, the target URL opens in a new tab off of the given |web_contents|.
void ShowProximityAuthErrorBubble(const base::string16& message,
                                  const gfx::Range& link_range,
                                  const GURL& link_url,
                                  const gfx::Rect& anchor_rect,
                                  content::WebContents* web_contents);

// Hides the currently displayed error bubble, if any.
void HideProximityAuthErrorBubble();

#endif  // CHROME_BROWSER_UI_PROXIMITY_AUTH_PROXIMITY_AUTH_ERROR_BUBBLE_H_
