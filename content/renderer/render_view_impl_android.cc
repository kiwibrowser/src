// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/render_view_impl.h"

#include "base/command_line.h"
#include "cc/trees/layer_tree_host.h"
#include "content/common/view_messages.h"
#include "content/renderer/gpu/render_widget_compositor.h"
#include "third_party/blink/public/web/web_view.h"

namespace content {

// Check content::BrowserControlsState, and
// blink::WebWidget::BrowserControlsState
// are kept in sync.
static_assert(int(BROWSER_CONTROLS_STATE_SHOWN) ==
                  int(blink::kWebBrowserControlsShown),
              "mismatching enums: SHOWN");
static_assert(int(BROWSER_CONTROLS_STATE_HIDDEN) ==
                  int(blink::kWebBrowserControlsHidden),
              "mismatching enums: HIDDEN");
static_assert(int(BROWSER_CONTROLS_STATE_BOTH) ==
                  int(blink::kWebBrowserControlsBoth),
              "mismatching enums: BOTH");

blink::WebBrowserControlsState ContentToBlink(BrowserControlsState state) {
  return static_cast<blink::WebBrowserControlsState>(state);
}

void RenderViewImpl::UpdateBrowserControlsState(
    BrowserControlsState constraints,
    BrowserControlsState current,
    bool animate) {
  if (GetWebWidget())
    GetWebWidget()->UpdateBrowserControlsState(
        ContentToBlink(constraints), ContentToBlink(current), animate);

  top_controls_constraints_ = constraints;
}

void RenderViewImpl::didScrollWithKeyboard(const blink::WebSize& delta) {
  if (delta.height == 0)
    return;

  BrowserControlsState current = delta.height < 0
                                     ? BROWSER_CONTROLS_STATE_SHOWN
                                     : BROWSER_CONTROLS_STATE_HIDDEN;

  UpdateBrowserControlsState(top_controls_constraints_, current, true);
}

}  // namespace content
