// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webrunner/common/webrunner_content_client.h"

#include "components/version_info/version_info.h"
#include "content/public/common/user_agent.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace webrunner {

WebRunnerContentClient::WebRunnerContentClient() = default;
WebRunnerContentClient::~WebRunnerContentClient() = default;

std::string WebRunnerContentClient::GetUserAgent() const {
  return content::BuildUserAgentFromProduct(
      version_info::GetProductNameAndVersionForUserAgent());
}

base::string16 WebRunnerContentClient::GetLocalizedString(
    int message_id) const {
  return l10n_util::GetStringUTF16(message_id);
}

base::StringPiece WebRunnerContentClient::GetDataResource(
    int resource_id,
    ui::ScaleFactor scale_factor) const {
  return ui::ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
      resource_id, scale_factor);
}

base::RefCountedMemory* WebRunnerContentClient::GetDataResourceBytes(
    int resource_id) const {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
      resource_id);
}

gfx::Image& WebRunnerContentClient::GetNativeImageNamed(int resource_id) const {
  return ui::ResourceBundle::GetSharedInstance().GetNativeImageNamed(
      resource_id);
}

blink::OriginTrialPolicy* WebRunnerContentClient::GetOriginTrialPolicy() {
  NOTIMPLEMENTED();
  return nullptr;
}

}  // namespace webrunner
