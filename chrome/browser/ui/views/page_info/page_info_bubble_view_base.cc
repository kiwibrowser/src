// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/page_info/page_info_bubble_view_base.h"

#include "base/strings/string16.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/page_info/page_info_dialog.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/ui_features.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace {

// NOTE(jdonnelly): The following two process-wide variables assume that there's
// never more than one page info bubble shown and that it's associated with the
// current window. If this assumption fails in the future, we'll need to return
// a weak pointer from ShowBubble so callers can associate it with the current
// window (or other context) and check if the bubble they care about is showing.
PageInfoBubbleViewBase::BubbleType g_shown_bubble_type =
    PageInfoBubbleViewBase::BUBBLE_NONE;
PageInfoBubbleViewBase* g_page_info_bubble = nullptr;

}  // namespace

// static
PageInfoBubbleViewBase::BubbleType
PageInfoBubbleViewBase::GetShownBubbleType() {
  return g_shown_bubble_type;
}

// static
views::BubbleDialogDelegateView* PageInfoBubbleViewBase::GetPageInfoBubble() {
  return g_page_info_bubble;
}

PageInfoBubbleViewBase::PageInfoBubbleViewBase(
    views::View* anchor_view,
    const gfx::Rect& anchor_rect,
    gfx::NativeView parent_window,
    PageInfoBubbleViewBase::BubbleType type,
    content::WebContents* web_contents)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::TOP_LEFT),
      content::WebContentsObserver(web_contents) {
  g_shown_bubble_type = type;
  g_page_info_bubble = this;

  set_parent_window(parent_window);
  if (!anchor_view)
    SetAnchorRect(anchor_rect);

  // Compensate for built-in vertical padding in the anchor view's image.
  set_anchor_view_insets(gfx::Insets(
      GetLayoutConstant(LOCATION_BAR_BUBBLE_ANCHOR_VERTICAL_INSET), 0));
}

int PageInfoBubbleViewBase::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_NONE;
}

base::string16 PageInfoBubbleViewBase::GetWindowTitle() const {
  return window_title_;
}

bool PageInfoBubbleViewBase::ShouldShowCloseButton() const {
  return true;
}

void PageInfoBubbleViewBase::OnWidgetDestroying(views::Widget* widget) {
  BubbleDialogDelegateView::OnWidgetDestroying(widget);
  g_shown_bubble_type = BUBBLE_NONE;
  g_page_info_bubble = nullptr;
}

void PageInfoBubbleViewBase::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host == web_contents()->GetMainFrame()) {
    GetWidget()->Close();
  }
}

void PageInfoBubbleViewBase::OnVisibilityChanged(
    content::Visibility visibility) {
  if (visibility == content::Visibility::HIDDEN)
    GetWidget()->Close();
}

void PageInfoBubbleViewBase::DidStartNavigation(
    content::NavigationHandle* handle) {
  if (handle->IsInMainFrame())
    GetWidget()->Close();
}
