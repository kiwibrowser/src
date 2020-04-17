// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_DISCOVERY_MDNS_MDNS_RESPONDER_ADAPTER_H_
#define OSP_IMPL_DISCOVERY_MDNS_MDNS_RESPONDER_ADAPTER_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "osp/impl/discovery/mdns/domain_name.h"
#include "osp/impl/discovery/mdns/mdns_responder_platform.h"
#include "osp_base/error.h"
#include "osp_base/ip_address.h"
#include "platform/api/network_interface.h"
#include "platform/api/udp_socket.h"
#include "platform/base/event_loop.h"

namespace openscreen {
namespace mdns {

struct QueryEventHeader {
  enum class Type {
    kAdded = 0,
    kAddedNoCache,
    kRemoved,
  };

  QueryEventHeader();
  QueryEventHeader(Type response_type, platform::UdpSocket* socket);
  QueryEventHeader(const QueryEventHeader&);
  ~QueryEventHeader();
  QueryEventHeader& operator=(const QueryEventHeader&);

  Type response_type;
  platform::UdpSocket* socket;
};

struct PtrEvent {
  PtrEvent();
  PtrEvent(const QueryEventHeader& header, DomainName service_instance);
  PtrEvent(PtrEvent&&);
  ~PtrEvent();
  PtrEvent& operator=(PtrEvent&&);

  QueryEventHeader header;
  DomainName service_instance;
};

struct SrvEvent {
  SrvEvent();
  SrvEvent(const QueryEventHeader& header,
           DomainName service_instance,
           DomainName domain_name,
           uint16_t port);
  SrvEvent(SrvEvent&&);
  ~SrvEvent();
  SrvEvent& operator=(SrvEvent&&);

  QueryEventHeader header;
  DomainName service_instance;
  DomainName domain_name;
  uint16_t port;
};

struct TxtEvent {
  TxtEvent();
  TxtEvent(const QueryEventHeader& header,
           DomainName service_instance,
           std::vector<std::string> txt_info);
  TxtEvent(TxtEvent&&);
  ~TxtEvent();
  TxtEvent& operator=(TxtEvent&&);

  QueryEventHeader header;
  DomainName service_instance;

  // NOTE: mDNS does not specify a character encoding for the data in TXT
  // records.
  std::vector<std::string> txt_info;
};

struct AEvent {
  AEvent();
  AEvent(const QueryEventHeader& header,
         DomainName domain_name,
         const IPAddress& address);
  AEvent(AEvent&&);
  ~AEvent();
  AEvent& operator=(AEvent&&);

  QueryEventHeader header;
  DomainName domain_name;
  IPAddress address;
};

struct AaaaEvent {
  AaaaEvent();
  AaaaEvent(const QueryEventHeader& header,
            DomainName domain_name,
            const IPAddress& address);
  AaaaEvent(AaaaEvent&&);
  ~AaaaEvent();
  AaaaEvent& operator=(AaaaEvent&&);

  QueryEventHeader header;
  DomainName domain_name;
  IPAddress address;
};

enum class MdnsResponderErrorCode {
  kNoError = 0,
  kUnsupportedError,
  kDomainOverflowError,
  kInvalidParameters,
  kUnknownError,
};

// This interface wraps all the functionality of mDNSResponder, which includes
// both listening and publishing.  As a result, some methods are only used by
// listeners, some are only used by publishers, and some are used by both.
//
// Listening for records might look like this:
//   adapter->Init();
//
//   // Once for each interface, the meaning of false is described below.
//   adapter->RegisterInterface(..., false);
//
//   adapter->StartPtrQuery("_openscreen._udp");
//   adapter->RunTasks();
//
//   // When receiving multicast UDP traffic from port 5353.
//   adapter->OnDataReceived(...);
//   adapter->RunTasks();
//
//   // Check |ptrs| for responses after pulling.
//   auto ptrs = adapter->TakePtrResponses();
//
//   // Eventually...
//   adapter->StopPtrQuery("_openscreen._udp");
//
// Publishing a service might look like this:
//   adapter->Init();
//
//   // Once for each interface, the meaning of true is described below.
//   adapter->RegisterInterface(..., true);
//
//   adapter->SetHostLabel("deadbeef");
//   adapter->RegisterService("living-room", "_openscreen._udp", ...);
//   adapter->RunTasks();
//
//   // When receiving multicast UDP traffic from port 5353.
//   adapter->OnDataReceived(...);
//   adapter->RunTasks();
//
//   // Eventually...
//   adapter->DeregisterService("living-room", "_openscreen", "_udp");
//
// Additionally, it's important to understand that mDNSResponder may defer some
// tasks (e.g. parsing responses, sending queries, etc.) and those deferred
// tasks are only run when RunTasks is called.  Therefore, RunTasks should be
// called after any sequence of calls to mDNSResponder.  It also returns a
// timeout value, after which it must be called again (e.g. for maintaining its
// cache).
class MdnsResponderAdapter {
 public:
  MdnsResponderAdapter();
  virtual ~MdnsResponderAdapter() = 0;

