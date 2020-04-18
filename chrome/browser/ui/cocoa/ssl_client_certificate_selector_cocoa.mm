// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/ssl_client_certificate_selector_cocoa.h"

#import <SecurityInterface/SFChooseIdentityPanel.h>
#include <stddef.h>

#include <utility>

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/buildflag.h"
#include "chrome/browser/ssl/ssl_client_auth_observer.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_mac.h"
#include "chrome/grit/generated_resources.h"
#include "components/guest_view/browser/guest_view_base.h"
#include "components/strings/grit/components_strings.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "content/public/browser/web_contents.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util_mac.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "net/ssl/ssl_platform_key_mac.h"
#include "ui/base/cocoa/window_size_constants.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/ui_features.h"

using content::BrowserThread;

@interface SFChooseIdentityPanel (SystemPrivate)
// A system-private interface that dismisses a panel whose sheet was started by
// -beginSheetForWindow:modalDelegate:didEndSelector:contextInfo:identities:message:
// as though the user clicked the button identified by returnCode. Verified
// present in 10.5 through 10.12.
- (void)_dismissWithCode:(NSInteger)code;
@end

@interface SSLClientCertificateSelectorCocoa ()
- (void)onConstrainedWindowClosed;
@end

class SSLClientAuthObserverCocoaBridge : public SSLClientAuthObserver,
                                         public ConstrainedWindowMacDelegate {
 public:
  SSLClientAuthObserverCocoaBridge(
      const content::BrowserContext* browser_context,
      net::SSLCertRequestInfo* cert_request_info,
      std::unique_ptr<content::ClientCertificateDelegate> delegate,
      SSLClientCertificateSelectorCocoa* controller)
      : SSLClientAuthObserver(browser_context,
                              cert_request_info,
                              std::move(delegate)),
        controller_(controller) {}

  // SSLClientAuthObserver implementation:
  void OnCertSelectedByNotification() override {
    [controller_ closeWebContentsModalDialog];
  }

  // ConstrainedWindowMacDelegate implementation:
  void OnConstrainedWindowClosed(ConstrainedWindowMac* window) override {
    // |onConstrainedWindowClosed| will delete the sheet which might be still
    // in use higher up the call stack. Wait for the next cycle of the event
    // loop to call this function.
    [controller_ performSelector:@selector(onConstrainedWindowClosed)
                      withObject:nil
                      afterDelay:0];
  }

 private:
  SSLClientCertificateSelectorCocoa* controller_;  // weak
};

namespace chrome {

void ShowSSLClientCertificateSelectorCocoa(
    content::WebContents* contents,
    net::SSLCertRequestInfo* cert_request_info,
    net::ClientCertIdentityList client_certs,
    std::unique_ptr<content::ClientCertificateDelegate> delegate) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Not all WebContentses can show modal dialogs.
  //
  // Use the top-level embedder if |contents| is a guest.
  // GetTopLevelWebContents() will return |contents| otherwise.
  // TODO(davidben): Move this hook to the WebContentsDelegate and only try to
  // show a dialog in Browser's implementation. https://crbug.com/456255
  if (web_modal::WebContentsModalDialogManager::FromWebContents(
          guest_view::GuestViewBase::GetTopLevelWebContents(contents)) ==
      nullptr)
    return;

  // The dialog manages its own lifetime.
  SSLClientCertificateSelectorCocoa* selector =
      [[SSLClientCertificateSelectorCocoa alloc]
          initWithBrowserContext:contents->GetBrowserContext()
                 certRequestInfo:cert_request_info
                        delegate:std::move(delegate)];
  [selector displayForWebContents:contents clientCerts:std::move(client_certs)];
}

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
void ShowSSLClientCertificateSelector(
    content::WebContents* contents,
    net::SSLCertRequestInfo* cert_request_info,
    net::ClientCertIdentityList client_certs,
    std::unique_ptr<content::ClientCertificateDelegate> delegate) {
  return ShowSSLClientCertificateSelectorCocoa(contents, cert_request_info,
                                               std::move(client_certs),
                                               std::move(delegate));
}
#endif

}  // namespace chrome

namespace {

// These ClearTableViewDataSources... functions help work around a bug in macOS
// 10.12 where SFChooseIdentityPanel leaks a window and some views, including
// an NSTableView. Future events may make cause the table view to query its
// dataSource, which will have been deallocated.
//
// Linking against the 10.12 SDK does not "fix" this issue, since
// NSTableView.dataSource is a "weak" reference, which in non-ARC land still
// translates to "raw pointer".
//
// See https://crbug.com/653093, https://crbug.com/750242 and rdar://29409207
// for more information.

void ClearTableViewDataSources(NSView* view) {
  if (auto table_view = base::mac::ObjCCast<NSTableView>(view)) {
    table_view.dataSource = nil;
  } else {
    for (NSView* subview in view.subviews) {
      ClearTableViewDataSources(subview);
    }
  }
}

void ClearTableViewDataSourcesIfWindowStillExists(NSWindow* leaked_window) {
  for (NSWindow* window in [NSApp windows]) {
    // If the window is still in the window list...
    if (window == leaked_window) {
      // ...search it for table views.
      ClearTableViewDataSources(window.contentView);
      break;
    }
  }
}

void ClearTableViewDataSourcesIfNeeded(NSWindow* leaked_window) {
  // Let the autorelease pool drain before deciding if the window was leaked.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(ClearTableViewDataSourcesIfWindowStillExists,
                            base::Unretained(leaked_window)));
}

}  // namespace

@implementation SSLClientCertificateSelectorCocoa

