// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_
#define CHROME_RENDERER_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_

#include <memory>

#include "base/threading/thread_checker.h"
#include "components/safe_browsing/common/safe_browsing.mojom.h"
#include "components/subresource_filter/content/common/ad_delay_throttle.h"
#include "content/public/renderer/url_loader_throttle_provider.h"

class ChromeContentRendererClient;

// Instances must be constructed on the render thread, and then used and
// destructed on a single thread, which can be different from the render thread.
class URLLoaderThrottleProviderImpl
    : public content::URLLoaderThrottleProvider {
 public:
  URLLoaderThrottleProviderImpl(
      content::URLLoaderThrottleProviderType type,
      ChromeContentRendererClient* chrome_content_renderer_client);

  ~URLLoaderThrottleProviderImpl() override;

  // content::URLLoaderThrottleProvider implementation.
  std::unique_ptr<content::URLLoaderThrottleProvider> Clone() override;
  std::vector<std::unique_ptr<content::URLLoaderThrottle>> CreateThrottles(
      int render_frame_id,
      const blink::WebURLRequest& request,
      content::ResourceType resource_type) override;

 private:
  // This copy constructor works in conjunction with Clone(), not intended for
  // general use.
  URLLoaderThrottleProviderImpl(const URLLoaderThrottleProviderImpl& other);

  std::unique_ptr<subresource_filter::AdDelayThrottle::Factory>
      ad_delay_factory_;

  content::URLLoaderThrottleProviderType type_;
  ChromeContentRendererClient* const chrome_content_renderer_client_;

  safe_browsing::mojom::SafeBrowsingPtrInfo safe_browsing_info_;
  safe_browsing::mojom::SafeBrowsingPtr safe_browsing_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_ASSIGN(URLLoaderThrottleProviderImpl);
};

#endif  // CHROME_RENDERER_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_
