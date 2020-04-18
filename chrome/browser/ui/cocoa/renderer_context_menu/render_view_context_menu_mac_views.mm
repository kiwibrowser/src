// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/renderer_context_menu/render_view_context_menu_mac_views.h"

#include "components/renderer_context_menu/views/toolkit_delegate_views.h"
#import "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/gfx/mac/coordinate_conversion.h"

class ToolkitDelegateViewsMac : public ToolkitDelegateViews {
 public:
  explicit ToolkitDelegateViewsMac(RenderViewContextMenuMacViews* context_menu)
      : context_menu_(context_menu) {}

  ~ToolkitDelegateViewsMac() override {}

 private:
  // ToolkitDelegateViews:
  void Init(ui::SimpleMenuModel* menu_model) override {
    context_menu_->InitToolkitMenu();
    ToolkitDelegateViews::Init(menu_model);
  }

  RenderViewContextMenuMacViews* context_menu_;
  DISALLOW_COPY_AND_ASSIGN(ToolkitDelegateViewsMac);
};

RenderViewContextMenuMacViews::RenderViewContextMenuMacViews(
    content::RenderFrameHost* render_frame_host,
    const content::ContextMenuParams& params,
    NSView* parent_view)
    : RenderViewContextMenuMac(render_frame_host, params),
      parent_view_(parent_view) {
  auto delegate = std::make_unique<ToolkitDelegateViewsMac>(this);
  set_toolkit_delegate(std::move(delegate));
}

RenderViewContextMenuMacViews::~RenderViewContextMenuMacViews() {}

void RenderViewContextMenuMacViews::Show() {
  NSPoint position =
      NSMakePoint(params().x, NSHeight([parent_view_ bounds]) - params().y);
  position = [parent_view_ convertPoint:position toView:nil];

  gfx::Point menu_point = gfx::ScreenPointFromNSPoint(
      ui::ConvertPointFromWindowToScreen([parent_view_ window], position));

  static_cast<ToolkitDelegateViews*>(toolkit_delegate())
      ->RunMenuAt(nullptr, menu_point, params().source_type);
}
