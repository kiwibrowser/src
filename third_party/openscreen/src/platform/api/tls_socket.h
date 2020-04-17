// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_API_TLS_SOCKET_H_
#define PLATFORM_API_TLS_SOCKET_H_

#include <cstdint>
#include <memory>
#include <string>

#include "osp_base/error.h"
#include "osp_base/ip_address.h"
#include "osp_base/macros.h"
#include "platform/api/network_interface.h"
#include "platform/api/tls_socket_creds.h"

namespace openscreen {
namespace platform {

class TlsSocket;

// Platform-specific deleter of a TlsSocket instance returned by
// TlsSocket::Create().
struct TlsSocketDeleter {
  void operator()(TlsSocket* socket) const;
};

using TlsSocketUniquePtr = std::unique_ptr<TlsSocket, TlsSocketDeleter>;

struct TlsSocketMessage {
  void* data;
  size_t length;
  IPEndpoint src;
  IPEndpoint original_destination;
};

class TlsSocket {
 public:
  class Delegate {
   public:
    // Provides a unique ID for use by the TlsSocketFactory.
    virtual const std::string& GetNewSocketId() = 0;

    // Called when a |socket| is created or accepted.
    virtual void OnAccepted(std::unique_ptr<TlsSocket> socket) = 0;

    // Called when |socket| is closed.
    virtual void OnClosed(TlsSocket* socket) = 0;

    // Called when a |message| arrives on |socket|.
    virtual void OnMessage(TlsSocket* socket,
                           const TlsSocketMessage& message) = 0;

   protected:
    virtual ~Delegate() = default;
  };

  enum CloseReason {
    kUnknown = 0,
    kClosedByPeer,
    kAbortedByPeer,
    kInvalidMessage,
    kTooLongInactive,
  };

  // Creates a new, scoped TlsSocket within the IPv4 or IPv6 family.
  static ErrorOr<TlsSocketUniquePtr> Create(IPAddress::Version version);

  // Returns true if |socket| belongs to the IPv4/IPv6 address family.
  bool IsIPv4() const;
  bool IsIPv6() const;

  // Closes this socket. Delegate::OnClosed is called when complete.
  virtual void Close(CloseReason reason) = 0;

  // Start reading data. Delegate::OnMessage is called when new data arrives.
  virtual Error Read() = 0;

  // Sends a message and returns the number of bytes sent, on success.
  virtual Error SendMessage(const TlsSocketMessage& message) = 0;

  // Returns the unique identifier of the factory that created this socket.
  virtual const std::string& GetFactoryId() const = 0;

  // Returns the unique identifier for this socket.
  const std::string& id() const { return id_; }

 protected:
  Delegate* delegate() const { return delegate_; }

  explicit TlsSocket(Delegate* delegate) {}
  virtual ~TlsSocket() = 0;

 private:
  const std::string id_;
  Delegate* const delegate_;

  OSP_DISALLOW_COPY_AND_ASSIGN(TlsSocket);
};

// Abstract factory class for building TlsSockets.
class TlsSocketFactory {
 public:
  TlsSocketFactory() = default;
  virtual ~TlsSocketFactory() = default;

  // Returns a unique identifier for this Factory.
  virtual const std::string& GetId() = 0;

  // Gets the local address, if set, otherwise nullptr.
  virtual IPEndpoint* GetLocalAddress() = 0;

  // Start accepting new sockets. Should call Delegate::OnAccepted().
  virtual void Accept() = 0;

  // Stop accepting new sockets.
  virtual void Stop() = 0;

  // Set the credentials used for communication.
  virtual void SetCredentials(const TlsSocketCreds& creds) = 0;

 protected:
  virtual TlsSocket::Delegate* GetDelegate() const = 0;

 private:
  OSP_DISALLOW_COPY_AND_ASSIGN(TlsSocketFactory);
}
}  // namespace platform
}  // namespace openscreen

#endif  // PLATFORM_API_TLS_SOCKET_H_
