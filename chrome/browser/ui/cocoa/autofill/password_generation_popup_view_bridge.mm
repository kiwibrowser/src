// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "chrome/browser/ui/cocoa/autofill/password_generation_popup_view_bridge.h"

#include "base/logging.h"
#include "chrome/browser/ui/autofill/autofill_popup_controller.h"
#import "chrome/browser/ui/cocoa/autofill/password_generation_popup_view_cocoa.h"
#include "ui/base/ui_features.h"
#include "ui/gfx/geometry/size.h"

namespace autofill {

PasswordGenerationPopupViewBridge::PasswordGenerationPopupViewBridge(
    PasswordGenerationPopupController* controller) {
  view_.reset(
      [[PasswordGenerationPopupViewCocoa alloc]
          initWithController:controller
                       frame:NSZeroRect]);
}

PasswordGenerationPopupViewBridge::~PasswordGenerationPopupViewBridge() {
  [view_ controllerDestroyed];
  [view_ hidePopup];
}

void PasswordGenerationPopupViewBridge::Hide() {
  delete this;
}

void PasswordGenerationPopupViewBridge::Show() {
  [view_ showPopup];
}

gfx::Size PasswordGenerationPopupViewBridge::GetPreferredSizeOfPasswordView() {
  return gfx::Size(NSSizeToCGSize([view_ preferredSize]));
}

void PasswordGenerationPopupViewBridge::UpdateBoundsAndRedrawPopup() {
  [view_ updateBoundsAndRedrawPopup];
}

void PasswordGenerationPopupViewBridge::PasswordSelectionUpdated() {
  [view_ setNeedsDisplay:YES];
}

bool PasswordGenerationPopupViewBridge::IsPointInPasswordBounds(
    const gfx::Point& point) {
  return [view_ isPointInPasswordBounds:NSPointFromCGPoint(point.ToCGPoint())];
}

PasswordGenerationPopupView* PasswordGenerationPopupView::CreateCocoa(
    PasswordGenerationPopupController* controller) {
  return new PasswordGenerationPopupViewBridge(controller);
}

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
PasswordGenerationPopupView* PasswordGenerationPopupView::Create(
    PasswordGenerationPopupController* controller) {
  return CreateCocoa(controller);
}
#endif

}  // namespace autofill
