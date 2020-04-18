// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/translate/translate_bubble_bridge_views.h"

#include "chrome/browser/ui/cocoa/bubble_anchor_helper_views.h"
#include "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#include "chrome/browser/ui/cocoa/location_bar/translate_decoration.h"
#include "chrome/browser/ui/views/translate/translate_bubble_view.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#include "ui/gfx/mac/coordinate_conversion.h"

void ShowTranslateBubbleViews(NSWindow* parent_window,
                              LocationBarViewMac* location_bar,
                              content::WebContents* web_contents,
                              translate::TranslateStep step,
                              translate::TranslateErrors::Type error_type,
                              bool is_user_gesture) {
  gfx::Point anchor_point =
      gfx::ScreenPointFromNSPoint(ui::ConvertPointFromWindowToScreen(
          parent_window, location_bar->GetBubblePointForDecoration(
                             location_bar->translate_decoration())));
  TranslateBubbleView::DisplayReason reason =
      is_user_gesture ? TranslateBubbleView::USER_GESTURE
                      : TranslateBubbleView::AUTOMATIC;
  TranslateBubbleView::ShowBubble(nullptr, anchor_point, web_contents, step,
                                  error_type, reason);
  if (TranslateBubbleView::GetCurrentBubble()) {
    KeepBubbleAnchored(TranslateBubbleView::GetCurrentBubble(),
                       location_bar->translate_decoration());
  }
}
