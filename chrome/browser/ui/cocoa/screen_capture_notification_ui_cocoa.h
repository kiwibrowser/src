// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_SCREEN_CAPTURE_NOTIFICATION_UI_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_SCREEN_CAPTURE_NOTIFICATION_UI_COCOA_H_

#import <Cocoa/Cocoa.h>

#include <string>

#include "base/callback.h"
#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/browser/ui/screen_capture_notification_ui.h"

// Controller for the screen capture notification window which allows the user
// to quickly stop screen capturing.
@interface ScreenCaptureNotificationController
    : NSWindowController<NSWindowDelegate> {
 @private
  base::Closure stop_callback_;
  base::scoped_nsobject<NSButton> stopButton_;
  base::scoped_nsobject<NSButton> minimizeButton_;
}

- (id)initWithCallback:(const base::Closure&)stop_callback
                  text:(const base::string16&)text;
- (void)stopSharing:(id)sender;
- (void)minimize:(id)sender;

@end

class ScreenCaptureNotificationUICocoa : public ScreenCaptureNotificationUI {
 public:
  explicit ScreenCaptureNotificationUICocoa(const base::string16& text);
  ~ScreenCaptureNotificationUICocoa() override;

  // ScreenCaptureNotificationUI interface.
  gfx::NativeViewId OnStarted(const base::Closure& stop_callback) override;

 private:
  friend class ScreenCaptureNotificationUICocoaTest;

  const base::string16 text_;
  base::scoped_nsobject<ScreenCaptureNotificationController> windowController_;

  DISALLOW_COPY_AND_ASSIGN(ScreenCaptureNotificationUICocoa);
};

#endif  // CHROME_BROWSER_UI_COCOA_SCREEN_CAPTURE_NOTIFICATION_UI_COCOA_H_
