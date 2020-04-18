// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/cast_web_view_extension.h"

#include "base/logging.h"
#include "chromecast/browser/cast_browser_process.h"
#include "chromecast/browser/cast_extension_host.h"
#include "chromecast/browser/devtools/remote_debugging_server.h"
#include "content/public/browser/web_contents_observer.h"
#include "extensions/browser/extension_system.h"
#include "net/base/net_errors.h"

namespace chromecast {

CastWebViewExtension::CastWebViewExtension(
    const CreateParams& params,
    content::BrowserContext* browser_context,
    scoped_refptr<content::SiteInstance> site_instance,
    const extensions::Extension* extension,
    const GURL& initial_url)
    : delegate_(params.delegate),
      window_(shell::CastContentWindow::Create(params.delegate,
                                               params.is_headless,
                                               params.enable_touch_input)),
      extension_host_(std::make_unique<CastExtensionHost>(
          browser_context,
          params.delegate,
          extension,
          initial_url,
          site_instance.get(),
          extensions::VIEW_TYPE_EXTENSION_POPUP)),
      remote_debugging_server_(
          shell::CastBrowserProcess::GetInstance()->remote_debugging_server()) {
  DCHECK(delegate_);
  content::WebContentsObserver::Observe(web_contents());
  // If this CastWebView is enabled for development, start the remote debugger.
  if (params.enabled_for_dev) {
    LOG(INFO) << "Enabling dev console for " << web_contents()->GetVisibleURL();
    remote_debugging_server_->EnableWebContentsForDebugging(web_contents());
  }
}

CastWebViewExtension::~CastWebViewExtension() {
  content::WebContentsObserver::Observe(nullptr);
}

shell::CastContentWindow* CastWebViewExtension::window() const {
  return window_.get();
}

content::WebContents* CastWebViewExtension::web_contents() const {
  return extension_host_->host_contents();
}

void CastWebViewExtension::LoadUrl(GURL url) {
  extension_host_->CreateRenderViewSoon();
}

void CastWebViewExtension::ClosePage(const base::TimeDelta& shutdown_delay) {}

void CastWebViewExtension::InitializeWindow(
    CastWindowManager* window_manager,
    bool is_visible,
    CastWindowManager::WindowId z_order,
    VisibilityPriority initial_priority) {
  window_->CreateWindowForWebContents(web_contents(), window_manager,
                                      is_visible, z_order, initial_priority);
  web_contents()->Focus();
}

void CastWebViewExtension::WebContentsDestroyed() {
  delegate_->OnPageStopped(net::OK);
}

void CastWebViewExtension::RenderViewCreated(
    content::RenderViewHost* render_view_host) {
  content::RenderWidgetHostView* view =
      render_view_host->GetWidget()->GetView();
  if (view) {
    view->SetBackgroundColor(SK_ColorTRANSPARENT);
  }
}

void CastWebViewExtension::RenderProcessGone(base::TerminationStatus status) {
  delegate_->OnPageStopped(net::ERR_UNEXPECTED);
}

}  // namespace chromecast
