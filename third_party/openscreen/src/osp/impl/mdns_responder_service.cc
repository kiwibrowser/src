// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/mdns_responder_service.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "osp/impl/internal_services.h"
#include "osp_base/error.h"
#include "platform/api/logging.h"

namespace openscreen {
namespace {

// TODO(btolsch): This should probably at least also contain network identity
// information.
std::string ServiceIdFromServiceInstanceName(
    const mdns::DomainName& service_instance) {
  std::string service_id;
  service_id.assign(
      reinterpret_cast<const char*>(service_instance.domain_name().data()),
      service_instance.domain_name().size());
  return service_id;
}

}  // namespace

MdnsResponderService::MdnsResponderService(
    const std::string& service_name,
    const std::string& service_protocol,
    std::unique_ptr<MdnsResponderAdapterFactory> mdns_responder_factory,
    std::unique_ptr<MdnsPlatformService> platform)
    : service_type_{{service_name, service_protocol}},
      mdns_responder_factory_(std::move(mdns_responder_factory)),
      platform_(std::move(platform)) {}

MdnsResponderService::~MdnsResponderService() = default;

void MdnsResponderService::SetServiceConfig(
    const std::string& hostname,
    const std::string& instance,
    uint16_t port,
    const std::vector<platform::NetworkInterfaceIndex> whitelist,
    const std::map<std::string, std::string>& txt_data) {
  OSP_DCHECK(!hostname.empty());
  OSP_DCHECK(!instance.empty());
  OSP_DCHECK_NE(0, port);
  service_hostname_ = hostname;
  service_instance_name_ = instance;
  service_port_ = port;
  interface_index_whitelist_ = whitelist;
  service_txt_data_ = txt_data;
}

void MdnsResponderService::HandleNewEvents(
    const std::vector<platform::ReceivedData>& data) {
  if (!mdns_responder_)
    return;
  for (auto& packet : data) {
    mdns_responder_->OnDataReceived(packet.source, packet.original_destination,
                                    packet.bytes.data(), packet.length,
                                    packet.socket);
  }
  mdns_responder_->RunTasks();

  HandleMdnsEvents();
}

void MdnsResponderService::StartListener() {
  if (!mdns_responder_)
    mdns_responder_ = mdns_responder_factory_->Create();

  StartListening();
  ServiceListenerImpl::Delegate::SetState(ServiceListener::State::kRunning);
}

void MdnsResponderService::StartAndSuspendListener() {
  mdns_responder_ = mdns_responder_factory_->Create();
  ServiceListenerImpl::Delegate::SetState(ServiceListener::State::kSuspended);
}

void MdnsResponderService::StopListener() {
  StopListening();
  if (!publisher_ || publisher_->state() == ServicePublisher::State::kStopped ||
      publisher_->state() == ServicePublisher::State::kSuspended) {
    StopMdnsResponder();
    if (!publisher_ || publisher_->state() == ServicePublisher::State::kStopped)
      mdns_responder_.reset();
  }
  ServiceListenerImpl::Delegate::SetState(ServiceListener::State::kStopped);
}

void MdnsResponderService::SuspendListener() {
  StopMdnsResponder();
  ServiceListenerImpl::Delegate::SetState(ServiceListener::State::kSuspended);
}

void MdnsResponderService::ResumeListener() {
  StartListening();
  ServiceListenerImpl::Delegate::SetState(ServiceListener::State::kRunning);
}

void MdnsResponderService::SearchNow(ServiceListener::State from) {
  ServiceListenerImpl::Delegate::SetState(from);
}

void MdnsResponderService::RunTasksListener() {
  InternalServices::RunEventLoopOnce();
}

void MdnsResponderService::StartPublisher() {
  if (!mdns_responder_)
    mdns_responder_ = mdns_responder_factory_->Create();

  StartService();
  ServicePublisherImpl::Delegate::SetState(ServicePublisher::State::kRunning);
}

void MdnsResponderService::StartAndSuspendPublisher() {
  mdns_responder_ = mdns_responder_factory_->Create();
  ServicePublisherImpl::Delegate::SetState(ServicePublisher::State::kSuspended);
}

void MdnsResponderService::StopPublisher() {
  StopService();
  if (!listener_ || listener_->state() == ServiceListener::State::kStopped ||
      listener_->state() == ServiceListener::State::kSuspended) {
    StopMdnsResponder();
    if (!listener_ || listener_->state() == ServiceListener::State::kStopped)
      mdns_responder_.reset();
  }
  ServicePublisherImpl::Delegate::SetState(ServicePublisher::State::kStopped);
}

void MdnsResponderService::SuspendPublisher() {
  StopService();
  ServicePublisherImpl::Delegate::SetState(ServicePublisher::State::kSuspended);
}

void MdnsResponderService::ResumePublisher() {
  StartService();
  ServicePublisherImpl::Delegate::SetState(ServicePublisher::State::kRunning);
}

void MdnsResponderService::RunTasksPublisher() {
  InternalServices::RunEventLoopOnce();
}

bool MdnsResponderService::NetworkScopedDomainNameComparator::operator()(
    const NetworkScopedDomainName& a,
    const NetworkScopedDomainName& b) const {
  if (a.socket != b.socket) {
    return (a.socket - b.socket) < 0;
  }
  return mdns::DomainNameComparator()(a.domain_name, b.domain_name);
}

void MdnsResponderService::HandleMdnsEvents() {
  // NOTE: In the common case, we will get a single combined packet for
  // PTR/SRV/TXT/A and then no other packets.  If we don't loop here, we would
  // start SRV/TXT queries based on the PTR response, but never check for events
  // again.  This should no longer be a problem when we have correct scheduling
  // of RunTasks.
  bool events_possible = false;
  // NOTE: This set will track which service instances were changed by all the
  // events throughout all the loop iterations.  At the end, we can dispatch our
  // ServiceInfo updates to |listener_| just once (e.g. instead of
  // OnReceiverChanged, OnReceiverChanged, ..., just a single
  // OnReceiverChanged).
  InstanceNameSet modified_instance_names;
  do {
    events_possible = false;
    for (auto& ptr_event : mdns_responder_->TakePtrResponses()) {
      events_possible = HandlePtrEvent(ptr_event, &modified_instance_names) ||
                        events_possible;
    }
    for (auto& srv_event : mdns_responder_->TakeSrvResponses()) {
      events_possible = HandleSrvEvent(srv_event, &modified_instance_names) ||
                        events_possible;
    }
    for (auto& txt_event : mdns_responder_->TakeTxtResponses()) {
      events_possible = HandleTxtEvent(txt_event, &modified_instance_names) ||
                        events_possible;
    }
    for (const auto& a_event : mdns_responder_->TakeAResponses()) {
      events_possible =
          HandleAEvent(a_event, &modified_instance_names) || events_possible;
    }
    for (const auto& aaaa_event : mdns_responder_->TakeAaaaResponses()) {
      events_possible = HandleAaaaEvent(aaaa_event, &modified_instance_names) ||
                        events_possible;
    }
    if (events_possible)
      mdns_responder_->RunTasks();
  } while (events_possible);

  for (const auto& instance_name : modified_instance_names) {
    auto service_entry = service_by_name_.find(instance_name);
    std::unique_ptr<ServiceInstance>& service = service_entry->second;

    std::string service_id = ServiceIdFromServiceInstanceName(instance_name);
    auto receiver_info_entry = receiver_info_.find(service_id);
    HostInfo* host = GetHostInfo(service->ptr_socket, service->domain_name);
    if (!IsServiceReady(*service, host)) {
      if (receiver_info_entry != receiver_info_.end()) {
        const ServiceInfo& receiver_info = receiver_info_entry->second;
        listener_->OnReceiverRemoved(receiver_info);
        receiver_info_.erase(receiver_info_entry);
      }
      if (!service->has_ptr_record && !service->has_srv())
        service_by_name_.erase(service_entry);
      continue;
    }

    // TODO(btolsch): Verify UTF-8 here.
    std::string friendly_name = instance_name.GetLabels()[0];

    if (receiver_info_entry == receiver_info_.end()) {
      ServiceInfo receiver_info{
          std::move(service_id),
          std::move(friendly_name),
          GetNetworkInterfaceIndexFromSocket(service->ptr_socket),
          {host->v4_address, service->port},
          {host->v6_address, service->port}};
      listener_->OnReceiverAdded(receiver_info);
      receiver_info_.emplace(receiver_info.service_id,
                             std::move(receiver_info));
    } else {
      ServiceInfo& receiver_info = receiver_info_entry->second;
      if (receiver_info.Update(
              std::move(friendly_name),
              GetNetworkInterfaceIndexFromSocket(service->ptr_socket),
              {host->v4_address, service->port},
              {host->v6_address, service->port})) {
        listener_->OnReceiverChanged(receiver_info);
      }
    }
  }
}

void MdnsResponderService::StartListening() {
  // TODO(btolsch): This needs the same |interface_index_whitelist_| logic as
  // StartService, but this can also wait until the network-change TODO is
  // addressed.
  if (bound_interfaces_.empty()) {
    mdns_responder_->Init();
    bound_interfaces_ = platform_->RegisterInterfaces({});
    for (auto& interface : bound_interfaces_) {
      mdns_responder_->RegisterInterface(interface.interface_info,
                                         interface.subnet, interface.socket);
    }
  }
  ErrorOr<mdns::DomainName> service_type =
      mdns::DomainName::FromLabels(service_type_.begin(), service_type_.end());
  OSP_CHECK(service_type);
  for (const auto& interface : bound_interfaces_)
    mdns_responder_->StartPtrQuery(interface.socket, service_type.value());
}

void MdnsResponderService::StopListening() {
  ErrorOr<mdns::DomainName> service_type =
      mdns::DomainName::FromLabels(service_type_.begin(), service_type_.end());
  OSP_CHECK(service_type);
  for (const auto& kv : network_scoped_domain_to_host_) {
    const NetworkScopedDomainName& scoped_domain = kv.first;

    mdns_responder_->StopAQuery(scoped_domain.socket,
                                scoped_domain.domain_name);
    mdns_responder_->StopAaaaQuery(scoped_domain.socket,
                                   scoped_domain.domain_name);
  }
  network_scoped_domain_to_host_.clear();
  for (const auto& service : service_by_name_) {
    platform::UdpSocket* const socket = service.second->ptr_socket;
    mdns_responder_->StopSrvQuery(socket, service.first);
    mdns_responder_->StopTxtQuery(socket, service.first);
  }
  service_by_name_.clear();
  for (const auto& interface : bound_interfaces_)
    mdns_responder_->StopPtrQuery(interface.socket, service_type.value());
  RemoveAllReceivers();
}

void MdnsResponderService::StartService() {
  // TODO(issue/45): This should really be a library-wide whitelist.
  if (!bound_interfaces_.empty() && !interface_index_whitelist_.empty()) {
    // TODO(btolsch): New interfaces won't be picked up on this path, but this
    // also highlights a larger issue of the interface list being frozen while
    // no state transitions are being made.  There should be another interface
    // on MdnsPlatformService for getting network interface updates.
    std::vector<MdnsPlatformService::BoundInterface> deregistered_interfaces;
    for (auto it = bound_interfaces_.begin(); it != bound_interfaces_.end();) {
      if (std::find(interface_index_whitelist_.begin(),
                    interface_index_whitelist_.end(),
                    it->interface_info.index) ==
          interface_index_whitelist_.end()) {
        mdns_responder_->DeregisterInterface(it->socket);
        deregistered_interfaces.push_back(*it);
        it = bound_interfaces_.erase(it);
      } else {
        ++it;
      }
    }
    platform_->DeregisterInterfaces(deregistered_interfaces);
  } else if (bound_interfaces_.empty()) {
    mdns_responder_->Init();
    mdns_responder_->SetHostLabel(service_hostname_);
    bound_interfaces_ =
        platform_->RegisterInterfaces(interface_index_whitelist_);
    for (auto& interface : bound_interfaces_) {
      mdns_responder_->RegisterInterface(interface.interface_info,
                                         interface.subnet, interface.socket);
    }
  }
  ErrorOr<mdns::DomainName> domain_name =
      mdns::DomainName::FromLabels(&service_hostname_, &service_hostname_ + 1);
  OSP_CHECK(domain_name) << "bad hostname configured: " << service_hostname_;
  mdns::DomainName name = domain_name.MoveValue();

  Error error = name.Append(mdns::DomainName::GetLocalDomain());
  OSP_CHECK(error.ok());

  mdns_responder_->RegisterService(service_instance_name_, service_type_[0],
                                   service_type_[1], name, service_port_,
                                   service_txt_data_);
}

void MdnsResponderService::StopService() {
  mdns_responder_->DeregisterService(service_instance_name_, service_type_[0],
                                     service_type_[1]);
}

void MdnsResponderService::StopMdnsResponder() {
  mdns_responder_->Close();
  platform_->DeregisterInterfaces(bound_interfaces_);
  bound_interfaces_.clear();
  network_scoped_domain_to_host_.clear();
  service_by_name_.clear();
  RemoveAllReceivers();
}

void MdnsResponderService::UpdatePendingServiceInfoSet(
    InstanceNameSet* modified_instance_names,
    const mdns::DomainName& domain_name) {
  for (auto& entry : service_by_name_) {
    const auto& instance_name = entry.first;
    const auto& instance = entry.second;
    if (instance->domain_name == domain_name) {
      modified_instance_names->emplace(instance_name);
    }
  }
}

void MdnsResponderService::RemoveAllReceivers() {
  bool had_receivers = !receiver_info_.empty();
  receiver_info_.clear();
  if (had_receivers)
    listener_->OnAllReceiversRemoved();
}

bool MdnsResponderService::HandlePtrEvent(
    const mdns::PtrEvent& ptr_event,
    InstanceNameSet* modified_instance_names) {
  bool events_possible = false;
  const auto& instance_name = ptr_event.service_instance;
  platform::UdpSocket* const socket = ptr_event.header.socket;
  auto entry = service_by_name_.find(ptr_event.service_instance);
  switch (ptr_event.header.response_type) {
    case mdns::QueryEventHeader::Type::kAddedNoCache:
      break;
    case mdns::QueryEventHeader::Type::kAdded: {
      if (entry != service_by_name_.end()) {
        entry->second->has_ptr_record = true;
        modified_instance_names->emplace(instance_name);
        break;
      }
      mdns_responder_->StartSrvQuery(socket, instance_name);
      mdns_responder_->StartTxtQuery(socket, instance_name);
      events_possible = true;

      auto new_instance = std::make_unique<ServiceInstance>();
      new_instance->ptr_socket = socket;
      new_instance->has_ptr_record = true;
      modified_instance_names->emplace(instance_name);
      service_by_name_.emplace(std::move(instance_name),
                               std::move(new_instance));
    } break;
    case mdns::QueryEventHeader::Type::kRemoved:
      if (entry == service_by_name_.end())
        break;
      if (entry->second->ptr_socket != socket)
        break;
      entry->second->has_ptr_record = false;
      // NOTE: Occasionally, we can observe this situation in the wild where the
      // PTR for a service is removed and then immediately re-added (like an odd
      // refresh).  Additionally, the recommended TTL of PTR records is much
      // shorter than the other records.  This means that short network drops or
      // latency spikes could cause the PTR refresh queries and/or responses to
      // be lost so the record isn't quite refreshed in time.  The solution here
      // and in HandleSrvEvent is to only remove the service records completely
      // when both the PTR and SRV have been removed.
      if (!entry->second->has_srv()) {
        mdns_responder_->StopSrvQuery(socket, instance_name);
        mdns_responder_->StopTxtQuery(socket, instance_name);
      }
      modified_instance_names->emplace(std::move(instance_name));
      break;
  }
  return events_possible;
}

bool MdnsResponderService::HandleSrvEvent(
    const mdns::SrvEvent& srv_event,
    InstanceNameSet* modified_instance_names) {
  bool events_possible = false;
  auto& domain_name = srv_event.domain_name;
  const auto& instance_name = srv_event.service_instance;
  platform::UdpSocket* const socket = srv_event.header.socket;
  auto entry = service_by_name_.find(srv_event.service_instance);
  if (entry == service_by_name_.end())
    return events_possible;
  switch (srv_event.header.response_type) {
    case mdns::QueryEventHeader::Type::kAddedNoCache:
      break;
    case mdns::QueryEventHeader::Type::kAdded: {
      NetworkScopedDomainName scoped_domain_name{socket, domain_name};
      auto host_entry = network_scoped_domain_to_host_.find(scoped_domain_name);
      if (host_entry == network_scoped_domain_to_host_.end()) {
        mdns_responder_->StartAQuery(socket, domain_name);
        mdns_responder_->StartAaaaQuery(socket, domain_name);
        events_possible = true;
        auto result = network_scoped_domain_to_host_.emplace(
            std::move(scoped_domain_name), HostInfo{});
        host_entry = result.first;
      }
      auto& dependent_services = host_entry->second.services;
      if (std::find_if(dependent_services.begin(), dependent_services.end(),
                       [entry](ServiceInstance* instance) {
                         return instance == entry->second.get();
                       }) == dependent_services.end()) {
        dependent_services.push_back(entry->second.get());
      }
      entry->second->domain_name = std::move(domain_name);
      entry->second->port = srv_event.port;
      modified_instance_names->emplace(std::move(instance_name));
    } break;
    case mdns::QueryEventHeader::Type::kRemoved: {
      NetworkScopedDomainName scoped_domain_name{socket, domain_name};
      auto host_entry = network_scoped_domain_to_host_.find(scoped_domain_name);
      if (host_entry != network_scoped_domain_to_host_.end()) {
        auto& dependent_services = host_entry->second.services;
        dependent_services.erase(
            std::remove_if(dependent_services.begin(), dependent_services.end(),
                           [entry](ServiceInstance* instance) {
                             return instance == entry->second.get();
                           }),
            dependent_services.end());
        if (dependent_services.empty()) {
          mdns_responder_->StopAQuery(socket, domain_name);
          mdns_responder_->StopAaaaQuery(socket, domain_name);
          network_scoped_domain_to_host_.erase(host_entry);
        }
      }
      entry->second->domain_name = mdns::DomainName();
      entry->second->port = 0;
      if (!entry->second->has_ptr_record) {
        mdns_responder_->StopSrvQuery(socket, instance_name);
        mdns_responder_->StopTxtQuery(socket, instance_name);
      }
      modified_instance_names->emplace(std::move(instance_name));
    } break;
  }
  return events_possible;
}

bool MdnsResponderService::HandleTxtEvent(
    const mdns::TxtEvent& txt_event,
    InstanceNameSet* modified_instance_names) {
  bool events_possible = false;
  const auto& instance_name = txt_event.service_instance;
  auto entry = service_by_name_.find(instance_name);
  if (entry == service_by_name_.end())
    return events_possible;
  switch (txt_event.header.response_type) {
    case mdns::QueryEventHeader::Type::kAddedNoCache:
      break;
    case mdns::QueryEventHeader::Type::kAdded:
      modified_instance_names->emplace(instance_name);
      if (entry == service_by_name_.end()) {
        auto result = service_by_name_.emplace(
            std::move(instance_name), std::make_unique<ServiceInstance>());
        entry = result.first;
      }
      entry->second->txt_info = std::move(txt_event.txt_info);
      break;
    case mdns::QueryEventHeader::Type::kRemoved:
      entry->second->txt_info.clear();
      modified_instance_names->emplace(std::move(instance_name));
      break;
  }
  return events_possible;
}

bool MdnsResponderService::HandleAddressEvent(
    platform::UdpSocket* socket,
    mdns::QueryEventHeader::Type response_type,
    const mdns::DomainName& domain_name,
    bool a_event,
    const IPAddress& address,
    InstanceNameSet* modified_instance_names) {
  bool events_possible = false;
  switch (response_type) {
    case mdns::QueryEventHeader::Type::kAddedNoCache:
      break;
    case mdns::QueryEventHeader::Type::kAdded: {
      HostInfo* host = AddOrGetHostInfo(socket, domain_name);
      if (a_event)
        host->v4_address = address;
      else
        host->v6_address = address;
      UpdatePendingServiceInfoSet(modified_instance_names, domain_name);
    } break;
    case mdns::QueryEventHeader::Type::kRemoved: {
      HostInfo* host = GetHostInfo(socket, domain_name);

      if (a_event)
        host->v4_address = IPAddress();
      else
        host->v6_address = IPAddress();

      if (host->v4_address || host->v6_address)
        UpdatePendingServiceInfoSet(modified_instance_names, domain_name);
    } break;
  }
  return events_possible;
}

bool MdnsResponderService::HandleAEvent(
    const mdns::AEvent& a_event,
    InstanceNameSet* modified_instance_names) {
  return HandleAddressEvent(a_event.header.socket, a_event.header.response_type,
                            a_event.domain_name, true, a_event.address,
                            modified_instance_names);
}

bool MdnsResponderService::HandleAaaaEvent(
    const mdns::AaaaEvent& aaaa_event,
    InstanceNameSet* modified_instance_names) {
  return HandleAddressEvent(aaaa_event.header.socket,
                            aaaa_event.header.response_type,
                            aaaa_event.domain_name, false, aaaa_event.address,
                            modified_instance_names);
}

MdnsResponderService::HostInfo* MdnsResponderService::AddOrGetHostInfo(
    platform::UdpSocket* socket,
    const mdns::DomainName& domain_name) {
  return &network_scoped_domain_to_host_[NetworkScopedDomainName{socket,
                                                                 domain_name}];
}

MdnsResponderService::HostInfo* MdnsResponderService::GetHostInfo(
    platform::UdpSocket* socket,
    const mdns::DomainName& domain_name) {
  auto kv = network_scoped_domain_to_host_.find(
      NetworkScopedDomainName{socket, domain_name});
  if (kv == network_scoped_domain_to_host_.end())
    return nullptr;

  return &kv->second;
}

bool MdnsResponderService::IsServiceReady(const ServiceInstance& instance,
                                          HostInfo* host) const {
  return (host && instance.has_ptr_record && instance.has_srv() &&
          !instance.txt_info.empty() && (host->v4_address || host->v6_address));
}

platform::NetworkInterfaceIndex
MdnsResponderService::GetNetworkInterfaceIndexFromSocket(
    const platform::UdpSocket* socket) const {
  auto it = std::find_if(
      bound_interfaces_.begin(), bound_interfaces_.end(),
      [socket](const MdnsPlatformService::BoundInterface& interface) {
        return interface.socket == socket;
      });
  if (it == bound_interfaces_.end())
    return platform::kInvalidNetworkInterfaceIndex;
  return it->interface_info.index;
}

}  // namespace openscreen
