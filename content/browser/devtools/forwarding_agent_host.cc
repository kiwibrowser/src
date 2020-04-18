// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/forwarding_agent_host.h"

#include "base/bind.h"
#include "content/browser/devtools/devtools_session.h"
#include "content/browser/devtools/protocol/inspector_handler.h"
#include "content/public/browser/devtools_external_agent_proxy.h"
#include "content/public/browser/devtools_external_agent_proxy_delegate.h"

namespace content {

class ForwardingAgentHost::SessionProxy : public DevToolsExternalAgentProxy {
 public:
  SessionProxy(ForwardingAgentHost* agent_host, DevToolsAgentHostClient* client)
      : agent_host_(agent_host), client_(client) {
    agent_host_->delegate_->Attach(this);
  }

  ~SessionProxy() override { agent_host_->delegate_->Detach(this); }

  void ConnectionClosed() override {
    DevToolsAgentHostClient* client = client_;
    ForwardingAgentHost* agent_host = agent_host_;
    agent_host_->session_proxies_.erase(client_);
    // |this| is delete here, do not use any fields below.
    client->AgentHostClosed(agent_host);
  }

 private:
  void DispatchOnClientHost(const std::string& message) override {
    client_->DispatchProtocolMessage(agent_host_, message);
  }

  ForwardingAgentHost* agent_host_;
  DevToolsAgentHostClient* client_;

  DISALLOW_COPY_AND_ASSIGN(SessionProxy);
};

ForwardingAgentHost::ForwardingAgentHost(
    const std::string& id,
    std::unique_ptr<DevToolsExternalAgentProxyDelegate> delegate)
      : DevToolsAgentHostImpl(id),
        delegate_(std::move(delegate)) {
  NotifyCreated();
}

ForwardingAgentHost::~ForwardingAgentHost() {
}

void ForwardingAgentHost::AttachClient(DevToolsAgentHostClient* client) {
  session_proxies_[client].reset(new SessionProxy(this, client));
}

bool ForwardingAgentHost::DetachClient(DevToolsAgentHostClient* client) {
  auto it = session_proxies_.find(client);
  if (it == session_proxies_.end())
    return false;
  session_proxies_.erase(it);
  return true;
}

bool ForwardingAgentHost::DispatchProtocolMessage(
    DevToolsAgentHostClient* client,
    const std::string& message) {
  auto it = session_proxies_.find(client);
  if (it == session_proxies_.end())
    return false;
  delegate_->SendMessageToBackend(it->second.get(), message);
  return true;
}

bool ForwardingAgentHost::IsAttached() {
  return !session_proxies_.empty();
}

std::string ForwardingAgentHost::GetType() {
  return delegate_->GetType();
}

std::string ForwardingAgentHost::GetTitle() {
  return delegate_->GetTitle();
}

GURL ForwardingAgentHost::GetURL() {
  return delegate_->GetURL();
}

GURL ForwardingAgentHost::GetFaviconURL() {
  return delegate_->GetFaviconURL();
}

std::string ForwardingAgentHost::GetFrontendURL() {
  return delegate_->GetFrontendURL();
}

bool ForwardingAgentHost::Activate() {
  return delegate_->Activate();
}

void ForwardingAgentHost::Reload() {
  delegate_->Reload();
}

bool ForwardingAgentHost::Close() {
  return delegate_->Close();
}

base::TimeTicks ForwardingAgentHost::GetLastActivityTime() {
  return delegate_->GetLastActivityTime();
}

}  // content
