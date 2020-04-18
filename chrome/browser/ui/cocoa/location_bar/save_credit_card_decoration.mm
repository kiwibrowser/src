// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/location_bar/save_credit_card_decoration.h"

#include "chrome/app/chrome_command_ids.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/command_updater.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"

SaveCreditCardDecoration::SaveCreditCardDecoration(
    CommandUpdater* command_updater)
    : command_updater_(command_updater) {
}

SaveCreditCardDecoration::~SaveCreditCardDecoration() {}

void SaveCreditCardDecoration::SetIcon(bool locationBarIsDark) {
  SkColor theColor = theColor =
      locationBarIsDark ? SK_ColorWHITE : gfx::kChromeIconGrey;

  SetImage(NSImageFromImageSkia(
      gfx::CreateVectorIcon(kCreditCardIcon, 16, theColor)));
}

NSPoint SaveCreditCardDecoration::GetBubblePointInFrame(NSRect frame) {
  const NSRect draw_frame = GetDrawRectInFrame(frame);
  return NSMakePoint(NSMidX(draw_frame), NSMaxY(draw_frame));
}

AcceptsPress SaveCreditCardDecoration::AcceptsMousePress() {
  return AcceptsPress::ALWAYS;
}

bool SaveCreditCardDecoration::OnMousePressed(NSRect frame, NSPoint location) {
  command_updater_->ExecuteCommand(IDC_SAVE_CREDIT_CARD_FOR_PAGE);
  return true;
}

NSString* SaveCreditCardDecoration::GetToolTip() {
  return l10n_util::GetNSStringWithFixup(IDS_TOOLTIP_SAVE_CREDIT_CARD);
}
