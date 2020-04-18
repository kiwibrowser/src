// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UI_DEVTOOLS_DEVTOOLS_SERVER_H_
#define COMPONENTS_UI_DEVTOOLS_DEVTOOLS_SERVER_H_

#include <vector>

#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "components/ui_devtools/DOM.h"
#include "components/ui_devtools/Forward.h"
#include "components/ui_devtools/Protocol.h"
#include "components/ui_devtools/devtools_client.h"
#include "components/ui_devtools/devtools_export.h"
#include "components/ui_devtools/string_util.h"
#include "services/network/public/cpp/server/http_server.h"

namespace ui_devtools {

class UI_DEVTOOLS_EXPORT UiDevToolsServer
    : public network::server::HttpServer::Delegate {
 public:
  ~UiDevToolsServer() override;

  // Returns an empty unique_ptr if ui devtools flag isn't enabled or if a
  // server instance has already been created. Server doesn't know anything
  // about the caller, so both UI and Viz pass their corresponding params.
  static std::unique_ptr<UiDevToolsServer> Create(
      network::mojom::NetworkContext* network_context,
      const char* enable_devtools_flag,
      int default_port);

  // Returns a list of attached UiDevToolsClient name + URL
  using NameUrlPair = std::pair<std::string, std::string>;
  static std::vector<NameUrlPair> GetClientNamesAndUrls();

  void AttachClient(std::unique_ptr<UiDevToolsClient> client);
  void SendOverWebSocket(int connection_id, const String& message);

  int port() const { return port_; }

 private:
  UiDevToolsServer(const char* enable_devtools_flag, int default_port);

  void Start(network::mojom::NetworkContext* network_context,
             const std::string& address_string);
  void MakeServer(network::mojom::TCPServerSocketPtr server_socket,
                  int result,
                  const base::Optional<net::IPEndPoint>& local_addr);

  // HttpServer::Delegate
  void OnConnect(int connection_id) override;
  void OnHttpRequest(
      int connection_id,
      const network::server::HttpServerRequestInfo& info) override;
  void OnWebSocketRequest(
      int connection_id,
      const network::server::HttpServerRequestInfo& info) override;
  void OnWebSocketMessage(int connection_id, const std::string& data) override;
  void OnClose(int connection_id) override;

  using ClientsList = std::vector<std::unique_ptr<UiDevToolsClient>>;
  using ConnectionsMap = std::map<uint32_t, UiDevToolsClient*>;
  ClientsList clients_;
  ConnectionsMap connections_;

  std::unique_ptr<network::server::HttpServer> server_;

  // The port the devtools server listens on
  const int port_;

  // The server (owned by ash for now)
  static UiDevToolsServer* devtools_server_;

  SEQUENCE_CHECKER(devtools_server_sequence_);
  base::WeakPtrFactory<UiDevToolsServer> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(UiDevToolsServer);
};

}  // namespace ui_devtools

#endif  // COMPONENTS_UI_DEVTOOLS_DEVTOOLS_SERVER_H_
