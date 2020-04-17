// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/discovery/mdns/mdns_responder_adapter.h"

namespace openscreen {
namespace mdns {

QueryEventHeader::QueryEventHeader() = default;
QueryEventHeader::QueryEventHeader(QueryEventHeader::Type response_type,
                                   platform::UdpSocket* socket)
    : response_type(response_type), socket(socket) {}
QueryEventHeader::QueryEventHeader(const QueryEventHeader&) = default;
QueryEventHeader::~QueryEventHeader() = default;
QueryEventHeader& QueryEventHeader::operator=(const QueryEventHeader&) =
    default;

AEvent::AEvent() = default;
AEvent::AEvent(const QueryEventHeader& header,
               DomainName domain_name,
               const IPAddress& address)
    : header(header), domain_name(std::move(domain_name)), address(address) {}
AEvent::AEvent(AEvent&&) = default;
AEvent::~AEvent() = default;
AEvent& AEvent::operator=(AEvent&&) = default;

AaaaEvent::AaaaEvent() = default;
AaaaEvent::AaaaEvent(const QueryEventHeader& header,
                     DomainName domain_name,
                     const IPAddress& address)
    : header(header), domain_name(std::move(domain_name)), address(address) {}
AaaaEvent::AaaaEvent(AaaaEvent&&) = default;
AaaaEvent::~AaaaEvent() = default;
AaaaEvent& AaaaEvent::operator=(AaaaEvent&&) = default;

PtrEvent::PtrEvent() = default;
PtrEvent::PtrEvent(const QueryEventHeader& header, DomainName service_instance)
    : header(header), service_instance(std::move(service_instance)) {}
PtrEvent::PtrEvent(PtrEvent&&) = default;
PtrEvent::~PtrEvent() = default;
PtrEvent& PtrEvent::operator=(PtrEvent&&) = default;

SrvEvent::SrvEvent() = default;
SrvEvent::SrvEvent(const QueryEventHeader& header,
                   DomainName service_instance,
                   DomainName domain_name,
                   uint16_t port)
    : header(header),
      service_instance(std::move(service_instance)),
      domain_name(std::move(domain_name)),
      port(port) {}
SrvEvent::SrvEvent(SrvEvent&&) = default;
SrvEvent::~SrvEvent() = default;
SrvEvent& SrvEvent::operator=(SrvEvent&&) = default;

TxtEvent::TxtEvent() = default;
TxtEvent::TxtEvent(const QueryEventHeader& header,
                   DomainName service_instance,
                   std::vector<std::string> txt_info)
    : header(header),
      service_instance(std::move(service_instance)),
      txt_info(std::move(txt_info)) {}
TxtEvent::TxtEvent(TxtEvent&&) = default;
TxtEvent::~TxtEvent() = default;
TxtEvent& TxtEvent::operator=(TxtEvent&&) = default;

MdnsResponderAdapter::MdnsResponderAdapter() = default;
MdnsResponderAdapter::~MdnsResponderAdapter() = default;

}  // namespace mdns
}  // namespace openscreen
