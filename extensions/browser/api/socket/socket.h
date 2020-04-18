// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_SOCKET_SOCKET_H_
#define EXTENSIONS_BROWSER_API_SOCKET_SOCKET_H_

#include <stdint.h>

#include <string>
#include <utility>

#include "base/callback.h"
#include "base/containers/queue.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/api/api_resource.h"
#include "extensions/browser/api/api_resource_manager.h"
#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"
#include "net/base/ip_endpoint.h"
#include "net/socket/tcp_client_socket.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

#if defined(OS_CHROMEOS)
#include "extensions/browser/api/socket/app_firewall_hole_manager.h"
#endif  // OS_CHROMEOS

namespace net {
class AddressList;
class IPEndPoint;
class Socket;
}

namespace extensions {

using CompletionCallback = base::Callback<void(int)>;
using ReadCompletionCallback = base::Callback<
    void(int, scoped_refptr<net::IOBuffer> io_buffer, bool socket_destroying)>;
using RecvFromCompletionCallback =
    base::Callback<void(int,
                        scoped_refptr<net::IOBuffer> io_buffer,
                        bool socket_destroying,
                        const std::string&,
                        uint16_t)>;
using AcceptCompletionCallback =
    base::Callback<void(int, std::unique_ptr<net::TCPClientSocket>)>;

// A Socket wraps a low-level socket and includes housekeeping information that
// we need to manage it in the context of an extension.
class Socket : public ApiResource {
 public:
  enum SocketType { TYPE_TCP, TYPE_UDP, TYPE_TLS };

  ~Socket() override;

  // The hostname of the remote host that this socket is connected to.  This
  // may be the empty string if the client does not intend to ever upgrade the
  // socket to TLS, and thusly has not invoked set_hostname().
  const std::string& hostname() const { return hostname_; }

  // Set the hostname of the remote host that this socket is connected to.
  // Note: This may be an IP literal. In the case of IDNs, this should be a
  // series of U-LABELs (UTF-8), not A-LABELs. IP literals for IPv6 will be
  // unbracketed.
  void set_hostname(const std::string& hostname) { hostname_ = hostname; }

#if defined(OS_CHROMEOS)
  void set_firewall_hole(
      std::unique_ptr<AppFirewallHole, content::BrowserThread::DeleteOnUIThread>
          firewall_hole) {
    firewall_hole_ = std::move(firewall_hole);
  }
#endif  // OS_CHROMEOS

  // Note: |address| contains the resolved IP address, not the hostname of
  // the remote endpoint. In order to upgrade this socket to TLS, callers
  // must also supply the hostname of the endpoint via set_hostname().
  virtual void Connect(const net::AddressList& address,
                       const CompletionCallback& callback) = 0;
  // |socket_destroying| is true if disconnect is due to destruction of the
  // socket.
  virtual void Disconnect(bool socket_destroying) = 0;
  virtual void Bind(const std::string& address,
                    uint16_t port,
                    const CompletionCallback& callback) = 0;

  // The |callback| will be called with the number of bytes read into the
  // buffer, or a negative number if an error occurred.
  virtual void Read(int count, const ReadCompletionCallback& callback) = 0;

  // The |callback| will be called with |byte_count| or a negative number if an
  // error occurred.
  void Write(scoped_refptr<net::IOBuffer> io_buffer,
             int byte_count,
             const CompletionCallback& callback);

  virtual void RecvFrom(int count,
                        const RecvFromCompletionCallback& callback) = 0;
  virtual void SendTo(scoped_refptr<net::IOBuffer> io_buffer,
                      int byte_count,
                      const net::IPEndPoint& address,
                      const CompletionCallback& callback) = 0;

  virtual bool SetKeepAlive(bool enable, int delay);
  virtual bool SetNoDelay(bool no_delay);
  virtual int Listen(const std::string& address,
                     uint16_t port,
                     int backlog,
                     std::string* error_msg);
  virtual void Accept(const AcceptCompletionCallback& callback);

  virtual bool IsConnected() = 0;

  virtual bool GetPeerAddress(net::IPEndPoint* address) = 0;
  virtual bool GetLocalAddress(net::IPEndPoint* address) = 0;

  virtual SocketType GetSocketType() const = 0;

  static bool StringAndPortToIPEndPoint(const std::string& ip_address_str,
                                        uint16_t port,
                                        net::IPEndPoint* ip_end_point);
  static void IPEndPointToStringAndPort(const net::IPEndPoint& address,
                                        std::string* ip_address_str,
                                        uint16_t* port);

  static net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag();

 protected:
  explicit Socket(const std::string& owner_extension_id_);

  void WriteData();
  virtual int WriteImpl(net::IOBuffer* io_buffer,
                        int io_buffer_size,
                        const net::CompletionCallback& callback) = 0;

  std::string hostname_;
  bool is_connected_;

 private:
  friend class ApiResourceManager<Socket>;
  static const char* service_name() { return "SocketManager"; }

  struct WriteRequest {
    WriteRequest(scoped_refptr<net::IOBuffer> io_buffer,
                 int byte_count,
                 const CompletionCallback& callback);
    WriteRequest(const WriteRequest& other);
    ~WriteRequest();
    scoped_refptr<net::IOBuffer> io_buffer;
    int byte_count;
    CompletionCallback callback;
    int bytes_written;
  };

  void OnWriteComplete(int result);

  base::queue<WriteRequest> write_queue_;
  scoped_refptr<net::IOBuffer> io_buffer_write_;

#if defined(OS_CHROMEOS)
  // Represents a hole punched in the system firewall for this socket.
  std::unique_ptr<AppFirewallHole, content::BrowserThread::DeleteOnUIThread>
      firewall_hole_;
#endif  // OS_CHROMEOS
};

}  //  namespace extensions

#endif  // EXTENSIONS_BROWSER_API_SOCKET_SOCKET_H_
