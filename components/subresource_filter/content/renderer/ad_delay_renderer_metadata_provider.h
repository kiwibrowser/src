// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CONTENT_RENDERER_AD_DELAY_RENDERER_METADATA_PROVIDER_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CONTENT_RENDERER_AD_DELAY_RENDERER_METADATA_PROVIDER_H_

#include "base/macros.h"
#include "components/subresource_filter/content/common/ad_delay_throttle.h"
#include "content/public/renderer/url_loader_throttle_provider.h"

namespace blink {
class WebURLRequest;
}  // namespace blink

namespace subresource_filter {

class AdDelayRendererMetadataProvider
    : public AdDelayThrottle::MetadataProvider {
 public:
  explicit AdDelayRendererMetadataProvider(
      const blink::WebURLRequest& request,
      content::URLLoaderThrottleProviderType type,
      int render_frame_id);
  ~AdDelayRendererMetadataProvider() override;

  // AdDelayThrottle::MetadataProvider:
  bool IsAdRequest() override;
  bool RequestIsInNonIsolatedSubframe() override;

 private:
  static bool IsSubframeAndNonIsolated(
      content::URLLoaderThrottleProviderType type,
      int render_frame_id);

  const bool is_ad_request_ = false;
  const bool is_non_isolated_ = false;

  DISALLOW_COPY_AND_ASSIGN(AdDelayRendererMetadataProvider);
};

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CONTENT_RENDERER_AD_DELAY_RENDERER_METADATA_PROVIDER_H_
