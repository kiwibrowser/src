// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BROWSER_DIALOGS_VIEWS_MAC_H_
#define CHROME_BROWSER_UI_COCOA_BROWSER_DIALOGS_VIEWS_MAC_H_

#include "chrome/browser/safe_browsing/chrome_password_protection_service.h"
#include "chrome/browser/ui/bubble_anchor_util.h"
#include "ui/gfx/native_widget_types.h"

class Browser;
class BubbleUi;
class ContentSettingBubbleModel;
class ExtensionInstalledBubble;
class GURL;
class LocationBarDecoration;

namespace bookmarks {
class BookmarkBubbleObserver;
}

namespace content {
class WebContents;
}

namespace gfx {
class Point;
}

namespace security_state {
struct SecurityInfo;
}

namespace chrome {

// Whether to use toolkit-views rather than Cocoa for dialogs ready to "pilot".
bool ShowPilotDialogsWithViewsToolkit();

// Whether to show all dialogs with toolkit-views on Mac, rather than Cocoa.
bool ShowAllDialogsWithViewsToolkit();

// Shows a Views page info bubble on the given |browser|.
void ShowPageInfoBubbleViews(Browser* browser,
                             content::WebContents* web_contents,
                             const GURL& virtual_url,
                             const security_state::SecurityInfo& security_info,
                             bubble_anchor_util::Anchor anchor);

// Show a Views bookmark bubble at the given point. This occurs when the
// bookmark star is clicked or "Bookmark This Page..." is selected from a menu
// or via a key equivalent.
void ShowBookmarkBubbleViewsAtPoint(const gfx::Point& anchor_point,
                                    gfx::NativeView parent,
                                    bookmarks::BookmarkBubbleObserver* observer,
                                    Browser* browser,
                                    const GURL& url,
                                    bool newly_bookmarked,
                                    LocationBarDecoration* decoration);

// Builds the Views version of an Extension installed bubble.
std::unique_ptr<BubbleUi> BuildViewsExtensionInstalledBubbleUi(
    ExtensionInstalledBubble* bubble);

// Shows a views zoom bubble at the |anchor_point|. This occurs when the zoom
// icon is clicked or when a shortcut key is pressed or whenever |web_contents|
// zoom factor changes. |user_action| is used to determine if the bubble will
// auto-close.
void ShowZoomBubbleViewsAtPoint(content::WebContents* web_contents,
                                const gfx::Point& anchor_point,
                                bool user_action,
                                LocationBarDecoration* decoration);

// Closes a views zoom bubble if currently shown.
void CloseZoomBubbleViews();

// Refreshes views zoom bubble if currently shown.
void RefreshZoomBubbleViews();

// Returns true if views zoom bubble is currently shown.
bool IsZoomBubbleViewsShown();

// This is a class so that it can be friended from ContentSettingBubbleContents,
// which allows it to call SetAnchorRect().
class ContentSettingBubbleViewsBridge {
 public:
  static gfx::NativeWindow Show(gfx::NativeView parent_view,
                                ContentSettingBubbleModel* model,
                                content::WebContents* web_contents,
                                const gfx::Point& anchor,
                                LocationBarDecoration* decoration);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ContentSettingBubbleViewsBridge);
};

// Shows the import lock dialog.
void ShowImportLockDialogViews(gfx::NativeWindow parent,
                               const base::Callback<void(bool)>& callback);

// Shows the first run bubble.
void ShowFirstRunBubbleViews(Browser* browser);

void ShowPasswordReuseWarningDialog(
    content::WebContents* web_contents,
    safe_browsing::ChromePasswordProtectionService* service,
    safe_browsing::OnWarningDone done_callback);

}  // namespace chrome

#endif  // CHROME_BROWSER_UI_COCOA_BROWSER_DIALOGS_VIEWS_MAC_H_
