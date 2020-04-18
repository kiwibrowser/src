// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_TEST_RUNNER_APP_BANNER_SERVICE_H_
#define CONTENT_SHELL_TEST_RUNNER_APP_BANNER_SERVICE_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "content/shell/test_runner/test_runner_export.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/blink/public/platform/modules/app_banner/app_banner.mojom.h"

namespace test_runner {

// Test app banner service that is registered as a Mojo service for
// BeforeInstallPromptEvents to look up when the test runner is executed.
class TEST_RUNNER_EXPORT AppBannerService
    : public blink::mojom::AppBannerService {
 public:
  AppBannerService();
  ~AppBannerService() override;

  blink::mojom::AppBannerControllerPtr& controller() { return controller_; }
  void ResolvePromise(const std::string& platform);
  void SendBannerPromptRequest(const std::vector<std::string>& platforms,
                               base::OnceCallback<void(bool)> callback);

  // blink::mojom::AppBannerService overrides.
  void DisplayAppBanner(bool user_gesture) override;

 private:
  void OnBannerPromptReply(base::OnceCallback<void(bool)> callback,
                           blink::mojom::AppBannerPromptReply,
                           const std::string& referrer);

  mojo::Binding<blink::mojom::AppBannerService> binding_;
  blink::mojom::AppBannerEventPtr event_;
  blink::mojom::AppBannerControllerPtr controller_;

  DISALLOW_COPY_AND_ASSIGN(AppBannerService);
};

}  // namespace test_runner

#endif  // CONTENT_SHELL_TEST_RUNNER_APP_BANNER_SERVICE_H_
