// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TEST_ASH_TEST_ENVIRONMENT_CONTENT_H_
#define ASH_TEST_ASH_TEST_ENVIRONMENT_CONTENT_H_

#include "ash/test/ash_test_environment.h"
#include "base/macros.h"
#include "ui/views/controls/webview/webview.h"

namespace content {
class TestBrowserThreadBundle;
}

namespace ash {

class ShellContentState;
class TestShellContentState;

// AshTestEnvironment implementation for tests that use content.
class AshTestEnvironmentContent : public AshTestEnvironment {
 public:
  AshTestEnvironmentContent();
  ~AshTestEnvironmentContent() override;

  TestShellContentState* test_shell_content_state() {
    return test_shell_content_state_;
  }
  void set_content_state(ShellContentState* content_state) {
    content_state_ = content_state;
  }

  // AshTestEnvironment:
  void SetUp() override;
  void TearDown() override;
  std::unique_ptr<AshTestViewsDelegate> CreateViewsDelegate() override;

 private:
  std::unique_ptr<content::TestBrowserThreadBundle> thread_bundle_;
  std::unique_ptr<views::WebView::ScopedWebContentsCreatorForTesting>
      scoped_web_contents_creator_;

  // An implementation of ShellContentState supplied by the user prior to
  // SetUp().
  ShellContentState* content_state_ = nullptr;

  // If |content_state_| is not set prior to SetUp(), this value will be
  // set to an instance of TestShellContentState created by this class. If
  // |content_state_| is non-null, this will be nullptr.
  TestShellContentState* test_shell_content_state_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AshTestEnvironmentContent);
};

}  // namespace ash

#endif  // ASH_TEST_ASH_TEST_ENVIRONMENT_CONTENT_H_
