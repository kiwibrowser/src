// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/certificate_viewer_mac_cocoa.h"

#include "base/logging.h"
#import "base/mac/foundation_util.h"
#include "chrome/browser/certificate_viewer.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_mac.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet_controller.h"

@interface SSLCertificateViewerCocoa ()
- (instancetype)initWithCertificate:(net::X509Certificate*)certificate
                     forWebContents:(content::WebContents*)webContents;
- (void)onConstrainedWindowClosed;
@end

namespace {

class SSLCertificateViewerCocoaBridge final
    : public ConstrainedWindowMacDelegate {
 public:
  explicit SSLCertificateViewerCocoaBridge(
      SSLCertificateViewerCocoa* controller)
      : controller_(controller) {}

  // ConstrainedWindowMacDelegate:
  void OnConstrainedWindowClosed(ConstrainedWindowMac* window) override {
    // |onConstrainedWindowClosed| will delete the sheet which might still be
    // in use higher up the call stack. Wait for the next cycle of the event
    // loop to call this function.
    [controller_ performSelector:@selector(onConstrainedWindowClosed)
                      withObject:nil
                      afterDelay:0];
  }

 private:
  SSLCertificateViewerCocoa* controller_;  // Weak. Owns this.

  DISALLOW_COPY_AND_ASSIGN(SSLCertificateViewerCocoaBridge);
};

}  // namespace

@implementation SSLCertificateViewerCocoa {
  std::unique_ptr<SSLCertificateViewerCocoaBridge> observer_;
  std::unique_ptr<ConstrainedWindowMac> constrainedWindow_;
  base::scoped_nsobject<NSWindow> overlayWindow_;
  // A copy of the sheet's frame. Used to restore on show.
  NSRect oldSheetFrame_;
  BOOL closePending_;
  // A copy of the sheet's |autoresizesSubviews| flag to restore on show.
  BOOL oldResizesSubviews_;
}

- (instancetype)initWithCertificate:(net::X509Certificate*)certificate
                     forWebContents:(content::WebContents*)webContents {
  if ((self = [super initWithCertificate:certificate
                          forWebContents:webContents])) {
    observer_.reset(new SSLCertificateViewerCocoaBridge(self));
    constrainedWindow_ =
        CreateAndShowWebModalDialogMac(observer_.get(), webContents, self);
  }
  return self;
}

- (void)onConstrainedWindowClosed {
  [self releaseSheetWindow];
  constrainedWindow_.reset();
  [self release];
}

// SSLCertificateViewerMac overrides:

- (void)sheetDidEnd:(NSWindow*)parent
         returnCode:(NSInteger)returnCode
            context:(void*)context {
  if (!closePending_)
    constrainedWindow_->CloseWebContentsModalDialog();
}

// ConstrainedWindowSheet protocol implementation.

- (void)showSheetForWindow:(NSWindow*)window {
  overlayWindow_.reset([window retain]);
  [self showCertificateSheet:window];
}

- (void)closeSheetWithAnimation:(BOOL)withAnimation {
  closePending_ = YES;
  overlayWindow_.reset();
  [self closeCertificateSheet];
}

- (void)hideSheet {
  NSWindow* sheetWindow = [overlayWindow_ attachedSheet];
  [sheetWindow setAlphaValue:0.0];
  [sheetWindow setIgnoresMouseEvents:YES];

  oldResizesSubviews_ = [[sheetWindow contentView] autoresizesSubviews];
  [[sheetWindow contentView] setAutoresizesSubviews:NO];
}

- (void)unhideSheet {
  NSWindow* sheetWindow = [overlayWindow_ attachedSheet];
  [sheetWindow setIgnoresMouseEvents:NO];

  [[sheetWindow contentView] setAutoresizesSubviews:oldResizesSubviews_];
  [[overlayWindow_ attachedSheet] setAlphaValue:1.0];
}

- (void)pulseSheet {
  // NOOP
}

- (void)makeSheetKeyAndOrderFront {
  [[overlayWindow_ attachedSheet] makeKeyAndOrderFront:nil];
}

- (void)updateSheetPosition {
  // NOOP
}

- (void)resizeWithNewSize:(NSSize)preferredSize {
  // NOOP
}

- (NSWindow*)sheetWindow {
  return [self certificatePanel];
}

@end

void ShowCertificateViewer(content::WebContents* web_contents,
                           gfx::NativeWindow parent,
                           net::X509Certificate* cert) {
  // SSLCertificateViewerCocoa will manage its own lifetime and will release
  // itself when the dialog is closed.
  // See -[SSLCertificateViewerCocoa onConstrainedWindowClosed].
  [[SSLCertificateViewerCocoa alloc] initWithCertificate:cert
                                          forWebContents:web_contents];
}
