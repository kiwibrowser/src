// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "content/shell/browser/renderer_host/shell_render_widget_host_view_mac_delegate.h"

#include "base/command_line.h"
#include "content/shell/common/shell_switches.h"

@interface ShellRenderWidgetHostViewMacDelegate () {
  BOOL drop_events_;
}
@end

@implementation ShellRenderWidgetHostViewMacDelegate

- (id)init {
  if ((self = [super init])) {
    // Throw out all native input events if we are running with layout test
    // enabled.
    drop_events_ = switches::IsRunWebTestsSwitchPresent();
  }
  return self;
}
- (BOOL)handleEvent:(NSEvent*)event {
  return drop_events_;
}

- (void)beginGestureWithEvent:(NSEvent*)event {
}

- (void)endGestureWithEvent:(NSEvent*)event {
}

- (void)touchesMovedWithEvent:(NSEvent*)event {
}

- (void)touchesBeganWithEvent:(NSEvent*)event {
}

- (void)touchesCancelledWithEvent:(NSEvent*)event {
}

- (void)touchesEndedWithEvent:(NSEvent*)event {
}

- (void)rendererHandledWheelEvent:(const blink::WebMouseWheelEvent&)event
                         consumed:(BOOL)consumed {
}

- (void)rendererHandledGestureScrollEvent:(const blink::WebGestureEvent&)event
                                 consumed:(BOOL)consumed {
}

- (void)rendererHandledOverscrollEvent:(const ui::DidOverscrollParams&)params {
}

@end
