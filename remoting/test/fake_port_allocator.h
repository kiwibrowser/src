// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_FAKE_PORT_ALLOCATOR_H_
#define REMOTING_TEST_FAKE_PORT_ALLOCATOR_H_

#include <memory>
#include <set>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "remoting/protocol/port_allocator_factory.h"
#include "third_party/webrtc/p2p/client/basicportallocator.h"

namespace remoting {

class FakeNetworkDispatcher;
class FakePacketSocketFactory;

class FakePortAllocator : public cricket::BasicPortAllocator {
 public:
  FakePortAllocator(
      rtc::NetworkManager* network_manager,
      rtc::PacketSocketFactory* socket_factory,
      scoped_refptr<protocol::TransportContext> transport_context_);
  ~FakePortAllocator() override;

  // cricket::BasicPortAllocator overrides.
  cricket::PortAllocatorSession* CreateSessionInternal(
      const std::string& content_name,
      int component,
      const std::string& ice_username_fragment,
      const std::string& ice_password) override;

 private:
  scoped_refptr<protocol::TransportContext> transport_context_;

  DISALLOW_COPY_AND_ASSIGN(FakePortAllocator);
};

class FakePortAllocatorFactory : public protocol::PortAllocatorFactory {
 public:
  FakePortAllocatorFactory(
      scoped_refptr<FakeNetworkDispatcher> fake_network_dispatcher);
  ~FakePortAllocatorFactory() override;

  FakePacketSocketFactory* socket_factory() { return socket_factory_.get(); }

   // PortAllocatorFactory interface.
  std::unique_ptr<cricket::PortAllocator> CreatePortAllocator(
      scoped_refptr<protocol::TransportContext> transport_context) override;

 private:
  std::unique_ptr<rtc::NetworkManager> network_manager_;
  std::unique_ptr<FakePacketSocketFactory> socket_factory_;

  DISALLOW_COPY_AND_ASSIGN(FakePortAllocatorFactory);
};

}  // namespace remoting

#endif  // REMOTING_TEST_FAKE_PORT_ALLOCATOR_H_
