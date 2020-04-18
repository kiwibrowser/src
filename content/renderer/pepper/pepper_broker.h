// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PEPPER_PEPPER_BROKER_H_
#define CONTENT_RENDERER_PEPPER_PEPPER_BROKER_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/process/process.h"
#include "base/sync_socket.h"
#include "content/common/content_export.h"
#include "content/renderer/pepper/ppb_broker_impl.h"
#include "ppapi/proxy/proxy_channel.h"

namespace IPC {
struct ChannelHandle;
}

namespace ppapi {
namespace proxy {
class BrokerDispatcher;
}
}

namespace content {

class PluginModule;

// This object is NOT thread-safe.
class CONTENT_EXPORT PepperBrokerDispatcherWrapper {
 public:
  PepperBrokerDispatcherWrapper();
  ~PepperBrokerDispatcherWrapper();

  bool Init(base::ProcessId broker_pid,
            const IPC::ChannelHandle& channel_handle);

  int32_t SendHandleToBroker(PP_Instance instance,
                             base::SyncSocket::Handle handle);

 private:
  std::unique_ptr<ppapi::proxy::BrokerDispatcher> dispatcher_;
  std::unique_ptr<ppapi::proxy::ProxyChannel::Delegate> dispatcher_delegate_;
};

class PepperBroker : public base::RefCountedThreadSafe<PepperBroker> {
 public:
  explicit PepperBroker(PluginModule* plugin_module);

  // Decrements the references to the broker.
  // When there are no more references, this renderer's dispatcher is
  // destroyed, allowing the broker to shutdown if appropriate.
  // Callers should not reference this object after calling Disconnect().
  void Disconnect(PPB_Broker_Impl* client);

  // Adds a pending connection to the broker. Balances out Disconnect() calls.
  void AddPendingConnect(PPB_Broker_Impl* client);

  // Called when the channel to the broker has been established.
  void OnBrokerChannelConnected(base::ProcessId broker_pid,
                                const IPC::ChannelHandle& channel_handle);

  // Called when we know whether permission to access the PPAPI broker was
  // granted.
  void OnBrokerPermissionResult(PPB_Broker_Impl* client, bool result);

 private:
  friend class base::RefCountedThreadSafe<PepperBroker>;

  struct PendingConnection {
    PendingConnection();
    PendingConnection(const PendingConnection& other);
    ~PendingConnection();

    bool is_authorized;
    base::WeakPtr<PPB_Broker_Impl> client;
  };

  virtual ~PepperBroker();

  // Reports failure to all clients that had pending operations.
  void ReportFailureToClients(int error_code);

  // Connects the plugin to the broker via a pipe.
  void ConnectPluginToBroker(PPB_Broker_Impl* client);

  std::unique_ptr<PepperBrokerDispatcherWrapper> dispatcher_;

  // A map of pointers to objects that have requested a connection to the weak
  // pointer we can use to reference them. The mapping is needed so we can clean
  // up entries for objects that may have been deleted.
  typedef std::map<PPB_Broker_Impl*, PendingConnection> ClientMap;
  ClientMap pending_connects_;

  // Pointer to the associated plugin module.
  // Always set and cleared at the same time as the module's pointer to this.
  PluginModule* plugin_module_;

  DISALLOW_COPY_AND_ASSIGN(PepperBroker);
};

}  // namespace content

#endif  // CONTENT_RENDERER_PEPPER_PEPPER_BROKER_H_
