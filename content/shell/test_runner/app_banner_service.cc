// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/test_runner/app_banner_service.h"

namespace test_runner {

AppBannerService::AppBannerService() : binding_(this) {}

AppBannerService::~AppBannerService() {}

void AppBannerService::ResolvePromise(const std::string& platform) {
  if (!event_.is_bound())
    return;

  // Empty platform means to resolve as a dismissal.
  if (platform.empty())
    event_->BannerDismissed();
  else
    event_->BannerAccepted(platform);
}

void AppBannerService::SendBannerPromptRequest(
    const std::vector<std::string>& platforms,
    base::OnceCallback<void(bool)> callback) {
  if (!controller_.is_bound())
    return;

  blink::mojom::AppBannerServicePtr proxy;
  binding_.Bind(mojo::MakeRequest(&proxy));
  controller_->BannerPromptRequest(
      std::move(proxy), mojo::MakeRequest(&event_), platforms,
      base::BindOnce(&AppBannerService::OnBannerPromptReply,
                     base::Unretained(this), std::move(callback)));
}

void AppBannerService::DisplayAppBanner(bool user_gesture) { /* do nothing */
}

void AppBannerService::OnBannerPromptReply(
    base::OnceCallback<void(bool)> callback,
    blink::mojom::AppBannerPromptReply reply,
    const std::string& referrer) {
  std::move(callback).Run(reply == blink::mojom::AppBannerPromptReply::CANCEL);
}

}  // namespace test_runner
