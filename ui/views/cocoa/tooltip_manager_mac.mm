// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/cocoa/tooltip_manager_mac.h"

#include "ui/base/cocoa/cocoa_base_utils.h"
#include "ui/gfx/font_list.h"
#import "ui/gfx/mac/coordinate_conversion.h"
#import "ui/views/cocoa/bridged_content_view.h"
#import "ui/views/cocoa/bridged_native_widget.h"

namespace {

// Max visual tooltip width in DIPs. Beyond this, Cocoa will wrap text.
const int kTooltipMaxWidthPixels = 250;

}  // namespace

namespace views {

TooltipManagerMac::TooltipManagerMac(BridgedNativeWidget* widget)
    : widget_(widget) {
}

TooltipManagerMac::~TooltipManagerMac() {
}

int TooltipManagerMac::GetMaxWidth(const gfx::Point& location) const {
  return kTooltipMaxWidthPixels;
}

const gfx::FontList& TooltipManagerMac::GetFontList() const {
  CR_DEFINE_STATIC_LOCAL(gfx::FontList, font_list,
                         (gfx::Font([NSFont toolTipsFontOfSize:0])));
  return font_list;
}

void TooltipManagerMac::UpdateTooltip() {
  NSWindow* window = widget_->ns_window();
  BridgedContentView* view = widget_->ns_view();

  NSPoint nspoint =
      ui::ConvertPointFromScreenToWindow(window, [NSEvent mouseLocation]);
  // Note: flip in the view's frame, which matches the window's contentRect.
  gfx::Point point(nspoint.x, NSHeight([view frame]) - nspoint.y);
  [view updateTooltipIfRequiredAt:point];
}

void TooltipManagerMac::TooltipTextChanged(View* view) {
  // The intensive part is View::GetTooltipHandlerForPoint(), which will be done
  // in [BridgedContentView updateTooltipIfRequiredAt:]. Don't do it here as
  // well.
  UpdateTooltip();
}

}  // namespace views