- (id)initWithBrowserContext:(const content::BrowserContext*)browserContext
             certRequestInfo:(net::SSLCertRequestInfo*)certRequestInfo
                    delegate:
                        (std::unique_ptr<content::ClientCertificateDelegate>)
                            delegate {
  DCHECK(browserContext);
  DCHECK(certRequestInfo);
  if ((self = [super init])) {
    observer_.reset(new SSLClientAuthObserverCocoaBridge(
        browserContext, certRequestInfo, std::move(delegate), self));
  }
  return self;
}

- (void)sheetDidEnd:(NSWindow*)sheet
         returnCode:(NSInteger)returnCode
            context:(void*)context {
  net::ClientCertIdentity* cert = nullptr;
  if (returnCode == NSFileHandlingPanelOKButton) {
    CFRange range = CFRangeMake(0, CFArrayGetCount(sec_identities_));
    CFIndex index =
        CFArrayGetFirstIndexOfValue(sec_identities_, range, [panel_ identity]);
    if (index != -1)
      cert = cert_identities_[index].get();
    else
      NOTREACHED();
  }

  // See comment at definition; this works around a 10.12 bug.
  ClearTableViewDataSourcesIfNeeded(sheet);

  if (!closePending_) {
    // If |closePending_| is already set, |closeSheetWithAnimation:| was called
    // already to cancel the selection rather than continue with no
    // certificate. Otherwise, tell the backend which identity (or none) the
    // user selected.
    userResponded_ = YES;

    if (cert) {
      observer_->CertificateSelected(
          cert->certificate(),
          CreateSSLPrivateKeyForSecIdentity(cert->certificate(),
                                            cert->sec_identity_ref())
              .get());
    } else {
      observer_->CertificateSelected(nullptr, nullptr);
    }

    constrainedWindow_->CloseWebContentsModalDialog();
  }
}

- (void)displayForWebContents:(content::WebContents*)webContents
                  clientCerts:(net::ClientCertIdentityList)inputClientCerts {
  cert_identities_ = std::move(inputClientCerts);
  // Create an array of CFIdentityRefs for the certificates:
  size_t numCerts = cert_identities_.size();
  sec_identities_.reset(CFArrayCreateMutable(kCFAllocatorDefault, numCerts,
                                             &kCFTypeArrayCallBacks));
  for (size_t i = 0; i < numCerts; ++i) {
    DCHECK(cert_identities_[i]->sec_identity_ref());
    CFArrayAppendValue(sec_identities_,
                       cert_identities_[i]->sec_identity_ref());
  }

  // Get the message to display:
  NSString* message = l10n_util::GetNSStringF(
      IDS_CLIENT_CERT_DIALOG_TEXT,
      base::ASCIIToUTF16(
          observer_->cert_request_info()->host_and_port.ToString()));

  // Create and set up a system choose-identity panel.
  panel_.reset([[SFChooseIdentityPanel alloc] init]);
  [panel_ setInformativeText:message];
  [panel_ setDefaultButtonTitle:l10n_util::GetNSString(IDS_OK)];
  [panel_ setAlternateButtonTitle:l10n_util::GetNSString(IDS_CANCEL)];
  SecPolicyRef sslPolicy;
  if (net::x509_util::CreateSSLClientPolicy(&sslPolicy) == noErr) {
    [panel_ setPolicies:(id)sslPolicy];
    CFRelease(sslPolicy);
  }

  constrainedWindow_ =
      CreateAndShowWebModalDialogMac(observer_.get(), webContents, self);
  observer_->StartObserving();
}

- (void)closeWebContentsModalDialog {
  DCHECK(constrainedWindow_);
  constrainedWindow_->CloseWebContentsModalDialog();
}

- (NSWindow*)overlayWindow {
  return overlayWindow_;
}

- (SFChooseIdentityPanel*)panel {
  return panel_;
}

- (void)showSheetForWindow:(NSWindow*)window {
  NSString* title = l10n_util::GetNSString(IDS_CLIENT_CERT_DIALOG_TITLE);
  overlayWindow_.reset([window retain]);
  [panel_ beginSheetForWindow:window
                modalDelegate:self
               didEndSelector:@selector(sheetDidEnd:returnCode:context:)
                  contextInfo:NULL
                   identities:base::mac::CFToNSCast(sec_identities_)
                      message:title];
}

- (void)closeSheetWithAnimation:(BOOL)withAnimation {
  if (!userResponded_) {
    // If the sheet is closed by closing the tab rather than the user explicitly
    // hitting Cancel, |closeSheetWithAnimation:| gets called before
    // |sheetDidEnd:|. In this case, the selection should be canceled rather
    // than continue with no certificate. The |returnCode| parameter to
    // |sheetDidEnd:| is the same in both cases.
    observer_->CancelCertificateSelection();
  }
  closePending_ = YES;
  overlayWindow_.reset();
  // Closing the sheet using -[NSApp endSheet:] doesn't work so use the private
  // method.
  [panel_ _dismissWithCode:NSFileHandlingPanelCancelButton];
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
  [[sheetWindow contentView] setAutoresizesSubviews:oldResizesSubviews_];
  [sheetWindow setAlphaValue:1.0];
  [sheetWindow setIgnoresMouseEvents:NO];
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

- (void)resizeWithNewSize:(NSSize)size {
  // NOOP
}

- (NSWindow*)sheetWindow {
  return panel_;
}

- (void)onConstrainedWindowClosed {
  observer_->StopObserving();
  panel_.reset();
  constrainedWindow_.reset();
  [self release];
}

@end
