// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"

#include "build/build_config.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/render_view_host.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/accessibility/view_accessibility.h"

LocationBarBubbleDelegateView::WebContentMouseHandler::WebContentMouseHandler(
    LocationBarBubbleDelegateView* bubble,
    content::WebContents* web_contents)
    : bubble_(bubble), web_contents_(web_contents) {
  DCHECK(bubble_);
  DCHECK(web_contents_);
  event_monitor_ = views::EventMonitor::CreateWindowMonitor(
      this, web_contents_->GetTopLevelNativeWindow());
}

LocationBarBubbleDelegateView::WebContentMouseHandler::
    ~WebContentMouseHandler() {}

void LocationBarBubbleDelegateView::WebContentMouseHandler::OnKeyEvent(
    ui::KeyEvent* event) {
  if ((event->key_code() == ui::VKEY_ESCAPE ||
       web_contents_->IsFocusedElementEditable()) &&
      event->type() == ui::ET_KEY_PRESSED)
    bubble_->CloseBubble();
}

void LocationBarBubbleDelegateView::WebContentMouseHandler::OnMouseEvent(
    ui::MouseEvent* event) {
  if (event->type() == ui::ET_MOUSE_PRESSED)
    bubble_->CloseBubble();
}

void LocationBarBubbleDelegateView::WebContentMouseHandler::OnTouchEvent(
    ui::TouchEvent* event) {
  if (event->type() == ui::ET_TOUCH_PRESSED)
    bubble_->CloseBubble();
}

LocationBarBubbleDelegateView::LocationBarBubbleDelegateView(
    views::View* anchor_view,
    const gfx::Point& anchor_point,
    content::WebContents* web_contents)
    : BubbleDialogDelegateView(anchor_view,
                               anchor_view ? views::BubbleBorder::TOP_RIGHT
                                           : views::BubbleBorder::NONE),
      WebContentsObserver(web_contents) {
  // Add observer to close the bubble if the fullscreen state changes.
  if (web_contents) {
    Browser* browser = chrome::FindBrowserWithWebContents(web_contents);
    registrar_.Add(
        this, chrome::NOTIFICATION_FULLSCREEN_CHANGED,
        content::Source<FullscreenController>(
            browser->exclusive_access_manager()->fullscreen_controller()));
  }
  if (!anchor_view)
    SetAnchorRect(gfx::Rect(anchor_point, gfx::Size()));

  // Compensate for built-in vertical padding in the anchor view's image.
  // In the case of Harmony, this is just compensating for the location bar's
  // border thickness, as the bubble's top border should overlap it.
  // When anchor is controlled by the |anchor_point| this inset is ignored.
  set_anchor_view_insets(gfx::Insets(
      GetLayoutConstant(LOCATION_BAR_BUBBLE_ANCHOR_VERTICAL_INSET), 0));
}

LocationBarBubbleDelegateView::~LocationBarBubbleDelegateView() {}

void LocationBarBubbleDelegateView::ShowForReason(DisplayReason reason) {
  if (reason == USER_GESTURE) {
#if defined(OS_MACOSX)
    // In the USER_GESTURE case, the icon will be in an active state so the
    // bubble doesn't need an arrow (except on non-MD MacViews).
    const bool hide_arrow =
        ui::MaterialDesignController::IsSecondaryUiMaterial();
#else
    const bool hide_arrow = true;
#endif
    if (hide_arrow)
      SetArrowPaintType(views::BubbleBorder::PAINT_TRANSPARENT);
    GetWidget()->Show();
  } else {
    GetWidget()->ShowInactive();

    // Since this widget is inactive (but shown), accessibility tools won't
    // alert the user to its presence. Accessibility tools such as screen
    // readers work by tracking system focus. Give users of these tools a hint
    // description and alert them to the presence of this widget.
    GetWidget()->GetRootView()->GetViewAccessibility().OverrideDescription(
        l10n_util::GetStringUTF8(IDS_SHOW_BUBBLE_INACTIVE_DESCRIPTION));
  }
  GetWidget()->GetRootView()->NotifyAccessibilityEvent(ax::mojom::Event::kAlert,
                                                       true);
}

void LocationBarBubbleDelegateView::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_FULLSCREEN_CHANGED, type);
  GetWidget()->SetVisibilityAnimationTransition(views::Widget::ANIMATE_NONE);
  CloseBubble();
}

void LocationBarBubbleDelegateView::OnVisibilityChanged(
    content::Visibility visibility) {
  if (visibility == content::Visibility::HIDDEN)
    CloseBubble();
}

void LocationBarBubbleDelegateView::WebContentsDestroyed() {
  CloseBubble();
}

void LocationBarBubbleDelegateView::AdjustForFullscreen(
    const gfx::Rect& screen_bounds) {
  if (GetAnchorView())
    return;

  const int kBubblePaddingFromScreenEdge = 20;
  int horizontal_offset = width() / 2 + kBubblePaddingFromScreenEdge;
  const int x_pos = base::i18n::IsRTL()
                        ? (screen_bounds.x() + horizontal_offset)
                        : (screen_bounds.right() - horizontal_offset);
  SetAnchorRect(gfx::Rect(x_pos, screen_bounds.y(), 0, 0));
}

void LocationBarBubbleDelegateView::CloseBubble() {
  GetWidget()->Close();
}
