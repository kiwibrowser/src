// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/pointer_lock_browsertest.h"

#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_mac.h"
#include "content/browser/web_contents/web_contents_view_mac.h"

namespace content {

namespace {

class MockRenderWidgetHostView : public RenderWidgetHostViewMac {
 public:
  MockRenderWidgetHostView(RenderWidgetHost* host, bool is_guest_view_hack)
      : RenderWidgetHostViewMac(host, is_guest_view_hack) {}
  ~MockRenderWidgetHostView() override {
    if (mouse_locked_)
      UnlockMouse();
  }

  bool LockMouse() override {
    mouse_locked_ = true;

    return true;
  }

  void UnlockMouse() override {
    if (RenderWidgetHostImpl* host =
            RenderWidgetHostImpl::From(GetRenderWidgetHost())) {
      host->LostMouseLock();
    }
    mouse_locked_ = false;
  }

  bool IsMouseLocked() override { return mouse_locked_; }

  bool HasFocus() const override { return true; }
};

}  // namespace

void InstallCreateHooksForPointerLockBrowserTests() {
  WebContentsViewMac::InstallCreateHookForTests(
      [](RenderWidgetHost* host,
         bool is_guest_view_hack) -> RenderWidgetHostViewMac* {
        return new MockRenderWidgetHostView(host, is_guest_view_hack);
      });
}

}  // namespace content
