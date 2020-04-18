// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBRUNNER_APP_WEBRUNNER_MAIN_DELEGATE_H_
#define WEBRUNNER_APP_WEBRUNNER_MAIN_DELEGATE_H_

#include <memory>

#include "base/macros.h"
#include "content/public/app/content_main_delegate.h"

namespace content {
class ContentClient;
}  // namespace content

namespace webrunner {

class WebRunnerMainDelegate : public content::ContentMainDelegate {
 public:
  WebRunnerMainDelegate();
  ~WebRunnerMainDelegate() override;

  // ContentMainDelegate implementation.
  bool BasicStartupComplete(int* exit_code) override;
  void PreSandboxStartup() override;
  int RunProcess(
      const std::string& process_type,
      const content::MainFunctionParams& main_function_params) override;
  content::ContentBrowserClient* CreateContentBrowserClient() override;

 private:
  std::unique_ptr<content::ContentClient> content_client_;
  std::unique_ptr<content::ContentBrowserClient> browser_client_;

  DISALLOW_COPY_AND_ASSIGN(WebRunnerMainDelegate);
};

}  // namespace webrunner

#endif  // WEBRUNNER_APP_WEBRUNNER_MAIN_DELEGATE_H_
