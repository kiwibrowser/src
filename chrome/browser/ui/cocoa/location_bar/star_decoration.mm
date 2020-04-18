// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/location_bar/star_decoration.h"

#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/command_updater.h"
#import "chrome/browser/ui/cocoa/omnibox/omnibox_view_mac.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "components/toolbar/vector_icons.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/color_palette.h"

namespace {

// The info-bubble point should look like it points to the point
// between the star's lower tips.  The popup should be where the
// Omnibox popup ends up (2px below field).  Determined via Pixie.app
// magnification.
const CGFloat kStarPointYOffset = 2.0;

}  // namespace

StarDecoration::StarDecoration(CommandUpdater* command_updater)
    : command_updater_(command_updater) {
  SetVisible(true);
  SetStarred(false, false);
}

StarDecoration::~StarDecoration() {
}

void StarDecoration::SetStarred(bool starred, bool location_bar_is_dark) {
  starred_ = starred;
  const int tip_id = starred ? IDS_TOOLTIP_STARRED : IDS_TOOLTIP_STAR;
  SetImage(GetMaterialIcon(location_bar_is_dark));
  tooltip_.reset([l10n_util::GetNSStringWithFixup(tip_id) retain]);
}

NSPoint StarDecoration::GetStarBubblePointInFrame(NSRect draw_frame) {
  return NSMakePoint(NSMidX(draw_frame),
                     NSMaxY(draw_frame) - kStarPointYOffset);
}

AcceptsPress StarDecoration::AcceptsMousePress() {
  return AcceptsPress::ALWAYS;
}

bool StarDecoration::OnMousePressed(NSRect frame, NSPoint location) {
  command_updater_->ExecuteCommand(IDC_BOOKMARK_PAGE);
  return true;
}

NSString* StarDecoration::GetToolTip() {
  return tooltip_.get();
}

NSPoint StarDecoration::GetBubblePointInFrame(NSRect frame) {
  return GetStarBubblePointInFrame(GetDrawRectInFrame(frame));
}

SkColor StarDecoration::GetMaterialIconColor(bool location_bar_is_dark) const {
  if (location_bar_is_dark) {
    return starred_ ? gfx::kGoogleBlue300 : SkColorSetA(SK_ColorWHITE, 0xCC);
  }
  return starred_ ? gfx::kGoogleBlue500 : gfx::kChromeIconGrey;
}

const gfx::VectorIcon* StarDecoration::GetMaterialVectorIcon() const {
  return starred_ ? &toolbar::kStarActiveIcon : &toolbar::kStarIcon;
}
