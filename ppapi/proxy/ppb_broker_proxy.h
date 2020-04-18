// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_PPB_BROKER_PROXY_H_
#define PPAPI_PROXY_PPB_BROKER_PROXY_H_

#include <stdint.h>

#include "base/sync_socket.h"
#include "ipc/ipc_platform_file.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/proxy/interface_proxy.h"
#include "ppapi/proxy/proxy_completion_callback_factory.h"
#include "ppapi/utility/completion_callback_factory.h"

namespace ppapi {

class HostResource;

namespace proxy {

class PPB_Broker_Proxy : public InterfaceProxy {
 public:
  explicit PPB_Broker_Proxy(Dispatcher* dispatcher);
  ~PPB_Broker_Proxy() override;

  static PP_Resource CreateProxyResource(PP_Instance instance);

  // InterfaceProxy implementation.
  bool OnMessageReceived(const IPC::Message& msg) override;

  static const ApiID kApiID = API_ID_PPB_BROKER;

 private:
  // Message handlers.
  void OnMsgCreate(PP_Instance instance, ppapi::HostResource* result_resource);
  void OnMsgConnect(const ppapi::HostResource& broker);
  void OnMsgConnectComplete(const ppapi::HostResource& broker,
                            IPC::PlatformFileForTransit foreign_socket_handle,
                            int32_t result);

  void ConnectCompleteInHost(int32_t result,
                             const ppapi::HostResource& host_resource);

  ProxyCompletionCallbackFactory<PPB_Broker_Proxy> callback_factory_;
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_PPB_BROKER_PROXY_H_
