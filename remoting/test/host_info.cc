// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/host_info.h"

#include "base/logging.h"

namespace remoting {
namespace test {

HostInfo::HostInfo() = default;
HostInfo::HostInfo(const HostInfo& other) = default;

HostInfo::~HostInfo() = default;

bool HostInfo::ParseHostInfo(const base::DictionaryValue& host_info) {
  const base::ListValue* list_value = nullptr;

  // Add TokenUrlPatterns to HostInfo.
  if (host_info.GetList("tokenUrlPatterns", &list_value)) {
    if (!list_value->empty()) {
      for (const auto& item : *list_value) {
        std::string token_url_pattern;
        if (!item.GetAsString(&token_url_pattern)) {
          return false;
        }
        token_url_patterns.push_back(token_url_pattern);
      }
    }
  }

  std::string response_status;
  host_info.GetString("status", &response_status);
  if (response_status == "ONLINE") {
    status = kHostStatusOnline;
  } else if (response_status == "OFFLINE") {
    status = kHostStatusOffline;
  } else {
    LOG(ERROR) << "Response Status is " << response_status;
    return false;
  }

  if (!host_info.GetString("hostId", &host_id)) {
    LOG(ERROR) << "hostId was not found in host_info";
    return false;
  }

  if (!host_info.GetString("hostName", &host_name)) {
    LOG(ERROR) << "hostName was not found in host_info";
    return false;
  }

  if (!host_info.GetString("publicKey", &public_key)) {
    LOG(ERROR) << "publicKey was not found for " << host_name;
    return false;
  }

  // If the host entry was created but the host was never online, then the jid
  // is never set.
  if (!host_info.GetString("jabberId", &host_jid) &&
      status == kHostStatusOnline) {
    LOG(ERROR) << host_name << " is online but is missing a jabberId";
    return false;
  }

  host_info.GetString("hostOfflineReason", &offline_reason);

  return true;
}

bool HostInfo::IsReadyForConnection() const {
  return !host_jid.empty() && status == kHostStatusOnline;
}

ConnectionSetupInfo HostInfo::GenerateConnectionSetupInfo(
    const std::string& access_token,
    const std::string& user_name,
    const std::string& pin) const {
  ConnectionSetupInfo connection_setup_info;
  connection_setup_info.access_token = access_token;
  connection_setup_info.host_id = host_id;
  connection_setup_info.host_jid = host_jid;
  connection_setup_info.host_name = host_name;
  connection_setup_info.pin = pin;
  connection_setup_info.public_key = public_key;
  connection_setup_info.user_name = user_name;
  return connection_setup_info;
}

}  // namespace test
}  // namespace remoting
