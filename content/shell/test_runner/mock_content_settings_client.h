// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_TEST_RUNNER_MOCK_CONTENT_SETTINGS_CLIENT_H_
#define CONTENT_SHELL_TEST_RUNNER_MOCK_CONTENT_SETTINGS_CLIENT_H_

#include "base/macros.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"

namespace test_runner {

class LayoutTestRuntimeFlags;
class WebTestDelegate;

class MockContentSettingsClient : public blink::WebContentSettingsClient {
 public:
  // Caller has to guarantee that |layout_test_runtime_flags| lives longer
  // than the MockContentSettingsClient being constructed here.
  MockContentSettingsClient(LayoutTestRuntimeFlags* layout_test_runtime_flags);

  ~MockContentSettingsClient() override;

  // blink::WebContentSettingsClient:
  bool AllowImage(bool enabled_per_settings,
                  const blink::WebURL& image_url) override;
  bool AllowScript(bool enabled_per_settings) override;
  bool AllowScriptFromSource(bool enabled_per_settings,
                             const blink::WebURL& script_url) override;
  bool AllowStorage(bool local) override;
  bool AllowRunningInsecureContent(bool enabled_per_settings,
                                   const blink::WebSecurityOrigin& context,
                                   const blink::WebURL& url) override;
  bool AllowAutoplay(bool default_value) override;

  void SetDelegate(WebTestDelegate* delegate);

 private:
  WebTestDelegate* delegate_;

  LayoutTestRuntimeFlags* flags_;

  DISALLOW_COPY_AND_ASSIGN(MockContentSettingsClient);
};

}  // namespace test_runner

#endif  // CONTENT_SHELL_TEST_RUNNER_MOCK_CONTENT_SETTINGS_CLIENT_H_
