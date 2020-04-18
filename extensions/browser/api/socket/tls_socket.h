// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_SOCKET_TLS_SOCKET_H_
#define EXTENSIONS_BROWSER_API_SOCKET_TLS_SOCKET_H_

#include <stdint.h>

#include <string>

#include "extensions/browser/api/socket/socket.h"
#include "extensions/browser/api/socket/socket_api.h"
#include "extensions/browser/api/socket/tcp_socket.h"
#include "net/ssl/ssl_config_service.h"

namespace net {
class Socket;
class CertVerifier;
class CTPolicyEnforcer;
class CTVerifier;
class TransportSecurityState;
}

namespace extensions {

class TLSSocket;

// TLS Sockets from the chrome.socket and chrome.sockets.tcp APIs. A regular
// TCPSocket is converted to a TLSSocket via chrome.socket.secure() or
// chrome.sockets.tcp.secure(). The inheritance here is for interface API
// compatibility, not for the implementation that comes with it. TLSSocket
// does not use its superclass's socket state, so all methods are overridden
// here to prevent any access of ResumableTCPSocket's socket state. Except
// for the implementation of a write queue in Socket::Write() (a super-super
// class of ResumableTCPSocket). That implementation only queues and
// serializes invocations to WriteImpl(), implemented here, and does not
// touch any socket state.
class TLSSocket : public ResumableTCPSocket {
 public:
  typedef base::Callback<void(std::unique_ptr<TLSSocket>, int)> SecureCallback;

  TLSSocket(std::unique_ptr<net::StreamSocket> tls_socket,
            const std::string& owner_extension_id);

  ~TLSSocket() override;

  // Most of these methods either fail or forward the method call on to the
  // inner net::StreamSocket. The remaining few do actual TLS work.

  // Fails.
  void Connect(const net::AddressList& address,
               const CompletionCallback& callback) override;
  // Forwards.
  void Disconnect(bool socket_destroying) override;

  // Attempts to read |count| bytes of decrypted data from the TLS socket,
  // invoking |callback| with the actual number of bytes read, or a network
  // error code if an error occurred.
  void Read(int count, const ReadCompletionCallback& callback) override;

  // Fails. This should have been called on the TCP socket before secure() was
  // invoked.
  bool SetKeepAlive(bool enable, int delay) override;

  // Fails. This should have been called on the TCP socket before secure() was
  // invoked.
  bool SetNoDelay(bool no_delay) override;

  // Fails. TLSSocket is only a client.
  int Listen(const std::string& address,
             uint16_t port,
             int backlog,
             std::string* error_msg) override;

  // Fails. TLSSocket is only a client.
  void Accept(const AcceptCompletionCallback& callback) override;

  // Forwards.
  bool IsConnected() override;

  // Forwards.
  bool GetPeerAddress(net::IPEndPoint* address) override;
  // Forwards.
  bool GetLocalAddress(net::IPEndPoint* address) override;

  // Returns TYPE_TLS.
  SocketType GetSocketType() const override;

  // Convert |socket| to a TLS socket. |socket| must be an open TCP client
  // socket. |socket| must not have a pending read. UpgradeSocketToTLS() must
  // be invoked in the IO thread. |callback| will always be invoked. |options|
  // may be NULL.
  // Note: |callback| may be synchronously invoked before
  // UpgradeSocketToTLS() returns. Currently using the older chrome.socket
  // version of SecureOptions, to avoid having the older API implementation
  // depend on the newer one.
  static void UpgradeSocketToTLS(
      Socket* socket,
      scoped_refptr<net::SSLConfigService> config_service,
      net::CertVerifier* cert_verifier,
      net::TransportSecurityState* transport_security_state,
      net::CTVerifier* ct_verifier,
      net::CTPolicyEnforcer* ct_policy_enforcer,
      const std::string& extension_id,
      api::socket::SecureOptions* options,
      const SecureCallback& callback);

 private:
  int WriteImpl(net::IOBuffer* io_buffer,
                int io_buffer_size,
                const net::CompletionCallback& callback) override;

  void OnReadComplete(const scoped_refptr<net::IOBuffer>& io_buffer,
                      int result);

  std::unique_ptr<net::StreamSocket> tls_socket_;
  ReadCompletionCallback read_callback_;
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_SOCKET_TLS_SOCKET_H_

