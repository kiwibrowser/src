// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <signal.h>
#include <unistd.h>

#include <algorithm>
#include <map>
#include <memory>
#include <vector>

#include "osp/impl/discovery/mdns/mdns_responder_adapter_impl.h"
#include "osp_base/error.h"
#include "platform/api/logging.h"

// This file contains a demo of our mDNSResponder wrapper code.  It can both
// listen for mDNS services and advertise an mDNS service.  The command-line
// usage is:
//   embedder_demo [service_type] [service_instance_name]
// service_type defaults to '_openscreen._udp' and service_instance_name
// defaults to ''.  service_type determines services the program listens for and
// when service_instance_name is not empty, a service of
// 'service_instance_name.service_type' is also advertised.
//
// The program will print a list of discovered services when it receives a USR1
// or INT signal.  The pid is printed at the beginning of the program to
// facilitate this.
//
// There are a few known bugs around the handling of record events, so this
// shouldn't be expected to be a source of truth, nor should it be expected to
// be correct after running for a long time.
namespace openscreen {
namespace {

bool g_done = false;
bool g_dump_services = false;

struct Service {
  explicit Service(mdns::DomainName service_instance)
      : service_instance(std::move(service_instance)) {}
  ~Service() = default;

  mdns::DomainName service_instance;
  mdns::DomainName domain_name;
  IPAddress address;
  uint16_t port;
  std::vector<std::string> txt;
};

using ServiceMap =
    std::map<mdns::DomainName, Service, mdns::DomainNameComparator>;
ServiceMap* g_services = nullptr;

void sigusr1_dump_services(int) {
  g_dump_services = true;
}

void sigint_stop(int) {
  OSP_LOG << "caught SIGINT, exiting...";
  g_done = true;
}

std::vector<std::string> SplitByDot(const std::string& domain_part) {
  std::vector<std::string> result;
  auto copy_it = domain_part.begin();
  for (auto it = domain_part.begin(); it != domain_part.end(); ++it) {
    if (*it == '.') {
      result.emplace_back(copy_it, it);
      copy_it = it + 1;
    }
  }
  if (copy_it != domain_part.end())
    result.emplace_back(copy_it, domain_part.end());

  return result;
}

void SignalThings() {
  struct sigaction usr1_sa;
  struct sigaction int_sa;
  struct sigaction unused;

  usr1_sa.sa_handler = &sigusr1_dump_services;
  sigemptyset(&usr1_sa.sa_mask);
  usr1_sa.sa_flags = 0;

  int_sa.sa_handler = &sigint_stop;
  sigemptyset(&int_sa.sa_mask);
  int_sa.sa_flags = 0;

  sigaction(SIGUSR1, &usr1_sa, &unused);
  sigaction(SIGINT, &int_sa, &unused);

  OSP_LOG << "signal handlers setup" << std::endl << "pid: " << getpid();
}

std::vector<platform::UdpSocketUniquePtr> SetUpMulticastSockets(
    const std::vector<platform::NetworkInterfaceIndex>& index_list) {
  std::vector<platform::UdpSocketUniquePtr> sockets;
  for (const auto ifindex : index_list) {
    auto create_result =
        platform::UdpSocket::Create(platform::UdpSocket::Version::kV4);
    if (!create_result) {
      OSP_LOG_ERROR << "failed to create IPv4 socket for interface " << ifindex
                    << ": " << create_result.error().message();
      continue;
    }
    platform::UdpSocketUniquePtr socket = create_result.MoveValue();

    Error result =
        socket->JoinMulticastGroup(IPAddress{224, 0, 0, 251}, ifindex);
    if (!result.ok()) {
      OSP_LOG_ERROR << "join multicast group failed for interface " << ifindex
                    << ": " << result.message();
      continue;
    }

    result = socket->SetMulticastOutboundInterface(ifindex);
    if (!result.ok()) {
      OSP_LOG_ERROR << "set multicast outbound interface failed for interface "
                    << ifindex << ": " << result.message();
      continue;
    }

    result = socket->Bind({{}, 5353});
    if (!result.ok()) {
      OSP_LOG_ERROR << "bind failed for interface " << ifindex << ": "
                    << result.message();
      continue;
    }

    OSP_LOG << "listening on interface " << ifindex;
    sockets.emplace_back(std::move(socket));
  }
  return sockets;
}

void LogService(const Service& s) {
  OSP_LOG << "PTR: (" << s.service_instance << ")" << std::endl
          << "SRV: " << s.domain_name << ":" << s.port << std::endl
          << "TXT:";

  for (const auto& l : s.txt) {
    OSP_LOG << " | " << l;
  }
  OSP_LOG << "A: " << s.address;
}

void HandleEvents(mdns::MdnsResponderAdapterImpl* mdns_adapter) {
  for (auto& ptr_event : mdns_adapter->TakePtrResponses()) {
    auto it = g_services->find(ptr_event.service_instance);
    switch (ptr_event.header.response_type) {
      case mdns::QueryEventHeader::Type::kAdded:
      case mdns::QueryEventHeader::Type::kAddedNoCache:
        mdns_adapter->StartSrvQuery(ptr_event.header.socket,
                                    ptr_event.service_instance);
        mdns_adapter->StartTxtQuery(ptr_event.header.socket,
                                    ptr_event.service_instance);
        if (it == g_services->end()) {
          g_services->emplace(ptr_event.service_instance,
                              Service(ptr_event.service_instance));
        }
        break;
      case mdns::QueryEventHeader::Type::kRemoved:
        // PTR may be removed and added without updating related entries (SRV
        // and friends) so this simple logic is actually broken, but I don't
        // want to do a better design or pointer hell for just a demo.
        OSP_LOG_WARN << "ptr-remove: " << ptr_event.service_instance;
        if (it != g_services->end())
          g_services->erase(it);

        break;
    }
  }
  for (auto& srv_event : mdns_adapter->TakeSrvResponses()) {
    auto it = g_services->find(srv_event.service_instance);
    if (it == g_services->end())
      continue;

    switch (srv_event.header.response_type) {
      case mdns::QueryEventHeader::Type::kAdded:
      case mdns::QueryEventHeader::Type::kAddedNoCache:
        mdns_adapter->StartAQuery(srv_event.header.socket,
                                  srv_event.domain_name);
        it->second.domain_name = std::move(srv_event.domain_name);
        it->second.port = srv_event.port;
        break;
      case mdns::QueryEventHeader::Type::kRemoved:
        OSP_LOG_WARN << "srv-remove: " << srv_event.service_instance;
        it->second.domain_name = mdns::DomainName();
        it->second.port = 0;
        break;
    }
  }
  for (auto& txt_event : mdns_adapter->TakeTxtResponses()) {
    auto it = g_services->find(txt_event.service_instance);
    if (it == g_services->end())
      continue;

    switch (txt_event.header.response_type) {
      case mdns::QueryEventHeader::Type::kAdded:
      case mdns::QueryEventHeader::Type::kAddedNoCache:
        it->second.txt = std::move(txt_event.txt_info);
        break;
      case mdns::QueryEventHeader::Type::kRemoved:
        OSP_LOG_WARN << "txt-remove: " << txt_event.service_instance;
        it->second.txt.clear();
        break;
    }
  }
  for (const auto& a_event : mdns_adapter->TakeAResponses()) {
    // TODO: If multiple SRV records specify the same domain, the A will only
    // update the first.  I didn't think this would happen but I noticed this
    // happens for cast groups.
    auto it =
        std::find_if(g_services->begin(), g_services->end(),
                     [&a_event](const std::pair<mdns::DomainName, Service>& s) {
                       return s.second.domain_name == a_event.domain_name;
                     });
    if (it == g_services->end())
      continue;

    switch (a_event.header.response_type) {
      case mdns::QueryEventHeader::Type::kAdded:
      case mdns::QueryEventHeader::Type::kAddedNoCache:
        it->second.address = a_event.address;
        break;
      case mdns::QueryEventHeader::Type::kRemoved:
        OSP_LOG_WARN << "a-remove: " << a_event.domain_name;
        it->second.address = IPAddress(0, 0, 0, 0);
        break;
    }
  }
}

void BrowseDemo(const std::string& service_name,
                const std::string& service_protocol,
                const std::string& service_instance) {
  SignalThings();

  std::vector<std::string> labels{service_name, service_protocol};
  ErrorOr<mdns::DomainName> service_type =
      mdns::DomainName::FromLabels(labels.begin(), labels.end());

  if (!service_type) {
    OSP_LOG_ERROR << "bad domain labels: " << service_name << ", "
                  << service_protocol;
    return;
  }

  auto mdns_adapter = std::make_unique<mdns::MdnsResponderAdapterImpl>();
  platform::EventWaiterPtr waiter = platform::CreateEventWaiter();
  mdns_adapter->Init();
  mdns_adapter->SetHostLabel("gigliorononomicon");
  auto interface_addresses = platform::GetInterfaceAddresses();
  for (const auto& ifa : interface_addresses) {
    OSP_LOG << "Found interface: " << ifa;
  }

  std::vector<platform::NetworkInterfaceIndex> index_list;
  for (const auto& interface : interface_addresses) {
    if (!interface.addresses.empty())
      index_list.push_back(interface.info.index);
  }

  auto sockets = SetUpMulticastSockets(index_list);
  // The code below assumes the elements in |sockets| is in exact 1:1
  // correspondence with the elements in |index_list|. Crash the demo if any
  // sockets are missing (i.e., failed to be set up).
  OSP_CHECK_EQ(sockets.size(), index_list.size());

  // Listen on all interfaces.
  auto socket_it = sockets.begin();
  for (platform::NetworkInterfaceIndex index : index_list) {
    const auto& addr = *std::find_if(
        interface_addresses.begin(), interface_addresses.end(),
        [index](const openscreen::platform::InterfaceAddresses& addr) {
          return addr.info.index == index;
        });
    // Pick any address for the given interface.
    mdns_adapter->RegisterInterface(addr.info, addr.addresses.front(),
                                    socket_it->get());
    ++socket_it;
  }

  if (!service_instance.empty()) {
    mdns_adapter->RegisterService(service_instance, service_name,
                                  service_protocol, mdns::DomainName(), 12345,
                                  {{"k1", "yurtle"}, {"k2", "turtle"}});
  }

  for (const platform::UdpSocketUniquePtr& socket : sockets) {
    platform::WatchUdpSocketReadable(waiter, socket.get());
    mdns_adapter->StartPtrQuery(socket.get(), service_type.value());
  }

  while (!g_done) {
    HandleEvents(mdns_adapter.get());
    if (g_dump_services) {
      OSP_LOG << "num services: " << g_services->size();
      for (const auto& s : *g_services) {
        LogService(s.second);
      }
      if (!service_instance.empty()) {
        mdns_adapter->UpdateTxtData(
            service_instance, service_name, service_protocol,
            {{"k1", "oogley"}, {"k2", "moogley"}, {"k3", "googley"}});
      }
      g_dump_services = false;
    }
    mdns_adapter->RunTasks();
    auto data = platform::OnePlatformLoopIteration(waiter);
    for (auto& packet : data) {
      mdns_adapter->OnDataReceived(packet.source, packet.original_destination,
                                   packet.bytes.data(), packet.length,
                                   packet.socket);
    }
  }
  OSP_LOG << "num services: " << g_services->size();
  for (const auto& s : *g_services) {
    LogService(s.second);
  }
  platform::StopWatchingNetworkChange(waiter);
  for (const platform::UdpSocketUniquePtr& socket : sockets) {
    platform::StopWatchingUdpSocketReadable(waiter, socket.get());
    mdns_adapter->DeregisterInterface(socket.get());
  }
  platform::DestroyEventWaiter(waiter);
  mdns_adapter->Close();
}

}  // namespace
}  // namespace openscreen

int main(int argc, char** argv) {
  openscreen::platform::LogInit(nullptr);
  openscreen::platform::SetLogLevel(openscreen::platform::LogLevel::kVerbose);

  std::string service_instance;
  std::string service_type("_openscreen._udp");
  if (argc >= 2)
    service_type = argv[1];

  if (argc >= 3)
    service_instance = argv[2];

  if (service_type.size() && service_type[0] == '.')
    return 1;

  auto labels = openscreen::SplitByDot(service_type);
  if (labels.size() != 2)
    return 1;

  openscreen::ServiceMap services;
  openscreen::g_services = &services;
  openscreen::BrowseDemo(labels[0], labels[1], service_instance);
  openscreen::g_services = nullptr;
  return 0;
}
