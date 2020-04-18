// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PEPPER_PPB_BROKER_IMPL_H_
#define CONTENT_RENDERER_PEPPER_PPB_BROKER_IMPL_H_

#include <stdint.h>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/process/process.h"
#include "ipc/ipc_listener.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/trusted/ppb_broker_trusted.h"
#include "ppapi/shared_impl/resource.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/ppb_broker_api.h"

class GURL;

namespace IPC {
struct ChannelHandle;
}

namespace content {
class PepperBroker;

class PPB_Broker_Impl : public ppapi::Resource,
                        public ppapi::thunk::PPB_Broker_API,
                        public IPC::Listener,
                        public base::SupportsWeakPtr<PPB_Broker_Impl> {
 public:
  explicit PPB_Broker_Impl(PP_Instance instance);

  // Resource override.
  ppapi::thunk::PPB_Broker_API* AsPPB_Broker_API() override;

  // PPB_BrokerTrusted implementation.
  int32_t Connect(
      scoped_refptr<ppapi::TrackedCallback> connect_callback) override;
  int32_t GetHandle(int32_t* handle) override;

  // Returns the URL of the document this plugin runs in. This is necessary to
  // decide whether to grant access to the PPAPI broker.
  GURL GetDocumentUrl();

  void BrokerConnected(int32_t handle, int32_t result);

 private:
  ~PPB_Broker_Impl() override;

  // IPC::Listener implementation.
  bool OnMessageReceived(const IPC::Message& message) override;

  void OnPpapiBrokerChannelCreated(base::ProcessId broker_pid,
                                   const IPC::ChannelHandle& handle);
  void OnPpapiBrokerPermissionResult(bool result);

  // PluginDelegate ppapi broker object.
  // We don't own this pointer but are responsible for calling Disconnect on it.
  PepperBroker* broker_;

  // Callback invoked from BrokerConnected.
  scoped_refptr<ppapi::TrackedCallback> connect_callback_;

  // Pipe handle for the plugin instance to use to communicate with the broker.
  // Never owned by this object.
  int32_t pipe_handle_;

  int routing_id_;

  DISALLOW_COPY_AND_ASSIGN(PPB_Broker_Impl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_PEPPER_PPB_BROKER_IMPL_H_
