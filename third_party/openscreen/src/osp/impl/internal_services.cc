// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/internal_services.h"

#include <algorithm>

#include "osp/impl/discovery/mdns/mdns_responder_adapter_impl.h"
#include "osp/impl/mdns_responder_service.h"
#include "osp_base/error.h"
#include "platform/api/logging.h"
#include "platform/api/udp_socket.h"

namespace openscreen {
namespace {

constexpr char kServiceName[] = "_openscreen";
constexpr char kServiceProtocol[] = "_udp";
const IPAddress kMulticastAddress{224, 0, 0, 251};
const IPAddress kMulticastIPv6Address{
    // ff02::fb
    0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfb,
};
const uint16_t kMulticastListeningPort = 5353;

class MdnsResponderAdapterImplFactory final
    : public MdnsResponderAdapterFactory {
 public:
  MdnsResponderAdapterImplFactory() = default;
  ~MdnsResponderAdapterImplFactory() override = default;

  std::unique_ptr<mdns::MdnsResponderAdapter> Create() override {
    return std::make_unique<mdns::MdnsResponderAdapterImpl>();
  }
};

Error SetUpMulticastSocket(platform::UdpSocket* socket,
                           platform::NetworkInterfaceIndex ifindex) {
  const IPAddress broadcast_address =
      socket->IsIPv6() ? kMulticastIPv6Address : kMulticastAddress;

  Error result = socket->JoinMulticastGroup(broadcast_address, ifindex);
  if (!result.ok()) {
    OSP_LOG_ERROR << "join multicast group failed for interface " << ifindex
                  << ": " << result.message();
    return result;
  }

  result = socket->SetMulticastOutboundInterface(ifindex);
  if (!result.ok()) {
    OSP_LOG_ERROR << "set multicast outbound interface failed for interface "
                  << ifindex << ": " << result.message();
    return result;
  }

  result = socket->Bind({{}, kMulticastListeningPort});
  if (!result.ok()) {
    OSP_LOG_ERROR << "bind failed for interface " << ifindex << ": "
                  << result.message();
    return result;
  }

  return Error::None();
}

// Ref-counted singleton instance of InternalServices. This lives only as long
// as there is at least one ServiceListener and/or ServicePublisher alive.
InternalServices* g_instance = nullptr;
int g_instance_ref_count = 0;

}  // namespace

// static
void InternalServices::RunEventLoopOnce() {
  OSP_CHECK(g_instance) << "No listener or publisher is alive.";
  g_instance->mdns_service_.HandleNewEvents(
      platform::OnePlatformLoopIteration(g_instance->mdns_waiter_));
}

// static
std::unique_ptr<ServiceListener> InternalServices::CreateListener(
    const MdnsServiceListenerConfig& config,
    ServiceListener::Observer* observer) {
  auto* services = ReferenceSingleton();
  auto listener =
      std::make_unique<ServiceListenerImpl>(&services->mdns_service_);
  listener->AddObserver(observer);
  listener->SetDestructionCallback(&InternalServices::DereferenceSingleton,
                                   services);
  return listener;
}

// static
std::unique_ptr<ServicePublisher> InternalServices::CreatePublisher(
    const ServicePublisher::Config& config,
    ServicePublisher::Observer* observer) {
  auto* services = ReferenceSingleton();
  services->mdns_service_.SetServiceConfig(
      config.hostname, config.service_instance_name,
      config.connection_server_port, config.network_interface_indices,
      {{"fn", config.friendly_name}});
  auto publisher = std::make_unique<ServicePublisherImpl>(
      observer, &services->mdns_service_);
  publisher->SetDestructionCallback(&InternalServices::DereferenceSingleton,
                                    services);
  return publisher;
}

InternalServices::InternalPlatformLinkage::InternalPlatformLinkage(
    InternalServices* parent)
    : parent_(parent) {}

InternalServices::InternalPlatformLinkage::~InternalPlatformLinkage() {
  // If there are open sockets, then there will be dangling references to
  // destroyed objects after destruction.
  OSP_CHECK(open_sockets_.empty());
}

std::vector<MdnsPlatformService::BoundInterface>
InternalServices::InternalPlatformLinkage::RegisterInterfaces(
    const std::vector<platform::NetworkInterfaceIndex>& whitelist) {
  auto addrinfo = platform::GetInterfaceAddresses();
  const bool do_filter_using_whitelist = !whitelist.empty();
  std::vector<platform::NetworkInterfaceIndex> index_list;
  for (const auto& interface : addrinfo) {
    OSP_VLOG << "Found interface: " << interface;
    if (do_filter_using_whitelist &&
        std::find(whitelist.begin(), whitelist.end(), interface.info.index) ==
            whitelist.end()) {
      OSP_VLOG << "Ignoring interface not in whitelist: " << interface.info;
      continue;
    }
    if (!interface.addresses.empty())
      index_list.push_back(interface.info.index);
  }

  // Set up sockets to send and listen to mDNS multicast traffic on all
  // interfaces.
  std::vector<BoundInterface> result;
  for (platform::NetworkInterfaceIndex index : index_list) {
    const auto& addr =
        *std::find_if(addrinfo.begin(), addrinfo.end(),
                      [index](const platform::InterfaceAddresses& addr) {
                        return addr.info.index == index;
                      });
    if (addr.addresses.empty()) {
      continue;
    }

    // Pick any address for the given interface.
    const platform::IPSubnet& primary_subnet = addr.addresses.front();

    auto create_result =
        platform::UdpSocket::Create(primary_subnet.address.version());
    if (!create_result) {
      OSP_LOG_ERROR << "failed to create socket for interface " << index << ": "
                    << create_result.error().message();
      continue;
    }
    platform::UdpSocketUniquePtr socket = create_result.MoveValue();
    if (!SetUpMulticastSocket(socket.get(), index).ok()) {
      continue;
    }
    result.emplace_back(addr.info, primary_subnet, socket.get());
    parent_->RegisterMdnsSocket(socket.get());
    open_sockets_.emplace_back(std::move(socket));
  }

  return result;
}

void InternalServices::InternalPlatformLinkage::DeregisterInterfaces(
    const std::vector<BoundInterface>& registered_interfaces) {
  for (const auto& interface : registered_interfaces) {
    platform::UdpSocket* const socket = interface.socket;
    parent_->DeregisterMdnsSocket(socket);

    const auto it =
        std::find_if(open_sockets_.begin(), open_sockets_.end(),
                     [socket](const platform::UdpSocketUniquePtr& s) {
                       return s.get() == socket;
                     });
    OSP_DCHECK(it != open_sockets_.end());
    open_sockets_.erase(it);
  }
}

InternalServices::InternalServices()
    : mdns_service_(kServiceName,
                    kServiceProtocol,
                    std::make_unique<MdnsResponderAdapterImplFactory>(),
                    std::make_unique<InternalPlatformLinkage>(this)),
      mdns_waiter_(platform::CreateEventWaiter()) {
  OSP_DCHECK(mdns_waiter_);
}

InternalServices::~InternalServices() {
  DestroyEventWaiter(mdns_waiter_);
}

void InternalServices::RegisterMdnsSocket(platform::UdpSocket* socket) {
  platform::WatchUdpSocketReadable(mdns_waiter_, socket);
}

void InternalServices::DeregisterMdnsSocket(platform::UdpSocket* socket) {
  platform::StopWatchingUdpSocketReadable(mdns_waiter_, socket);
}

// static
InternalServices* InternalServices::ReferenceSingleton() {
  if (!g_instance) {
    OSP_CHECK_EQ(g_instance_ref_count, 0);
    g_instance = new InternalServices();
  }
  ++g_instance_ref_count;
  return g_instance;
}

// static
void InternalServices::DereferenceSingleton(void* instance) {
  OSP_CHECK_EQ(static_cast<InternalServices*>(instance), g_instance);
  OSP_CHECK_GT(g_instance_ref_count, 0);
  --g_instance_ref_count;
  if (g_instance_ref_count == 0) {
    delete g_instance;
    g_instance = nullptr;
  }
}

}  // namespace openscreen
