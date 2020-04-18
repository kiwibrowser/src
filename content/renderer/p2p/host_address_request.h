// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_P2P_HOST_ADDRESS_REQUEST_H_
#define CONTENT_RENDERER_P2P_HOST_ADDRESS_REQUEST_H_

#include <stdint.h>

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "net/base/ip_address.h"
#include "third_party/webrtc/rtc_base/asyncresolverinterface.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace content {

class P2PSocketDispatcher;

// P2PAsyncAddressResolver performs DNS hostname resolution. It's used
// to resolve addresses of STUN and relay servers.
class P2PAsyncAddressResolver
    : public base::RefCountedThreadSafe<P2PAsyncAddressResolver> {
 public:
  typedef base::Callback<void(const net::IPAddressList&)> DoneCallback;

  P2PAsyncAddressResolver(P2PSocketDispatcher* dispatcher);
  // Start address resolve process.
  void Start(const rtc::SocketAddress& addr,
             const DoneCallback& done_callback);
  // Clients must unregister before exiting for cleanup.
  void Cancel();

 private:
  enum State {
    STATE_CREATED,
    STATE_SENT,
    STATE_FINISHED,
  };

  friend class P2PSocketDispatcher;

  friend class base::RefCountedThreadSafe<P2PAsyncAddressResolver>;

  virtual ~P2PAsyncAddressResolver();

  void DoSendRequest(const rtc::SocketAddress& host_name,
                     const DoneCallback& done_callback);
  void DoUnregister();
  void OnResponse(const net::IPAddressList& address);
  void DeliverResponse(const net::IPAddressList& address);

  P2PSocketDispatcher* dispatcher_;
  scoped_refptr<base::SingleThreadTaskRunner> ipc_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> delegate_task_runner_;

  // State must be accessed from delegate thread only.
  State state_;

  // Accessed on the IPC thread only.
  int32_t request_id_;
  bool registered_;
  std::vector<rtc::IPAddress> addresses_;
  DoneCallback done_callback_;

  DISALLOW_COPY_AND_ASSIGN(P2PAsyncAddressResolver);
};

}  // namespace content

#endif  // CONTENT_RENDERER_P2P_HOST_ADDRESS_REQUEST_H_
