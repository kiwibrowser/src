// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CAPTURE_CURSOR_RENDERER_MAC_H_
#define CONTENT_BROWSER_MEDIA_CAPTURE_CURSOR_RENDERER_MAC_H_

#import <AppKit/AppKit.h>

#include "base/mac/scoped_nsobject.h"
#include "content/browser/media/capture/cursor_renderer.h"
#import "ui/base/cocoa/tracking_area.h"

@interface CursorRendererMouseTracker : NSObject {
 @private
  ui::ScopedCrTrackingArea trackingArea_;

  // The view on which mouse movement is detected.
  base::scoped_nsobject<NSView> capturedView_;

  // Runs on any mouse interaction from user.
  base::RepeatingClosure mouseInteractionObserver_;
}

- (instancetype)initWithView:(NSView*)nsView;

// Register an observer for mouse interaction.
- (void)registerMouseInteractionObserver:
    (const base::RepeatingClosure&)observer;

@end

namespace content {

class CONTENT_EXPORT CursorRendererMac : public CursorRenderer {
 public:
  explicit CursorRendererMac(CursorDisplaySetting cursor_display);
  ~CursorRendererMac() final;

  // CursorRenderer implementation.
  void SetTargetView(gfx::NativeView window) final;
  bool IsCapturedViewActive() final;
  gfx::Size GetCapturedViewSize() final;
  gfx::Point GetCursorPositionInView() final;
  gfx::NativeCursor GetLastKnownCursor() final;
  SkBitmap GetLastKnownCursorImage(gfx::Point* hot_point) final;

 private:
  friend class CursorRendererMacTest;

  // Called for mouse activity events.
  void OnMouseEvent();

  NSView* view_ = nil;

  base::scoped_nsobject<CursorRendererMouseTracker> mouse_tracker_;

  DISALLOW_COPY_AND_ASSIGN(CursorRendererMac);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CAPTURE_CURSOR_RENDERER_MAC_H_
