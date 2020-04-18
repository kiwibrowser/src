// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/window_activity_tracker_mac.h"

#include <AppKit/AppKit.h>
#include <CoreFoundation/CoreFoundation.h>

#include "base/bind.h"
#include "ui/base/cocoa/tracking_area.h"

@implementation MouseTracker

- (instancetype)initWithView:(NSView*)nsView {
  self = [super init];
  // TODO(isheriff): why are there no pressed/released events ?
  NSTrackingAreaOptions trackingOptions =
      NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited |
      NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect;
  trackingArea_.reset([[CrTrackingArea alloc] initWithRect:NSZeroRect
                                                   options:trackingOptions
                                                     owner:self
                                                  userInfo:nil]);
  [nsView addTrackingArea:trackingArea_.get()];
  nsView_ = nsView;
  return self;
}

- (void)stopTracking {
  if (trackingArea_.get()) {
    [nsView_ removeTrackingArea:trackingArea_.get()];
    trackingArea_.reset();
  }
}

- (void)registerMouseInteractionObserver:(const base::Closure&)observer {
  mouseInteractionObserver_ = observer;
}

- (void)mouseMoved:(NSEvent*)theEvent {
  mouseInteractionObserver_.Run();
}

- (void)mouseEntered:(NSEvent*)theEvent {
}

- (void)mouseExited:(NSEvent*)theEvent {
}

@end

namespace content {

// static
std::unique_ptr<WindowActivityTracker> WindowActivityTracker::Create(
    gfx::NativeView view) {
  return std::unique_ptr<WindowActivityTracker>(
      new WindowActivityTrackerMac(view));
}

WindowActivityTrackerMac::WindowActivityTrackerMac(NSView* view)
    : weak_factory_(this) {
  mouse_tracker_.reset([[MouseTracker alloc] initWithView:view]);
  [mouse_tracker_ registerMouseInteractionObserver:
                      base::Bind(&WindowActivityTrackerMac::OnMouseActivity,
                                 base::Unretained(this))];
}

WindowActivityTrackerMac::~WindowActivityTrackerMac() {
  [mouse_tracker_ stopTracking];
}

void WindowActivityTrackerMac::OnMouseActivity() {
  WindowActivityTracker::OnMouseActivity();
}

base::WeakPtr<WindowActivityTracker> WindowActivityTrackerMac::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace content
