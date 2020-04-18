// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef JINGLE_NOTIFIER_COMMUNICATOR_LOGIN_SETTINGS_H_
#define JINGLE_NOTIFIER_COMMUNICATOR_LOGIN_SETTINGS_H_
#include <string>

#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "jingle/notifier/base/server_information.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_request_context_getter.h"
#include "third_party/libjingle_xmpp/xmpp/xmppclientsettings.h"

namespace notifier {

class LoginSettings {
 public:
  LoginSettings(
      const buzz::XmppClientSettings& user_settings,
      const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
      const ServerList& default_servers,
      bool try_ssltcp_first,
      const std::string& auth_mechanism,
      const net::NetworkTrafficAnnotationTag& traffic_annotation);

  LoginSettings(const LoginSettings& other);

  ~LoginSettings();

  // Copy constructor and assignment operator welcome.

  const buzz::XmppClientSettings& user_settings() const {
    return user_settings_;
  }

  void set_user_settings(const buzz::XmppClientSettings& user_settings);

  scoped_refptr<net::URLRequestContextGetter> request_context_getter() const {
    return request_context_getter_;
  }

  bool try_ssltcp_first() const {
    return try_ssltcp_first_;
  }

  const std::string& auth_mechanism() const {
    return auth_mechanism_;
  }

  ServerList GetServers() const;

  const net::NetworkTrafficAnnotationTag traffic_annotation() const {
    return traffic_annotation_;
  }

  // The redirect server will eventually expire.
  void SetRedirectServer(const ServerInformation& redirect_server);

  ServerList GetServersForTimeForTest(base::Time now) const;

  base::Time GetRedirectExpirationForTest() const;

 private:
  ServerList GetServersForTime(base::Time now) const;

  buzz::XmppClientSettings user_settings_;
  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  ServerList default_servers_;
  bool try_ssltcp_first_;
  std::string auth_mechanism_;
  const net::NetworkTrafficAnnotationTag traffic_annotation_;

  // Used to handle redirects
  ServerInformation redirect_server_;
  base::Time redirect_expiration_;

};

}  // namespace notifier

#endif  // JINGLE_NOTIFIER_COMMUNICATOR_LOGIN_SETTINGS_H_
