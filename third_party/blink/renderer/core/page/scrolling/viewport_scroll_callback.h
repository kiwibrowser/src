// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_SCROLLING_VIEWPORT_SCROLL_CALLBACK_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_SCROLLING_VIEWPORT_SCROLL_CALLBACK_H_

#include "third_party/blink/renderer/core/page/scrolling/scroll_state_callback.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/scroll/scroll_types.h"

namespace blink {

class BrowserControls;
class ScrollableArea;
class ScrollState;
class OverscrollController;
class RootFrameViewport;

// ViewportScrollCallback is a ScrollStateCallback, meaning that it's applied
// during the applyScroll step of ScrollCustomization. It implements viewport
// actions like moving browser controls and showing overscroll glow as well as
// scrolling the Element.
//
// ScrollCustomization generally relies on using the nativeApplyScroll to
// scroll the element; however, the rootScroller may need to execute actions
// both before and after the native scroll which is currently unsupported.
// Because of this, the ViewportScrollCallback can scroll the Element directly.
// This is accomplished by passing the ScrollableArea directly using
// setScroller() which RootScrollerController will call to set the appropriate
// ScrollableArea to use.
class ViewportScrollCallback : public ScrollStateCallback {
 public:
  // The BrowserControls and OverscrollController are given to the
  // ViewportScrollCallback but are not owned or kept alive by it.
  static ViewportScrollCallback* Create(
      BrowserControls* browser_controls,
      OverscrollController* overscroll_controller,
      RootFrameViewport& root_frame_viewport) {
    return new ViewportScrollCallback(browser_controls, overscroll_controller,
                                      root_frame_viewport);
  }

  ~ViewportScrollCallback() override;

  void Invoke(ScrollState*) override;
  void SetScroller(ScrollableArea*);

  void Trace(blink::Visitor*) override;

 private:
  // ViewportScrollCallback does not assume ownership of BrowserControls or of
  // OverscrollController.
  ViewportScrollCallback(BrowserControls*,
                         OverscrollController*,
                         RootFrameViewport&);

  bool ShouldScrollBrowserControls(const ScrollOffset&,
                                   ScrollGranularity) const;
  bool ScrollBrowserControls(ScrollState&);

  ScrollResult PerformNativeScroll(ScrollState&);

  WeakMember<BrowserControls> browser_controls_;
  WeakMember<OverscrollController> overscroll_controller_;
  WeakMember<RootFrameViewport> root_frame_viewport_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_SCROLLING_VIEWPORT_SCROLL_CALLBACK_H_
