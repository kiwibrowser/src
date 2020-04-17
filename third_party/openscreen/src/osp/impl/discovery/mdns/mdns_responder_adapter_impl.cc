// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/discovery/mdns/mdns_responder_adapter_impl.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <memory>

#include "platform/api/logging.h"

namespace openscreen {
namespace mdns {
namespace {

// RFC 1035 specifies a max string length of 256, including the leading length
// octet.
constexpr size_t kMaxDnsStringLength = 255;

// RFC 6763 recommends a maximum key length of 9 characters.
constexpr size_t kMaxTxtKeyLength = 9;

constexpr size_t kMaxStaticTxtDataSize = 256;

static_assert(sizeof(std::declval<RData>().u.txt) == kMaxStaticTxtDataSize,
              "mDNSResponder static TXT data size expected to be 256 bytes");

static_assert(sizeof(mDNSAddr::ip.v4.b) == 4u,
              "mDNSResponder IPv4 address must be 4 bytes");
static_assert(sizeof(mDNSAddr::ip.v6.b) == 16u,
              "mDNSResponder IPv6 address must be 16 bytes");

void AssignMdnsPort(mDNSIPPort* mdns_port, uint16_t port) {
  mdns_port->b[0] = (port >> 8) & 0xff;
  mdns_port->b[1] = port & 0xff;
}

uint16_t GetNetworkOrderPort(const mDNSOpaque16& port) {
  return port.b[0] << 8 | port.b[1];
}

bool IsValidServiceName(const std::string& service_name) {
  // Service name requirements come from RFC 6335:
  //  - No more than 16 characters.
  //  - Begin with '_'.
  //  - Next is a letter or digit and end with a letter or digit.
  //  - May contain hyphens, but no consecutive hyphens.
  //  - Must contain at least one letter.
  if (service_name.size() <= 1 || service_name.size() > 16)
    return false;

  if (service_name[0] != '_' || !std::isalnum(service_name[1]) ||
      !std::isalnum(service_name.back())) {
    return false;
  }
  bool has_alpha = false;
  bool previous_hyphen = false;
  for (auto it = service_name.begin() + 1; it != service_name.end(); ++it) {
    if (*it == '-' && previous_hyphen)
      return false;

    previous_hyphen = *it == '-';
    has_alpha = has_alpha || std::isalpha(*it);
  }
  return has_alpha && !previous_hyphen;
}

bool IsValidServiceProtocol(const std::string& protocol) {
  // RFC 6763 requires _tcp be used for TCP services and _udp for all others.
  return protocol == "_tcp" || protocol == "_udp";
}

void MakeLocalServiceNameParts(const std::string& service_instance,
                               const std::string& service_name,
                               const std::string& service_protocol,
                               domainlabel* instance,
                               domainlabel* name,
                               domainlabel* protocol,
                               domainname* type,
                               domainname* domain) {
  MakeDomainLabelFromLiteralString(instance, service_instance.c_str());
  MakeDomainLabelFromLiteralString(name, service_name.c_str());
  MakeDomainLabelFromLiteralString(protocol, service_protocol.c_str());
  type->c[0] = 0;
  AppendDomainLabel(type, name);
  AppendDomainLabel(type, protocol);
  const DomainName local_domain = DomainName::GetLocalDomain();
  std::copy(local_domain.domain_name().begin(),
            local_domain.domain_name().end(), domain->c);
}

void MakeSubnetMaskFromPrefixLengthV4(uint8_t mask[4], uint8_t prefix_length) {
  for (int i = 0; i < 4; prefix_length -= 8, ++i) {
    if (prefix_length >= 8) {
      mask[i] = 0xff;
    } else if (prefix_length > 0) {
      mask[i] = 0xff << (8 - prefix_length);
    } else {
      mask[i] = 0;
    }
  }
}

void MakeSubnetMaskFromPrefixLengthV6(uint8_t mask[16], uint8_t prefix_length) {
  for (int i = 0; i < 16; prefix_length -= 8, ++i) {
    if (prefix_length >= 8) {
      mask[i] = 0xff;
    } else if (prefix_length > 0) {
      mask[i] = 0xff << (8 - prefix_length);
    } else {
      mask[i] = 0;
    }
  }
}

bool IsValidTxtDataKey(const std::string& s) {
  if (s.size() > kMaxTxtKeyLength)
    return false;
  for (unsigned char c : s)
    if (c < 0x20 || c > 0x7e || c == '=')
      return false;
  return true;
}

std::string MakeTxtData(const std::map<std::string, std::string>& txt_data) {
  std::string txt;
  txt.reserve(kMaxStaticTxtDataSize);
  for (const auto& line : txt_data) {
    const auto key_size = line.first.size();
    const auto value_size = line.second.size();
    const auto line_size = value_size ? (key_size + 1 + value_size) : key_size;
    if (!IsValidTxtDataKey(line.first) || line_size > kMaxDnsStringLength ||
        (txt.size() + 1 + line_size) > kMaxStaticTxtDataSize) {
      return {};
    }
    txt.push_back(line_size);
    txt += line.first;
    if (value_size) {
      txt.push_back('=');
      txt += line.second;
    }
  }
  return txt;
}

MdnsResponderErrorCode MapMdnsError(int err) {
  switch (err) {
    case mStatus_NoError:
      return MdnsResponderErrorCode::kNoError;
    case mStatus_UnsupportedErr:
      return MdnsResponderErrorCode::kUnsupportedError;
    case mStatus_UnknownErr:
      return MdnsResponderErrorCode::kUnknownError;
    default:
      break;
  }
  OSP_DLOG_WARN << "unmapped mDNSResponder error: " << err;
  return MdnsResponderErrorCode::kUnknownError;
}

std::vector<std::string> ParseTxtResponse(
    const uint8_t data[kMaxStaticTxtDataSize],
    uint16_t length) {
  OSP_DCHECK(length <= kMaxStaticTxtDataSize);
  if (length == 0)
    return {};

  std::vector<std::string> lines;
  int total_pos = 0;
  while (total_pos < length) {
    uint8_t line_length = data[total_pos];
    if ((line_length > kMaxDnsStringLength) ||
        (total_pos + line_length >= length)) {
      return {};
    }
    lines.emplace_back(&data[total_pos + 1],
                       &data[total_pos + line_length + 1]);
    total_pos += line_length + 1;
  }
  return lines;
}

void MdnsStatusCallback(mDNS* mdns, mStatus result) {
  OSP_LOG << "status good? " << (result == mStatus_NoError);
}

}  // namespace

MdnsResponderAdapterImpl::MdnsResponderAdapterImpl() = default;
MdnsResponderAdapterImpl::~MdnsResponderAdapterImpl() = default;

Error MdnsResponderAdapterImpl::Init() {
  const auto err =
      mDNS_Init(&mdns_, &platform_storage_, rr_cache_, kRrCacheSize,
                mDNS_Init_DontAdvertiseLocalAddresses, &MdnsStatusCallback,
                mDNS_Init_NoInitCallbackContext);

  return (err == mStatus_NoError) ? Error::None()
                                  : Error::Code::kInitializationFailure;
}

void MdnsResponderAdapterImpl::Close() {
  mDNS_StartExit(&mdns_);
  // Let all services send goodbyes.
  while (!service_records_.empty()) {
    RunTasks();
  }
  mDNS_FinalExit(&mdns_);

  socket_to_questions_.clear();

  responder_interface_info_.clear();

  a_responses_.clear();
  aaaa_responses_.clear();
  ptr_responses_.clear();
  srv_responses_.clear();
  txt_responses_.clear();

  service_records_.clear();
}

Error MdnsResponderAdapterImpl::SetHostLabel(const std::string& host_label) {
  if (host_label.size() > DomainName::kDomainNameMaxLabelLength)
    return Error::Code::kDomainNameTooLong;

  MakeDomainLabelFromLiteralString(&mdns_.hostlabel, host_label.c_str());
  mDNS_SetFQDN(&mdns_);
  if (!service_records_.empty()) {
    DeadvertiseInterfaces();
    AdvertiseInterfaces();
  }
  return Error::None();
}

Error MdnsResponderAdapterImpl::RegisterInterface(
    const platform::InterfaceInfo& interface_info,
    const platform::IPSubnet& interface_address,
    platform::UdpSocket* socket) {
  OSP_DCHECK(socket);

  const auto info_it = responder_interface_info_.find(socket);
  if (info_it != responder_interface_info_.end())
    return Error::None();

  NetworkInterfaceInfo& info = responder_interface_info_[socket];
  std::memset(&info, 0, sizeof(NetworkInterfaceInfo));
  info.InterfaceID = reinterpret_cast<decltype(info.InterfaceID)>(socket);
  info.Advertise = mDNSfalse;
  if (interface_address.address.IsV4()) {
    info.ip.type = mDNSAddrType_IPv4;
    interface_address.address.CopyToV4(info.ip.ip.v4.b);
    info.mask.type = mDNSAddrType_IPv4;
    MakeSubnetMaskFromPrefixLengthV4(info.mask.ip.v4.b,
                                     interface_address.prefix_length);
  } else {
    info.ip.type = mDNSAddrType_IPv6;
    interface_address.address.CopyToV6(info.ip.ip.v6.b);
    info.mask.type = mDNSAddrType_IPv6;
    MakeSubnetMaskFromPrefixLengthV6(info.mask.ip.v6.b,
                                     interface_address.prefix_length);
  }

  interface_info.CopyHardwareAddressTo(info.MAC.b);
  info.McastTxRx = 1;
  platform_storage_.sockets.push_back(socket);
  auto result = mDNS_RegisterInterface(&mdns_, &info, mDNSfalse);
  OSP_LOG_IF(WARN, result != mStatus_NoError)
      << "mDNS_RegisterInterface failed: " << result;

  return (result == mStatus_NoError) ? Error::None()
                                     : Error::Code::kMdnsRegisterFailure;
}

Error MdnsResponderAdapterImpl::DeregisterInterface(
    platform::UdpSocket* socket) {
  const auto info_it = responder_interface_info_.find(socket);
  if (info_it == responder_interface_info_.end())
    return Error::Code::kNoItemFound;

  const auto it = std::find(platform_storage_.sockets.begin(),
                            platform_storage_.sockets.end(), socket);
  OSP_DCHECK(it != platform_storage_.sockets.end());
  platform_storage_.sockets.erase(it);
  if (info_it->second.RR_A.namestorage.c[0]) {
    mDNS_Deregister(&mdns_, &info_it->second.RR_A);
    info_it->second.RR_A.namestorage.c[0] = 0;
  }
  mDNS_DeregisterInterface(&mdns_, &info_it->second, mDNSfalse);
  responder_interface_info_.erase(info_it);
  return Error::None();
}

void MdnsResponderAdapterImpl::OnDataReceived(
    const IPEndpoint& source,
    const IPEndpoint& original_destination,
    const uint8_t* data,
    size_t length,
    platform::UdpSocket* receiving_socket) {
  mDNSAddr src;
  if (source.address.IsV4()) {
    src.type = mDNSAddrType_IPv4;
    source.address.CopyToV4(src.ip.v4.b);
  } else {
    src.type = mDNSAddrType_IPv6;
    source.address.CopyToV6(src.ip.v6.b);
  }
  mDNSIPPort srcport;
  AssignMdnsPort(&srcport, source.port);

  mDNSAddr dst;
  if (source.address.IsV4()) {
    dst.type = mDNSAddrType_IPv4;
    original_destination.address.CopyToV4(dst.ip.v4.b);
  } else {
    dst.type = mDNSAddrType_IPv6;
    original_destination.address.CopyToV6(dst.ip.v6.b);
  }
  mDNSIPPort dstport;
  AssignMdnsPort(&dstport, original_destination.port);

  mDNSCoreReceive(&mdns_, const_cast<uint8_t*>(data), data + length, &src,
                  srcport, &dst, dstport,
                  reinterpret_cast<mDNSInterfaceID>(receiving_socket));
}

int MdnsResponderAdapterImpl::RunTasks() {
  const auto t = mDNS_Execute(&mdns_);
  const auto now = mDNSPlatformRawTime();
  const auto next = t - now;
  return next;
}

std::vector<PtrEvent> MdnsResponderAdapterImpl::TakePtrResponses() {
  return std::move(ptr_responses_);
}

std::vector<SrvEvent> MdnsResponderAdapterImpl::TakeSrvResponses() {
  return std::move(srv_responses_);
}

std::vector<TxtEvent> MdnsResponderAdapterImpl::TakeTxtResponses() {
  return std::move(txt_responses_);
}

std::vector<AEvent> MdnsResponderAdapterImpl::TakeAResponses() {
  return std::move(a_responses_);
}

std::vector<AaaaEvent> MdnsResponderAdapterImpl::TakeAaaaResponses() {
  return std::move(aaaa_responses_);
}

MdnsResponderErrorCode MdnsResponderAdapterImpl::StartPtrQuery(
    platform::UdpSocket* socket,
    const DomainName& service_type) {
  auto& ptr_questions = socket_to_questions_[socket].ptr;
  if (ptr_questions.find(service_type) != ptr_questions.end())
    return MdnsResponderErrorCode::kNoError;

  auto& question = ptr_questions[service_type];

  question.InterfaceID = reinterpret_cast<mDNSInterfaceID>(socket);
  question.Target = {0};
  if (service_type.EndsWithLocalDomain()) {
    std::copy(service_type.domain_name().begin(),
              service_type.domain_name().end(), question.qname.c);
  } else {
    const DomainName local_domain = DomainName::GetLocalDomain();
    ErrorOr<DomainName> service_type_with_local =
        DomainName::Append(service_type, local_domain);
    if (!service_type_with_local) {
      return MdnsResponderErrorCode::kDomainOverflowError;
    }
    std::copy(service_type_with_local.value().domain_name().begin(),
              service_type_with_local.value().domain_name().end(),
              question.qname.c);
  }
  question.qtype = kDNSType_PTR;
  question.qclass = kDNSClass_IN;
  question.LongLived = mDNStrue;
  question.ExpectUnique = mDNSfalse;
  question.ForceMCast = mDNStrue;
  question.ReturnIntermed = mDNSfalse;
  question.SuppressUnusable = mDNSfalse;
  question.RetryWithSearchDomains = mDNSfalse;
  question.TimeoutQuestion = 0;
  question.WakeOnResolve = 0;
  question.SearchListIndex = 0;
  question.AppendSearchDomains = 0;
  question.AppendLocalSearchDomains = 0;
  question.qnameOrig = nullptr;
  question.QuestionCallback = &MdnsResponderAdapterImpl::PtrQueryCallback;
  question.QuestionContext = this;
  const auto err = mDNS_StartQuery(&mdns_, &question);
  OSP_LOG_IF(WARN, err != mStatus_NoError) << "mDNS_StartQuery failed: " << err;
  return MapMdnsError(err);
}

MdnsResponderErrorCode MdnsResponderAdapterImpl::StartSrvQuery(
    platform::UdpSocket* socket,
    const DomainName& service_instance) {
  if (!service_instance.EndsWithLocalDomain())
    return MdnsResponderErrorCode::kInvalidParameters;

  auto& srv_questions = socket_to_questions_[socket].srv;
  if (srv_questions.find(service_instance) != srv_questions.end())
    return MdnsResponderErrorCode::kNoError;

  auto& question = srv_questions[service_instance];

  question.InterfaceID = reinterpret_cast<mDNSInterfaceID>(socket);
  question.Target = {0};
  std::copy(service_instance.domain_name().begin(),
            service_instance.domain_name().end(), question.qname.c);
  question.qtype = kDNSType_SRV;
  question.qclass = kDNSClass_IN;
  question.LongLived = mDNStrue;
  question.ExpectUnique = mDNSfalse;
  question.ForceMCast = mDNStrue;
  question.ReturnIntermed = mDNSfalse;
  question.SuppressUnusable = mDNSfalse;
  question.RetryWithSearchDomains = mDNSfalse;
  question.TimeoutQuestion = 0;
  question.WakeOnResolve = 0;
  question.SearchListIndex = 0;
  question.AppendSearchDomains = 0;
  question.AppendLocalSearchDomains = 0;
  question.qnameOrig = nullptr;
  question.QuestionCallback = &MdnsResponderAdapterImpl::SrvQueryCallback;
  question.QuestionContext = this;
  const auto err = mDNS_StartQuery(&mdns_, &question);
  OSP_LOG_IF(WARN, err != mStatus_NoError) << "mDNS_StartQuery failed: " << err;
  return MapMdnsError(err);
}

MdnsResponderErrorCode MdnsResponderAdapterImpl::StartTxtQuery(
    platform::UdpSocket* socket,
    const DomainName& service_instance) {
  if (!service_instance.EndsWithLocalDomain())
    return MdnsResponderErrorCode::kInvalidParameters;

  auto& txt_questions = socket_to_questions_[socket].txt;
  if (txt_questions.find(service_instance) != txt_questions.end())
    return MdnsResponderErrorCode::kNoError;

  auto& question = txt_questions[service_instance];

  question.InterfaceID = reinterpret_cast<mDNSInterfaceID>(socket);
  question.Target = {0};
  std::copy(service_instance.domain_name().begin(),
            service_instance.domain_name().end(), question.qname.c);
  question.qtype = kDNSType_TXT;
  question.qclass = kDNSClass_IN;
  question.LongLived = mDNStrue;
  question.ExpectUnique = mDNSfalse;
  question.ForceMCast = mDNStrue;
  question.ReturnIntermed = mDNSfalse;
  question.SuppressUnusable = mDNSfalse;
  question.RetryWithSearchDomains = mDNSfalse;
  question.TimeoutQuestion = 0;
  question.WakeOnResolve = 0;
  question.SearchListIndex = 0;
  question.AppendSearchDomains = 0;
  question.AppendLocalSearchDomains = 0;
  question.qnameOrig = nullptr;
  question.QuestionCallback = &MdnsResponderAdapterImpl::TxtQueryCallback;
  question.QuestionContext = this;
  const auto err = mDNS_StartQuery(&mdns_, &question);
  OSP_LOG_IF(WARN, err != mStatus_NoError) << "mDNS_StartQuery failed: " << err;
  return MapMdnsError(err);
}

MdnsResponderErrorCode MdnsResponderAdapterImpl::StartAQuery(
    platform::UdpSocket* socket,
    const DomainName& domain_name) {
  if (!domain_name.EndsWithLocalDomain())
    return MdnsResponderErrorCode::kInvalidParameters;

  auto& a_questions = socket_to_questions_[socket].a;
  if (a_questions.find(domain_name) != a_questions.end())
    return MdnsResponderErrorCode::kNoError;

  auto& question = a_questions[domain_name];
  std::copy(domain_name.domain_name().begin(), domain_name.domain_name().end(),
            question.qname.c);

  question.InterfaceID = reinterpret_cast<mDNSInterfaceID>(socket);
  question.Target = {0};
  question.qtype = kDNSType_A;
  question.qclass = kDNSClass_IN;
  question.LongLived = mDNStrue;
  question.ExpectUnique = mDNSfalse;
  question.ForceMCast = mDNStrue;
  question.ReturnIntermed = mDNSfalse;
  question.SuppressUnusable = mDNSfalse;
  question.RetryWithSearchDomains = mDNSfalse;
  question.TimeoutQuestion = 0;
  question.WakeOnResolve = 0;
  question.SearchListIndex = 0;
  question.AppendSearchDomains = 0;
  question.AppendLocalSearchDomains = 0;
  question.qnameOrig = nullptr;
  question.QuestionCallback = &MdnsResponderAdapterImpl::AQueryCallback;
  question.QuestionContext = this;
  const auto err = mDNS_StartQuery(&mdns_, &question);
  OSP_LOG_IF(WARN, err != mStatus_NoError) << "mDNS_StartQuery failed: " << err;
  return MapMdnsError(err);
}

MdnsResponderErrorCode MdnsResponderAdapterImpl::StartAaaaQuery(
    platform::UdpSocket* socket,
    const DomainName& domain_name) {
  if (!domain_name.EndsWithLocalDomain())
    return MdnsResponderErrorCode::kInvalidParameters;

  auto& aaaa_questions = socket_to_questions_[socket].aaaa;
  if (aaaa_questions.find(domain_name) != aaaa_questions.end())
    return MdnsResponderErrorCode::kNoError;

  auto& question = aaaa_questions[domain_name];
  std::copy(domain_name.domain_name().begin(), domain_name.domain_name().end(),
            question.qname.c);

  question.InterfaceID = reinterpret_cast<mDNSInterfaceID>(socket);
  question.Target = {0};
  question.qtype = kDNSType_AAAA;
  question.qclass = kDNSClass_IN;
  question.LongLived = mDNStrue;
  question.ExpectUnique = mDNSfalse;
  question.ForceMCast = mDNStrue;
  question.ReturnIntermed = mDNSfalse;
  question.SuppressUnusable = mDNSfalse;
  question.RetryWithSearchDomains = mDNSfalse;
  question.TimeoutQuestion = 0;
  question.WakeOnResolve = 0;
  question.SearchListIndex = 0;
  question.AppendSearchDomains = 0;
  question.AppendLocalSearchDomains = 0;
  question.qnameOrig = nullptr;
  question.QuestionCallback = &MdnsResponderAdapterImpl::AaaaQueryCallback;
  question.QuestionContext = this;
  const auto err = mDNS_StartQuery(&mdns_, &question);
  OSP_LOG_IF(WARN, err != mStatus_NoError) << "mDNS_StartQuery failed: " << err;
  return MapMdnsError(err);
}

MdnsResponderErrorCode MdnsResponderAdapterImpl::StopPtrQuery(
    platform::UdpSocket* socket,
    const DomainName& service_type) {
  auto interface_entry = socket_to_questions_.find(socket);
  if (interface_entry == socket_to_questions_.end())
    return MdnsResponderErrorCode::kNoError;
  auto entry = interface_entry->second.ptr.find(service_type);
  if (entry == interface_entry->second.ptr.end())
    return MdnsResponderErrorCode::kNoError;

  const auto err = mDNS_StopQuery(&mdns_, &entry->second);
  interface_entry->second.ptr.erase(entry);
  OSP_LOG_IF(WARN, err != mStatus_NoError) << "mDNS_StopQuery failed: " << err;
  RemoveQuestionsIfEmpty(socket);
  return MapMdnsError(err);
}

MdnsResponderErrorCode MdnsResponderAdapterImpl::StopSrvQuery(
    platform::UdpSocket* socket,
    const DomainName& service_instance) {
  auto interface_entry = socket_to_questions_.find(socket);
  if (interface_entry == socket_to_questions_.end())
    return MdnsResponderErrorCode::kNoError;
  auto entry = interface_entry->second.srv.find(service_instance);
  if (entry == interface_entry->second.srv.end())
    return MdnsResponderErrorCode::kNoError;

  const auto err = mDNS_StopQuery(&mdns_, &entry->second);
  interface_entry->second.srv.erase(entry);
  OSP_LOG_IF(WARN, err != mStatus_NoError) << "mDNS_StopQuery failed: " << err;
  RemoveQuestionsIfEmpty(socket);
  return MapMdnsError(err);
}

MdnsResponderErrorCode MdnsResponderAdapterImpl::StopTxtQuery(
    platform::UdpSocket* socket,
    const DomainName& service_instance) {
  auto interface_entry = socket_to_questions_.find(socket);
  if (interface_entry == socket_to_questions_.end())
    return MdnsResponderErrorCode::kNoError;
  auto entry = interface_entry->second.txt.find(service_instance);
  if (entry == interface_entry->second.txt.end())
    return MdnsResponderErrorCode::kNoError;

  const auto err = mDNS_StopQuery(&mdns_, &entry->second);
  interface_entry->second.txt.erase(entry);
  OSP_LOG_IF(WARN, err != mStatus_NoError) << "mDNS_StopQuery failed: " << err;
  RemoveQuestionsIfEmpty(socket);
  return MapMdnsError(err);
}

MdnsResponderErrorCode MdnsResponderAdapterImpl::StopAQuery(
    platform::UdpSocket* socket,
    const DomainName& domain_name) {
  auto interface_entry = socket_to_questions_.find(socket);
  if (interface_entry == socket_to_questions_.end())
    return MdnsResponderErrorCode::kNoError;
  auto entry = interface_entry->second.a.find(domain_name);
  if (entry == interface_entry->second.a.end())
    return MdnsResponderErrorCode::kNoError;

  const auto err = mDNS_StopQuery(&mdns_, &entry->second);
  interface_entry->second.a.erase(entry);
  OSP_LOG_IF(WARN, err != mStatus_NoError) << "mDNS_StopQuery failed: " << err;
  RemoveQuestionsIfEmpty(socket);
  return MapMdnsError(err);
}

MdnsResponderErrorCode MdnsResponderAdapterImpl::StopAaaaQuery(
    platform::UdpSocket* socket,
    const DomainName& domain_name) {
  auto interface_entry = socket_to_questions_.find(socket);
  if (interface_entry == socket_to_questions_.end())
    return MdnsResponderErrorCode::kNoError;
  auto entry = interface_entry->second.aaaa.find(domain_name);
  if (entry == interface_entry->second.aaaa.end())
    return MdnsResponderErrorCode::kNoError;

  const auto err = mDNS_StopQuery(&mdns_, &entry->second);
  interface_entry->second.aaaa.erase(entry);
  OSP_LOG_IF(WARN, err != mStatus_NoError) << "mDNS_StopQuery failed: " << err;
  RemoveQuestionsIfEmpty(socket);
  return MapMdnsError(err);
}

MdnsResponderErrorCode MdnsResponderAdapterImpl::RegisterService(
    const std::string& service_instance,
    const std::string& service_name,
    const std::string& service_protocol,
    const DomainName& target_host,
    uint16_t target_port,
    const std::map<std::string, std::string>& txt_data) {
  OSP_DCHECK(IsValidServiceName(service_name));
  OSP_DCHECK(IsValidServiceProtocol(service_protocol));
  service_records_.push_back(std::make_unique<ServiceRecordSet>());
  auto* service_record = service_records_.back().get();
  domainlabel instance;
  domainlabel name;
  domainlabel protocol;
  domainname type;
  domainname domain;
  domainname host;
  mDNSIPPort port;

  MakeLocalServiceNameParts(service_instance, service_name, service_protocol,
                            &instance, &name, &protocol, &type, &domain);
  std::copy(target_host.domain_name().begin(), target_host.domain_name().end(),
            host.c);
  AssignMdnsPort(&port, target_port);
  auto txt = MakeTxtData(txt_data);
  if (txt.size() > kMaxStaticTxtDataSize) {
    // Not handling oversized TXT records.
    return MdnsResponderErrorCode::kUnsupportedError;
  }

  if (service_records_.size() == 1)
    AdvertiseInterfaces();

  auto result = mDNS_RegisterService(
      &mdns_, service_record, &instance, &type, &domain, &host, port,
      reinterpret_cast<const uint8_t*>(txt.data()), txt.size(), nullptr, 0,
      mDNSInterface_Any, &MdnsResponderAdapterImpl::ServiceCallback, this, 0);

  if (result != mStatus_NoError) {
    service_records_.pop_back();
    if (service_records_.empty())
      DeadvertiseInterfaces();
  }
  return MapMdnsError(result);
}

MdnsResponderErrorCode MdnsResponderAdapterImpl::DeregisterService(
    const std::string& service_instance,
    const std::string& service_name,
    const std::string& service_protocol) {
  domainlabel instance;
  domainlabel name;
  domainlabel protocol;
  domainname type;
  domainname domain;
  domainname full_instance_name;

  MakeLocalServiceNameParts(service_instance, service_name, service_protocol,
                            &instance, &name, &protocol, &type, &domain);
  if (!ConstructServiceName(&full_instance_name, &instance, &type, &domain))
    return MdnsResponderErrorCode::kInvalidParameters;

  for (auto it = service_records_.begin(); it != service_records_.end(); ++it) {
    if (SameDomainName(&full_instance_name, &(*it)->RR_SRV.namestorage)) {
      // |it| will be removed from |service_records_| in ServiceCallback, when
      // mDNSResponder is done with the memory.
      mDNS_DeregisterService(&mdns_, it->get());
      return MdnsResponderErrorCode::kNoError;
    }
  }
  return MdnsResponderErrorCode::kNoError;
}

MdnsResponderErrorCode MdnsResponderAdapterImpl::UpdateTxtData(
    const std::string& service_instance,
    const std::string& service_name,
    const std::string& service_protocol,
    const std::map<std::string, std::string>& txt_data) {
  domainlabel instance;
  domainlabel name;
  domainlabel protocol;
  domainname type;
  domainname domain;
  domainname full_instance_name;

  MakeLocalServiceNameParts(service_instance, service_name, service_protocol,
                            &instance, &name, &protocol, &type, &domain);
  if (!ConstructServiceName(&full_instance_name, &instance, &type, &domain))
    return MdnsResponderErrorCode::kInvalidParameters;
  std::string txt = MakeTxtData(txt_data);
  if (txt.size() > kMaxStaticTxtDataSize) {
    // Not handling oversized TXT records.
    return MdnsResponderErrorCode::kUnsupportedError;
  }

  for (std::unique_ptr<ServiceRecordSet>& record : service_records_) {
    if (SameDomainName(&full_instance_name, &record->RR_SRV.namestorage)) {
      std::copy(txt.begin(), txt.end(), record->RR_TXT.rdatastorage.u.txt.c);
      mDNS_Update(&mdns_, &record->RR_TXT, 0, txt.size(),
                  &record->RR_TXT.rdatastorage, nullptr);
      return MdnsResponderErrorCode::kNoError;
    }
  }
  return MdnsResponderErrorCode::kNoError;
}

// static
void MdnsResponderAdapterImpl::AQueryCallback(mDNS* m,
                                              DNSQuestion* question,
                                              const ResourceRecord* answer,
                                              QC_result added) {
  OSP_DCHECK(question);
  OSP_DCHECK(answer);
  OSP_DCHECK_EQ(answer->rrtype, kDNSType_A);
  DomainName domain(std::vector<uint8_t>(
      question->qname.c,
      question->qname.c + DomainNameLength(&question->qname)));
  IPAddress address(answer->rdata->u.ipv4.b);

  auto* adapter =
      reinterpret_cast<MdnsResponderAdapterImpl*>(question->QuestionContext);
  OSP_DCHECK(adapter);
  auto event_type = QueryEventHeader::Type::kAddedNoCache;
  if (added == QC_add) {
    event_type = QueryEventHeader::Type::kAdded;
  } else if (added == QC_rmv) {
    event_type = QueryEventHeader::Type::kRemoved;
  } else {
    OSP_DCHECK_EQ(added, QC_addnocache);
  }
  adapter->a_responses_.emplace_back(
      QueryEventHeader{event_type, reinterpret_cast<platform::UdpSocket*>(
                                       answer->InterfaceID)},
      std::move(domain), address);
}

// static
void MdnsResponderAdapterImpl::AaaaQueryCallback(mDNS* m,
                                                 DNSQuestion* question,
                                                 const ResourceRecord* answer,
                                                 QC_result added) {
  OSP_DCHECK(question);
  OSP_DCHECK(answer);
  OSP_DCHECK_EQ(answer->rrtype, kDNSType_A);
  DomainName domain(std::vector<uint8_t>(
      question->qname.c,
      question->qname.c + DomainNameLength(&question->qname)));
  IPAddress address(IPAddress::Version::kV6, answer->rdata->u.ipv6.b);

  auto* adapter =
      reinterpret_cast<MdnsResponderAdapterImpl*>(question->QuestionContext);
  OSP_DCHECK(adapter);
  auto event_type = QueryEventHeader::Type::kAddedNoCache;
  if (added == QC_add) {
    event_type = QueryEventHeader::Type::kAdded;
  } else if (added == QC_rmv) {
    event_type = QueryEventHeader::Type::kRemoved;
  } else {
    OSP_DCHECK_EQ(added, QC_addnocache);
  }
  adapter->aaaa_responses_.emplace_back(
      QueryEventHeader{event_type, reinterpret_cast<platform::UdpSocket*>(
                                       answer->InterfaceID)},
      std::move(domain), address);
}

// static
void MdnsResponderAdapterImpl::PtrQueryCallback(mDNS* m,
                                                DNSQuestion* question,
                                                const ResourceRecord* answer,
                                                QC_result added) {
  OSP_DCHECK(question);
  OSP_DCHECK(answer);
  OSP_DCHECK_EQ(answer->rrtype, kDNSType_PTR);
  DomainName result(std::vector<uint8_t>(
      answer->rdata->u.name.c,
      answer->rdata->u.name.c + DomainNameLength(&answer->rdata->u.name)));

  auto* adapter =
      reinterpret_cast<MdnsResponderAdapterImpl*>(question->QuestionContext);
  OSP_DCHECK(adapter);
  auto event_type = QueryEventHeader::Type::kAddedNoCache;
  if (added == QC_add) {
    event_type = QueryEventHeader::Type::kAdded;
  } else if (added == QC_rmv) {
    event_type = QueryEventHeader::Type::kRemoved;
  } else {
    OSP_DCHECK_EQ(added, QC_addnocache);
  }
  adapter->ptr_responses_.emplace_back(
      QueryEventHeader{event_type, reinterpret_cast<platform::UdpSocket*>(
                                       answer->InterfaceID)},
      std::move(result));
}

// static
void MdnsResponderAdapterImpl::SrvQueryCallback(mDNS* m,
                                                DNSQuestion* question,
                                                const ResourceRecord* answer,
                                                QC_result added) {
  OSP_DCHECK(question);
  OSP_DCHECK(answer);
  OSP_DCHECK_EQ(answer->rrtype, kDNSType_SRV);
  DomainName service(std::vector<uint8_t>(
      question->qname.c,
      question->qname.c + DomainNameLength(&question->qname)));
  DomainName result(
      std::vector<uint8_t>(answer->rdata->u.srv.target.c,
                           answer->rdata->u.srv.target.c +
                               DomainNameLength(&answer->rdata->u.srv.target)));

  auto* adapter =
      reinterpret_cast<MdnsResponderAdapterImpl*>(question->QuestionContext);
  OSP_DCHECK(adapter);
  auto event_type = QueryEventHeader::Type::kAddedNoCache;
  if (added == QC_add) {
    event_type = QueryEventHeader::Type::kAdded;
  } else if (added == QC_rmv) {
    event_type = QueryEventHeader::Type::kRemoved;
  } else {
    OSP_DCHECK_EQ(added, QC_addnocache);
  }
  adapter->srv_responses_.emplace_back(
      QueryEventHeader{event_type, reinterpret_cast<platform::UdpSocket*>(
                                       answer->InterfaceID)},
      std::move(service), std::move(result),
      GetNetworkOrderPort(answer->rdata->u.srv.port));
}

// static
void MdnsResponderAdapterImpl::TxtQueryCallback(mDNS* m,
                                                DNSQuestion* question,
                                                const ResourceRecord* answer,
                                                QC_result added) {
  OSP_DCHECK(question);
  OSP_DCHECK(answer);
  OSP_DCHECK_EQ(answer->rrtype, kDNSType_TXT);
  DomainName service(std::vector<uint8_t>(
      question->qname.c,
      question->qname.c + DomainNameLength(&question->qname)));
  auto lines = ParseTxtResponse(answer->rdata->u.txt.c, answer->rdlength);

  auto* adapter =
      reinterpret_cast<MdnsResponderAdapterImpl*>(question->QuestionContext);
  OSP_DCHECK(adapter);
  auto event_type = QueryEventHeader::Type::kAddedNoCache;
  if (added == QC_add) {
    event_type = QueryEventHeader::Type::kAdded;
  } else if (added == QC_rmv) {
    event_type = QueryEventHeader::Type::kRemoved;
  } else {
    OSP_DCHECK_EQ(added, QC_addnocache);
  }
  adapter->txt_responses_.emplace_back(
      QueryEventHeader{event_type, reinterpret_cast<platform::UdpSocket*>(
                                       answer->InterfaceID)},
      std::move(service), std::move(lines));
}

// static
void MdnsResponderAdapterImpl::ServiceCallback(mDNS* m,
                                               ServiceRecordSet* service_record,
                                               mStatus result) {
  // TODO(btolsch): Handle mStatus_NameConflict.
  if (result == mStatus_MemFree) {
    OSP_DLOG_INFO << "free service record";
    auto* adapter = reinterpret_cast<MdnsResponderAdapterImpl*>(
        service_record->ServiceContext);
    auto& service_records = adapter->service_records_;
    service_records.erase(
        std::remove_if(
            service_records.begin(), service_records.end(),
            [service_record](const std::unique_ptr<ServiceRecordSet>& sr) {
              return sr.get() == service_record;
            }),
        service_records.end());

    if (service_records.empty())
      adapter->DeadvertiseInterfaces();
  }
}

void MdnsResponderAdapterImpl::AdvertiseInterfaces() {
  for (auto& info : responder_interface_info_) {
    platform::UdpSocket* socket = info.first;
    NetworkInterfaceInfo& interface_info = info.second;
    mDNS_SetupResourceRecord(&interface_info.RR_A, /** RDataStorage */ nullptr,
                             reinterpret_cast<mDNSInterfaceID>(socket),
                             kDNSType_A, kHostNameTTL, kDNSRecordTypeUnique,
                             AuthRecordAny,
                             /** Callback */ nullptr, /** Context */ nullptr);
    AssignDomainName(&interface_info.RR_A.namestorage,
                     &mdns_.MulticastHostname);
    if (interface_info.ip.type == mDNSAddrType_IPv4) {
      interface_info.RR_A.resrec.rdata->u.ipv4 = interface_info.ip.ip.v4;
    } else {
      interface_info.RR_A.resrec.rdata->u.ipv6 = interface_info.ip.ip.v6;
    }
    mDNS_Register(&mdns_, &interface_info.RR_A);
  }
}

void MdnsResponderAdapterImpl::DeadvertiseInterfaces() {
  // Both loops below use the A resource record's domain name to determine
  // whether the record was advertised.  AdvertiseInterfaces sets the domain
  // name before registering the A record, and this clears it after
  // deregistering.
  for (auto& info : responder_interface_info_) {
    NetworkInterfaceInfo& interface_info = info.second;
    if (interface_info.RR_A.namestorage.c[0]) {
      mDNS_Deregister(&mdns_, &interface_info.RR_A);
      interface_info.RR_A.namestorage.c[0] = 0;
    }
  }
}

void MdnsResponderAdapterImpl::RemoveQuestionsIfEmpty(
    platform::UdpSocket* socket) {
  auto entry = socket_to_questions_.find(socket);
  bool empty = entry->second.a.empty() || entry->second.aaaa.empty() ||
               entry->second.ptr.empty() || entry->second.srv.empty() ||
               entry->second.txt.empty();
  if (empty)
    socket_to_questions_.erase(entry);
}

}  // namespace mdns
}  // namespace openscreen
