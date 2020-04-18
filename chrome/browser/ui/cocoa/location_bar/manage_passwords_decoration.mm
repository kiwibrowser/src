// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/location_bar/manage_passwords_decoration.h"

#include "chrome/app/chrome_command_ids.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/command_updater.h"
#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#include "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#include "chrome/browser/ui/cocoa/omnibox/omnibox_view_mac.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "chrome/browser/ui/views/passwords/password_bubble_view_base.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"

// ManagePasswordsIconCocoa

ManagePasswordsIconCocoa::ManagePasswordsIconCocoa(
    ManagePasswordsDecoration* decoration)
    : decoration_(decoration) {
}

ManagePasswordsIconCocoa::~ManagePasswordsIconCocoa() {
}

void ManagePasswordsIconCocoa::UpdateVisibleUI() {
  decoration_->UpdateVisibleUI();
}

void ManagePasswordsIconCocoa::OnChangingState() {
  decoration_->HideBubble();
}

// ManagePasswordsDecoration

ManagePasswordsDecoration::ManagePasswordsDecoration(
    CommandUpdater* command_updater,
    LocationBarViewMac* location_bar)
    : command_updater_(command_updater),
      location_bar_(location_bar),
      icon_(new ManagePasswordsIconCocoa(this)) {
  UpdateUIState();
}

ManagePasswordsDecoration::~ManagePasswordsDecoration() = default;

NSPoint ManagePasswordsDecoration::GetBubblePointInFrame(NSRect frame) {
  const NSRect draw_frame = GetDrawRectInFrame(frame);
  return NSMakePoint(NSMidX(draw_frame), NSMaxY(draw_frame));
}

AcceptsPress ManagePasswordsDecoration::AcceptsMousePress() {
  return AcceptsPress::ALWAYS;
}

bool ManagePasswordsDecoration::OnMousePressed(NSRect frame, NSPoint location) {
  if (IsBubbleShowing())
    HideBubble();
  else
    command_updater_->ExecuteCommand(IDC_MANAGE_PASSWORDS_FOR_PAGE);
  return true;
}

NSString* ManagePasswordsDecoration::GetToolTip() {
  return icon_->tooltip_text_id()
             ? l10n_util::GetNSStringWithFixup(icon_->tooltip_text_id())
             : nil;
}

void ManagePasswordsDecoration::OnChange() {
  // |location_bar_| can be NULL in tests.
  if (location_bar_)
    location_bar_->OnDecorationsChanged();
}

void ManagePasswordsDecoration::UpdateUIState() {
  if (icon_->state() == password_manager::ui::INACTIVE_STATE) {
    SetVisible(false);
    SetImage(nil);
    return;
  }
  SetVisible(true);
  // |location_bar_| can be NULL in tests.
  bool location_bar_is_dark = location_bar_ &&
      [[location_bar_->GetAutocompleteTextField() window]
           inIncognitoModeWithSystemTheme];
  SetImage(GetMaterialIcon(location_bar_is_dark));
}

void ManagePasswordsDecoration::UpdateVisibleUI() {
  UpdateUIState();
  OnChange();
}

void ManagePasswordsDecoration::HideBubble() {
  if (IsBubbleShowing())
    PasswordBubbleViewBase::CloseCurrentBubble();
}

const gfx::VectorIcon* ManagePasswordsDecoration::GetMaterialVectorIcon()
    const {
  // Note: update unit tests if this vector icon ever changes (it's hard-coded
  // there).
  return &kKeyIcon;
}

bool ManagePasswordsDecoration::IsBubbleShowing() {
  return PasswordBubbleViewBase::manage_password_bubble() != nullptr;
}
