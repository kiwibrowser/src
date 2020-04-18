// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/renderer/ad_delay_renderer_metadata_provider.h"

#include "base/logging.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace subresource_filter {

AdDelayRendererMetadataProvider::AdDelayRendererMetadataProvider(
    const blink::WebURLRequest& request,
    content::URLLoaderThrottleProviderType type,
    int render_frame_id)
    : is_ad_request_(request.IsAdResource()),
      is_non_isolated_(IsSubframeAndNonIsolated(type, render_frame_id)) {}

AdDelayRendererMetadataProvider::~AdDelayRendererMetadataProvider() = default;

// TODO(csharrison): Update |is_ad_request_| across redirects.
bool AdDelayRendererMetadataProvider::IsAdRequest() {
  return is_ad_request_;
}

bool AdDelayRendererMetadataProvider::RequestIsInNonIsolatedSubframe() {
  return is_non_isolated_;
}

// static
bool AdDelayRendererMetadataProvider::IsSubframeAndNonIsolated(
    content::URLLoaderThrottleProviderType type,
    int render_frame_id) {
  // TODO(csharrison): Handle the worker case via threading information from the
  // URLLoaderThrottleProvider's constructor.
  if (type != content::URLLoaderThrottleProviderType::kFrame)
    return false;

  auto* render_frame = content::RenderFrame::FromRoutingID(render_frame_id);
  if (!render_frame || render_frame->IsMainFrame())
    return false;

  blink::WebFrame* web_frame = render_frame->GetWebFrame();

  // The frame is non-isolated if it can access the top frame.
  return web_frame->GetSecurityOrigin().CanAccess(
      web_frame->Top()->GetSecurityOrigin());
}

}  // namespace subresource_filter
