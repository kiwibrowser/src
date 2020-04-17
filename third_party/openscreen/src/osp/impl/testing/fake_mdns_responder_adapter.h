// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_TESTING_FAKE_MDNS_RESPONDER_ADAPTER_H_
#define OSP_IMPL_TESTING_FAKE_MDNS_RESPONDER_ADAPTER_H_

#include <set>
#include <vector>

#include "osp/impl/discovery/mdns/mdns_responder_adapter.h"

namespace openscreen {

class FakeMdnsResponderAdapter;

mdns::PtrEvent MakePtrEvent(const std::string& service_instance,
                            const std::string& service_type,
                            const std::string& service_protocol,
                            platform::UdpSocket* socket);

mdns::SrvEvent MakeSrvEvent(const std::string& service_instance,
                            const std::string& service_type,
                            const std::string& service_protocol,
                            const std::string& hostname,
                            uint16_t port,
                            platform::UdpSocket* socket);

mdns::TxtEvent MakeTxtEvent(const std::string& service_instance,
                            const std::string& service_type,
                            const std::string& service_protocol,
                            const std::vector<std::string>& txt_lines,
                            platform::UdpSocket* socket);

mdns::AEvent MakeAEvent(const std::string& hostname,
                        IPAddress address,
                        platform::UdpSocket* socket);

mdns::AaaaEvent MakeAaaaEvent(const std::string& hostname,
                              IPAddress address,
                              platform::UdpSocket* socket);

void AddEventsForNewService(FakeMdnsResponderAdapter* mdns_responder,
                            const std::string& service_instance,
                            const std::string& service_name,
                            const std::string& service_protocol,
                            const std::string& hostname,
                            uint16_t port,
                            const std::vector<std::string>& txt_lines,
                            const IPAddress& address,
                            platform::UdpSocket* socket);

class FakeMdnsResponderAdapter final : public mdns::MdnsResponderAdapter {
 public:
  struct RegisteredInterface {
    platform::InterfaceInfo interface_info;
    platform::IPSubnet interface_address;
    platform::UdpSocket* socket;
  };

  struct RegisteredService {
    std::string service_instance;
    std::string service_name;
    std::string service_protocol;
    mdns::DomainName target_host;
    uint16_t target_port;
    std::map<std::string, std::string> txt_data;
  };

  class LifetimeObserver {
   public:
    virtual ~LifetimeObserver() = default;

    virtual void OnDestroyed() = 0;
  };

  virtual ~FakeMdnsResponderAdapter() override;

  void SetLifetimeObserver(LifetimeObserver* observer) { observer_ = observer; }

  void AddPtrEvent(mdns::PtrEvent&& ptr_event);
  void AddSrvEvent(mdns::SrvEvent&& srv_event);
  void AddTxtEvent(mdns::TxtEvent&& txt_event);
  void AddAEvent(mdns::AEvent&& a_event);
  void AddAaaaEvent(mdns::AaaaEvent&& aaaa_event);

  const std::vector<RegisteredInterface>& registered_interfaces() {
    return registered_interfaces_;
  }
  const std::vector<RegisteredService>& registered_services() {
    return registered_services_;
  }
  bool ptr_queries_empty() const;
  bool srv_queries_empty() const;
  bool txt_queries_empty() const;
  bool a_queries_empty() const;
  bool aaaa_queries_empty() const;
  bool running() const { return running_; }

  // mdns::MdnsResponderAdapter overrides.
  Error Init() override;
  void Close() override;

  Error SetHostLabel(const std::string& host_label) override;

  // TODO(btolsch): Reject/OSP_CHECK events that don't match any registered
  // interface?
  Error RegisterInterface(const platform::InterfaceInfo& interface_info,
                          const platform::IPSubnet& interface_address,
                          platform::UdpSocket* socket) override;
  Error DeregisterInterface(platform::UdpSocket* socket) override;

  void OnDataReceived(const IPEndpoint& source,
                      const IPEndpoint& original_destination,
                      const uint8_t* data,
                      size_t length,
                      platform::UdpSocket* receiving_socket) override;

