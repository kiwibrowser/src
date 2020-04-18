// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOM_DISTILLER_CONTENT_BROWSER_DISTILLER_JAVASCRIPT_SERVICE_IMPL_H_
#define COMPONENTS_DOM_DISTILLER_CONTENT_BROWSER_DISTILLER_JAVASCRIPT_SERVICE_IMPL_H_

#include "base/macros.h"
#include "components/dom_distiller/content/browser/distiller_ui_handle.h"
#include "components/dom_distiller/content/common/distiller_javascript_service.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace dom_distiller {

// This is the receiving end of "distiller" JavaScript object calls.
class DistillerJavaScriptServiceImpl
    : public mojom::DistillerJavaScriptService {
 public:
  DistillerJavaScriptServiceImpl(content::RenderFrameHost* render_frame_host,
                                 DistillerUIHandle* distiller_ui_handle);
  ~DistillerJavaScriptServiceImpl() override;

  // Mojo mojom::DistillerJavaScriptService implementation.

  // Show the Android view containing Reader Mode settings.
  void HandleDistillerOpenSettingsCall() override;

 private:
  content::RenderFrameHost* render_frame_host_;
  DistillerUIHandle* distiller_ui_handle_;

  DISALLOW_COPY_AND_ASSIGN(DistillerJavaScriptServiceImpl);
};

// static
void CreateDistillerJavaScriptService(
    DistillerUIHandle* distiller_ui_handle,
    mojom::DistillerJavaScriptServiceRequest request,
    content::RenderFrameHost* render_frame_host);

}  // namespace dom_distiller

#endif  // COMPONENTS_DOM_DISTILLER_CONTENT_BROWSER_DISTILLER_JAVASCRIPT_SERVICE_IMPL_H_
