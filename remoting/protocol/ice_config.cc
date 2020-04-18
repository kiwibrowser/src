// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/ice_config.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "net/base/url_util.h"

namespace remoting {
namespace protocol {

namespace {

// See draft-petithuguenin-behave-turn-uris-01.
const int kDefaultStunTurnPort = 3478;
const int kDefaultTurnsPort = 5349;

bool ParseLifetime(const std::string& string, base::TimeDelta* result) {
  double seconds = 0;
  if (!base::EndsWith(string, "s", base::CompareCase::INSENSITIVE_ASCII) ||
      !base::StringToDouble(string.substr(0, string.size() - 1), &seconds)) {
    return false;
  }
  *result = base::TimeDelta::FromSecondsD(seconds);
  return true;
}

// Parses url in form of <stun|turn|turns>:<host>[:<port>][?transport=<udp|tcp>]
// and adds an entry to the |config|.
bool AddServerToConfig(std::string url,
                       const std::string& username,
                       const std::string& password,
                       IceConfig* config) {
  cricket::ProtocolType turn_transport_type = cricket::PROTO_LAST;

  const char kTcpTransportSuffix[] = "?transport=tcp";
  const char kUdpTransportSuffix[] = "?transport=udp";
  if (base::EndsWith(url, kTcpTransportSuffix,
                     base::CompareCase::INSENSITIVE_ASCII)) {
    turn_transport_type = cricket::PROTO_TCP;
    url.resize(url.size() - strlen(kTcpTransportSuffix));
  } else if (base::EndsWith(url, kUdpTransportSuffix,
                            base::CompareCase::INSENSITIVE_ASCII)) {
    turn_transport_type = cricket::PROTO_UDP;
    url.resize(url.size() - strlen(kUdpTransportSuffix));
  }

  size_t colon_pos = url.find(':');
  if (colon_pos == std::string::npos)
    return false;

  std::string protocol = url.substr(0, colon_pos);

  std::string host;
  int port;
  if (!net::ParseHostAndPort(url.substr(colon_pos + 1), &host, &port))
    return false;

  if (protocol == "stun") {
    if (port == -1)
      port = kDefaultStunTurnPort;
    config->stun_servers.push_back(rtc::SocketAddress(host, port));
  } else if (protocol == "turn") {
    if (port == -1)
      port = kDefaultStunTurnPort;
    if (turn_transport_type == cricket::PROTO_LAST)
      turn_transport_type = cricket::PROTO_UDP;
    config->turn_servers.push_back(cricket::RelayServerConfig(
        host, port, username, password, turn_transport_type, false));
  } else if (protocol == "turns") {
    if (port == -1)
      port = kDefaultTurnsPort;
    if (turn_transport_type == cricket::PROTO_LAST)
      turn_transport_type = cricket::PROTO_TCP;
    config->turn_servers.push_back(cricket::RelayServerConfig(
        host, port, username, password, turn_transport_type, true));
  } else {
    return false;
  }

  return true;
}

}  // namespace

IceConfig::IceConfig() = default;
IceConfig::IceConfig(const IceConfig& other) = default;
IceConfig::~IceConfig() = default;

// static
IceConfig IceConfig::Parse(const base::DictionaryValue& dictionary) {
  const base::ListValue* ice_servers_list = nullptr;
  if (!dictionary.GetList("iceServers", &ice_servers_list)) {
    return IceConfig();
  }

  IceConfig ice_config;

  // Parse lifetimeDuration field.
  std::string lifetime_str;
  base::TimeDelta lifetime;
  if (!dictionary.GetString("lifetimeDuration", &lifetime_str) ||
      !ParseLifetime(lifetime_str, &lifetime)) {
    LOG(ERROR) << "Received invalid lifetimeDuration value: " << lifetime_str;

    // If the |lifetimeDuration| field is missing or cannot be parsed then mark
    // the config as expired so it will refreshed for the next session.
    ice_config.expiration_time = base::Time::Now();
  } else {
    ice_config.expiration_time = base::Time::Now() + lifetime;
  }

  // Parse iceServers list and store them in |ice_config|.
  bool errors_found = false;
  for (const auto& server : *ice_servers_list) {
    const base::DictionaryValue* server_dict;
    if (!server.GetAsDictionary(&server_dict)) {
      errors_found = true;
      continue;
    }

    const base::ListValue* urls_list = nullptr;
    if (!server_dict->GetList("urls", &urls_list)) {
      errors_found = true;
      continue;
    }

    std::string username;
    server_dict->GetString("username", &username);

    std::string password;
    server_dict->GetString("credential", &password);

    for (const auto& url : *urls_list) {
      std::string url_str;
      if (!url.GetAsString(&url_str)) {
        errors_found = true;
        continue;
      }
      if (!AddServerToConfig(url_str, username, password, &ice_config)) {
        LOG(ERROR) << "Invalid ICE server URL: " << url_str;
      }
    }
  }

  if (errors_found) {
    std::string json;
    if (!base::JSONWriter::WriteWithOptions(
            dictionary, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json)) {
      NOTREACHED();
    }
    LOG(ERROR) << "Received ICE config with errors: " << json;
  }

  // If there are no STUN or no TURN servers then mark the config as expired so
  // it will refreshed for the next session.
  if (errors_found || ice_config.stun_servers.empty() ||
      ice_config.turn_servers.empty()) {
    ice_config.expiration_time = base::Time::Now();
  }

  return ice_config;
}

// static
IceConfig IceConfig::Parse(const std::string& config_json) {
  std::unique_ptr<base::Value> json = base::JSONReader::Read(config_json);
  if (!json) {
    return IceConfig();
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!json->GetAsDictionary(&dictionary)) {
    return IceConfig();
  }

  // Handle the case when the config is wrapped in 'data', i.e. as {'data': {
  // 'iceServers': {...} }}.
  base::DictionaryValue* data_dictionary = nullptr;
  if (!dictionary->HasKey("iceServers") &&
      dictionary->GetDictionary("data", &data_dictionary)) {
    return Parse(*data_dictionary);
  }

  return Parse(*dictionary);
}

}  // namespace protocol
}  // namespace remoting