  // Initializes mDNSResponder.  This should be called before any queries or
  // service registrations are made.
  virtual Error Init() = 0;

  // Stops all open queries and service registrations.  If this is not called
  // before destruction, any registered services will not send their goodbye
  // messages.
  virtual void Close() = 0;

  // Called to change the name published by the A and AAAA records for the host
  // when any service is active (via RegisterService).  Returns true if the
  // label was set successfully, false otherwise (e.g. the label did not meet
  // DNS name requirements).
  virtual Error SetHostLabel(const std::string& host_label) = 0;

  // The following methods register and deregister a network interface with
  // mDNSResponder.  |socket| will be used to identify which interface received
  // the data in OnDataReceived and will be used to send data via the platform
  // layer.
  virtual Error RegisterInterface(const platform::InterfaceInfo& interface_info,
                                  const platform::IPSubnet& interface_address,
                                  platform::UdpSocket* socket) = 0;
  virtual Error DeregisterInterface(platform::UdpSocket* socket) = 0;

  virtual void OnDataReceived(const IPEndpoint& source,
                              const IPEndpoint& original_destination,
                              const uint8_t* data,
                              size_t length,
                              platform::UdpSocket* receiving_socket) = 0;

  // Returns the number of seconds after which this method must be called again.
  virtual int RunTasks() = 0;

  virtual std::vector<PtrEvent> TakePtrResponses() = 0;
  virtual std::vector<SrvEvent> TakeSrvResponses() = 0;
  virtual std::vector<TxtEvent> TakeTxtResponses() = 0;
  virtual std::vector<AEvent> TakeAResponses() = 0;
  virtual std::vector<AaaaEvent> TakeAaaaResponses() = 0;

  virtual MdnsResponderErrorCode StartPtrQuery(
      platform::UdpSocket* socket,
      const DomainName& service_type) = 0;
  virtual MdnsResponderErrorCode StartSrvQuery(
      platform::UdpSocket* socket,
      const DomainName& service_instance) = 0;
  virtual MdnsResponderErrorCode StartTxtQuery(
      platform::UdpSocket* socket,
      const DomainName& service_instance) = 0;
  virtual MdnsResponderErrorCode StartAQuery(platform::UdpSocket* socket,
                                             const DomainName& domain_name) = 0;
  virtual MdnsResponderErrorCode StartAaaaQuery(
      platform::UdpSocket* socket,
      const DomainName& domain_name) = 0;

  virtual MdnsResponderErrorCode StopPtrQuery(
      platform::UdpSocket* socket,
      const DomainName& service_type) = 0;
  virtual MdnsResponderErrorCode StopSrvQuery(
      platform::UdpSocket* socket,
      const DomainName& service_instance) = 0;
  virtual MdnsResponderErrorCode StopTxtQuery(
      platform::UdpSocket* socket,
      const DomainName& service_instance) = 0;
  virtual MdnsResponderErrorCode StopAQuery(platform::UdpSocket* socket,
                                            const DomainName& domain_name) = 0;
  virtual MdnsResponderErrorCode StopAaaaQuery(
      platform::UdpSocket* socket,
      const DomainName& domain_name) = 0;

  // The following methods concern advertising a service via mDNS.  The
  // arguments correspond to values needed in the PTR, SRV, and TXT records that
  // will be published for the service.  An A or AAAA record will also be
  // published with the service for each active interface known to mDNSResponder
  // via RegisterInterface.
  virtual MdnsResponderErrorCode RegisterService(
      const std::string& service_instance,
      const std::string& service_name,
      const std::string& service_protocol,
      const DomainName& target_host,
      uint16_t target_port,
      const std::map<std::string, std::string>& txt_data) = 0;
  virtual MdnsResponderErrorCode DeregisterService(
      const std::string& service_instance,
      const std::string& service_name,
      const std::string& service_protocol) = 0;
  virtual MdnsResponderErrorCode UpdateTxtData(
      const std::string& service_instance,
      const std::string& service_name,
      const std::string& service_protocol,
      const std::map<std::string, std::string>& txt_data) = 0;
};

}  // namespace mdns
}  // namespace openscreen

#endif  // OSP_IMPL_DISCOVERY_MDNS_MDNS_RESPONDER_ADAPTER_H_
