// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_DISCOVERY_MDNS_MDNS_RESPONDER_ADAPTER_IMPL_H_
#define OSP_IMPL_DISCOVERY_MDNS_MDNS_RESPONDER_ADAPTER_IMPL_H_

#include <map>
#include <memory>
#include <vector>

#include "osp/impl/discovery/mdns/mdns_responder_adapter.h"
#include "osp_base/error.h"
#include "platform/api/udp_socket.h"
#include "third_party/mDNSResponder/src/mDNSCore/mDNSEmbeddedAPI.h"

namespace openscreen {
namespace mdns {

class MdnsResponderAdapterImpl final : public MdnsResponderAdapter {
 public:
  static constexpr int kRrCacheSize = 500;

  MdnsResponderAdapterImpl();
  ~MdnsResponderAdapterImpl() override;

  Error Init() override;
  void Close() override;

  Error SetHostLabel(const std::string& host_label) override;

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

  std::vector<PtrEvent> TakePtrResponses() override;
  std::vector<SrvEvent> TakeSrvResponses() override;
  std::vector<TxtEvent> TakeTxtResponses() override;
  std::vector<AEvent> TakeAResponses() override;
  std::vector<AaaaEvent> TakeAaaaResponses() override;

  MdnsResponderErrorCode StartPtrQuery(platform::UdpSocket* socket,
                                       const DomainName& service_type) override;
  MdnsResponderErrorCode StartSrvQuery(
      platform::UdpSocket* socket,
      const DomainName& service_instance) override;
  MdnsResponderErrorCode StartTxtQuery(
      platform::UdpSocket* socket,
      const DomainName& service_instance) override;
  MdnsResponderErrorCode StartAQuery(platform::UdpSocket* socket,
                                     const DomainName& domain_name) override;
  MdnsResponderErrorCode StartAaaaQuery(platform::UdpSocket* socket,
                                        const DomainName& domain_name) override;
  MdnsResponderErrorCode StopPtrQuery(platform::UdpSocket* socket,
                                      const DomainName& service_type) override;
  MdnsResponderErrorCode StopSrvQuery(
      platform::UdpSocket* socket,
      const DomainName& service_instance) override;
  MdnsResponderErrorCode StopTxtQuery(
      platform::UdpSocket* socket,
      const DomainName& service_instance) override;
  MdnsResponderErrorCode StopAQuery(platform::UdpSocket* socket,
                                    const DomainName& domain_name) override;
  MdnsResponderErrorCode StopAaaaQuery(platform::UdpSocket* socket,
                                       const DomainName& domain_name) override;

  MdnsResponderErrorCode RegisterService(
      const std::string& service_instance,
      const std::string& service_name,
      const std::string& service_protocol,
      const DomainName& target_host,
      uint16_t target_port,
      const std::map<std::string, std::string>& txt_data) override;
  MdnsResponderErrorCode DeregisterService(
      const std::string& service_instance,
      const std::string& service_name,
      const std::string& service_protocol) override;
  MdnsResponderErrorCode UpdateTxtData(
      const std::string& service_instance,
      const std::string& service_name,
      const std::string& service_protocol,
      const std::map<std::string, std::string>& txt_data) override;

 private:
  struct Questions {
    std::map<DomainName, DNSQuestion, DomainNameComparator> a;
    std::map<DomainName, DNSQuestion, DomainNameComparator> aaaa;
    std::map<DomainName, DNSQuestion, DomainNameComparator> ptr;
    std::map<DomainName, DNSQuestion, DomainNameComparator> srv;
    std::map<DomainName, DNSQuestion, DomainNameComparator> txt;
  };

  static void AQueryCallback(mDNS* m,
                             DNSQuestion* question,
                             const ResourceRecord* answer,
                             QC_result added);
  static void AaaaQueryCallback(mDNS* m,
                                DNSQuestion* question,
                                const ResourceRecord* answer,
                                QC_result added);
  static void PtrQueryCallback(mDNS* m,
                               DNSQuestion* question,
                               const ResourceRecord* answer,
                               QC_result added);
  static void SrvQueryCallback(mDNS* m,
                               DNSQuestion* question,
                               const ResourceRecord* answer,
                               QC_result added);
  static void TxtQueryCallback(mDNS* m,
                               DNSQuestion* question,
                               const ResourceRecord* answer,
                               QC_result added);
  static void ServiceCallback(mDNS* m,
                              ServiceRecordSet* service_record,
                              mStatus result);

  void AdvertiseInterfaces();
  void DeadvertiseInterfaces();
  void RemoveQuestionsIfEmpty(platform::UdpSocket* socket);

  CacheEntity rr_cache_[kRrCacheSize];

  //  The main context structure for mDNSResponder.
  mDNS mdns_;

  // Our own storage that is placed inside |mdns_|.  The intent in C is to allow
  // us access to our own state during callbacks.  Here we just use it to group
  // platform sockets.
  mDNS_PlatformSupport platform_storage_;

  std::map<platform::UdpSocket*, Questions> socket_to_questions_;

  std::map<platform::UdpSocket*, NetworkInterfaceInfo>
      responder_interface_info_;

  std::vector<AEvent> a_responses_;
  std::vector<AaaaEvent> aaaa_responses_;
  std::vector<PtrEvent> ptr_responses_;
  std::vector<SrvEvent> srv_responses_;
  std::vector<TxtEvent> txt_responses_;

  // A list of services we are advertising.  ServiceRecordSet is an
  // mDNSResponder structure which holds all the resource record data
  // (PTR/SRV/TXT/A and misc.) that is necessary to advertise a service.
  std::vector<std::unique_ptr<ServiceRecordSet>> service_records_;
};

}  // namespace mdns
}  // namespace openscreen

#endif  // OSP_IMPL_DISCOVERY_MDNS_MDNS_RESPONDER_ADAPTER_IMPL_H_
