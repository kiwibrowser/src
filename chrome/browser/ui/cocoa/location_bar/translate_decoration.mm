// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/location_bar/translate_decoration.h"

#include "chrome/app/chrome_command_ids.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/command_updater.h"
#import "chrome/browser/ui/cocoa/omnibox/omnibox_view_mac.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"

TranslateDecoration::TranslateDecoration(CommandUpdater* command_updater)
    : command_updater_(command_updater) {
  SetLit(false, false);
}

TranslateDecoration::~TranslateDecoration() {}

void TranslateDecoration::SetLit(bool on, bool location_bar_is_dark) {
  SetImage(GetMaterialIcon(location_bar_is_dark));
}

NSPoint TranslateDecoration::GetBubblePointInFrame(NSRect frame) {
  const NSRect draw_frame = GetDrawRectInFrame(frame);
  return NSMakePoint(NSMidX(draw_frame), NSMaxY(draw_frame));
}

AcceptsPress TranslateDecoration::AcceptsMousePress() {
  return AcceptsPress::ALWAYS;
}

bool TranslateDecoration::OnMousePressed(NSRect frame, NSPoint location) {
  command_updater_->ExecuteCommand(IDC_TRANSLATE_PAGE);
  return true;
}

NSString* TranslateDecoration::GetToolTip() {
  return l10n_util::GetNSStringWithFixup(IDS_TOOLTIP_TRANSLATE);
}

const gfx::VectorIcon* TranslateDecoration::GetMaterialVectorIcon() const {
  return &kTranslateIcon;
}
