// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_TEST_TEST_SHELL_MAIN_DELEGATE_H_
#define EXTENSIONS_SHELL_TEST_TEST_SHELL_MAIN_DELEGATE_H_

#include <memory>

#include "base/macros.h"
#include "extensions/shell/app/shell_main_delegate.h"

namespace content {
class ContentUtilityClient;
}

namespace extensions {

class TestShellMainDelegate : public extensions::ShellMainDelegate {
 public:
  TestShellMainDelegate();
  ~TestShellMainDelegate() override;

 protected:
  // content::ContentMainDelegate implementation:
  content::ContentUtilityClient* CreateContentUtilityClient() override;

 private:
  std::unique_ptr<content::ContentUtilityClient> utility_client_;

  DISALLOW_COPY_AND_ASSIGN(TestShellMainDelegate);
};

}  // namespace extensions

#endif  // EXTENSIONS_SHELL_TEST_TEST_SHELL_MAIN_DELEGATE_H_
