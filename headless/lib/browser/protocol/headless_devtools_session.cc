// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/protocol/headless_devtools_session.h"

#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_agent_host_client.h"
#include "headless/lib/browser/protocol/browser_handler.h"
#include "headless/lib/browser/protocol/headless_handler.h"
#include "headless/lib/browser/protocol/network_handler.h"
#include "headless/lib/browser/protocol/page_handler.h"
#include "headless/lib/browser/protocol/target_handler.h"

namespace headless {
namespace protocol {

HeadlessDevToolsSession::HeadlessDevToolsSession(
    base::WeakPtr<HeadlessBrowserImpl> browser,
    content::DevToolsAgentHost* agent_host,
    content::DevToolsAgentHostClient* client)
    : browser_(browser),
      agent_host_(agent_host),
      client_(client),
      dispatcher_(std::make_unique<UberDispatcher>(this)) {
  dispatcher_->setFallThroughForNotFound(true);
  if (agent_host->GetWebContents() &&
      agent_host->GetType() == content::DevToolsAgentHost::kTypePage) {
    AddHandler(std::make_unique<HeadlessHandler>(browser_,
                                                 agent_host->GetWebContents()));
    AddHandler(
        std::make_unique<PageHandler>(browser_, agent_host->GetWebContents()));
  }
  if (agent_host->GetType() == content::DevToolsAgentHost::kTypeBrowser)
    AddHandler(std::make_unique<BrowserHandler>(browser_));
  AddHandler(std::make_unique<NetworkHandler>(browser_));
  AddHandler(std::make_unique<TargetHandler>(browser_));
}

HeadlessDevToolsSession::~HeadlessDevToolsSession() {
  dispatcher_.reset();
  for (auto& pair : handlers_)
    pair.second->Disable();
  handlers_.clear();
}

void HeadlessDevToolsSession::AddHandler(
    std::unique_ptr<protocol::DomainHandler> handler) {
  handler->Wire(dispatcher_.get());
  handlers_[handler->name()] = std::move(handler);
}

void HeadlessDevToolsSession::sendProtocolResponse(
    int call_id,
    std::unique_ptr<Serializable> message) {
  client_->DispatchProtocolMessage(agent_host_, message->serialize());
}

void HeadlessDevToolsSession::sendProtocolNotification(
    std::unique_ptr<Serializable> message) {
  client_->DispatchProtocolMessage(agent_host_, message->serialize());
}

void HeadlessDevToolsSession::flushProtocolNotifications() {}

}  // namespace protocol
}  // namespace headless
