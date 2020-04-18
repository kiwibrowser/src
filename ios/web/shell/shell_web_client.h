// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_SHELL_SHELL_WEB_CLIENT_H_
#define IOS_WEB_SHELL_SHELL_WEB_CLIENT_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#import "ios/web/public/web_client.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace web {
class ShellBrowserState;
class ShellWebMainParts;

class ShellWebClient : public WebClient {
 public:
  ShellWebClient();
  ~ShellWebClient() override;

  // WebClient implementation.
  std::unique_ptr<WebMainParts> CreateWebMainParts() override;
  std::string GetProduct() const override;
  std::string GetUserAgent(UserAgentType type) const override;
  base::StringPiece GetDataResource(
      int resource_id,
      ui::ScaleFactor scale_factor) const override;
  base::RefCountedMemory* GetDataResourceBytes(int resource_id) const override;
  void RegisterServices(StaticServiceMap* services) override;
  std::unique_ptr<base::Value> GetServiceManifestOverlay(
      base::StringPiece name) override;
  void BindInterfaceRequestFromMainFrame(
      WebState* web_state,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle interface_pipe) override;
  void AllowCertificateError(
      WebState* web_state,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      bool overridable,
      const base::Callback<void(bool)>& callback) override;

  ShellBrowserState* browser_state() const;

 private:
  void InitMainFrameInterfaces();

  ShellWebMainParts* web_main_parts_;

  // Interfaces exposed to the main frame whose implementations do not need the
  // WebState associated with that main frame as a creation argument.
  std::unique_ptr<service_manager::BinderRegistry> main_frame_interfaces_;
  // Interfaces exposed to the main frame whose implementations *do* need the
  // WebState associated with that main frame as a creation argument.
  std::unique_ptr<service_manager::BinderRegistryWithArgs<WebState*>>
      main_frame_interfaces_parameterized_;

  DISALLOW_COPY_AND_ASSIGN(ShellWebClient);
};

}  // namespace web

#endif  // IOS_WEB_SHELL_SHELL_WEB_CLIENT_H_
