// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/certificate_viewer_mac.h"

#import "base/mac/scoped_nsobject.h"
#include "chrome/browser/certificate_viewer.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

// Certificate viewer class for MacViews which handles displaying and closing
// the Cocoa certificate viewer.
@interface SSLCertificateViewerMacViews : SSLCertificateViewerMac {
  // Invisible overlay window used to block interaction with the tab underneath.
  views::Widget* overlayWindow_;
}

- (void)setOverlayWindow:(views::Widget*)overlayWindow;
@end

// A fully transparent, borderless web-modal dialog used to display the
// OS-provided window-modal sheet that displays certificate information.
class CertificateAnchorWidgetDelegate : public views::WidgetDelegateView {
 public:
  CertificateAnchorWidgetDelegate(content::WebContents* web_contents,
                                  net::X509Certificate* cert)
      : certificate_viewer_([[SSLCertificateViewerMacViews alloc]
            initWithCertificate:cert
                 forWebContents:web_contents]) {
    views::Widget* overlayWindow =
        constrained_window::ShowWebModalDialogWithOverlayViews(this,
                                                               web_contents);
    [certificate_viewer_ showCertificateSheet:overlayWindow->GetNativeWindow()];
    [certificate_viewer_ setOverlayWindow:overlayWindow];
  }

  // WidgetDelegate:
  ui::ModalType GetModalType() const override { return ui::MODAL_TYPE_CHILD; }

 private:
  base::scoped_nsobject<SSLCertificateViewerMacViews> certificate_viewer_;

  DISALLOW_COPY_AND_ASSIGN(CertificateAnchorWidgetDelegate);
};

@implementation SSLCertificateViewerMacViews

- (void)sheetDidEnd:(NSWindow*)parent
         returnCode:(NSInteger)returnCode
            context:(void*)context {
  [self closeCertificateSheet];
  overlayWindow_->Close();  // Asynchronously releases |self|.
  [self releaseSheetWindow];
}

- (void)setOverlayWindow:(views::Widget*)overlayWindow {
  overlayWindow_ = overlayWindow;
}

@end
