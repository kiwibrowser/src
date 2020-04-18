// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_COMMON_CAST_CONTENT_CLIENT_H_
#define CHROMECAST_COMMON_CAST_CONTENT_CLIENT_H_

#include "content/public/common/content_client.h"

namespace chromecast {
namespace shell {

std::string GetUserAgent();

class CastContentClient : public content::ContentClient {
 public:
  ~CastContentClient() override;

  // content::ContentClient implementation:
  void AddAdditionalSchemes(Schemes* schemes) override;
  std::string GetUserAgent() const override;
  base::string16 GetLocalizedString(int message_id) const override;
  base::StringPiece GetDataResource(
      int resource_id,
      ui::ScaleFactor scale_factor) const override;
  base::RefCountedMemory* GetDataResourceBytes(
      int resource_id) const override;
  gfx::Image& GetNativeImageNamed(int resource_id) const override;
#if defined(OS_ANDROID)
  ::media::MediaDrmBridgeClient* GetMediaDrmBridgeClient() override;
#endif  // OS_ANDROID
};

}  // namespace shell
}  // namespace chromecast

#endif  // CHROMECAST_COMMON_CAST_CONTENT_CLIENT_H_
