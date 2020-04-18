// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BUBBLE_ANCHOR_HELPER_VIEWS_H_
#define CHROME_BROWSER_UI_COCOA_BUBBLE_ANCHOR_HELPER_VIEWS_H_

#include "ui/gfx/native_widget_types.h"

namespace views {
class BubbleDialogDelegateView;
}

class LocationBarDecoration;

// Returns the manage password icon decoration in the omnibox.
LocationBarDecoration* GetManagePasswordDecoration(gfx::NativeWindow window);

// Returns the page info decoration in the omnibox.
LocationBarDecoration* GetPageInfoDecoration(gfx::NativeWindow window);

// Monitors |bubble|'s parent window for size changes, and updates the bubble
// anchor. The monitor will be deleted when |bubble| is closed. If |decoration|
// is provided, the decoration will be set to active in this function. It will
// be set to inactive when |bubble| is closed.
void KeepBubbleAnchored(views::BubbleDialogDelegateView* bubble,
                        LocationBarDecoration* decoration = nullptr);

// Simplified version of KeepBubbleAnchored() for bubbles that manage their own
// anchoring. Causes |decoration| to be set active until |bubble| is closed.
void TrackBubbleState(views::BubbleDialogDelegateView* bubble,
                      LocationBarDecoration* decoration);

#endif  // CHROME_BROWSER_UI_COCOA_BUBBLE_ANCHOR_HELPER_VIEWS_H_
