// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/insecure_sensitive_input_driver.h"

#include <utility>

#include "chrome/browser/ssl/insecure_sensitive_input_driver_factory.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

InsecureSensitiveInputDriver::InsecureSensitiveInputDriver(
    content::RenderFrameHost* render_frame_host)
    : render_frame_host_(render_frame_host) {}

InsecureSensitiveInputDriver::~InsecureSensitiveInputDriver() {}

void InsecureSensitiveInputDriver::BindInsecureInputServiceRequest(
    blink::mojom::InsecureInputServiceRequest request) {
  insecure_input_bindings_.AddBinding(this, std::move(request));
}

void InsecureSensitiveInputDriver::PasswordFieldVisibleInInsecureContext() {
  InsecureSensitiveInputDriverFactory* parent =
      InsecureSensitiveInputDriverFactory::GetOrCreateForWebContents(
          content::WebContents::FromRenderFrameHost(render_frame_host_));
  parent->RenderFrameHasVisiblePasswordField(render_frame_host_);
}

void InsecureSensitiveInputDriver::
    AllPasswordFieldsInInsecureContextInvisible() {
  InsecureSensitiveInputDriverFactory* parent =
      InsecureSensitiveInputDriverFactory::GetOrCreateForWebContents(
          content::WebContents::FromRenderFrameHost(render_frame_host_));
  parent->RenderFrameHasNoVisiblePasswordFields(render_frame_host_);
}

void InsecureSensitiveInputDriver::DidEditFieldInInsecureContext() {
  InsecureSensitiveInputDriverFactory* parent =
      InsecureSensitiveInputDriverFactory::GetOrCreateForWebContents(
          content::WebContents::FromRenderFrameHost(render_frame_host_));
  parent->DidEditFieldInInsecureContext();
}
