// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_UTILITY_AW_CONTENT_UTILITY_CLIENT_H_
#define ANDROID_WEBVIEW_UTILITY_AW_CONTENT_UTILITY_CLIENT_H_

#include <memory>

#include "content/public/utility/content_utility_client.h"

namespace android_webview {

class AwContentUtilityClient : public content::ContentUtilityClient {
 public:
  AwContentUtilityClient();
  ~AwContentUtilityClient() override;

  // content::ContentUtilityClient:
  void UtilityThreadStarted() override;
  void RegisterServices(StaticServiceMap* services) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AwContentUtilityClient);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_UTILITY_AW_CONTENT_UTILITY_CLIENT_H_
