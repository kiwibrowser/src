// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/port_allocator.h"

#include <algorithm>
#include <map>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "net/base/escape.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "remoting/protocol/native_ip_synthesizer.h"
#include "remoting/protocol/network_settings.h"
#include "remoting/protocol/transport_context.h"

namespace {

typedef std::map<std::string, std::string> StringMap;

// Parses the lines in the result of the HTTP request that are of the form
// 'a=b' and returns them in a map.
StringMap ParseMap(const std::string& string) {
  StringMap map;
  base::StringPairs pairs;
  base::SplitStringIntoKeyValuePairs(string, '=', '\n', &pairs);

  for (auto& pair : pairs) {
    map[pair.first] = pair.second;
  }
  return map;
}

const int kNumRetries = 5;

}  // namespace

namespace remoting {
namespace protocol {

PortAllocator::PortAllocator(
    std::unique_ptr<rtc::NetworkManager> network_manager,
    std::unique_ptr<rtc::PacketSocketFactory> socket_factory,
    scoped_refptr<TransportContext> transport_context)
    : BasicPortAllocator(network_manager.get(), socket_factory.get()),
      network_manager_(std::move(network_manager)),
      socket_factory_(std::move(socket_factory)),
      transport_context_(transport_context) {
  // We always use PseudoTcp to provide a reliable channel. It provides poor
  // performance when combined with TCP-based transport, so we have to disable
  // TCP ports. ENABLE_SHARED_UFRAG flag is specified so that the same username
  // fragment is shared between all candidates.
  // TODO(crbug.com/488760): Ideally we want to add
  // PORTALLOCATOR_DISABLE_COSTLY_NETWORKS, but this is unreliable on iOS and
  // may end up removing mobile networks when no WiFi is available. We may want
  // to add this flag only if there is WiFi interface.
  int flags = cricket::PORTALLOCATOR_DISABLE_TCP |
              cricket::PORTALLOCATOR_ENABLE_IPV6 |
              cricket::PORTALLOCATOR_ENABLE_IPV6_ON_WIFI;

  NetworkSettings network_settings = transport_context_->network_settings();

  if (!(network_settings.flags & NetworkSettings::NAT_TRAVERSAL_STUN))
    flags |= cricket::PORTALLOCATOR_DISABLE_STUN;

  if (!(network_settings.flags & NetworkSettings::NAT_TRAVERSAL_RELAY))
    flags |= cricket::PORTALLOCATOR_DISABLE_RELAY;

  set_flags(flags);
  SetPortRange(network_settings.port_range.min_port,
               network_settings.port_range.max_port);
  Initialize();
}

PortAllocator::~PortAllocator() = default;

cricket::PortAllocatorSession* PortAllocator::CreateSessionInternal(
    const std::string& content_name,
    int component,
    const std::string& ice_username_fragment,
    const std::string& ice_password) {
  return new PortAllocatorSession(this, content_name, component,
                                  ice_username_fragment, ice_password);
}

PortAllocatorSession::PortAllocatorSession(PortAllocator* allocator,
                                           const std::string& content_name,
                                           int component,
                                           const std::string& ice_ufrag,
                                           const std::string& ice_pwd)
    : BasicPortAllocatorSession(allocator,
                                content_name,
                                component,
                                ice_ufrag,
                                ice_pwd),
      transport_context_(allocator->transport_context()),
      weak_factory_(this) {}

PortAllocatorSession::~PortAllocatorSession() = default;

void PortAllocatorSession::GetPortConfigurations() {
  transport_context_->GetIceConfig(base::Bind(
      &PortAllocatorSession::OnIceConfig, weak_factory_.GetWeakPtr()));
}

void PortAllocatorSession::OnIceConfig(const IceConfig& ice_config) {
  ice_config_ = ice_config;
  ConfigReady(GetPortConfiguration().release());

  if (relay_enabled() && !ice_config_.relay_servers.empty() &&
      !ice_config_.relay_token.empty()) {
    TryCreateRelaySession();
  }
}

std::unique_ptr<cricket::PortConfiguration>
PortAllocatorSession::GetPortConfiguration() {
  cricket::ServerAddresses stun_servers;
  for (const auto& host : ice_config_.stun_servers) {
    stun_servers.insert(host);
  }

  std::unique_ptr<cricket::PortConfiguration> config(
      new cricket::PortConfiguration(stun_servers, username(), password()));

  if (relay_enabled()) {
    for (const auto& turn_server : ice_config_.turn_servers) {
      config->AddRelay(turn_server);
    }
  }

  return config;
}

void PortAllocatorSession::TryCreateRelaySession() {
  DCHECK(!ice_config_.relay_servers.empty());
  DCHECK(!ice_config_.relay_token.empty());

  if (attempts_ == kNumRetries) {
    LOG(ERROR) << "PortAllocator: maximum number of requests reached; "
               << "giving up on relay.";
    return;
  }

  // Choose the next host to try.
  std::string host =
      ice_config_.relay_servers[attempts_ % ice_config_.relay_servers.size()];
  attempts_++;

  DCHECK(!username().empty());
  DCHECK(!password().empty());
  std::string url = "https://" + host + "/create_session?username=" +
                    net::EscapeUrlEncodedData(username(), false) +
                    "&password=" +
                    net::EscapeUrlEncodedData(password(), false) + "&sn=1";
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("CRD_relay_session_request", R"(
        semantics {
          sender: "Chrome Remote Desktop"
          description:
            "Request is sent by Chrome Remote Desktop to allocate relay "
            "session. Returned relay session credentials are used over UDP to "
            "connect to Google-owned relay servers, which is required for NAT "
            "traversal."
          trigger:
            "Start of each Chrome Remote Desktop and during connection when "
            "peer-to-peer transport needs to be reconnected."
          data:
            "A temporary authentication token issued by Google services (over "
            "XMPP connection)."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled by settings. You can block Chrome "
            "Remote Desktop as specified here: "
            "https://support.google.com/chrome/?p=remote_desktop"
          chrome_policy {
            RemoteAccessHostFirewallTraversal {
              policy_options {mode: MANDATORY}
              RemoteAccessHostFirewallTraversal: false
            }
          }
        }
        comments:
          "Above specified policy is only applicable on the host side and "
          "doesn't have effect in Android and iOS client apps. The product "
          "is shipped separately from Chromium, except on Chrome OS."
        )");
  std::unique_ptr<UrlRequest> url_request =
      transport_context_->url_request_factory()->CreateUrlRequest(
          UrlRequest::Type::GET, url, traffic_annotation);
  url_request->AddHeader("X-Talk-Google-Relay-Auth: " +
                         ice_config_.relay_token);
  url_request->AddHeader("X-Google-Relay-Auth: " + ice_config_.relay_token);
  url_request->AddHeader("X-Stream-Type: chromoting");
  url_request->Start(base::Bind(&PortAllocatorSession::OnSessionRequestResult,
                                base::Unretained(this)));
  url_requests_.insert(std::move(url_request));
}

void PortAllocatorSession::OnSessionRequestResult(
    const UrlRequest::Result& result) {
  if (!result.success || result.status != net::HTTP_OK) {
    LOG(WARNING) << "Received error when allocating relay session: "
                 << result.status;
    TryCreateRelaySession();
    return;
  }

  StringMap map = ParseMap(result.response_body);

  if (!username().empty() && map["username"] != username()) {
    LOG(WARNING) << "Received unexpected username value from relay server.";
  }
  if (!password().empty() && map["password"] != password()) {
    LOG(WARNING) << "Received unexpected password value from relay server.";
  }

  std::unique_ptr<cricket::PortConfiguration> config = GetPortConfiguration();

  std::string relay_ip = map["relay.ip"];
  std::string relay_port = map["relay.udp_port"];
  unsigned relay_port_int;

  if (!relay_ip.empty() && !relay_port.empty() &&
      base::StringToUint(relay_port, &relay_port_int)) {
    cricket::RelayServerConfig relay_config(cricket::RELAY_GTURN);
    rtc::SocketAddress address(relay_ip, relay_port_int);
    // |relay_ip| is in IPv4 so we will need to do an IPv6 synthesis.
    relay_config.ports.push_back(
        cricket::ProtocolAddress(ToNativeSocket(address), cricket::PROTO_UDP));
    config->AddRelay(relay_config);
  }

  ConfigReady(config.release());
}

}  // namespace protocol
}  // namespace remoting
