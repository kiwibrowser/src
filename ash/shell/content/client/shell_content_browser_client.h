// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELL_CONTENT_CLIENT_SHELL_CONTENT_BROWSER_CLIENT_H_
#define ASH_SHELL_CONTENT_CLIENT_SHELL_CONTENT_BROWSER_CLIENT_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/content_browser_client.h"

namespace content {
class ShellBrowserMainParts;
}

namespace ash {
namespace shell {

class ShellBrowserMainParts;

class ShellContentBrowserClient : public content::ContentBrowserClient {
 public:
  ShellContentBrowserClient();
  ~ShellContentBrowserClient() override;

  // Overridden from content::ContentBrowserClient:
  content::BrowserMainParts* CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) override;
  void GetQuotaSettings(
      content::BrowserContext* context,
      content::StoragePartition* partition,
      storage::OptionalQuotaSettingsCallback callback) override;
  std::unique_ptr<base::Value> GetServiceManifestOverlay(
      base::StringPiece name) override;
  std::vector<ServiceManifestInfo> GetExtraServiceManifests() override;
  void RegisterOutOfProcessServices(OutOfProcessServiceMap* services) override;
  void RegisterInProcessServices(
      StaticServiceMap* services,
      content::ServiceManagerConnection* connection) override;

 private:
  ShellBrowserMainParts* shell_browser_main_parts_;

  DISALLOW_COPY_AND_ASSIGN(ShellContentBrowserClient);
};

}  // namespace shell
}  // namespace ash

#endif  // ASH_SHELL_CONTENT_CLIENT_SHELL_CONTENT_BROWSER_CLIENT_H_
