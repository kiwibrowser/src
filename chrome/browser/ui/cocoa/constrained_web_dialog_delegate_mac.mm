// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/constrained_web_dialog_delegate_base.h"

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_window.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_mac.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_web_dialog_sheet.h"
#include "chrome/browser/ui/webui/chrome_web_contents_handler.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/base/cocoa/window_size_constants.h"
#include "ui/gfx/geometry/size.h"
#include "ui/web_dialogs/web_dialog_delegate.h"
#include "ui/web_dialogs/web_dialog_ui.h"
#include "ui/web_dialogs/web_dialog_web_contents_delegate.h"

using content::WebContents;
using ui::WebDialogDelegate;
using ui::WebDialogWebContentsDelegate;

namespace {

class ConstrainedWebDialogDelegateMac;

// This class is to trigger a resize to the dialog window when
// ResizeDueToAutoResize() is invoked.
class WebDialogWebContentsDelegateMac
    : public ui::WebDialogWebContentsDelegate {
 public:
  WebDialogWebContentsDelegateMac(content::BrowserContext* browser_context,
                                  content::WebContentsObserver* observer,
                                  ConstrainedWebDialogDelegateBase* delegate)
      : ui::WebDialogWebContentsDelegate(browser_context,
                                         new ChromeWebContentsHandler()),
        observer_(observer),
        delegate_(delegate) {
  }
  ~WebDialogWebContentsDelegateMac() override {}

  void ResizeDueToAutoResize(content::WebContents* source,
                             const gfx::Size& preferred_size) override {
    if (!observer_->web_contents())
      return;
    delegate_->ResizeToGivenSize(preferred_size);
  }

 private:
  // These members must outlive the instance.
  content::WebContentsObserver* const observer_;
  ConstrainedWebDialogDelegateBase* delegate_;

  DISALLOW_COPY_AND_ASSIGN(WebDialogWebContentsDelegateMac);
};

class ConstrainedWebDialogDelegateMac
    : public ConstrainedWebDialogDelegateBase {
 public:
  ConstrainedWebDialogDelegateMac(
      content::BrowserContext* browser_context,
      WebDialogDelegate* delegate,
      content::WebContentsObserver* observer)
      : ConstrainedWebDialogDelegateBase(browser_context, delegate,
          new WebDialogWebContentsDelegateMac(browser_context, observer,
                                              this)) {}

  // WebDialogWebContentsDelegate interface.
  void CloseContents(WebContents* source) override {
    window_->CloseWebContentsModalDialog();
  }

  // ConstrainedWebDialogDelegateBase:
  void ResizeToGivenSize(const gfx::Size size) override {
    NSSize updated_preferred_size = NSMakeSize(size.width(),
                                               size.height());
    [window_->sheet() resizeWithNewSize:updated_preferred_size];
  }

  void set_window(ConstrainedWindowMac* window) { window_ = window; }
  ConstrainedWindowMac* window() const { return window_; }

 private:
  // Weak, owned by ConstrainedWebDialogDelegateViewMac.
  ConstrainedWindowMac* window_;

  DISALLOW_COPY_AND_ASSIGN(ConstrainedWebDialogDelegateMac);
};

}  // namespace

