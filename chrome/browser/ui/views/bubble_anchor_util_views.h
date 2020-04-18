// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_BUBBLE_ANCHOR_UTIL_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_BUBBLE_ANCHOR_UTIL_VIEWS_H_

#include "chrome/browser/ui/bubble_anchor_util.h"

namespace views {
class View;
}

class Browser;

namespace bubble_anchor_util {

// Returns the PageInfo |anchor| View for |browser|, or null if it should not be
// used.
views::View* GetPageInfoAnchorView(Browser* browser, Anchor = kLocationBar);

}  // namespace bubble_anchor_util

#endif  // CHROME_BROWSER_UI_VIEWS_BUBBLE_ANCHOR_UTIL_VIEWS_H_
