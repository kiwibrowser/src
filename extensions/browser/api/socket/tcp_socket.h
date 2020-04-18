// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_SOCKET_TCP_SOCKET_H_
#define EXTENSIONS_BROWSER_API_SOCKET_TCP_SOCKET_H_

#include <stdint.h>

#include <string>

#include "extensions/browser/api/socket/socket.h"

// This looks like it should be forward-declarable, but it does some tricky
// moves that make it easier to just include it.
#include "net/socket/tcp_client_socket.h"
#include "net/socket/tcp_server_socket.h"

namespace net {
class Socket;
}

namespace extensions {

class TCPSocket : public Socket {
 public:
  explicit TCPSocket(const std::string& owner_extension_id);
  TCPSocket(std::unique_ptr<net::TCPClientSocket> tcp_client_socket,
            const std::string& owner_extension_id,
            bool is_connected = false);

  ~TCPSocket() override;

  void Connect(const net::AddressList& address,
               const CompletionCallback& callback) override;
  void Disconnect(bool socket_destroying) override;
  void Bind(const std::string& address,
            uint16_t port,
            const CompletionCallback& callback) override;
  void Read(int count, const ReadCompletionCallback& callback) override;
  void RecvFrom(int count, const RecvFromCompletionCallback& callback) override;
  void SendTo(scoped_refptr<net::IOBuffer> io_buffer,
              int byte_count,
              const net::IPEndPoint& address,
              const CompletionCallback& callback) override;
  bool SetKeepAlive(bool enable, int delay) override;
  bool SetNoDelay(bool no_delay) override;
  int Listen(const std::string& address,
             uint16_t port,
             int backlog,
             std::string* error_msg) override;
  void Accept(const AcceptCompletionCallback& callback) override;

  bool IsConnected() override;

  bool GetPeerAddress(net::IPEndPoint* address) override;
  bool GetLocalAddress(net::IPEndPoint* address) override;

  // Like Disconnect(), only Release() doesn't delete the underlying stream
  // or attempt to close it. Useful when giving away ownership with
  // ClientStream().
  virtual void Release();

  Socket::SocketType GetSocketType() const override;

  static TCPSocket* CreateSocketForTesting(
      std::unique_ptr<net::TCPClientSocket> tcp_client_socket,
      const std::string& owner_extension_id,
      bool is_connected = false);
  static TCPSocket* CreateServerSocketForTesting(
      std::unique_ptr<net::TCPServerSocket> tcp_server_socket,
      const std::string& owner_extension_id);

  // Returns NULL if GetSocketType() isn't TYPE_TCP or if the connection
  // wasn't set up via Connect() (vs Listen()/Accept()).
  net::TCPClientSocket* ClientStream();

  // Whether a Read() has been issued, that hasn't come back yet.
  bool HasPendingRead() const;

 protected:
  int WriteImpl(net::IOBuffer* io_buffer,
                int io_buffer_size,
                const net::CompletionCallback& callback) override;

 private:
  void RefreshConnectionStatus();
  void OnConnectComplete(int result);
  void OnReadComplete(scoped_refptr<net::IOBuffer> io_buffer, int result);
  void OnAccept(int result);

  TCPSocket(std::unique_ptr<net::TCPServerSocket> tcp_server_socket,
            const std::string& owner_extension_id);

  std::unique_ptr<net::TCPClientSocket> socket_;
  std::unique_ptr<net::TCPServerSocket> server_socket_;

  enum SocketMode { UNKNOWN = 0, CLIENT, SERVER, };
  SocketMode socket_mode_;

  CompletionCallback connect_callback_;

  ReadCompletionCallback read_callback_;

  std::unique_ptr<net::StreamSocket> accept_socket_;
  AcceptCompletionCallback accept_callback_;
};

// TCP Socket instances from the "sockets.tcp" namespace. These are regular
// socket objects with additional properties related to the behavior defined in
// the "sockets.tcp" namespace.
class ResumableTCPSocket : public TCPSocket {
 public:
  explicit ResumableTCPSocket(const std::string& owner_extension_id);
  explicit ResumableTCPSocket(
      std::unique_ptr<net::TCPClientSocket> tcp_client_socket,
      const std::string& owner_extension_id,
      bool is_connected);

  ~ResumableTCPSocket() override;

  // Overriden from ApiResource
  bool IsPersistent() const override;

  const std::string& name() const { return name_; }
  void set_name(const std::string& name) { name_ = name; }

  bool persistent() const { return persistent_; }
  void set_persistent(bool persistent) { persistent_ = persistent; }

  int buffer_size() const { return buffer_size_; }
  void set_buffer_size(int buffer_size) { buffer_size_ = buffer_size; }

  bool paused() const { return paused_; }
  void set_paused(bool paused) { paused_ = paused; }

 private:
  friend class ApiResourceManager<ResumableTCPSocket>;
  static const char* service_name() { return "ResumableTCPSocketManager"; }

  // Application-defined string - see sockets_tcp.idl.
  std::string name_;
  // Flag indicating whether the socket is left open when the application is
  // suspended - see sockets_tcp.idl.
  bool persistent_;
  // The size of the buffer used to receive data - see sockets_tcp.idl.
  int buffer_size_;
  // Flag indicating whether a connected socket blocks its peer from sending
  // more data - see sockets_tcp.idl.
  bool paused_;
};

// TCP Socket instances from the "sockets.tcpServer" namespace. These are
// regular socket objects with additional properties related to the behavior
// defined in the "sockets.tcpServer" namespace.
class ResumableTCPServerSocket : public TCPSocket {
 public:
  explicit ResumableTCPServerSocket(const std::string& owner_extension_id);

  // Overriden from ApiResource
  bool IsPersistent() const override;

  const std::string& name() const { return name_; }
  void set_name(const std::string& name) { name_ = name; }

  bool persistent() const { return persistent_; }
  void set_persistent(bool persistent) { persistent_ = persistent; }

  bool paused() const { return paused_; }
  void set_paused(bool paused) { paused_ = paused; }

 private:
  friend class ApiResourceManager<ResumableTCPServerSocket>;
  static const char* service_name() {
    return "ResumableTCPServerSocketManager";
  }

  // Application-defined string - see sockets_tcp_server.idl.
  std::string name_;
  // Flag indicating whether the socket is left open when the application is
  // suspended - see sockets_tcp_server.idl.
  bool persistent_;
  // Flag indicating whether a connected socket blocks its peer from sending
  // more data - see sockets_tcp_server.idl.
  bool paused_;
};

}  //  namespace extensions

#endif  // EXTENSIONS_BROWSER_API_SOCKET_TCP_SOCKET_H_