  int RunTasks() override;

  std::vector<mdns::PtrEvent> TakePtrResponses() override;
  std::vector<mdns::SrvEvent> TakeSrvResponses() override;
  std::vector<mdns::TxtEvent> TakeTxtResponses() override;
  std::vector<mdns::AEvent> TakeAResponses() override;
  std::vector<mdns::AaaaEvent> TakeAaaaResponses() override;

  mdns::MdnsResponderErrorCode StartPtrQuery(
      platform::UdpSocket* socket,
      const mdns::DomainName& service_type) override;
  mdns::MdnsResponderErrorCode StartSrvQuery(
      platform::UdpSocket* socket,
      const mdns::DomainName& service_instance) override;
  mdns::MdnsResponderErrorCode StartTxtQuery(
      platform::UdpSocket* socket,
      const mdns::DomainName& service_instance) override;
  mdns::MdnsResponderErrorCode StartAQuery(
      platform::UdpSocket* socket,
      const mdns::DomainName& domain_name) override;
  mdns::MdnsResponderErrorCode StartAaaaQuery(
      platform::UdpSocket* socket,
      const mdns::DomainName& domain_name) override;

  mdns::MdnsResponderErrorCode StopPtrQuery(
      platform::UdpSocket* socket,
      const mdns::DomainName& service_type) override;
  mdns::MdnsResponderErrorCode StopSrvQuery(
      platform::UdpSocket* socket,
      const mdns::DomainName& service_instance) override;
  mdns::MdnsResponderErrorCode StopTxtQuery(
      platform::UdpSocket* socket,
      const mdns::DomainName& service_instance) override;
  mdns::MdnsResponderErrorCode StopAQuery(
      platform::UdpSocket* socket,
      const mdns::DomainName& domain_name) override;
  mdns::MdnsResponderErrorCode StopAaaaQuery(
      platform::UdpSocket* socket,
      const mdns::DomainName& domain_name) override;

  mdns::MdnsResponderErrorCode RegisterService(
      const std::string& service_instance,
      const std::string& service_name,
      const std::string& service_protocol,
      const mdns::DomainName& target_host,
      uint16_t target_port,
      const std::map<std::string, std::string>& txt_data) override;
  mdns::MdnsResponderErrorCode DeregisterService(
      const std::string& service_instance,
      const std::string& service_name,
      const std::string& service_protocol) override;
  mdns::MdnsResponderErrorCode UpdateTxtData(
      const std::string& service_instance,
      const std::string& service_name,
      const std::string& service_protocol,
      const std::map<std::string, std::string>& txt_data) override;

 private:
  struct InterfaceQueries {
    std::set<mdns::DomainName, mdns::DomainNameComparator> a_queries;
    std::set<mdns::DomainName, mdns::DomainNameComparator> aaaa_queries;
    std::set<mdns::DomainName, mdns::DomainNameComparator> ptr_queries;
    std::set<mdns::DomainName, mdns::DomainNameComparator> srv_queries;
    std::set<mdns::DomainName, mdns::DomainNameComparator> txt_queries;
  };

  bool running_ = false;
  LifetimeObserver* observer_ = nullptr;

  std::map<platform::UdpSocket*, InterfaceQueries> queries_;
  // NOTE: One of many simplifications here is that there is no cache.  This
  // means that calling StartQuery, StopQuery, StartQuery will only return an
  // event the first time, unless the test also adds the event a second time.
  std::vector<mdns::PtrEvent> ptr_events_;
  std::vector<mdns::SrvEvent> srv_events_;
  std::vector<mdns::TxtEvent> txt_events_;
  std::vector<mdns::AEvent> a_events_;
  std::vector<mdns::AaaaEvent> aaaa_events_;

  std::vector<RegisteredInterface> registered_interfaces_;
  std::vector<RegisteredService> registered_services_;
};

}  // namespace openscreen

#endif  // OSP_IMPL_TESTING_FAKE_MDNS_RESPONDER_ADAPTER_H_
