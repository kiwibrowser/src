// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "chrome/browser/ui/bookmarks/bookmark_bubble_sign_in_delegate.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/bubble_anchor_util.h"
#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#include "chrome/browser/ui/cocoa/bubble_anchor_helper_views.h"
#include "chrome/browser/ui/content_settings/content_setting_bubble_model.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bubble_view.h"
#include "chrome/browser/ui/views/bubble_anchor_util_views.h"
#include "chrome/browser/ui/views/content_setting_bubble_contents.h"
#include "chrome/browser/ui/views/extensions/extension_installed_bubble_view.h"
#include "chrome/browser/ui/views/importer/import_lock_dialog_view.h"
#include "chrome/browser/ui/views/location_bar/zoom_bubble_view.h"
#include "chrome/browser/ui/views/page_info/page_info_bubble_view.h"
#include "chrome/browser/ui/views/safe_browsing/password_reuse_modal_warning_dialog.h"
#include "chrome/browser/ui/views/task_manager_view.h"
#include "chrome/browser/ui/views/update_recommended_message_box.h"
#include "chrome/common/chrome_features.h"
#include "components/constrained_window/constrained_window_views.h"
#include "ui/base/material_design/material_design_controller.h"

// This file provides definitions of desktop browser dialog-creation methods for
// Mac where a Cocoa browser is using Views dialogs. I.e. it is included in the
// Cocoa build and definitions under chrome/browser/ui/cocoa may select at
// runtime whether to show a Cocoa dialog, or the toolkit-views dialog defined
// here (declared in browser_dialogs.h).

namespace chrome {

bool ShowPilotDialogsWithViewsToolkit() {
  return ui::MaterialDesignController::IsSecondaryUiMaterial();
}

bool ShowAllDialogsWithViewsToolkit() {
  return ShowPilotDialogsWithViewsToolkit() &&
         base::FeatureList::IsEnabled(
             features::kShowAllDialogsWithViewsToolkit);
}

void ShowPageInfoBubbleViews(Browser* browser,
                             content::WebContents* web_contents,
                             const GURL& virtual_url,
                             const security_state::SecurityInfo& security_info,
                             bubble_anchor_util::Anchor anchor) {
  // Don't show the bubble again if it's already showing. A second click on the
  // location icon in the omnibox will dismiss an open bubble. This behaviour is
  // consistent with the non-Mac views implementation.
  // Note that when the browser is toolkit-views, IsBubbleShowing() is checked
  // earlier because the bubble is shown on mouse release (but dismissed on
  // mouse pressed). A Cocoa browser does both on mouse pressed, so a check
  // when showing is sufficient.
  if (PageInfoBubbleView::GetShownBubbleType() !=
      PageInfoBubbleView::BUBBLE_NONE) {
    return;
  }

  views::View* anchor_view =
      bubble_anchor_util::GetPageInfoAnchorView(browser, anchor);
  gfx::Rect anchor_rect =
      anchor_view ? gfx::Rect()
                  : bubble_anchor_util::GetPageInfoAnchorRect(browser);
  gfx::NativeWindow parent_window = browser->window()->GetNativeWindow();
  views::BubbleDialogDelegateView* bubble =
      PageInfoBubbleView::CreatePageInfoBubble(
          anchor_view, anchor_rect, parent_window, browser->profile(),
          web_contents, virtual_url, security_info);
  bubble->GetWidget()->Show();
  KeepBubbleAnchored(bubble, GetPageInfoDecoration(parent_window));
}

void ShowBookmarkBubbleViewsAtPoint(const gfx::Point& anchor_point,
                                    gfx::NativeView parent,
                                    bookmarks::BookmarkBubbleObserver* observer,
                                    Browser* browser,
                                    const GURL& virtual_url,
                                    bool already_bookmarked,
                                    LocationBarDecoration* decoration) {
  // The Views dialog may prompt for sign in.
  std::unique_ptr<BubbleSyncPromoDelegate> delegate(
      new BookmarkBubbleSignInDelegate(browser));

  BookmarkBubbleView::ShowBubble(
      nullptr, gfx::Rect(anchor_point, gfx::Size()), parent, observer,
      std::move(delegate), browser->profile(), virtual_url, already_bookmarked);

  views::BubbleDialogDelegateView* bubble =
      BookmarkBubbleView::bookmark_bubble();
  KeepBubbleAnchored(bubble, decoration);
}

std::unique_ptr<BubbleUi> BuildViewsExtensionInstalledBubbleUi(
    ExtensionInstalledBubble* bubble) {
  return std::make_unique<ExtensionInstalledBubbleUi>(bubble);
}

void ShowZoomBubbleViewsAtPoint(content::WebContents* web_contents,
                                const gfx::Point& anchor_point,
                                bool user_action,
                                LocationBarDecoration* decoration) {
  ZoomBubbleView::ShowBubble(web_contents, anchor_point,
                             user_action
                                 ? LocationBarBubbleDelegateView::USER_GESTURE
                                 : LocationBarBubbleDelegateView::AUTOMATIC);
  if (ZoomBubbleView::GetZoomBubble())
    KeepBubbleAnchored(ZoomBubbleView::GetZoomBubble(), decoration);
}

void CloseZoomBubbleViews() {
  ZoomBubbleView::CloseCurrentBubble();
}

void RefreshZoomBubbleViews() {
  if (ZoomBubbleView::GetZoomBubble())
    ZoomBubbleView::GetZoomBubble()->Refresh();
}

bool IsZoomBubbleViewsShown() {
  return ZoomBubbleView::GetZoomBubble() != nullptr;
}

gfx::NativeWindow ContentSettingBubbleViewsBridge::Show(
    gfx::NativeView parent_view,
    ContentSettingBubbleModel* model,
    content::WebContents* web_contents,
    const gfx::Point& anchor,
    LocationBarDecoration* decoration) {
  ContentSettingBubbleContents* contents = new ContentSettingBubbleContents(
      model, web_contents, nullptr, views::BubbleBorder::Arrow::TOP_RIGHT);
  contents->set_parent_window(parent_view);
  contents->SetAnchorRect(gfx::Rect(anchor, gfx::Size()));
  views::Widget* widget =
      views::BubbleDialogDelegateView::CreateBubble(contents);
  widget->Show();
  KeepBubbleAnchored(contents, decoration);
  return widget->GetNativeWindow();
}

void ShowUpdateChromeDialogViews(gfx::NativeWindow parent) {
  UpdateRecommendedMessageBox::Show(parent);
}

void ShowImportLockDialogViews(gfx::NativeWindow parent,
                               const base::Callback<void(bool)>& callback) {
  return ImportLockDialogView::Show(parent, callback);
}

void ShowPasswordReuseWarningDialog(
    content::WebContents* web_contents,
    safe_browsing::ChromePasswordProtectionService* service,
    safe_browsing::OnWarningDone done_callback) {
  safe_browsing::PasswordReuseModalWarningDialog* dialog =
      new safe_browsing::PasswordReuseModalWarningDialog(
          web_contents, service, std::move(done_callback));
  constrained_window::CreateBrowserModalDialogViews(
      dialog, web_contents->GetTopLevelNativeWindow())
      ->Show();
}

}  // namespace chrome
