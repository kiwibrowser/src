// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PPB_FLASH_MESSAGE_LOOP_PROXY_H_
#define PPAPI_PPB_FLASH_MESSAGE_LOOP_PROXY_H_

#include <stdint.h>

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/proxy/interface_proxy.h"

namespace IPC {
class Message;
}

namespace ppapi {

class HostResource;

namespace proxy {

class PPB_Flash_MessageLoop_Proxy
    : public InterfaceProxy,
      public base::SupportsWeakPtr<PPB_Flash_MessageLoop_Proxy> {
 public:
  explicit PPB_Flash_MessageLoop_Proxy(Dispatcher* dispatcher);
  ~PPB_Flash_MessageLoop_Proxy() override;

  static PP_Resource CreateProxyResource(PP_Instance instance);

  // InterfaceProxy implementation.
  bool OnMessageReceived(const IPC::Message& msg) override;

  static const ApiID kApiID = API_ID_PPB_FLASH_MESSAGELOOP;

 private:
  void OnMsgCreate(PP_Instance instance, ppapi::HostResource* resource);
  void OnMsgRun(const ppapi::HostResource& flash_message_loop,
                IPC::Message* reply);
  void OnMsgQuit(const ppapi::HostResource& flash_message_loop);

  void WillQuitSoon(std::unique_ptr<IPC::Message> reply_message,
                    int32_t result);

  DISALLOW_COPY_AND_ASSIGN(PPB_Flash_MessageLoop_Proxy);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PPB_FLASH_MESSAGE_LOOP_PROXY_H_
