// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/cursor_renderer_mac.h"

#include <Cocoa/Cocoa.h>
#include <CoreFoundation/CoreFoundation.h>

#include "base/bind.h"
#include "base/logging.h"
#include "base/mac/mac_util.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/gfx/image/image.h"

@implementation CursorRendererMouseTracker

- (instancetype)initWithView:(NSView*)nsView {
  if ((self = [super init])) {
    NSTrackingAreaOptions trackingOptions =
        NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited |
        NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect;
    trackingArea_.reset([[CrTrackingArea alloc] initWithRect:NSZeroRect
                                                     options:trackingOptions
                                                       owner:self
                                                    userInfo:nil]);
    [nsView addTrackingArea:trackingArea_.get()];
    capturedView_.reset([nsView retain]);
  }
  return self;
}

- (void)stopTracking {
  if (trackingArea_.get()) {
    [capturedView_ removeTrackingArea:trackingArea_.get()];
    trackingArea_.reset();
    capturedView_.reset();
  }
}

- (void)registerMouseInteractionObserver:
    (const base::RepeatingClosure&)observer {
  mouseInteractionObserver_ = observer;
}

- (void)mouseMoved:(NSEvent*)theEvent {
  mouseInteractionObserver_.Run();
}

- (void)mouseEntered:(NSEvent*)theEvent {
  mouseInteractionObserver_.Run();
}

- (void)mouseExited:(NSEvent*)theEvent {
}

@end

namespace content {

// static
std::unique_ptr<CursorRenderer> CursorRenderer::Create(
    CursorRenderer::CursorDisplaySetting display) {
  return std::make_unique<CursorRendererMac>(display);
}

CursorRendererMac::CursorRendererMac(CursorDisplaySetting display)
    : CursorRenderer(display) {}

CursorRendererMac::~CursorRendererMac() {
  SetTargetView(nil);
}

void CursorRendererMac::SetTargetView(NSView* view) {
  if (view_) {
    [mouse_tracker_ stopTracking];
    mouse_tracker_.reset();
  }
  view_ = view;
  OnMouseHasGoneIdle();
  if (view_) {
    mouse_tracker_.reset(
        [[CursorRendererMouseTracker alloc] initWithView:view_]);
    [mouse_tracker_
        registerMouseInteractionObserver:base::BindRepeating(
                                             &CursorRendererMac::OnMouseEvent,
                                             base::Unretained(this))];
  }
}

bool CursorRendererMac::IsCapturedViewActive() {
  if (![[view_ window] isKeyWindow]) {
    return false;
  }
  return true;
}

gfx::Size CursorRendererMac::GetCapturedViewSize() {
  NSRect frame_rect = [view_ bounds];
  return gfx::Size(frame_rect.size.width, frame_rect.size.height);
}

gfx::Point CursorRendererMac::GetCursorPositionInView() {
  // Mouse location in window co-ordinates.
  NSPoint mouse_window_location =
      [view_ window].mouseLocationOutsideOfEventStream;
  // Mouse location with respect to the view within the window.
  NSPoint mouse_view_location =
      [view_ convertPoint:mouse_window_location fromView:nil];

  // Invert y coordinate to unify with Aura.
  gfx::Point cursor_position_in_view(
      mouse_view_location.x,
      GetCapturedViewSize().height() - mouse_view_location.y);

  return cursor_position_in_view;
}

gfx::NativeCursor CursorRendererMac::GetLastKnownCursor() {
  // Grab system cursor.
  return [NSCursor currentSystemCursor];
}

SkBitmap CursorRendererMac::GetLastKnownCursorImage(gfx::Point* hot_point) {
  // Grab system cursor.
  NSCursor* nscursor = [NSCursor currentSystemCursor];
  NSImage* nsimage = [nscursor image];
  NSPoint nshotspot = [nscursor hotSpot];

  *hot_point = gfx::Point(nshotspot.x, nshotspot.y);
  return skia::NSImageToSkBitmapWithColorSpace(
      nsimage, /*is_opaque=*/false, base::mac::GetSystemColorSpace());
}

void CursorRendererMac::OnMouseEvent() {
  // Update cursor movement info to CursorRenderer.
  OnMouseMoved(GetCursorPositionInView());
}

}  // namespace content
