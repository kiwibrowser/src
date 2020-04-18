// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CERTIFICATE_VIEWER_MAC_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_CERTIFICATE_VIEWER_MAC_COCOA_H_

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/certificate_viewer_mac.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet.h"

// Certificate viewer class for the Mac Cocoa build which handles displaying and
// closing the Cocoa certificate viewer.
@interface SSLCertificateViewerCocoa
    : SSLCertificateViewerMac<ConstrainedWindowSheet>
@end

#endif  // CHROME_BROWSER_UI_COCOA_CERTIFICATE_VIEWER_MAC_COCOA_H_
