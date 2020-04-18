// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_FORWARDING_AGENT_HOST_H_
#define CONTENT_BROWSER_DEVTOOLS_FORWARDING_AGENT_HOST_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "base/memory/ref_counted.h"
#include "content/browser/devtools/devtools_agent_host_impl.h"

namespace content {

class DevToolsExternalAgentProxyDelegate;

class ForwardingAgentHost : public DevToolsAgentHostImpl {
 public:
  ForwardingAgentHost(
      const std::string& id,
      std::unique_ptr<DevToolsExternalAgentProxyDelegate> delegate);

 private:
  class SessionProxy;

  ~ForwardingAgentHost() override;

  // DevToolsAgentHost implementation
  void AttachClient(DevToolsAgentHostClient* client) override;
  bool DetachClient(DevToolsAgentHostClient* client) override;
  bool DispatchProtocolMessage(DevToolsAgentHostClient* client,
                               const std::string& message) override;
  bool IsAttached() override;
  std::string GetType() override;
  std::string GetTitle() override;
  GURL GetURL() override;
  GURL GetFaviconURL() override;
  std::string GetFrontendURL() override;
  bool Activate() override;
  void Reload() override;
  bool Close() override;
  base::TimeTicks GetLastActivityTime() override;

  std::unique_ptr<DevToolsExternalAgentProxyDelegate> delegate_;
  base::flat_map<DevToolsAgentHostClient*, std::unique_ptr<SessionProxy>>
      session_proxies_;

  DISALLOW_COPY_AND_ASSIGN(ForwardingAgentHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_FORWARDING_AGENT_HOST_H_
