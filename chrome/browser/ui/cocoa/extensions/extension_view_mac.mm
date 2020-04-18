// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/extensions/extension_view_mac.h"

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/foundation_util.h"
#include "chrome/browser/extensions/extension_view_host.h"
#import "chrome/browser/ui/cocoa/chrome_event_processing_window.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_host.h"
#include "extensions/common/view_type.h"
#include "ui/base/ui_features.h"

// The minimum/maximum dimensions of the popup.
const CGFloat ExtensionViewMac::kMinWidth = 25.0;
const CGFloat ExtensionViewMac::kMinHeight = 25.0;
const CGFloat ExtensionViewMac::kMaxWidth = 800.0;
const CGFloat ExtensionViewMac::kMaxHeight = 600.0;

ExtensionViewMac::ExtensionViewMac(extensions::ExtensionHost* extension_host,
                                   Browser* browser)
    : browser_(browser),
      extension_host_(extension_host),
      container_(NULL) {
  DCHECK(extension_host_);
  [GetNativeView() setHidden:YES];
}

ExtensionViewMac::~ExtensionViewMac() {
}

void ExtensionViewMac::CreateWidgetHostViewIn(gfx::NativeView superview) {
  [superview addSubview:GetNativeView()];
  extension_host_->CreateRenderViewSoon();
}

Browser* ExtensionViewMac::GetBrowser() {
  return browser_;
}

gfx::NativeView ExtensionViewMac::GetNativeView() {
  return extension_host_->host_contents()->GetNativeView();
}

void ExtensionViewMac::ResizeDueToAutoResize(content::WebContents* web_contents,
                                             const gfx::Size& new_size) {
  if (container_)
    container_->OnExtensionSizeChanged(this, new_size);
}

void ExtensionViewMac::RenderViewCreated(content::RenderViewHost* host) {
  extensions::ViewType host_type = extension_host_->extension_host_type();
  if (host_type == extensions::VIEW_TYPE_EXTENSION_POPUP) {
    gfx::Size min_size(ExtensionViewMac::kMinWidth,
                       ExtensionViewMac::kMinHeight);
    gfx::Size max_size(ExtensionViewMac::kMaxWidth,
                       ExtensionViewMac::kMaxHeight);
    extension_host_->host_contents()
        ->GetRenderWidgetHostView()
        ->EnableAutoResize(min_size, max_size);
  }
}

void ExtensionViewMac::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  if (event.skip_in_browser ||
      event.GetType() == content::NativeWebKeyboardEvent::kChar ||
      extension_host_->extension_host_type() !=
          extensions::VIEW_TYPE_EXTENSION_POPUP)
    return;

  ChromeEventProcessingWindow* event_window =
      base::mac::ObjCCastStrict<ChromeEventProcessingWindow>(
          [GetNativeView() window]);
  [event_window redispatchKeyEvent:event.os_event];
}

void ExtensionViewMac::OnLoaded() {
  ShowIfCompletelyLoaded();
}

content::RenderViewHost* ExtensionViewMac::render_view_host() const {
  return extension_host_->render_view_host();
}

void ExtensionViewMac::ShowIfCompletelyLoaded() {
  // We wait to show the ExtensionView until it has loaded, and the view has
  // actually been created. These can happen in different orders.
  if (extension_host_->has_loaded_once()) {
    [GetNativeView() setHidden:NO];
    if (container_)
      container_->OnExtensionViewDidShow(this);
  }
}

namespace extensions {

// static
std::unique_ptr<ExtensionView> ExtensionViewHost::CreateExtensionViewCocoa(
    ExtensionViewHost* host,
    Browser* browser) {
  return std::make_unique<ExtensionViewMac>(host, browser);
}

}  // namespace extensions
