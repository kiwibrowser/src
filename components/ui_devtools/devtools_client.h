// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UI_DEVTOOLS_DEVTOOLS_CLIENT_H_
#define COMPONENTS_UI_DEVTOOLS_DEVTOOLS_CLIENT_H_

#include <string>

#include "components/ui_devtools/DOM.h"
#include "components/ui_devtools/Forward.h"
#include "components/ui_devtools/Protocol.h"
#include "components/ui_devtools/devtools_base_agent.h"
#include "components/ui_devtools/devtools_export.h"

namespace ui_devtools {

class UiDevToolsServer;

// Every UI component that wants to be inspectable must instantiate
// this class and attach the corresponding backends/frontends (i.e: DOM, CSS,
// etc). This client is then attached to the UiDevToolsServer and all messages
// from this client are sent over the web socket owned by the server.
class UI_DEVTOOLS_EXPORT UiDevToolsClient : public protocol::FrontendChannel {
 public:
  static const int kNotConnected = -1;

  UiDevToolsClient(const std::string& name, UiDevToolsServer* server);
  ~UiDevToolsClient() override;

  void AddAgent(std::unique_ptr<UiDevToolsAgent> agent);
  void Disconnect();
  void Dispatch(const std::string& data);

  bool connected() const;
  void set_connection_id(int connection_id);
  const std::string& name() const;

 private:
  void DisableAllAgents();

  // protocol::FrontendChannel
  void sendProtocolResponse(
      int callId,
      std::unique_ptr<protocol::Serializable> message) override;
  void sendProtocolNotification(
      std::unique_ptr<protocol::Serializable> message) override;
  void flushProtocolNotifications() override;

  std::string name_;
  int connection_id_;

  std::vector<std::unique_ptr<UiDevToolsAgent>> agents_;
  protocol::UberDispatcher dispatcher_;
  UiDevToolsServer* server_;

  DISALLOW_COPY_AND_ASSIGN(UiDevToolsClient);
};

}  // namespace ui_devtools

#endif  // COMPONENTS_UI_DEVTOOLS_DEVTOOLS_CLIENT_H_
