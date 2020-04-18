// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_COMMON_AW_CONTENT_CLIENT_H_
#define ANDROID_WEBVIEW_COMMON_AW_CONTENT_CLIENT_H_

#include "content/public/common/content_client.h"

#include "base/compiler_specific.h"

namespace gpu {
struct GPUInfo;
}

namespace android_webview {

std::string GetProduct();
std::string GetUserAgent();

class AwContentClient : public content::ContentClient {
 public:
  // ContentClient implementation.
  void AddAdditionalSchemes(Schemes* schemes) override;
  std::string GetProduct() const override;
  std::string GetUserAgent() const override;
  base::string16 GetLocalizedString(int message_id) const override;
  base::StringPiece GetDataResource(
      int resource_id,
      ui::ScaleFactor scale_factor) const override;
  bool CanSendWhileSwappedOut(const IPC::Message* message) override;
  void SetGpuInfo(const gpu::GPUInfo& gpu_info) override;
  bool UsingSynchronousCompositing() override;
  media::MediaDrmBridgeClient* GetMediaDrmBridgeClient() override;
  void OnServiceManagerConnected(
      content::ServiceManagerConnection* connection) override;

  const std::string& gpu_fingerprint() const { return gpu_fingerprint_; }

 private:
  std::string gpu_fingerprint_;
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_COMMON_AW_CONTENT_CLIENT_H_
