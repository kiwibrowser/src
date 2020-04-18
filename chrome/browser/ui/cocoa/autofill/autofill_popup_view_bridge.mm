// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "chrome/browser/ui/cocoa/autofill/autofill_popup_view_bridge.h"

#include "base/logging.h"
#include "build/buildflag.h"
#include "chrome/browser/ui/autofill/autofill_popup_controller.h"
#include "chrome/browser/ui/autofill/autofill_popup_layout_model.h"
#include "chrome/browser/ui/autofill/autofill_popup_view_delegate.h"
#import "chrome/browser/ui/cocoa/autofill/autofill_popup_view_cocoa.h"
#include "ui/base/ui_features.h"
#include "ui/gfx/geometry/rect.h"

namespace autofill {

AutofillPopupViewBridge::AutofillPopupViewBridge(
    AutofillPopupController* controller)
    : controller_(controller) {
  view_.reset([[AutofillPopupViewCocoa alloc] initWithController:controller
                                                           frame:NSZeroRect
                                                        delegate:this]);
}

AutofillPopupViewBridge::~AutofillPopupViewBridge() {
  [view_ controllerDestroyed];
  [view_ hidePopup];
}

gfx::Rect AutofillPopupViewBridge::GetRowBounds(int index) {
  return controller_->layout_model().GetRowBounds(index);
}

int AutofillPopupViewBridge::GetIconResourceID(
    const base::string16& resource_name) {
  return controller_->layout_model().GetIconResourceID(resource_name);
}

void AutofillPopupViewBridge::Hide() {
  delete this;
}

void AutofillPopupViewBridge::Show() {
  [view_ showPopup];
}

void AutofillPopupViewBridge::OnSelectedRowChanged(
    base::Optional<int> previous_row_selection,
    base::Optional<int> current_row_selection) {
  if (previous_row_selection) {
    [view_ invalidateRow:*previous_row_selection];
  }

  if (current_row_selection) {
    [view_ invalidateRow:*current_row_selection];
  }
}

void AutofillPopupViewBridge::OnSuggestionsChanged() {
  [view_ updateBoundsAndRedrawPopup];
}

AutofillPopupView* AutofillPopupView::CreateCocoa(
    AutofillPopupController* controller) {
  return new AutofillPopupViewBridge(controller);
}

}  // namespace autofill
