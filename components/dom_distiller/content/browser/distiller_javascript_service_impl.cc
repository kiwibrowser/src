// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/content/browser/distiller_javascript_service_impl.h"

#include <memory>
#include <utility>

#include "base/metrics/user_metrics.h"
#include "components/dom_distiller/content/browser/distiller_ui_handle.h"
#include "components/dom_distiller/core/feedback_reporter.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace dom_distiller {

DistillerJavaScriptServiceImpl::DistillerJavaScriptServiceImpl(
    content::RenderFrameHost* render_frame_host,
    DistillerUIHandle* distiller_ui_handle)
    : render_frame_host_(render_frame_host),
      distiller_ui_handle_(distiller_ui_handle) {}

DistillerJavaScriptServiceImpl::~DistillerJavaScriptServiceImpl() {}

void DistillerJavaScriptServiceImpl::HandleDistillerOpenSettingsCall() {
  if (!distiller_ui_handle_) {
    return;
  }
  content::WebContents* contents =
      content::WebContents::FromRenderFrameHost(render_frame_host_);
  distiller_ui_handle_->OpenSettings(contents);
}

void CreateDistillerJavaScriptService(
    DistillerUIHandle* distiller_ui_handle,
    mojom::DistillerJavaScriptServiceRequest request,
    content::RenderFrameHost* render_frame_host) {
  mojo::MakeStrongBinding(std::make_unique<DistillerJavaScriptServiceImpl>(
                              render_frame_host, distiller_ui_handle),
                          std::move(request));
}

}  // namespace dom_distiller
