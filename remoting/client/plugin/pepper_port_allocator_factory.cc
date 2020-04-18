// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/plugin/pepper_port_allocator_factory.h"

#include <memory>
#include <utility>

#include "remoting/client/plugin/pepper_network_manager.h"
#include "remoting/client/plugin/pepper_packet_socket_factory.h"
#include "remoting/protocol/port_allocator.h"
#include "remoting/protocol/transport_context.h"

namespace remoting {

PepperPortAllocatorFactory::PepperPortAllocatorFactory(
    pp::InstanceHandle pp_instance,
    PepperNetworkManager::NetworkInfoCallback callback)
    : pp_instance_(pp_instance), network_info_callback_(callback) {}

PepperPortAllocatorFactory::~PepperPortAllocatorFactory() {}

std::unique_ptr<cricket::PortAllocator>
PepperPortAllocatorFactory::CreatePortAllocator(
    scoped_refptr<protocol::TransportContext> transport_context) {
  auto network_manager = std::make_unique<PepperNetworkManager>(pp_instance_);
  if (!transport_context->network_manager()) {
    // Only store (and notify network changes from) the first NetworkManager
    // instance.
    // TODO(crbug.com/848045): Remove this when NetworkManager becomes a
    // singleton.
    transport_context->set_network_manager(network_manager.get());
    network_manager->set_network_info_callback(network_info_callback_);
  }
  return std::make_unique<protocol::PortAllocator>(
      std::move(network_manager),
      std::make_unique<PepperPacketSocketFactory>(pp_instance_),
      transport_context);
}

}  // namespace remoting
