// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_APP_SHELL_MAIN_DELEGATE_H_
#define EXTENSIONS_SHELL_APP_SHELL_MAIN_DELEGATE_H_

#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "content/public/app/content_main_delegate.h"

namespace content {
class ContentBrowserClient;
class ContentClient;
class ContentRendererClient;
}

namespace extensions {

class ShellMainDelegate : public content::ContentMainDelegate {
 public:
  ShellMainDelegate();
  ~ShellMainDelegate() override;

  // ContentMainDelegate implementation:
  bool BasicStartupComplete(int* exit_code) override;
  void PreSandboxStartup() override;
  content::ContentBrowserClient* CreateContentBrowserClient() override;
  content::ContentRendererClient* CreateContentRendererClient() override;
  void ProcessExiting(const std::string& process_type) override;
#if defined(OS_POSIX) && !defined(OS_MACOSX) && !defined(OS_ANDROID)
  void ZygoteStarting(
      std::vector<std::unique_ptr<service_manager::ZygoteForkDelegate>>*
          delegates) override;
#endif
#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
  void ZygoteForked() override;
#endif

 protected:
  // The created object is owned by this object.
  virtual content::ContentClient* CreateContentClient();
  virtual content::ContentBrowserClient* CreateShellContentBrowserClient();
  virtual content::ContentRendererClient* CreateShellContentRendererClient();
  virtual content::ContentUtilityClient* CreateShellContentUtilityClient();

  // Initializes the resource bundle and resources.pak.
  virtual void InitializeResourceBundle();

 private:
  // |process_type| is zygote, renderer, utility, etc. Returns true if the
  // process needs data from resources.pak.
  static bool ProcessNeedsResourceBundle(const std::string& process_type);

  std::unique_ptr<content::ContentClient> content_client_;
  std::unique_ptr<content::ContentBrowserClient> browser_client_;
  std::unique_ptr<content::ContentRendererClient> renderer_client_;
  std::unique_ptr<content::ContentUtilityClient> utility_client_;

  DISALLOW_COPY_AND_ASSIGN(ShellMainDelegate);
};

}  // namespace extensions

#endif  // EXTENSIONS_SHELL_APP_SHELL_MAIN_DELEGATE_H_
