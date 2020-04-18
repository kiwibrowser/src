// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_PLUGIN_PEPPER_PORT_ALLOCATOR_FACTORY_H_
#define REMOTING_CLIENT_PLUGIN_PEPPER_PORT_ALLOCATOR_FACTORY_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ppapi/cpp/instance_handle.h"
#include "remoting/client/plugin/pepper_network_manager.h"
#include "remoting/protocol/port_allocator_factory.h"

namespace remoting {

class PepperPortAllocatorFactory : public protocol::PortAllocatorFactory {
 public:
  PepperPortAllocatorFactory(
      pp::InstanceHandle pp_instance,
      PepperNetworkManager::NetworkInfoCallback callback);
  ~PepperPortAllocatorFactory() override;

   // PortAllocatorFactory interface.
  std::unique_ptr<cricket::PortAllocator> CreatePortAllocator(
      scoped_refptr<protocol::TransportContext> transport_context) override;

 private:
  pp::InstanceHandle pp_instance_;
  PepperNetworkManager::NetworkInfoCallback network_info_callback_;

  DISALLOW_COPY_AND_ASSIGN(PepperPortAllocatorFactory);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_PLUGIN_PEPPER_PORT_ALLOCATOR_FACTORY_H_
