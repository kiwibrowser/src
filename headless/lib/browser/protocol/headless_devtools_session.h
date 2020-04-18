// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_PROTOCOL_HEADLESS_DEVTOOLS_SESSION_H_
#define HEADLESS_LIB_BROWSER_PROTOCOL_HEADLESS_DEVTOOLS_SESSION_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "headless/lib/browser/protocol/forward.h"
#include "headless/lib/browser/protocol/protocol.h"

namespace content {
class DevToolsAgentHost;
class DevToolsAgentHostClient;
}  // namespace content

namespace headless {
class HeadlessBrowserImpl;
class UberDispatcher;

namespace protocol {

class DomainHandler;

class HeadlessDevToolsSession : public FrontendChannel {
 public:
  HeadlessDevToolsSession(base::WeakPtr<HeadlessBrowserImpl> browser,
                          content::DevToolsAgentHost* agent_host,
                          content::DevToolsAgentHostClient* client);
  ~HeadlessDevToolsSession() override;

  UberDispatcher* dispatcher() { return dispatcher_.get(); }

 private:
  void AddHandler(std::unique_ptr<DomainHandler> handler);

  // FrontendChannel:
  void sendProtocolResponse(int call_id,
                            std::unique_ptr<Serializable> message) override;
  void sendProtocolNotification(std::unique_ptr<Serializable> message) override;
  void flushProtocolNotifications() override;

  base::WeakPtr<HeadlessBrowserImpl> browser_;
  content::DevToolsAgentHost* const agent_host_;
  content::DevToolsAgentHostClient* const client_;
  std::unique_ptr<UberDispatcher> dispatcher_;
  base::flat_map<std::string, std::unique_ptr<DomainHandler>> handlers_;

  DISALLOW_COPY_AND_ASSIGN(HeadlessDevToolsSession);
};

}  // namespace protocol
}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_PROTOCOL_HEADLESS_DEVTOOLS_SESSION_H_
