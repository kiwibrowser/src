// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_SSL_CLIENT_CERTIFICATE_SELECTOR_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_SSL_CLIENT_CERTIFICATE_SELECTOR_COCOA_H_

#import <Cocoa/Cocoa.h>

#include <memory>
#include <vector>

#include "base/mac/scoped_cftyperef.h"
#include "base/mac/scoped_nsobject.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/ssl/ssl_client_certificate_selector.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_sheet.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet_controller.h"
#include "net/ssl/client_cert_identity.h"

namespace content {
class BrowserContext;
class ClientCertificateDelegate;
}

class ConstrainedWindowMac;
@class SFChooseIdentityPanel;
class SSLClientAuthObserverCocoaBridge;

@interface SSLClientCertificateSelectorCocoa
    : NSObject<ConstrainedWindowSheet> {
 @private
  // The list of SecIdentityRefs offered to the user.
  base::ScopedCFTypeRef<CFMutableArrayRef> sec_identities_;
  // The corresponding list of ClientCertIdentities.
  net::ClientCertIdentityList cert_identities_;
  // A C++ object to bridge SSLClientAuthObserver notifications to us.
  std::unique_ptr<SSLClientAuthObserverCocoaBridge> observer_;
  base::scoped_nsobject<SFChooseIdentityPanel> panel_;
  std::unique_ptr<ConstrainedWindowMac> constrainedWindow_;
  base::scoped_nsobject<NSWindow> overlayWindow_;
  BOOL closePending_;
  // A copy of the sheet's |autoresizesSubviews| flag to restore on show.
  BOOL oldResizesSubviews_;
  // True if the user dismissed the dialog directly, either via the OK (continue
  // the request with a certificate) or Cancel (continue the request with no
  // certificate) buttons.
  BOOL userResponded_;
}

@property (readonly, nonatomic) SFChooseIdentityPanel* panel;

- (id)initWithBrowserContext:(const content::BrowserContext*)browserContext
             certRequestInfo:(net::SSLCertRequestInfo*)certRequestInfo
                    delegate:
                        (std::unique_ptr<content::ClientCertificateDelegate>)
                            delegate;
- (void)displayForWebContents:(content::WebContents*)webContents
                  clientCerts:(net::ClientCertIdentityList)inputClientCerts;
- (void)closeWebContentsModalDialog;

- (NSWindow*)overlayWindow;

@end

#endif  // CHROME_BROWSER_UI_COCOA_SSL_CLIENT_CERTIFICATE_SELECTOR_COCOA_H_
