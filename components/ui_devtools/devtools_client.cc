// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ui_devtools/devtools_client.h"

#include "components/ui_devtools/devtools_server.h"

namespace ui_devtools {

UiDevToolsClient::UiDevToolsClient(const std::string& name,
                                   UiDevToolsServer* server)
    : name_(name),
      connection_id_(kNotConnected),
      dispatcher_(this),
      server_(server) {
  DCHECK(server_);
}

UiDevToolsClient::~UiDevToolsClient() {}

void UiDevToolsClient::AddAgent(std::unique_ptr<UiDevToolsAgent> agent) {
  agent->Init(&dispatcher_);
  agents_.push_back(std::move(agent));
}

void UiDevToolsClient::Disconnect() {
  connection_id_ = kNotConnected;
  DisableAllAgents();
}

void UiDevToolsClient::Dispatch(const std::string& data) {
  dispatcher_.dispatch(protocol::StringUtil::parseJSON(data));
}

bool UiDevToolsClient::connected() const {
  return connection_id_ != kNotConnected;
}

void UiDevToolsClient::set_connection_id(int connection_id) {
  connection_id_ = connection_id;
}

const std::string& UiDevToolsClient::name() const {
  return name_;
}

void UiDevToolsClient::DisableAllAgents() {
  for (std::unique_ptr<UiDevToolsAgent>& agent : agents_)
    agent->Disable();
}

void UiDevToolsClient::sendProtocolResponse(
    int callId,
    std::unique_ptr<protocol::Serializable> message) {
  if (connected())
    server_->SendOverWebSocket(connection_id_, message->serialize());
}

void UiDevToolsClient::sendProtocolNotification(
    std::unique_ptr<protocol::Serializable> message) {
  if (connected())
    server_->SendOverWebSocket(connection_id_, message->serialize());
}

void UiDevToolsClient::flushProtocolNotifications() {
  NOTIMPLEMENTED();
}

}  // namespace ui_devtools
