// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_UDP_SOCKET_MESSAGE_FILTER_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_UDP_SOCKET_MESSAGE_FILTER_H_

#include <stddef.h>

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "content/common/content_export.h"
#include "content/public/common/process_type.h"
#include "net/base/completion_callback.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/socket/udp_socket.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/ppb_udp_socket.h"
#include "ppapi/host/resource_message_filter.h"

struct PP_NetAddress_Private;

#if defined(OS_CHROMEOS)
#include "chromeos/network/firewall_hole.h"
#include "content/public/browser/browser_thread.h"
#endif  // defined(OS_CHROMEOS)

namespace net {
class IOBuffer;
class IOBufferWithSize;
}

namespace ppapi {

class SocketOptionData;

namespace host {
struct ReplyMessageContext;
}
}

namespace content {

class BrowserPpapiHostImpl;

class CONTENT_EXPORT PepperUDPSocketMessageFilter
    : public ppapi::host::ResourceMessageFilter {
 public:
  PepperUDPSocketMessageFilter(BrowserPpapiHostImpl* host,
                               PP_Instance instance,
                               bool private_api);

  static size_t GetNumInstances();

 protected:
  ~PepperUDPSocketMessageFilter() override;

 private:
  enum SocketOption {
    SOCKET_OPTION_ADDRESS_REUSE = 1 << 0,
    SOCKET_OPTION_BROADCAST = 1 << 1,
    SOCKET_OPTION_RCVBUF_SIZE = 1 << 2,
    SOCKET_OPTION_SNDBUF_SIZE = 1 << 3,
    SOCKET_OPTION_MULTICAST_LOOP = 1 << 4,
    SOCKET_OPTION_MULTICAST_TTL = 1 << 5
  };

  struct PendingSend {
    PendingSend(const net::IPAddress& address,
                int port,
                const scoped_refptr<net::IOBufferWithSize>& buffer,
                const ppapi::host::ReplyMessageContext& context);
    PendingSend(const PendingSend& other);
    ~PendingSend();

    net::IPAddress address;
    int port;
    scoped_refptr<net::IOBufferWithSize> buffer;
    ppapi::host::ReplyMessageContext context;
  };

  // ppapi::host::ResourceMessageFilter overrides.
  scoped_refptr<base::TaskRunner> OverrideTaskRunnerForMessage(
      const IPC::Message& message) override;
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

  int32_t OnMsgSetOption(const ppapi::host::HostMessageContext* context,
                         PP_UDPSocket_Option name,
                         const ppapi::SocketOptionData& value);
  int32_t OnMsgBind(const ppapi::host::HostMessageContext* context,
                    const PP_NetAddress_Private& addr);
  int32_t OnMsgSendTo(const ppapi::host::HostMessageContext* context,
                      const std::string& data,
                      const PP_NetAddress_Private& addr);
  int32_t OnMsgClose(const ppapi::host::HostMessageContext* context);
  int32_t OnMsgRecvSlotAvailable(
      const ppapi::host::HostMessageContext* context);
  int32_t OnMsgJoinGroup(const ppapi::host::HostMessageContext* context,
                         const PP_NetAddress_Private& addr);
  int32_t OnMsgLeaveGroup(const ppapi::host::HostMessageContext* context,
                          const PP_NetAddress_Private& addr);

  void DoBind(const ppapi::host::ReplyMessageContext& context,
              const PP_NetAddress_Private& addr);
  void OnBindComplete(std::unique_ptr<net::UDPSocket> socket,
                      const ppapi::host::ReplyMessageContext& context,
                      const PP_NetAddress_Private& net_address);
#if defined(OS_CHROMEOS)
  void OpenFirewallHole(const net::IPEndPoint& local_address,
                        base::Closure bind_complete);
  void OnFirewallHoleOpened(base::Closure bind_complete,
                            std::unique_ptr<chromeos::FirewallHole> hole);
#endif  // defined(OS_CHROMEOS)
  void DoRecvFrom();
  void DoSendTo(const ppapi::host::ReplyMessageContext& context,
                const std::string& data,
                const PP_NetAddress_Private& addr);
  int StartPendingSend();
  void Close();

  void OnRecvFromCompleted(int net_result);
  void OnSendToCompleted(int net_result);
  void FinishPendingSend(int net_result);

  void SendBindReply(const ppapi::host::ReplyMessageContext& context,
                     int32_t result,
                     const PP_NetAddress_Private& addr);
  void SendRecvFromResult(int32_t result,
                          const std::string& data,
                          const PP_NetAddress_Private& addr);
  void SendSendToReply(const ppapi::host::ReplyMessageContext& context,
                       int32_t result,
                       int32_t bytes_written);

  void SendBindError(const ppapi::host::ReplyMessageContext& context,
                     int32_t result);
  void SendRecvFromError(int32_t result);
  void SendSendToError(const ppapi::host::ReplyMessageContext& context,
                       int32_t result);

  int32_t CanUseMulticastAPI(const PP_NetAddress_Private& addr);

  // Bitwise-or of SocketOption flags. This stores the state about whether
  // each option is set before Bind() is called.
  int socket_options_;

  // Locally cached value of buffer size.
  int32_t rcvbuf_size_;
  int32_t sndbuf_size_;

  // Multicast options, if socket hasn't been bound
  int multicast_ttl_;
  int32_t can_use_multicast_;

  std::unique_ptr<net::UDPSocket> socket_;
  bool closed_;
#if defined(OS_CHROMEOS)
  std::unique_ptr<chromeos::FirewallHole,
                  content::BrowserThread::DeleteOnUIThread>
      firewall_hole_;
#endif  // defined(OS_CHROMEOS)

  scoped_refptr<net::IOBuffer> recvfrom_buffer_;

  base::queue<PendingSend> pending_sends_;

  net::IPEndPoint recvfrom_address_;

  size_t remaining_recv_slots_;

  bool external_plugin_;
  bool private_api_;

  int render_process_id_;
  int render_frame_id_;

  const bool is_potentially_secure_plugin_context_;

  DISALLOW_COPY_AND_ASSIGN(PepperUDPSocketMessageFilter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_UDP_SOCKET_MESSAGE_FILTER_H_
