// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_SIGNALING_XMPP_SIGNAL_STRATEGY_H_
#define REMOTING_SIGNALING_XMPP_SIGNAL_STRATEGY_H_

#include "remoting/signaling/signal_strategy.h"

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace net {
class ClientSocketFactory;
class URLRequestContextGetter;
}  // namespace net

namespace remoting {

// XmppSignalStrategy implements SignalStrategy using direct XMPP connection.
// This class can be created on a different thread from the one it is used (when
// Connect() is called).
class XmppSignalStrategy : public SignalStrategy {
 public:
  // XMPP Server configuration for XmppSignalStrategy.
  struct XmppServerConfig {
    XmppServerConfig();
    XmppServerConfig(const XmppServerConfig& other);
    ~XmppServerConfig();

    std::string host;
    int port;
    bool use_tls;

    std::string username;
    std::string auth_token;
  };

  XmppSignalStrategy(
      net::ClientSocketFactory* socket_factory,
      const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
      const XmppServerConfig& xmpp_server_config);
  ~XmppSignalStrategy() override;

  // SignalStrategy interface.
  void Connect() override;
  void Disconnect() override;
  State GetState() const override;
  Error GetError() const override;
  const SignalingAddress& GetLocalAddress() const override;
  void AddListener(Listener* listener) override;
  void RemoveListener(Listener* listener) override;
  bool SendStanza(std::unique_ptr<buzz::XmlElement> stanza) override;
  std::string GetNextId() override;

  // This method is used to update the auth info (for example when the OAuth
  // access token is renewed). It is OK to call this even when we are in the
  // CONNECTED state. It will be used on the next Connect() call.
  void SetAuthInfo(const std::string& username,
                   const std::string& auth_token);

 private:
  // This ensures that even if a Listener deletes the current instance during
  // OnSignalStrategyIncomingStanza(), we can delete |core_| asynchronously.
  class Core;

  std::unique_ptr<Core> core_;

  DISALLOW_COPY_AND_ASSIGN(XmppSignalStrategy);
};

}  // namespace remoting

#endif  // REMOTING_SIGNALING_XMPP_SIGNAL_STRATEGY_H_
