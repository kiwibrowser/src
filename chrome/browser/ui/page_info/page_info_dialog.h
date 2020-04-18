// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_DIALOG_H_
#define CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_DIALOG_H_

#include "base/callback_forward.h"
#include "chrome/browser/ui/bubble_anchor_util.h"

namespace content {
class WebContents;
}

namespace security_state {
struct SecurityInfo;
}

class GURL;
class Browser;

// Shows PageInfo for the given |web_contents| in its browser. Returns false if
// the URL or parent Browser* can not be determined.
bool ShowPageInfoDialog(
    content::WebContents* web_contents,
    bubble_anchor_util::Anchor = bubble_anchor_util::kLocationBar);

// Shows Page Info using the specified information. |virtual_url| is the
// virtual url of the page/frame the info applies to, and |security_info|
// contains the security state for that page/frame.
// Implemented in platform-specific files.
void ShowPageInfoDialogImpl(Browser* browser,
                            content::WebContents* web_contents,
                            const GURL& virtual_url,
                            const security_state::SecurityInfo& security_info,
                            bubble_anchor_util::Anchor);

// Gets the callback to run after a dialog is created. Only used in tests.
base::OnceClosure& GetPageInfoDialogCreatedCallbackForTesting();

#endif  // CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_DIALOG_H_
