// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr/vr_web_contents_observer.h"

#include "chrome/browser/android/vr/vr_shell.h"
#include "chrome/browser/vr/toolbar_helper.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"

namespace vr {

VrWebContentsObserver::VrWebContentsObserver(content::WebContents* web_contents,
                                             VrShell* vr_shell,
                                             BrowserUiInterface* ui_interface,
                                             ToolbarHelper* toolbar)
    : WebContentsObserver(web_contents),
      vr_shell_(vr_shell),
      ui_interface_(ui_interface),
      toolbar_(toolbar) {
  toolbar_->Update();
}

VrWebContentsObserver::~VrWebContentsObserver() {}

void VrWebContentsObserver::SetUiInterface(BrowserUiInterface* ui_interface) {
  ui_interface_ = ui_interface;
}

void VrWebContentsObserver::DidStartLoading() {
  ui_interface_->SetLoading(true);
}

void VrWebContentsObserver::DidStopLoading() {
  ui_interface_->SetLoading(false);
}

void VrWebContentsObserver::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  toolbar_->Update();
}

void VrWebContentsObserver::DidRedirectNavigation(
    content::NavigationHandle* navigation_handle) {
  toolbar_->Update();
}

void VrWebContentsObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  toolbar_->Update();
}

void VrWebContentsObserver::DidChangeVisibleSecurityState() {
  toolbar_->Update();
}

void VrWebContentsObserver::DidToggleFullscreenModeForTab(
    bool entered_fullscreen,
    bool will_cause_resize) {
  vr_shell_->OnFullscreenChanged(entered_fullscreen);
}

void VrWebContentsObserver::WebContentsDestroyed() {
  vr_shell_->ContentWebContentsDestroyed();
}

void VrWebContentsObserver::RenderViewHostChanged(
    content::RenderViewHost* old_host,
    content::RenderViewHost* new_host) {
  new_host->GetWidget()->GetView()->SetIsInVR(true);
}

}  // namespace vr
