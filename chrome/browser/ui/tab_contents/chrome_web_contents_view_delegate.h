// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TAB_CONTENTS_CHROME_WEB_CONTENTS_VIEW_DELEGATE_H_
#define CHROME_BROWSER_UI_TAB_CONTENTS_CHROME_WEB_CONTENTS_VIEW_DELEGATE_H_

namespace content {
class WebContents;
class WebContentsViewDelegate;
}

content::WebContentsViewDelegate* CreateWebContentsViewDelegate(
    content::WebContents* web_contents);

#endif  // CHROME_BROWSER_UI_TAB_CONTENTS_CHROME_WEB_CONTENTS_VIEW_DELEGATE_H_
