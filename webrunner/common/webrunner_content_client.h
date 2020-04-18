// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBRUNNER_COMMON_WEBRUNNER_CONTENT_CLIENT_H_
#define WEBRUNNER_COMMON_WEBRUNNER_CONTENT_CLIENT_H_

#include "base/macros.h"
#include "content/public/common/content_client.h"

namespace webrunner {

class WebRunnerContentClient : public content::ContentClient {
 public:
  WebRunnerContentClient();
  ~WebRunnerContentClient() override;

  // content::ContentClient implementation.
  std::string GetUserAgent() const override;
  base::string16 GetLocalizedString(int message_id) const override;
  base::StringPiece GetDataResource(
      int resource_id,
      ui::ScaleFactor scale_factor) const override;
  base::RefCountedMemory* GetDataResourceBytes(int resource_id) const override;
  gfx::Image& GetNativeImageNamed(int resource_id) const override;
  blink::OriginTrialPolicy* GetOriginTrialPolicy() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebRunnerContentClient);
};

}  // namespace webrunner

#endif  // WEBRUNNER_COMMON_WEBRUNNER_CONTENT_CLIENT_H_
