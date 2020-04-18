// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_IMPL_NET_SYNC_SERVER_CONNECTION_MANAGER_H_
#define COMPONENTS_SYNC_ENGINE_IMPL_NET_SYNC_SERVER_CONNECTION_MANAGER_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "components/sync/engine_impl/net/server_connection_manager.h"

namespace syncer {

class HttpPostProviderFactory;
class HttpPostProviderInterface;

// This provides HTTP Post functionality through the interface provided
// by the application hosting the syncer backend.
class SyncBridgedConnection : public ServerConnectionManager::Connection,
                              public CancelationObserver {
 public:
  SyncBridgedConnection(ServerConnectionManager* scm,
                        HttpPostProviderFactory* factory,
                        CancelationSignal* cancelation_signal);

  ~SyncBridgedConnection() override;

  bool Init(const char* path,
            const std::string& auth_token,
            const std::string& payload,
            HttpResponse* response) override;

  void Abort() override;

  void OnSignalReceived() override;

 private:
  // Pointer to the factory we use for creating HttpPostProviders. We do not
  // own |factory_|.
  HttpPostProviderFactory* factory_;

  // Cancelation signal is signalled when engine shuts down. Current blocking
  // operation should be aborted.
  CancelationSignal* cancelation_signal_;

  HttpPostProviderInterface* post_provider_;

  DISALLOW_COPY_AND_ASSIGN(SyncBridgedConnection);
};

// A ServerConnectionManager subclass that generates a POST object using an
// instance of the HttpPostProviderFactory class.
class SyncServerConnectionManager : public ServerConnectionManager {
 public:
  // Takes ownership of factory.
  SyncServerConnectionManager(const std::string& server,
                              int port,
                              bool use_ssl,
                              HttpPostProviderFactory* factory,
                              CancelationSignal* cancelation_signal);
  ~SyncServerConnectionManager() override;

  // ServerConnectionManager overrides.
  std::unique_ptr<Connection> MakeConnection() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(SyncServerConnectionManagerTest, VeryEarlyAbortPost);
  FRIEND_TEST_ALL_PREFIXES(SyncServerConnectionManagerTest, EarlyAbortPost);
  FRIEND_TEST_ALL_PREFIXES(SyncServerConnectionManagerTest, AbortPost);
  FRIEND_TEST_ALL_PREFIXES(SyncServerConnectionManagerTest,
                           FailPostWithTimedOut);

  // A factory creating concrete HttpPostProviders for use whenever we need to
  // issue a POST to sync servers.
  std::unique_ptr<HttpPostProviderFactory> post_provider_factory_;

  // Cancelation signal is signalled when engine shuts down. Current blocking
  // operation should be aborted.
  CancelationSignal* cancelation_signal_;

  DISALLOW_COPY_AND_ASSIGN(SyncServerConnectionManager);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_IMPL_NET_SYNC_SERVER_CONNECTION_MANAGER_H_
