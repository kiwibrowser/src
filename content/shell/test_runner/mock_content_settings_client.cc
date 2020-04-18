// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/test_runner/mock_content_settings_client.h"

#include "content/shell/test_runner/layout_test_runtime_flags.h"
#include "content/shell/test_runner/test_common.h"
#include "content/shell/test_runner/web_test_delegate.h"
#include "third_party/blink/public/platform/web_url.h"

namespace test_runner {

MockContentSettingsClient::MockContentSettingsClient(
    LayoutTestRuntimeFlags* layout_test_runtime_flags)
    : delegate_(nullptr), flags_(layout_test_runtime_flags) {}

MockContentSettingsClient::~MockContentSettingsClient() {}

bool MockContentSettingsClient::AllowImage(bool enabled_per_settings,
                                           const blink::WebURL& image_url) {
  bool allowed = enabled_per_settings && flags_->images_allowed();
  if (flags_->dump_web_content_settings_client_callbacks() && delegate_) {
    delegate_->PrintMessage(
        std::string("MockContentSettingsClient: allowImage(") +
        NormalizeLayoutTestURL(image_url.GetString().Utf8()) +
        "): " + (allowed ? "true" : "false") + "\n");
  }
  return allowed;
}

bool MockContentSettingsClient::AllowScript(bool enabled_per_settings) {
  return enabled_per_settings && flags_->scripts_allowed();
}

bool MockContentSettingsClient::AllowScriptFromSource(
    bool enabled_per_settings,
    const blink::WebURL& script_url) {
  bool allowed = enabled_per_settings && flags_->scripts_allowed();
  if (flags_->dump_web_content_settings_client_callbacks() && delegate_) {
    delegate_->PrintMessage(
        std::string("MockContentSettingsClient: allowScriptFromSource(") +
        NormalizeLayoutTestURL(script_url.GetString().Utf8()) +
        "): " + (allowed ? "true" : "false") + "\n");
  }
  return allowed;
}

bool MockContentSettingsClient::AllowStorage(bool enabled_per_settings) {
  return flags_->storage_allowed();
}

bool MockContentSettingsClient::AllowRunningInsecureContent(
    bool enabled_per_settings,
    const blink::WebSecurityOrigin& context,
    const blink::WebURL& url) {
  return enabled_per_settings || flags_->running_insecure_content_allowed();
}

bool MockContentSettingsClient::AllowAutoplay(bool default_value) {
  return flags_->autoplay_allowed();
}

void MockContentSettingsClient::SetDelegate(WebTestDelegate* delegate) {
  delegate_ = delegate;
}

}  // namespace test_runner
