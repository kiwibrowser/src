// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_PUBLIC_NETWORK_SERVICE_MANAGER_H_
#define OSP_PUBLIC_NETWORK_SERVICE_MANAGER_H_

#include <memory>

#include "osp/public/protocol_connection_client.h"
#include "osp/public/protocol_connection_server.h"
#include "osp/public/service_listener.h"
#include "osp/public/service_publisher.h"

namespace openscreen {

// Manages services run as part of the Open Screen Protocol Library.  Library
// embedders should pass instances of required services to Create(), which will
// instantiate the singleton instance of the NetworkServiceManager and take
// ownership of the services.
//
// NOTES: If we add more injectable services, consider implementing a struct to
// hold the config vs. passsing long argument lists.
class NetworkServiceManager final {
 public:
  // Creates the singleton instance of the NetworkServiceManager.  nullptr may
  // be passed for services not provided by the embedder.
  static NetworkServiceManager* Create(
      std::unique_ptr<ServiceListener> mdns_listener,
      std::unique_ptr<ServicePublisher> mdns_publisher,
      std::unique_ptr<ProtocolConnectionClient> connection_client,
      std::unique_ptr<ProtocolConnectionServer> connection_server);

  // Returns the singleton instance of the NetworkServiceManager previously
  // created by Create().
  static NetworkServiceManager* Get();

  // Destroys the NetworkServiceManager singleton and its owned services.  The
  // services must be shut down and no I/O or asynchronous work should be done
  // by the service instance destructors.
  static void Dispose();

  // Runs the event loop once for all of its owned services.  This mostly
  // consists of check for available network events and passing that data to the
  // listening services.
  void RunEventLoopOnce();

  // Returns an instance of the mDNS receiver listener, or nullptr if
  // not provided.
  ServiceListener* GetMdnsServiceListener();

  // Returns an instance of the mDNS receiver publisher, or nullptr
  // if not provided.
  ServicePublisher* GetMdnsServicePublisher();

  // Returns an instance of the protocol connection client, or nullptr
  // if not provided.
  ProtocolConnectionClient* GetProtocolConnectionClient();

  // Returns an instance of the protocol connection server, or nullptr if
  // not provided.
  ProtocolConnectionServer* GetProtocolConnectionServer();

 private:
  NetworkServiceManager(
      std::unique_ptr<ServiceListener> mdns_listener,
      std::unique_ptr<ServicePublisher> mdns_publisher,
      std::unique_ptr<ProtocolConnectionClient> connection_client,
      std::unique_ptr<ProtocolConnectionServer> connection_server);

  ~NetworkServiceManager();

  std::unique_ptr<ServiceListener> mdns_listener_;
  std::unique_ptr<ServicePublisher> mdns_publisher_;
  std::unique_ptr<ProtocolConnectionClient> connection_client_;
  std::unique_ptr<ProtocolConnectionServer> connection_server_;
};

}  // namespace openscreen

#endif  // OSP_PUBLIC_NETWORK_SERVICE_MANAGER_H_