class ConstrainedWebDialogDelegateViewMac :
    public ConstrainedWindowMacDelegate,
    public ConstrainedWebDialogDelegate,
    public content::WebContentsObserver {

 public:
  ConstrainedWebDialogDelegateViewMac(
      content::BrowserContext* browser_context,
      WebDialogDelegate* delegate,
      content::WebContents* web_contents,
      const gfx::Size& min_size,
      const gfx::Size& max_size);
  ~ConstrainedWebDialogDelegateViewMac() override {}

  // ConstrainedWebDialogDelegate interface
  const WebDialogDelegate* GetWebDialogDelegate() const override {
    return impl_->GetWebDialogDelegate();
  }
  WebDialogDelegate* GetWebDialogDelegate() override {
    return impl_->GetWebDialogDelegate();
  }
  void OnDialogCloseFromWebUI() override {
    return impl_->OnDialogCloseFromWebUI();
  }
  std::unique_ptr<content::WebContents> ReleaseWebContents() override {
    return impl_->ReleaseWebContents();
  }
  gfx::NativeWindow GetNativeDialog() override { return window_; }
  WebContents* GetWebContents() override { return impl_->GetWebContents(); }
  gfx::Size GetConstrainedWebDialogMinimumSize() const override {
    return min_size_;
  }
  gfx::Size GetConstrainedWebDialogMaximumSize() const override {
    return max_size_;
  }
  gfx::Size GetConstrainedWebDialogPreferredSize() const override {
    gfx::Size size;
    if (!impl_->closed_via_webui()) {
      NSRect frame = [window_ frame];
      size = gfx::Size(frame.size.width, frame.size.height);
    }
    return size;
  }

  // content::WebContentsObserver:
  void RenderViewCreated(content::RenderViewHost* render_view_host) override {
    if (IsDialogAutoResizable())
      EnableAutoResize();
  }
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override {
    if (IsDialogAutoResizable())
      EnableAutoResize();
  }
  void DocumentOnLoadCompletedInMainFrame() override {
    if (IsDialogAutoResizable() && GetWebContents())
      constrained_window_->ShowWebContentsModalDialog();
  }

  // ConstrainedWindowMacDelegate interface
  void OnConstrainedWindowClosed(ConstrainedWindowMac* window) override {
    if (!impl_->closed_via_webui())
      GetWebDialogDelegate()->OnDialogClosed("");
    delete this;
  }

 private:
  void EnableAutoResize() {
    if (!GetWebContents())
      return;

    content::RenderViewHost* render_view_host =
        GetWebContents()->GetRenderViewHost();
    render_view_host->EnableAutoResize(min_size_, max_size_);
  }

  // Whether or not the dialog is autoresizable is determined based on whether
  // |max_size_| was specified.
  bool IsDialogAutoResizable() {
    return !max_size_.IsEmpty();
  }

  std::unique_ptr<ConstrainedWebDialogDelegateMac> impl_;
  std::unique_ptr<ConstrainedWindowMac> constrained_window_;
  base::scoped_nsobject<NSWindow> window_;

  // Minimum and maximum sizes to determine dialog bounds for auto-resizing.
  const gfx::Size min_size_;
  const gfx::Size max_size_;

  DISALLOW_COPY_AND_ASSIGN(ConstrainedWebDialogDelegateViewMac);
};

ConstrainedWebDialogDelegateViewMac::ConstrainedWebDialogDelegateViewMac(
    content::BrowserContext* browser_context,
    WebDialogDelegate* delegate,
    content::WebContents* web_contents,
    const gfx::Size& min_size,
    const gfx::Size& max_size)
    : content::WebContentsObserver(web_contents),
      impl_(new ConstrainedWebDialogDelegateMac(browser_context, delegate,
            this)),
      min_size_(min_size),
      max_size_(max_size) {
  if (IsDialogAutoResizable()) {
    Observe(GetWebContents());
    EnableAutoResize();
  }

  // Create a window to hold web_contents in the constrained sheet:
  gfx::Size size;
  delegate->GetDialogSize(&size);
  // The window size for autoresizing dialogs will be determined at a later
  // time.
  NSRect frame = IsDialogAutoResizable() ? ui::kWindowSizeDeterminedLater :
      NSMakeRect(0, 0, size.width(), size.height());

  window_.reset([[ConstrainedWindowCustomWindow alloc]
                initWithContentRect:frame]);
  [GetWebContents()->GetNativeView() setFrame:frame];
  [GetWebContents()->GetNativeView() setAutoresizingMask:
      NSViewWidthSizable|NSViewHeightSizable];
  [[window_ contentView] addSubview:GetWebContents()->GetNativeView()];

  base::scoped_nsobject<WebDialogConstrainedWindowSheet> sheet(
      [[WebDialogConstrainedWindowSheet alloc] initWithCustomWindow:window_
                                                  webDialogDelegate:delegate]);

  if (IsDialogAutoResizable()) {
    constrained_window_ = CreateWebModalDialogMac(
        this, web_contents, sheet);
  } else {
    constrained_window_ = CreateAndShowWebModalDialogMac(
        this, web_contents, sheet);
  }

  impl_->set_window(constrained_window_.get());
}

ConstrainedWebDialogDelegate* ShowConstrainedWebDialog(
        content::BrowserContext* browser_context,
        WebDialogDelegate* delegate,
        content::WebContents* web_contents) {
  // Deleted when the dialog closes.
  ConstrainedWebDialogDelegateViewMac* constrained_delegate =
      new ConstrainedWebDialogDelegateViewMac(
          browser_context, delegate, web_contents,
          gfx::Size(), gfx::Size());
  return constrained_delegate;
}

ConstrainedWebDialogDelegate* ShowConstrainedWebDialogWithAutoResize(
    content::BrowserContext* browser_context,
    WebDialogDelegate* delegate,
    content::WebContents* web_contents,
    const gfx::Size& min_size,
    const gfx::Size& max_size) {
  DCHECK(!min_size.IsEmpty());
  DCHECK(!max_size.IsEmpty());
  // Deleted when the dialog closes.
  ConstrainedWebDialogDelegateViewMac* constrained_delegate =
      new ConstrainedWebDialogDelegateViewMac(
          browser_context, delegate, web_contents,
          min_size, max_size);
  return constrained_delegate;
}
